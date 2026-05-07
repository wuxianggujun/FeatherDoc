#include <featherdoc/pdf/pdf_parser.hpp>

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

extern "C" {
#include <fpdf_text.h>
#include <fpdfview.h>
}

namespace featherdoc::pdf {
namespace {

struct PdfiumDocumentCloser {
    void operator()(FPDF_DOCUMENT document) const {
        if (document != nullptr) {
            FPDF_CloseDocument(document);
        }
    }
};

struct PdfiumPageCloser {
    void operator()(FPDF_PAGE page) const {
        if (page != nullptr) {
            FPDF_ClosePage(page);
        }
    }
};

struct PdfiumTextPageCloser {
    void operator()(FPDF_TEXTPAGE text_page) const {
        if (text_page != nullptr) {
            FPDFText_ClosePage(text_page);
        }
    }
};

using PdfiumDocumentPtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_DOCUMENT>, PdfiumDocumentCloser>;
using PdfiumPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_PAGE>, PdfiumPageCloser>;
using PdfiumTextPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_TEXTPAGE>, PdfiumTextPageCloser>;

struct TextCluster {
    std::string text;
    PdfRect bounds;
};

struct CandidateLine {
    const PdfParsedTextLine *line{nullptr};
    std::vector<TextCluster> clusters;
    double center_y{0.0};
};

struct PdfiumLibrary {
    PdfiumLibrary() {
        FPDF_InitLibrary();
    }

    ~PdfiumLibrary() {
        FPDF_DestroyLibrary();
    }

    PdfiumLibrary(const PdfiumLibrary &) = delete;
    PdfiumLibrary &operator=(const PdfiumLibrary &) = delete;
};

void ensure_pdfium_initialized() {
    static PdfiumLibrary library;
}

[[nodiscard]] std::string pdfium_error_to_string(unsigned long error_code) {
    switch (error_code) {
    case FPDF_ERR_SUCCESS:
        return "success";
    case FPDF_ERR_UNKNOWN:
        return "unknown error";
    case FPDF_ERR_FILE:
        return "file access or format error";
    case FPDF_ERR_FORMAT:
        return "invalid PDF format";
    case FPDF_ERR_PASSWORD:
        return "password is required or incorrect";
    case FPDF_ERR_SECURITY:
        return "unsupported security scheme";
    case FPDF_ERR_PAGE:
        return "page not found or content error";
    default:
        return "unrecognized PDFium error " + std::to_string(error_code);
    }
}

void append_utf8(std::string &output, std::uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        output.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

[[nodiscard]] PdfParsedTextSpan parse_text_span(FPDF_TEXTPAGE text_page,
                                                int char_index,
                                                const PdfParseOptions &options) {
    PdfParsedTextSpan span;
    append_utf8(span.text,
                static_cast<std::uint32_t>(
                    FPDFText_GetUnicode(text_page, char_index)));
    span.font_size_points = FPDFText_GetFontSize(text_page, char_index);

    if (options.extract_geometry) {
        double left = 0.0;
        double right = 0.0;
        double bottom = 0.0;
        double top = 0.0;
        if (FPDFText_GetCharBox(text_page, char_index, &left, &right, &bottom,
                                &top)) {
            span.bounds = PdfRect{left, bottom, right - left, top - bottom};
        }
    }

    return span;
}

[[nodiscard]] double rect_bottom(const PdfRect &rect) noexcept {
    return rect.y_points;
}

[[nodiscard]] double rect_top(const PdfRect &rect) noexcept {
    return rect.y_points + rect.height_points;
}

[[nodiscard]] double rect_right(const PdfRect &rect) noexcept {
    return rect.x_points + rect.width_points;
}

void expand_rect(PdfRect &target, const PdfRect &candidate) {
    if (target.width_points <= 0.0 && target.height_points <= 0.0) {
        target = candidate;
        return;
    }

    const double left = (std::min)(target.x_points, candidate.x_points);
    const double bottom = (std::min)(rect_bottom(target), rect_bottom(candidate));
    const double right = (std::max)(rect_right(target), rect_right(candidate));
    const double top = (std::max)(rect_top(target), rect_top(candidate));
    target = PdfRect{left, bottom, right - left, top - bottom};
}

void append_span_to_line(PdfParsedTextLine &line, const PdfParsedTextSpan &span) {
    line.text += span.text;
    expand_rect(line.bounds, span.bounds);
    line.text_spans.push_back(span);
}

void append_span_to_cluster(TextCluster &cluster,
                            const PdfParsedTextSpan &span) {
    cluster.text += span.text;
    expand_rect(cluster.bounds, span.bounds);
}

[[nodiscard]] bool should_start_new_line(const PdfParsedTextLine &line,
                                         const PdfParsedTextSpan &span) {
    if (line.text_spans.empty()) {
        return false;
    }

    const double line_height = (std::max)(line.bounds.height_points, 1.0);
    const double span_center_y = span.bounds.y_points + span.bounds.height_points / 2.0;
    const double line_center_y = line.bounds.y_points + line.bounds.height_points / 2.0;
    return std::abs(span_center_y - line_center_y) > line_height * 0.65;
}

[[nodiscard]] std::vector<PdfParsedTextLine>
group_text_spans_into_lines(const std::vector<PdfParsedTextSpan> &spans) {
    std::vector<PdfParsedTextLine> lines;

    for (const auto &span : spans) {
        if (span.text == "\r" || span.text == "\n") {
            if (!lines.empty() && !lines.back().text.empty()) {
                lines.emplace_back();
            }
            continue;
        }
        if (span.text.empty()) {
            continue;
        }

        if (lines.empty() || should_start_new_line(lines.back(), span)) {
            lines.emplace_back();
        }
        append_span_to_line(lines.back(), span);
    }

    lines.erase(std::remove_if(lines.begin(), lines.end(),
                               [](const PdfParsedTextLine &line) {
                                   return line.text.empty();
                               }),
                lines.end());
    return lines;
}

[[nodiscard]] bool should_start_new_paragraph(const PdfParsedTextLine &previous,
                                              const PdfParsedTextLine &current) {
    const double previous_height = (std::max)(previous.bounds.height_points, 1.0);
    const double vertical_gap = previous.bounds.y_points - rect_top(current.bounds);
    if (vertical_gap > previous_height * 0.85) {
        return true;
    }

    const double indent_delta = current.bounds.x_points - previous.bounds.x_points;
    return indent_delta > previous_height * 1.5;
}

void append_line_to_paragraph(PdfParsedParagraph &paragraph,
                              const PdfParsedTextLine &line) {
    if (!paragraph.text.empty()) {
        paragraph.text.push_back('\n');
    }
    paragraph.text += line.text;
    expand_rect(paragraph.bounds, line.bounds);
    paragraph.lines.push_back(line);
}

[[nodiscard]] bool should_start_new_cell_cluster(
    const TextCluster &cluster, const PdfParsedTextSpan &span,
    double line_height) {
    if (cluster.text.empty()) {
        return false;
    }

    const double horizontal_gap = span.bounds.x_points - rect_right(cluster.bounds);
    const double gap_threshold = (std::max)(18.0, line_height * 1.5);
    return horizontal_gap > gap_threshold;
}

[[nodiscard]] std::vector<TextCluster>
split_line_into_cell_clusters(const PdfParsedTextLine &line) {
    std::vector<TextCluster> clusters;
    const double line_height = (std::max)(line.bounds.height_points, 1.0);

    for (const auto &span : line.text_spans) {
        if (span.text == "\r" || span.text == "\n" || span.text.empty()) {
            continue;
        }

        if (clusters.empty() ||
            should_start_new_cell_cluster(clusters.back(), span, line_height)) {
            clusters.emplace_back();
        }
        append_span_to_cluster(clusters.back(), span);
    }

    clusters.erase(std::remove_if(clusters.begin(), clusters.end(),
                                  [](const TextCluster &cluster) {
                                      return cluster.text.empty();
                                  }),
                   clusters.end());
    return clusters;
}

[[nodiscard]] double line_center_y(const PdfParsedTextLine &line) noexcept {
    return line.bounds.y_points + line.bounds.height_points / 2.0;
}

[[nodiscard]] std::vector<CandidateLine>
collect_candidate_lines(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<CandidateLine> candidate_lines;
    candidate_lines.reserve(lines.size());

    for (const auto &line : lines) {
        auto clusters = split_line_into_cell_clusters(line);
        if (clusters.empty()) {
            continue;
        }

        candidate_lines.push_back(
            CandidateLine{&line, std::move(clusters), line_center_y(line)});
    }

    return candidate_lines;
}

[[nodiscard]] std::vector<double> build_column_anchors(
    const std::vector<CandidateLine> &rows) {
    std::vector<double> anchors;
    std::vector<std::size_t> anchor_counts;
    std::vector<double> cluster_positions;

    for (const auto &row : rows) {
        for (const auto &cluster : row.clusters) {
            cluster_positions.push_back(cluster.bounds.x_points);
        }
    }

    std::sort(cluster_positions.begin(), cluster_positions.end());
    for (const double position : cluster_positions) {
        if (anchors.empty() || position - anchors.back() > 24.0) {
            anchors.push_back(position);
            anchor_counts.push_back(1U);
            continue;
        }

        const auto last_index = anchors.size() - 1U;
        const auto count = anchor_counts[last_index];
        anchors[last_index] =
            ((anchors[last_index] * static_cast<double>(count)) + position) /
            static_cast<double>(count + 1U);
        anchor_counts[last_index] = count + 1U;
    }

    return anchors;
}

[[nodiscard]] bool has_regular_column_spacing(
    const std::vector<double> &anchors) {
    if (anchors.size() < 3U) {
        return true;
    }

    double min_gap = 0.0;
    double max_gap = 0.0;
    double total_gap = 0.0;
    bool has_gap = false;

    for (std::size_t index = 1U; index < anchors.size(); ++index) {
        const double gap = anchors[index] - anchors[index - 1U];
        if (!has_gap) {
            min_gap = gap;
            max_gap = gap;
            has_gap = true;
        } else {
            min_gap = (std::min)(min_gap, gap);
            max_gap = (std::max)(max_gap, gap);
        }
        total_gap += gap;
    }

    if (!has_gap) {
        return false;
    }

    const double average_gap =
        total_gap / static_cast<double>(anchors.size() - 1U);
    return (max_gap - min_gap) <= (std::max)(24.0, average_gap * 0.25);
}

[[nodiscard]] std::size_t nearest_column_index(
    const std::vector<double> &anchors, double x_points) {
    std::size_t best_index = 0U;
    double best_distance = std::abs(x_points - anchors.front());

    for (std::size_t index = 1U; index < anchors.size(); ++index) {
        const double distance = std::abs(x_points - anchors[index]);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = index;
        }
    }

    return best_index;
}

[[nodiscard]] bool build_table_candidate(
    const std::vector<CandidateLine> &rows, PdfParsedTableCandidate &candidate) {
    if (rows.size() < 3U) {
        return false;
    }

    const auto anchors = build_column_anchors(rows);
    if (anchors.size() < 3U) {
        return false;
    }
    if (!has_regular_column_spacing(anchors)) {
        return false;
    }

    candidate.column_anchor_x_points = anchors;
    candidate.rows.reserve(rows.size());

    for (const auto &source_row : rows) {
        PdfParsedTableRow row;
        row.cells.reserve(anchors.size());
        for (std::size_t column_index = 0U; column_index < anchors.size();
             ++column_index) {
            PdfParsedTableCell cell;
            cell.column_index = column_index;
            row.cells.push_back(cell);
        }

        for (const auto &cluster : source_row.clusters) {
            const auto column_index =
                nearest_column_index(anchors, cluster.bounds.x_points);
            auto &cell = row.cells[column_index];
            if (!cell.has_text) {
                cell.text = cluster.text;
                cell.bounds = cluster.bounds;
                cell.has_text = true;
            } else {
                cell.text.push_back('\n');
                cell.text += cluster.text;
                expand_rect(cell.bounds, cluster.bounds);
            }

            expand_rect(row.bounds, cluster.bounds);
            expand_rect(candidate.bounds, cluster.bounds);
        }

        candidate.rows.push_back(std::move(row));
    }

    return !candidate.rows.empty();
}

[[nodiscard]] std::vector<PdfParsedTableCandidate>
detect_table_candidates(const std::vector<PdfParsedTextLine> &lines) {
    const auto candidate_lines = collect_candidate_lines(lines);
    std::vector<PdfParsedTableCandidate> tables;

    if (candidate_lines.size() < 3U) {
        return tables;
    }

    for (std::size_t gap_index = 0U; gap_index + 1U < candidate_lines.size();
         ++gap_index) {
        const double first_gap =
            candidate_lines[gap_index].center_y -
            candidate_lines[gap_index + 1U].center_y;
        if (first_gap <= 0.0) {
            continue;
        }

        const double gap_tolerance = (std::max)(4.0, first_gap * 0.15);
        std::size_t run_end = gap_index;
        while (run_end + 1U < candidate_lines.size() - 1U) {
            const double next_gap =
                candidate_lines[run_end + 1U].center_y -
                candidate_lines[run_end + 2U].center_y;
            if (std::abs(next_gap - first_gap) > gap_tolerance) {
                break;
            }
            ++run_end;
        }

        if (run_end == gap_index) {
            continue;
        }

        std::vector<CandidateLine> rows;
        rows.reserve(run_end - gap_index + 2U);
        for (std::size_t row_index = gap_index; row_index <= run_end + 1U;
             ++row_index) {
            rows.push_back(candidate_lines[row_index]);
        }

        PdfParsedTableCandidate table;
        if (build_table_candidate(rows, table)) {
            tables.push_back(std::move(table));
            gap_index = run_end;
        }
    }

    return tables;
}

[[nodiscard]] std::vector<PdfParsedParagraph>
group_lines_into_paragraphs(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<PdfParsedParagraph> paragraphs;
    for (const auto &line : lines) {
        if (paragraphs.empty() ||
            should_start_new_paragraph(paragraphs.back().lines.back(), line)) {
            paragraphs.emplace_back();
        }
        append_line_to_paragraph(paragraphs.back(), line);
    }
    return paragraphs;
}

void enrich_text_structure(PdfParsedPage &page) {
    page.text_lines = group_text_spans_into_lines(page.text_spans);
    page.paragraphs = group_lines_into_paragraphs(page.text_lines);
    page.table_candidates = detect_table_candidates(page.text_lines);
}

}  // namespace

PdfParseResult PdfiumParser::parse(const std::filesystem::path &input_path,
                                   const PdfParseOptions &options) {
    PdfParseResult result;

    if (!std::filesystem::exists(input_path)) {
        result.error_message = "PDF input does not exist: " + input_path.string();
        return result;
    }

    ensure_pdfium_initialized();

    const auto input_string = input_path.string();
    PdfiumDocumentPtr document(FPDF_LoadDocument(input_string.c_str(), nullptr));
    if (!document) {
        result.error_message =
            "Unable to load PDF: " + pdfium_error_to_string(FPDF_GetLastError());
        return result;
    }

    const int page_count = FPDF_GetPageCount(document.get());
    result.document.pages.reserve(static_cast<std::size_t>(page_count));

    for (int page_index = 0; page_index < page_count; ++page_index) {
        PdfiumPagePtr page(FPDF_LoadPage(document.get(), page_index));
        if (!page) {
            result.error_message =
                "Unable to load PDF page " + std::to_string(page_index);
            return result;
        }

        PdfParsedPage parsed_page;
        parsed_page.size =
            PdfPageSize{FPDF_GetPageWidth(page.get()), FPDF_GetPageHeight(page.get())};

        if (options.extract_text) {
            PdfiumTextPagePtr text_page(FPDFText_LoadPage(page.get()));
            if (!text_page) {
                result.error_message =
                    "Unable to load PDF text page " + std::to_string(page_index);
                return result;
            }

            const int char_count = FPDFText_CountChars(text_page.get());
            parsed_page.text_spans.reserve(static_cast<std::size_t>(char_count));
            for (int char_index = 0; char_index < char_count; ++char_index) {
                if (FPDFText_GetUnicode(text_page.get(), char_index) == 0) {
                    continue;
                }
                parsed_page.text_spans.push_back(
                    parse_text_span(text_page.get(), char_index, options));
            }
            if (options.extract_geometry) {
                enrich_text_structure(parsed_page);
            }
        }

        result.document.pages.push_back(std::move(parsed_page));
    }

    result.success = true;
    return result;
}

}  // namespace featherdoc::pdf
