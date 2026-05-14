#include <featherdoc/pdf/pdf_parser.hpp>

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
    std::size_t line_index{0U};
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

[[nodiscard]] double rect_area(const PdfRect &rect) noexcept {
    return (std::max)(0.0, rect.width_points) *
           (std::max)(0.0, rect.height_points);
}

[[nodiscard]] bool rect_contains_point(const PdfRect &rect, double x_points,
                                       double y_points) noexcept {
    return x_points >= rect.x_points && x_points <= rect_right(rect) &&
           y_points >= rect.y_points &&
           y_points <= rect.y_points + rect.height_points;
}

[[nodiscard]] double rect_intersection_area(const PdfRect &left,
                                            const PdfRect &right) noexcept {
    const double intersection_left =
        (std::max)(left.x_points, right.x_points);
    const double intersection_bottom =
        (std::max)(left.y_points, right.y_points);
    const double intersection_right =
        (std::min)(rect_right(left), rect_right(right));
    const double intersection_top = (std::min)(
        left.y_points + left.height_points, right.y_points + right.height_points);
    if (intersection_right <= intersection_left ||
        intersection_top <= intersection_bottom) {
        return 0.0;
    }

    return (intersection_right - intersection_left) *
           (intersection_top - intersection_bottom);
}

[[nodiscard]] bool rect_meaningfully_overlaps_rect(const PdfRect &subject,
                                                   const PdfRect &rect) noexcept {
    const double subject_area = rect_area(subject);
    if (subject_area <= 0.0) {
        return false;
    }

    const double center_x = subject.x_points + subject.width_points / 2.0;
    const double center_y = subject.y_points + subject.height_points / 2.0;
    const bool center_inside_rect =
        rect_contains_point(rect, center_x, center_y);
    const double overlap_ratio =
        rect_intersection_area(subject, rect) / subject_area;
    return center_inside_rect || overlap_ratio >= 0.5;
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
    if (std::abs(span_center_y - line_center_y) > line_height * 0.65) {
        return true;
    }

    const double x_backtrack = line.bounds.x_points - span.bounds.x_points;
    const double x_backtrack_threshold = (std::max)(12.0, line_height * 1.5);
    return x_backtrack > x_backtrack_threshold;
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

[[nodiscard]] bool should_start_new_paragraph(
    const PdfParsedTextLine &previous, const PdfParsedTextLine &current,
    const std::optional<std::size_t> &previous_table_candidate_index,
    const std::optional<std::size_t> &current_table_candidate_index) {
    if (previous_table_candidate_index != current_table_candidate_index) {
        return true;
    }

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

[[nodiscard]] double line_top_y(const PdfParsedTextLine &line) noexcept {
    return rect_top(line.bounds);
}

[[nodiscard]] double line_left_x(const PdfParsedTextLine &line) noexcept {
    return line.bounds.x_points;
}

void sort_lines_by_reading_order(std::vector<PdfParsedTextLine> &lines) {
    std::stable_sort(lines.begin(), lines.end(),
                     [](const PdfParsedTextLine &left,
                        const PdfParsedTextLine &right) {
                         const auto left_top_key =
                             std::llround(line_top_y(left) * 100.0);
                         const auto right_top_key =
                             std::llround(line_top_y(right) * 100.0);
                         if (left_top_key != right_top_key) {
                             return left_top_key > right_top_key;
                         }

                         const auto left_x_key =
                             std::llround(line_left_x(left) * 100.0);
                         const auto right_x_key =
                             std::llround(line_left_x(right) * 100.0);
                         if (left_x_key != right_x_key) {
                             return left_x_key < right_x_key;
                         }

                         return false;
                     });
}

[[nodiscard]] std::vector<CandidateLine>
collect_candidate_lines(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<CandidateLine> candidate_lines;
    candidate_lines.reserve(lines.size());

    for (std::size_t line_index = 0U; line_index < lines.size(); ++line_index) {
        const auto &line = lines[line_index];
        auto clusters = split_line_into_cell_clusters(line);
        if (clusters.empty()) {
            continue;
        }

        candidate_lines.push_back(
            CandidateLine{line_index, std::move(clusters), line_center_y(line)});
    }

    return candidate_lines;
}

[[nodiscard]] std::vector<double> build_column_anchors(
    const std::vector<CandidateLine> &rows) {
    std::vector<double> anchors;
    std::vector<std::size_t> anchor_counts;
    std::vector<double> cluster_positions;

    std::size_t max_cluster_count = 0U;
    for (const auto &row : rows) {
        max_cluster_count = (std::max)(max_cluster_count, row.clusters.size());
    }
    const auto full_width_row_count = static_cast<std::size_t>(std::count_if(
        rows.begin(), rows.end(), [max_cluster_count](const CandidateLine &row) {
            return row.clusters.size() == max_cluster_count;
        }));
    const bool has_full_table_width_rows =
        max_cluster_count >= 3U && full_width_row_count >= 2U;

    for (const auto &row : rows) {
        if (has_full_table_width_rows &&
            row.clusters.size() < max_cluster_count) {
            continue;
        }
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

[[nodiscard]] bool is_short_header_label(const std::string &text) {
    if (text.empty() || text.size() > 24U) {
        return false;
    }

    bool has_alpha = false;
    for (unsigned char ch : text) {
        if (std::isdigit(ch)) {
            return false;
        }
        if (std::isalpha(ch)) {
            has_alpha = true;
        }
    }

    return has_alpha;
}

[[nodiscard]] bool is_data_like_cell_text(const std::string &text) {
    if (text.empty()) {
        return false;
    }

    for (unsigned char ch : text) {
        if (std::isdigit(ch)) {
            return true;
        }
    }

    return !is_short_header_label(text);
}

[[nodiscard]] bool is_conservative_two_row_table(
    const std::vector<CandidateLine> &rows) {
    if (rows.size() != 2U) {
        return false;
    }

    const auto &header_row = rows.front().clusters;
    const auto &data_row = rows.back().clusters;
    if (header_row.size() < 3U || data_row.size() < 3U) {
        return false;
    }

    if (!std::all_of(header_row.begin(), header_row.end(),
                     [](const TextCluster &cluster) {
                         return is_short_header_label(cluster.text);
                     })) {
        return false;
    }

    const auto data_like_count = static_cast<std::size_t>(std::count_if(
        data_row.begin(), data_row.end(), [](const TextCluster &cluster) {
            return is_data_like_cell_text(cluster.text);
        }));
    return data_like_count >= 1U;
}

[[nodiscard]] bool is_conservative_two_column_key_value_table(
    const std::vector<CandidateLine> &rows) {
    if (rows.size() < 3U) {
        return false;
    }

    std::size_t data_like_value_count = 0U;
    for (const auto &row : rows) {
        if (row.clusters.size() != 2U) {
            return false;
        }

        if (!is_short_header_label(row.clusters[0].text)) {
            return false;
        }

        if (is_data_like_cell_text(row.clusters[1].text)) {
            ++data_like_value_count;
        }
    }

    const auto minimum_data_values =
        (std::max)(std::size_t{2U}, (rows.size() + 1U) / 2U);
    return data_like_value_count >= minimum_data_values;
}

[[nodiscard]] bool is_conservative_irregular_header_table(
    const std::vector<CandidateLine> &rows,
    const std::vector<double> &anchors) {
    if (rows.size() < 3U || anchors.size() < 3U) {
        return false;
    }

    const auto &header_row = rows.front().clusters;
    if (header_row.size() != anchors.size()) {
        return false;
    }
    if (!std::all_of(header_row.begin(), header_row.end(),
                     [](const TextCluster &cluster) {
                         return is_short_header_label(cluster.text);
                     })) {
        return false;
    }

    const auto minimum_data_cells = (anchors.size() + 1U) / 2U;
    for (std::size_t row_index = 1U; row_index < rows.size(); ++row_index) {
        const auto &row = rows[row_index].clusters;
        if (row.size() != anchors.size()) {
            return false;
        }

        const auto data_like_count = static_cast<std::size_t>(std::count_if(
            row.begin(), row.end(), [](const TextCluster &cluster) {
                return is_data_like_cell_text(cluster.text);
            }));
        if (data_like_count < minimum_data_cells) {
            return false;
        }
    }

    return true;
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

[[nodiscard]] double average_column_gap(
    const std::vector<double> &anchors) noexcept {
    if (anchors.size() < 2U) {
        return 0.0;
    }

    return (anchors.back() - anchors.front()) /
           static_cast<double>(anchors.size() - 1U);
}

[[nodiscard]] std::size_t detect_column_span(
    const std::vector<double> &anchors, const TextCluster &cluster,
    std::size_t column_index) {
    if (anchors.size() < 2U || column_index >= anchors.size()) {
        return 1U;
    }

    const double average_gap = average_column_gap(anchors);
    if (average_gap <= 0.0 ||
        cluster.bounds.width_points < average_gap * 1.35) {
        return 1U;
    }

    // 仅对明显宽于单列的文本簇做最小跨列推断，避免把普通长单元格过度提升成合并单元格。
    const auto estimated_span = static_cast<std::size_t>(
        std::ceil(cluster.bounds.width_points / average_gap));
    if (estimated_span <= 1U) {
        return 1U;
    }

    return (std::min)(anchors.size() - column_index, estimated_span);
}

[[nodiscard]] double cluster_center_x(const TextCluster &cluster) noexcept {
    return cluster.bounds.x_points + cluster.bounds.width_points / 2.0;
}

struct GroupHeaderSpan final {
    std::size_t column_index{0U};
    std::size_t column_span{1U};
};

[[nodiscard]] std::optional<GroupHeaderSpan>
infer_group_header_span(const std::vector<double> &anchors,
                        const CandidateLine &row,
                        std::size_t cluster_index) {
    if (anchors.size() < 3U || row.clusters.empty() ||
        row.clusters.size() >= anchors.size() ||
        cluster_index >= row.clusters.size()) {
        return std::nullopt;
    }

    const double average_gap = average_column_gap(anchors);
    if (average_gap <= 0.0) {
        return std::nullopt;
    }

    const auto &cluster = row.clusters[cluster_index];
    const double cluster_center = cluster_center_x(cluster);
    const double table_center = (anchors.front() + anchors.back()) / 2.0;
    if (row.clusters.size() == 1U) {
        const double center_tolerance = (std::max)(36.0, average_gap * 0.4);
        if (std::abs(cluster_center - table_center) > center_tolerance) {
            return std::nullopt;
        }
    }

    double left_boundary = anchors.front() - average_gap;
    if (cluster_index > 0U) {
        left_boundary =
            (cluster_center_x(row.clusters[cluster_index - 1U]) +
             cluster_center) /
            2.0;
    }

    double right_boundary = anchors.back() + average_gap;
    if (cluster_index + 1U < row.clusters.size()) {
        right_boundary =
            (cluster_center +
             cluster_center_x(row.clusters[cluster_index + 1U])) /
            2.0;
    }

    std::optional<std::size_t> first_column;
    std::size_t covered_columns = 0U;
    const double boundary_tolerance = (std::max)(4.0, average_gap * 0.05);
    for (std::size_t column_index = 0U; column_index < anchors.size();
         ++column_index) {
        const double anchor = anchors[column_index];
        if (anchor + boundary_tolerance < left_boundary ||
            anchor - boundary_tolerance > right_boundary) {
            continue;
        }

        if (!first_column.has_value()) {
            first_column = column_index;
        }
        ++covered_columns;
    }

    if (!first_column.has_value() || covered_columns <= 1U) {
        return std::nullopt;
    }

    return GroupHeaderSpan{*first_column, covered_columns};
}

[[nodiscard]] std::vector<std::optional<GroupHeaderSpan>>
detect_group_header_row_spans(const std::vector<double> &anchors,
                              const CandidateLine &row) {
    std::vector<std::optional<GroupHeaderSpan>> spans;
    spans.reserve(row.clusters.size());

    std::size_t next_column = 0U;
    for (std::size_t cluster_index = 0U; cluster_index < row.clusters.size();
         ++cluster_index) {
        auto span = infer_group_header_span(anchors, row, cluster_index);
        if (!span.has_value() || span->column_span <= 1U ||
            span->column_index != next_column ||
            span->column_index + span->column_span > anchors.size()) {
            return {};
        }

        next_column = span->column_index + span->column_span;
        spans.push_back(span);
    }

    if (next_column != anchors.size()) {
        return {};
    }

    return spans;
}

[[nodiscard]] bool is_summary_label_text(std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            normalized.push_back(
                static_cast<char>(std::tolower(ch)));
        }
    }

    return normalized.find("total") != std::string::npos;
}

[[nodiscard]] std::optional<GroupHeaderSpan>
detect_summary_label_span(const std::vector<double> &anchors,
                          const CandidateLine &row,
                          std::size_t cluster_index,
                          std::size_t first_complete_row_index,
                          std::size_t row_index) {
    if (anchors.size() < 3U || row_index <= first_complete_row_index ||
        row.clusters.size() != 2U || cluster_index != 0U) {
        return std::nullopt;
    }

    const auto first_column =
        nearest_column_index(anchors, row.clusters.front().bounds.x_points);
    const auto last_column =
        nearest_column_index(anchors, row.clusters.back().bounds.x_points);
    if (first_column >= last_column) {
        return std::nullopt;
    }

    if (!is_summary_label_text(row.clusters.front().text) ||
        !is_short_header_label(row.clusters.front().text) ||
        !is_data_like_cell_text(row.clusters.back().text)) {
        return std::nullopt;
    }

    return GroupHeaderSpan{first_column, last_column - first_column};
}

[[nodiscard]] std::size_t count_text_cells(const PdfParsedTableRow &row) {
    return static_cast<std::size_t>(std::count_if(
        row.cells.begin(), row.cells.end(),
        [](const PdfParsedTableCell &cell) { return cell.has_text; }));
}

[[nodiscard]] bool is_column_covered_by_text_span(
    const PdfParsedTableRow &row, std::size_t column_index) {
    return std::any_of(row.cells.begin(), row.cells.end(),
                       [column_index](const PdfParsedTableCell &cell) {
                           return cell.has_text &&
                                  cell.column_index < column_index &&
                                  cell.column_index + cell.column_span >
                                      column_index;
                       });
}

[[nodiscard]] bool row_has_text_span(const PdfParsedTableRow &row) {
    return std::any_of(row.cells.begin(), row.cells.end(),
                       [](const PdfParsedTableCell &cell) {
                           return cell.has_text && cell.column_span > 1U;
                       });
}

[[nodiscard]] std::size_t detect_row_span(
    const std::vector<PdfParsedTableRow> &rows, std::size_t row_index,
    std::size_t column_index) {
    if (row_index >= rows.size()) {
        return 1U;
    }

    const auto &current_row = rows[row_index];
    if (current_row.cells.size() <= column_index) {
        return 1U;
    }

    const auto &anchor_cell = current_row.cells[column_index];
    if (!anchor_cell.has_text || count_text_cells(current_row) < 2U) {
        return 1U;
    }

    std::size_t row_span = 1U;
    for (std::size_t next_row_index = row_index + 1U;
         next_row_index < rows.size(); ++next_row_index) {
        const auto &next_row = rows[next_row_index];
        if (next_row.cells.size() <= column_index) {
            break;
        }
        if (count_text_cells(next_row) < 2U) {
            break;
        }

        const auto &continuation_cell = next_row.cells[column_index];
        if (continuation_cell.has_text) {
            break;
        }
        if (row_has_text_span(next_row)) {
            break;
        }
        if (is_column_covered_by_text_span(next_row, column_index)) {
            break;
        }

        ++row_span;
    }

    return row_span;
}

[[nodiscard]] bool build_table_candidate(
    const std::vector<CandidateLine> &rows, PdfParsedTableCandidate &candidate) {
    const bool is_two_row_table = is_conservative_two_row_table(rows);
    const bool is_two_column_key_value_table =
        is_conservative_two_column_key_value_table(rows);

    if (rows.size() < 3U && !is_two_row_table) {
        return false;
    }

    const auto anchors = build_column_anchors(rows);
    const bool has_regular_spacing = has_regular_column_spacing(anchors);
    const bool is_irregular_header_table =
        !has_regular_spacing &&
        is_conservative_irregular_header_table(rows, anchors);
    if (anchors.size() < 3U &&
        !(anchors.size() == 2U && is_two_column_key_value_table)) {
        return false;
    }
    if (!has_regular_spacing && !is_irregular_header_table) {
        return false;
    }

    candidate.column_anchor_x_points = anchors;
    candidate.rows.reserve(rows.size());

    const auto first_complete_column_row = std::find_if(
        rows.begin(), rows.end(), [&anchors](const CandidateLine &row) {
            return row.clusters.size() == anchors.size();
        });
    const std::optional<std::size_t> first_complete_column_row_index =
        first_complete_column_row == rows.end()
            ? std::nullopt
            : std::make_optional(static_cast<std::size_t>(
                  std::distance(rows.begin(), first_complete_column_row)));
    std::vector<std::vector<std::optional<GroupHeaderSpan>>> group_header_spans(
        rows.size());
    if (first_complete_column_row_index.has_value()) {
        for (std::size_t row_index = 0U;
             row_index < *first_complete_column_row_index;
             ++row_index) {
            auto row_spans =
                detect_group_header_row_spans(anchors, rows[row_index]);
            if (row_spans.empty()) {
                break;
            }
            group_header_spans[row_index] = std::move(row_spans);
        }
    }

    for (std::size_t source_row_index = 0U; source_row_index < rows.size();
         ++source_row_index) {
        const auto &source_row = rows[source_row_index];
        PdfParsedTableRow row;
        row.cells.reserve(anchors.size());
        for (std::size_t column_index = 0U; column_index < anchors.size();
             ++column_index) {
            PdfParsedTableCell cell;
            cell.column_index = column_index;
            row.cells.push_back(cell);
        }

        for (std::size_t cluster_index = 0U;
             cluster_index < source_row.clusters.size(); ++cluster_index) {
            const auto &cluster = source_row.clusters[cluster_index];
            std::optional<GroupHeaderSpan> group_header_span;
            if (source_row_index < group_header_spans.size() &&
                cluster_index < group_header_spans[source_row_index].size()) {
                group_header_span =
                    group_header_spans[source_row_index][cluster_index];
            }
            if (!group_header_span.has_value() &&
                first_complete_column_row_index.has_value()) {
                group_header_span = detect_summary_label_span(
                    anchors, source_row, cluster_index,
                    *first_complete_column_row_index, source_row_index);
            }
            const auto column_index =
                group_header_span.has_value()
                    ? group_header_span->column_index
                    : nearest_column_index(anchors, cluster.bounds.x_points);
            auto &cell = row.cells[column_index];
            cell.column_span =
                group_header_span.has_value()
                    ? group_header_span->column_span
                    : detect_column_span(anchors, cluster, column_index);
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

    for (std::size_t row_index = 0U; row_index < candidate.rows.size();
         ++row_index) {
        auto &row = candidate.rows[row_index];
        for (std::size_t column_index = 0U; column_index < row.cells.size();
             ++column_index) {
            row.cells[column_index].row_span =
                detect_row_span(candidate.rows, row_index, column_index);
        }
    }

    return !candidate.rows.empty();
}

[[nodiscard]] std::vector<PdfParsedTableCandidate>
detect_table_candidates(std::vector<PdfParsedTextLine> &lines) {
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

        std::vector<CandidateLine> rows;
        rows.reserve(run_end - gap_index + 2U);
        for (std::size_t row_index = gap_index; row_index <= run_end + 1U;
             ++row_index) {
            rows.push_back(candidate_lines[row_index]);
        }

        PdfParsedTableCandidate table;
        if (build_table_candidate(rows, table)) {
            const auto table_index = tables.size();
            for (const auto &row : rows) {
                lines[row.line_index].table_candidate_index = table_index;
            }
            tables.push_back(std::move(table));
            gap_index = run_end;
        }
    }

    return tables;
}

[[nodiscard]] std::vector<PdfParsedParagraph>
group_lines_into_paragraphs(const std::vector<PdfParsedTextLine> &lines) {
    std::vector<PdfParsedParagraph> paragraphs;
    std::optional<std::size_t> previous_table_candidate_index;
    for (const auto &line : lines) {
        const auto current_table_candidate_index = line.table_candidate_index;
        if (paragraphs.empty() ||
            should_start_new_paragraph(paragraphs.back().lines.back(), line,
                                       previous_table_candidate_index,
                                       current_table_candidate_index)) {
            paragraphs.emplace_back();
        }
        paragraphs.back().table_candidate_index = current_table_candidate_index;
        append_line_to_paragraph(paragraphs.back(), line);
        previous_table_candidate_index = current_table_candidate_index;
    }
    return paragraphs;
}

[[nodiscard]] std::vector<PdfParsedContentBlock> build_content_blocks(
    const PdfParsedPage &page) {
    std::vector<PdfParsedContentBlock> blocks;
    blocks.reserve(page.paragraphs.size() + page.table_candidates.size());

    std::vector<bool> emitted_tables(page.table_candidates.size(), false);
    for (std::size_t paragraph_index = 0U;
         paragraph_index < page.paragraphs.size(); ++paragraph_index) {
        const auto &paragraph = page.paragraphs[paragraph_index];
        if (paragraph.table_candidate_index.has_value() &&
            *paragraph.table_candidate_index < page.table_candidates.size()) {
            const auto table_index = *paragraph.table_candidate_index;
            if (!emitted_tables[table_index]) {
                blocks.push_back(PdfParsedContentBlock{
                    PdfParsedContentBlockKind::table_candidate,
                    page.table_candidates[table_index].bounds, table_index});
                emitted_tables[table_index] = true;
            }
            continue;
        }

        blocks.push_back(PdfParsedContentBlock{
            PdfParsedContentBlockKind::paragraph, paragraph.bounds,
            paragraph_index});
    }

    for (std::size_t table_index = 0U; table_index < page.table_candidates.size();
         ++table_index) {
        if (emitted_tables[table_index]) {
            continue;
        }

        blocks.push_back(PdfParsedContentBlock{
            PdfParsedContentBlockKind::table_candidate,
            page.table_candidates[table_index].bounds, table_index});
    }

    return blocks;
}

void enrich_text_structure(PdfParsedPage &page) {
    page.text_lines = group_text_spans_into_lines(page.text_spans);
    sort_lines_by_reading_order(page.text_lines);
    page.table_candidates = detect_table_candidates(page.text_lines);
    page.paragraphs = group_lines_into_paragraphs(page.text_lines);
    page.content_blocks = build_content_blocks(page);
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
