#include <featherdoc/pdf/pdf_document_importer.hpp>

#include <featherdoc/pdf/pdf_parser.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <utility>
#include <vector>

namespace featherdoc::pdf {
namespace {

constexpr double kCrossPageTableTopOffsetThresholdPoints = 160.0;

[[nodiscard]] bool has_importable_text(const PdfParsedParagraph &paragraph) {
    return !paragraph.text.empty();
}

[[nodiscard]] double rect_right(const PdfRect &rect) noexcept {
    return rect.x_points + rect.width_points;
}

[[nodiscard]] double rect_top(const PdfRect &rect) noexcept {
    return rect.y_points + rect.height_points;
}

[[nodiscard]] bool rects_intersect(const PdfRect &left,
                                   const PdfRect &right) noexcept {
    return left.x_points < rect_right(right) &&
           rect_right(left) > right.x_points &&
           left.y_points < rect_top(right) &&
           rect_top(left) > right.y_points;
}

[[nodiscard]] double rect_area(const PdfRect &rect) noexcept {
    return (std::max)(rect.width_points, 0.0) *
           (std::max)(rect.height_points, 0.0);
}

[[nodiscard]] double rect_intersection_area(const PdfRect &left,
                                            const PdfRect &right) noexcept {
    if (!rects_intersect(left, right)) {
        return 0.0;
    }

    const double intersection_left = (std::max)(left.x_points, right.x_points);
    const double intersection_right =
        (std::min)(rect_right(left), rect_right(right));
    const double intersection_bottom = (std::max)(left.y_points, right.y_points);
    const double intersection_top = (std::min)(rect_top(left), rect_top(right));
    return (std::max)(intersection_right - intersection_left, 0.0) *
           (std::max)(intersection_top - intersection_bottom, 0.0);
}

[[nodiscard]] bool rect_contains_point(const PdfRect &rect, double x_points,
                                       double y_points) noexcept {
    return x_points >= rect.x_points && x_points <= rect_right(rect) &&
           y_points >= rect.y_points && y_points <= rect_top(rect);
}

void expand_rect(PdfRect &target, const PdfRect &source) noexcept {
    if (target.width_points <= 0.0 && target.height_points <= 0.0) {
        target = source;
        return;
    }

    const double left = (std::min)(target.x_points, source.x_points);
    const double bottom = (std::min)(target.y_points, source.y_points);
    const double right = (std::max)(rect_right(target), rect_right(source));
    const double top = (std::max)(rect_top(target), rect_top(source));
    target.x_points = left;
    target.y_points = bottom;
    target.width_points = right - left;
    target.height_points = top - bottom;
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

[[nodiscard]] bool paragraph_meaningfully_overlaps_rect(
    const PdfParsedParagraph &paragraph, const PdfRect &rect) noexcept {
    if (!paragraph.lines.empty()) {
        std::size_t overlapping_line_count = 0U;
        for (const auto &line : paragraph.lines) {
            if (rect_meaningfully_overlaps_rect(line.bounds, rect)) {
                ++overlapping_line_count;
            }
        }
        return overlapping_line_count > paragraph.lines.size() / 2U;
    }

    return rect_meaningfully_overlaps_rect(paragraph.bounds, rect);
}

[[nodiscard]] bool has_table_candidates(const PdfParsedDocument &document) {
    for (const auto &page : document.pages) {
        if (!page.table_candidates.empty()) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool overlaps_table_candidate(
    const PdfParsedParagraph &paragraph, const PdfParsedPage &page) {
    if (paragraph.table_candidate_index.has_value() &&
        *paragraph.table_candidate_index < page.table_candidates.size()) {
        return true;
    }

    return std::any_of(page.table_candidates.begin(),
                       page.table_candidates.end(),
                       [&paragraph](const PdfParsedTableCandidate &table) {
                           return std::any_of(
                               table.rows.begin(), table.rows.end(),
                               [&paragraph](const PdfParsedTableRow &row) {
                                   return paragraph_meaningfully_overlaps_rect(
                                       paragraph, row.bounds);
                               });
                       });
}

[[nodiscard]] bool line_overlaps_table_candidate(const PdfParsedTextLine &line,
                                                 const PdfParsedPage &page) {
    if (line.table_candidate_index.has_value() &&
        *line.table_candidate_index < page.table_candidates.size()) {
        return true;
    }

    return std::any_of(page.table_candidates.begin(),
                       page.table_candidates.end(),
                       [&line](const PdfParsedTableCandidate &table) {
                           return std::any_of(
                               table.rows.begin(), table.rows.end(),
                               [&line](const PdfParsedTableRow &row) {
                                   return rect_meaningfully_overlaps_rect(
                                       line.bounds, row.bounds);
                               });
                       });
}

struct ParagraphFragment {
    std::string text;
    PdfRect bounds;
};

[[nodiscard]] std::vector<ParagraphFragment> build_paragraph_fragments(
    const PdfParsedParagraph &paragraph, const PdfParsedPage &page,
    bool import_table_candidates_as_tables) {
    if (!has_importable_text(paragraph)) {
        return {};
    }

    if (!import_table_candidates_as_tables || page.table_candidates.empty()) {
        return {{paragraph.text, paragraph.bounds}};
    }

    if (paragraph.lines.empty()) {
        return overlaps_table_candidate(paragraph, page)
                   ? std::vector<ParagraphFragment>{}
                   : std::vector<ParagraphFragment>{
                         ParagraphFragment{paragraph.text, paragraph.bounds}};
    }

    const bool has_overlapping_line = std::any_of(
        paragraph.lines.begin(), paragraph.lines.end(),
        [&page](const PdfParsedTextLine &line) {
            return line_overlaps_table_candidate(line, page);
        });

    // 只有当段内存在明确落在表格里的行时，才按行切分并剔除重叠行；
    // 纯边界擦边的正文段落应优先作为完整段落保留，避免误丢正文。
    if (!has_overlapping_line) {
        return {{paragraph.text, paragraph.bounds}};
    }

    std::vector<ParagraphFragment> fragments;
    std::optional<ParagraphFragment> current_fragment;
    for (const auto &line : paragraph.lines) {
        if (line_overlaps_table_candidate(line, page)) {
            if (current_fragment.has_value()) {
                fragments.push_back(std::move(*current_fragment));
                current_fragment.reset();
            }
            continue;
        }

        if (!current_fragment.has_value()) {
            current_fragment = ParagraphFragment{line.text, line.bounds};
            continue;
        }

        current_fragment->text.push_back('\n');
        current_fragment->text += line.text;
        expand_rect(current_fragment->bounds, line.bounds);
    }

    if (current_fragment.has_value()) {
        fragments.push_back(std::move(*current_fragment));
    }

    return fragments;
}

[[nodiscard]] bool append_imported_paragraph(featherdoc::Paragraph &cursor,
                                             bool &has_written_paragraph,
                                             const std::string &text) {
    if (!has_written_paragraph) {
        if (!cursor.has_next()) {
            return false;
        }
        if (!cursor.set_text(text)) {
            return false;
        }
        has_written_paragraph = true;
        return true;
    }

    cursor = cursor.insert_paragraph_after(text);
    return cursor.has_next();
}

struct ImportBlock {
    enum class Kind { paragraph, table };

    Kind kind{Kind::paragraph};
    double top_points{0.0};
    double left_points{0.0};
    std::string paragraph_text;
    const PdfParsedTableCandidate *table{nullptr};
};

struct ImportCursor {
    featherdoc::Paragraph paragraph;
    std::optional<featherdoc::Table> table;
    std::size_t table_row_count{0U};
    std::size_t table_column_count{0U};
    std::vector<double> table_column_anchor_x_points;
    std::vector<std::string> table_header_cell_texts;
    bool has_written_block{false};
};

enum class TableAppendResult { failed, created, merged };

[[nodiscard]] std::vector<ImportBlock> build_import_blocks(
    const PdfParsedPage &page, bool import_table_candidates_as_tables) {
    std::vector<ImportBlock> blocks;
    blocks.reserve(page.paragraphs.size() + page.table_candidates.size());

    if (!page.content_blocks.empty()) {
        for (const auto &content_block : page.content_blocks) {
            if (content_block.kind ==
                PdfParsedContentBlockKind::paragraph) {
                if (content_block.source_index >= page.paragraphs.size()) {
                    continue;
                }

                const auto &paragraph = page.paragraphs[content_block.source_index];
                for (auto &fragment : build_paragraph_fragments(
                         paragraph, page, import_table_candidates_as_tables)) {
                    blocks.push_back(ImportBlock{
                        ImportBlock::Kind::paragraph, rect_top(fragment.bounds),
                        fragment.bounds.x_points, std::move(fragment.text),
                        nullptr});
                }
                continue;
            }

            if (!import_table_candidates_as_tables ||
                content_block.source_index >= page.table_candidates.size()) {
                continue;
            }

            const auto &table = page.table_candidates[content_block.source_index];
            blocks.push_back(ImportBlock{
                ImportBlock::Kind::table, rect_top(table.bounds),
                table.bounds.x_points, {}, &table});
        }
        return blocks;
    }

    for (const auto &paragraph : page.paragraphs) {
        for (auto &fragment : build_paragraph_fragments(
                 paragraph, page, import_table_candidates_as_tables)) {
            blocks.push_back(ImportBlock{
                ImportBlock::Kind::paragraph, rect_top(fragment.bounds),
                fragment.bounds.x_points, std::move(fragment.text), nullptr});
        }
    }

    if (import_table_candidates_as_tables) {
        for (const auto &table : page.table_candidates) {
            blocks.push_back(ImportBlock{
                ImportBlock::Kind::table, rect_top(table.bounds),
                table.bounds.x_points, {}, &table});
        }
    }

    std::stable_sort(blocks.begin(), blocks.end(),
                     [](const ImportBlock &left, const ImportBlock &right) {
                         const auto left_top_key =
                             std::llround(left.top_points * 100.0);
                         const auto right_top_key =
                             std::llround(right.top_points * 100.0);
                         if (left_top_key != right_top_key) {
                             return left_top_key > right_top_key;
                         }

                         const auto left_x_key =
                             std::llround(left.left_points * 100.0);
                         const auto right_x_key =
                             std::llround(right.left_points * 100.0);
                         if (left_x_key != right_x_key) {
                             return left_x_key < right_x_key;
                         }

                         return false;
                     });
    return blocks;
}

[[nodiscard]] std::uint32_t points_to_twips(double points) {
    const auto rounded = std::lround((std::max)(points, 1.0) * 20.0);
    return static_cast<std::uint32_t>((std::max)(rounded, 1L));
}

[[nodiscard]] std::uint32_t table_width_twips(
    const PdfParsedTableCandidate &table) {
    if (table.column_anchor_x_points.size() >= 2U) {
        const double first = table.column_anchor_x_points.front();
        const double last = table.column_anchor_x_points.back();
        const double average_gap =
            (last - first) /
            static_cast<double>(table.column_anchor_x_points.size() - 1U);
        return points_to_twips(
            (std::max)(table.bounds.width_points,
                       average_gap *
                           static_cast<double>(
                               table.column_anchor_x_points.size())));
    }

    return points_to_twips(table.bounds.width_points);
}

[[nodiscard]] std::uint32_t table_row_height_twips(
    const PdfParsedTableCandidate &table, const PdfParsedTableRow &row) {
    const double average_row_height_points =
        table.rows.empty()
            ? row.bounds.height_points
            : table.bounds.height_points /
                  static_cast<double>(table.rows.size());
    return points_to_twips(
        (std::max)(row.bounds.height_points, average_row_height_points));
}

[[nodiscard]] std::uint32_t column_width_twips(
    const PdfParsedTableCandidate &table, std::size_t column_index) {
    if (table.column_anchor_x_points.size() >= 2U) {
        if (column_index + 1U < table.column_anchor_x_points.size()) {
            return points_to_twips(table.column_anchor_x_points[column_index + 1U] -
                                   table.column_anchor_x_points[column_index]);
        }

        return points_to_twips(
            table.column_anchor_x_points[column_index] -
            table.column_anchor_x_points[column_index - 1U]);
    }

    const auto column_count =
        table.rows.empty() ? 1U : table.rows.front().cells.size();
    return points_to_twips(table.bounds.width_points /
                           static_cast<double>(
                               (std::max)(column_count, std::size_t{1U})));
}

[[nodiscard]] std::size_t table_column_count(
    const PdfParsedTableCandidate &source) {
    return source.rows.empty() ? 0U : source.rows.front().cells.size();
}

[[nodiscard]] bool has_consistent_row_widths(
    const PdfParsedTableCandidate &source, std::size_t column_count) {
    if (source.rows.empty() || column_count == 0U) {
        return false;
    }

    return std::all_of(source.rows.begin(), source.rows.end(),
                       [column_count](const PdfParsedTableRow &row) {
                           return row.cells.size() == column_count;
                       });
}

[[nodiscard]] bool have_compatible_column_anchors(
    const std::vector<double> &left, const std::vector<double> &right) {
    if (left.size() != right.size()) {
        return false;
    }
    if (left.size() < 2U) {
        return left.size() == right.size();
    }

    for (std::size_t index = 1U; index < left.size(); ++index) {
        const double left_gap = left[index] - left[index - 1U];
        const double right_gap = right[index] - right[index - 1U];
        const double tolerance = (std::max)(8.0, std::abs(left_gap) * 0.10);
        if (std::abs(left_gap - right_gap) > tolerance) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::size_t count_text_cells(const PdfParsedTableRow &row) {
    std::size_t count = 0U;
    for (const auto &cell : row.cells) {
        if (cell.has_text) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::size_t count_text_cell_coverage(
    const PdfParsedTableRow &row) {
    std::size_t coverage = 0U;
    for (const auto &cell : row.cells) {
        if (!cell.has_text) {
            continue;
        }
        coverage += (std::max)(cell.column_span, std::size_t{1U});
    }
    return coverage;
}

[[nodiscard]] bool apply_imported_table_column_spans(featherdoc::Table &table,
                                                     std::size_t row_index,
                                                     const PdfParsedTableRow &row) {
    for (const auto &cell : row.cells) {
        if (!cell.has_text || cell.column_span <= 1U) {
            continue;
        }

        auto imported_cell = table.find_cell_by_grid_column(row_index,
                                                            cell.column_index);
        if (!imported_cell.has_value() ||
            !imported_cell->merge_right(cell.column_span - 1U)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool apply_imported_table_row_span_column_spans(
    featherdoc::Table &table, std::size_t start_row_index,
    const PdfParsedTableCandidate &source, std::size_t source_row_offset,
    std::size_t source_row_count) {
    if (source_row_offset > source.rows.size() ||
        source_row_count > source.rows.size() - source_row_offset) {
        return false;
    }

    for (std::size_t local_row_index = 0U; local_row_index < source_row_count;
         ++local_row_index) {
        const auto &row = source.rows[source_row_offset + local_row_index];
        for (const auto &cell : row.cells) {
            if (!cell.has_text || cell.row_span <= 1U || cell.column_span <= 1U) {
                continue;
            }

            for (std::size_t row_offset = 1U; row_offset < cell.row_span;
                 ++row_offset) {
                if (local_row_index + row_offset >= source_row_count) {
                    return false;
                }

                const auto target_row_index =
                    start_row_index + local_row_index + row_offset;
                auto target_cell = table.find_cell_by_grid_column(
                    target_row_index, cell.column_index);
                if (!target_cell.has_value() ||
                    !target_cell->merge_right(cell.column_span - 1U)) {
                    return false;
                }
            }
        }
    }

    return true;
}

[[nodiscard]] bool apply_imported_table_row_spans(
    featherdoc::Table &table, std::size_t start_row_index,
    const PdfParsedTableCandidate &source, std::size_t source_row_offset,
    std::size_t source_row_count) {
    if (source_row_offset > source.rows.size() ||
        source_row_count > source.rows.size() - source_row_offset) {
        return false;
    }

    for (std::size_t local_row_index = 0U; local_row_index < source_row_count;
         ++local_row_index) {
        const auto source_row_index = source_row_offset + local_row_index;
        const auto &row = source.rows[source_row_index];
        for (const auto &cell : row.cells) {
            if (!cell.has_text || cell.row_span <= 1U) {
                continue;
            }

            auto imported_cell = table.find_cell_by_grid_column(
                start_row_index + local_row_index, cell.column_index);
            if (!imported_cell.has_value() ||
                !imported_cell->merge_down(cell.row_span - 1U)) {
                return false;
            }
        }
    }

    return true;
}

[[nodiscard]] double average_text_length(const PdfParsedTableRow &row) {
    std::size_t text_cell_count = 0U;
    std::size_t text_length = 0U;

    for (const auto &cell : row.cells) {
        if (!cell.has_text) {
            continue;
        }
        ++text_cell_count;
        text_length += cell.text.size();
    }

    if (text_cell_count == 0U) {
        return 0.0;
    }

    return static_cast<double>(text_length) /
           static_cast<double>(text_cell_count);
}

[[nodiscard]] std::vector<std::string> collect_row_cell_texts(
    const PdfParsedTableRow &row) {
    std::vector<std::string> texts;
    texts.reserve(row.cells.size());
    for (const auto &cell : row.cells) {
        texts.push_back(cell.has_text ? cell.text : std::string{});
    }
    return texts;
}

[[nodiscard]] std::string normalize_header_match_text(
    std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());

    bool pending_space = false;
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            if (pending_space) {
                normalized.push_back(' ');
                pending_space = false;
            }
            normalized.push_back(
                static_cast<char>(std::tolower(ch)));
            continue;
        }

        if (std::isspace(ch) || ch == '-' || ch == '_' || ch == '/' ||
            ch == '\\' || ch == ':' || ch == '.' || ch == ',' ||
            ch == ';' || ch == '(' || ch == ')' || ch == '[' ||
            ch == ']') {
            pending_space = !normalized.empty();
            continue;
        }

        if (pending_space) {
            normalized.push_back(' ');
            pending_space = false;
        }
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }

    return normalized;
}

[[nodiscard]] std::string canonicalize_header_match_token(
    std::string_view token) {
    if (token == "qty" || token == "quantity" ||
        token == "quantities") {
        return "quantity";
    }
    if (token == "amt" || token == "amount" || token == "amounts") {
        return "amount";
    }
    if (token == "desc" || token == "description" ||
        token == "descriptions") {
        return "description";
    }
    if (token == "no" || token == "num" || token == "number" ||
        token == "numbers") {
        return "number";
    }

    return std::string{token};
}

[[nodiscard]] std::vector<std::string> collect_canonical_header_match_tokens(
    std::string_view text) {
    std::vector<std::string> tokens;

    std::size_t token_start = 0U;
    while (token_start < text.size()) {
        const auto token_end = text.find(' ', token_start);
        const auto token =
            text.substr(token_start,
                        token_end == std::string_view::npos
                            ? std::string_view::npos
                            : token_end - token_start);
        if (!token.empty()) {
            tokens.push_back(canonicalize_header_match_token(token));
        }

        if (token_end == std::string_view::npos) {
            break;
        }
        token_start = token_end + 1U;
    }

    return tokens;
}

[[nodiscard]] std::string canonicalize_header_match_text(
    std::string_view text) {
    const auto tokens = collect_canonical_header_match_tokens(text);
    std::string canonical;
    canonical.reserve(text.size());
    for (const auto &token : tokens) {
        if (!canonical.empty()) {
            canonical.push_back(' ');
        }
        canonical += token;
    }

    return canonical;
}

[[nodiscard]] bool header_texts_match_exact_or_plural_variant(
    std::string_view left, std::string_view right) {
    if (left == right) {
        return true;
    }

    const auto shorter = left.size() < right.size() ? left : right;
    const auto longer = left.size() < right.size() ? right : left;
    if (shorter.size() < 2U ||
        longer.compare(0U, shorter.size(), shorter) != 0) {
        return false;
    }

    const auto suffix = longer.substr(shorter.size());
    return suffix == "s" || suffix == "es";
}

[[nodiscard]] bool header_tokens_match_in_any_order(std::string_view left,
                                                    std::string_view right) {
    auto left_tokens = collect_canonical_header_match_tokens(left);
    auto right_tokens = collect_canonical_header_match_tokens(right);
    if (left_tokens.size() != right_tokens.size() ||
        left_tokens.size() < 2U || left_tokens.size() > 4U) {
        return false;
    }

    std::sort(left_tokens.begin(), left_tokens.end());
    std::sort(right_tokens.begin(), right_tokens.end());
    return left_tokens == right_tokens;
}

[[nodiscard]] int header_match_kind_rank(
    PdfTableContinuationHeaderMatchKind kind) {
    switch (kind) {
    case PdfTableContinuationHeaderMatchKind::none:
    case PdfTableContinuationHeaderMatchKind::not_required:
        return 0;
    case PdfTableContinuationHeaderMatchKind::exact:
        return 1;
    case PdfTableContinuationHeaderMatchKind::normalized_text:
        return 2;
    case PdfTableContinuationHeaderMatchKind::plural_variant:
        return 3;
    case PdfTableContinuationHeaderMatchKind::canonical_text:
        return 4;
    case PdfTableContinuationHeaderMatchKind::token_set:
        return 5;
    }

    return 0;
}

[[nodiscard]] PdfTableContinuationHeaderMatchKind strongest_header_match_kind(
    PdfTableContinuationHeaderMatchKind left,
    PdfTableContinuationHeaderMatchKind right) {
    return header_match_kind_rank(left) >= header_match_kind_rank(right)
               ? left
               : right;
}

[[nodiscard]] PdfTableContinuationHeaderMatchKind header_cell_text_match_kind(
    std::string_view left, std::string_view right) {
    if (left == right) {
        return PdfTableContinuationHeaderMatchKind::exact;
    }

    const auto normalized_left = normalize_header_match_text(left);
    const auto normalized_right = normalize_header_match_text(right);
    if (normalized_left == normalized_right) {
        return PdfTableContinuationHeaderMatchKind::normalized_text;
    }
    if (header_texts_match_exact_or_plural_variant(normalized_left,
                                                   normalized_right)) {
        return PdfTableContinuationHeaderMatchKind::plural_variant;
    }

    const auto canonical_left = canonicalize_header_match_text(normalized_left);
    const auto canonical_right =
        canonicalize_header_match_text(normalized_right);
    if (canonical_left == canonical_right ||
        header_texts_match_exact_or_plural_variant(canonical_left,
                                                   canonical_right)) {
        return PdfTableContinuationHeaderMatchKind::canonical_text;
    }
    if (header_tokens_match_in_any_order(canonical_left, canonical_right)) {
        return PdfTableContinuationHeaderMatchKind::token_set;
    }

    return PdfTableContinuationHeaderMatchKind::none;
}

[[nodiscard]] bool is_header_label_text(std::string_view text) {
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

[[nodiscard]] bool is_header_label_cell(const PdfParsedTableCell &cell) {
    if (!cell.has_text || cell.text.empty()) {
        return false;
    }

    const auto max_text_length = cell.column_span > 1U ? 48U : 24U;
    if (cell.text.size() > max_text_length) {
        return false;
    }

    bool has_alpha = false;
    for (unsigned char ch : cell.text) {
        if (std::isdigit(ch)) {
            return false;
        }
        if (std::isalpha(ch)) {
            has_alpha = true;
        }
    }

    return has_alpha;
}

[[nodiscard]] bool should_mark_repeating_header_row(
    const PdfParsedTableCandidate &source) {
    if (source.rows.size() < 3U) {
        return false;
    }

    const auto &header_row = source.rows.front();
    if (header_row.cells.size() < 3U) {
        return false;
    }
    const auto header_text_cell_count = count_text_cells(header_row);
    if (header_text_cell_count < 2U) {
        return false;
    }
    if (header_text_cell_count != header_row.cells.size() &&
        count_text_cell_coverage(header_row) < header_row.cells.size()) {
        return false;
    }
    if (!std::all_of(header_row.cells.begin(), header_row.cells.end(),
                     [](const PdfParsedTableCell &cell) {
                         return !cell.has_text ||
                                is_header_label_cell(cell);
                     })) {
        return false;
    }

    const bool header_has_spanning_cell = std::any_of(
        header_row.cells.begin(), header_row.cells.end(),
        [](const PdfParsedTableCell &cell) {
            return cell.has_text && cell.column_span > 1U;
        });
    if (header_has_spanning_cell) {
        return std::all_of(source.rows.begin() + 1U, source.rows.end(),
                           [](const PdfParsedTableRow &row) {
                               return count_text_cells(row) >= 2U;
                           });
    }

    const double header_average_length = average_text_length(header_row);
    if (header_average_length <= 0.0 || header_average_length > 18.0) {
        return false;
    }

    std::size_t body_text_cell_count = 0U;
    std::size_t body_text_length = 0U;
    std::size_t body_longer_than_header_count = 0U;
    std::size_t body_rows_with_longer_text = 0U;
    for (std::size_t row_index = 1U; row_index < source.rows.size();
         ++row_index) {
        bool row_has_longer_text = false;
        for (const auto &cell : source.rows[row_index].cells) {
            if (!cell.has_text) {
                continue;
            }
            ++body_text_cell_count;
            body_text_length += cell.text.size();
            if (cell.text.size() >=
                static_cast<std::size_t>(std::ceil(header_average_length + 2.0))) {
                ++body_longer_than_header_count;
                row_has_longer_text = true;
            }
        }
        if (row_has_longer_text) {
            ++body_rows_with_longer_text;
        }
    }

    if (body_text_cell_count == 0U) {
        return false;
    }

    const double body_average_length =
        static_cast<double>(body_text_length) /
        static_cast<double>(body_text_cell_count);
    return body_average_length >= header_average_length * 1.5 ||
           (body_average_length >= header_average_length * 1.15 &&
            body_rows_with_longer_text >= 2U &&
            body_longer_than_header_count >= header_row.cells.size());
}

[[nodiscard]] PdfTableContinuationHeaderMatchKind row_cell_texts_match_kind(
    const std::vector<std::string> &left, const PdfParsedTableRow &right) {
    if (left.size() != right.cells.size()) {
        return PdfTableContinuationHeaderMatchKind::none;
    }

    auto match_kind = PdfTableContinuationHeaderMatchKind::exact;
    for (std::size_t cell_index = 0U; cell_index < left.size(); ++cell_index) {
        const auto &right_cell = right.cells[cell_index];
        const auto right_text =
            right_cell.has_text ? right_cell.text : std::string{};
        const auto cell_match_kind =
            header_cell_text_match_kind(left[cell_index], right_text);
        if (cell_match_kind == PdfTableContinuationHeaderMatchKind::none) {
            return PdfTableContinuationHeaderMatchKind::none;
        }
        match_kind = strongest_header_match_kind(match_kind, cell_match_kind);
    }

    return match_kind;
}

[[nodiscard]] bool configure_imported_table(featherdoc::Table &table,
                                            const PdfParsedTableCandidate &source) {
    const auto column_count = table_column_count(source);
    if (!has_consistent_row_widths(source, column_count)) {
        return false;
    }

    if (!table.has_next()) {
        return false;
    }

    if (!table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_width_twips(table_width_twips(source))) {
        return false;
    }

    for (std::size_t column_index = 0U; column_index < column_count;
         ++column_index) {
        if (!table.set_column_width_twips(
                column_index, column_width_twips(source, column_index))) {
            return false;
        }
    }

    for (std::size_t row_index = 0U; row_index < source.rows.size();
         ++row_index) {
        const auto &row = source.rows[row_index];
        if (row.cells.size() != column_count) {
            return false;
        }

        for (const auto &cell : row.cells) {
            if (cell.has_text &&
                !table.set_cell_text_by_grid_column(row_index,
                                                    cell.column_index,
                                                    cell.text)) {
                return false;
            }
        }

        auto row_handle = table.find_row(row_index);
        if (!row_handle.has_value() ||
            !row_handle->set_height_twips(
                table_row_height_twips(source, row),
                featherdoc::row_height_rule::exact)) {
            return false;
        }

        if (!apply_imported_table_column_spans(table, row_index, row)) {
            return false;
        }
    }

    if (!apply_imported_table_row_span_column_spans(table, 0U, source, 0U,
                                                    source.rows.size())) {
        return false;
    }

    if (!apply_imported_table_row_spans(table, 0U, source, 0U,
                                        source.rows.size())) {
        return false;
    }

    if (should_mark_repeating_header_row(source)) {
        auto header_row = table.find_row(0U);
        if (!header_row.has_value() || !header_row->set_repeats_header()) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::uint32_t continuation_confidence(
    const PdfTableContinuationDiagnostic &diagnostic) {
    switch (diagnostic.blocker) {
    case PdfTableContinuationBlocker::none:
        return diagnostic.skipped_repeating_header ? 95U : 85U;
    case PdfTableContinuationBlocker::no_previous_table:
        return 0U;
    case PdfTableContinuationBlocker::not_first_block_on_page:
        return 35U;
    case PdfTableContinuationBlocker::not_near_page_top:
        return 45U;
    case PdfTableContinuationBlocker::inconsistent_source_rows:
        return 25U;
    case PdfTableContinuationBlocker::column_count_mismatch:
        return 30U;
    case PdfTableContinuationBlocker::column_anchors_mismatch:
        return 55U;
    case PdfTableContinuationBlocker::repeated_header_mismatch:
        return 70U;
    case PdfTableContinuationBlocker::continuation_confidence_below_threshold:
        return diagnostic.continuation_confidence;
    }

    return 0U;
}

[[nodiscard]] PdfTableContinuationDiagnostic evaluate_table_continuation(
    const ImportCursor &cursor, std::size_t page_index, std::size_t block_index,
    double block_top_points, double page_height_points,
    const PdfParsedTableCandidate &source,
    std::uint32_t min_continuation_confidence) {
    PdfTableContinuationDiagnostic diagnostic;
    diagnostic.page_index = page_index;
    diagnostic.block_index = block_index;
    diagnostic.minimum_continuation_confidence = min_continuation_confidence;
    diagnostic.is_first_block_on_page = block_index == 0U;
    diagnostic.is_near_page_top =
        block_top_points >= 0.0 &&
        (page_height_points - block_top_points) <=
            kCrossPageTableTopOffsetThresholdPoints;
    diagnostic.has_previous_table =
        cursor.table.has_value() && cursor.table_row_count > 0U;
    diagnostic.previous_has_repeating_header =
        !cursor.table_header_cell_texts.empty();
    diagnostic.source_has_repeating_header =
        !source.rows.empty() && should_mark_repeating_header_row(source);

    const auto column_count = table_column_count(source);
    diagnostic.source_rows_consistent =
        has_consistent_row_widths(source, column_count);
    diagnostic.column_count_matches =
        diagnostic.has_previous_table &&
        cursor.table_column_count == column_count;
    diagnostic.column_anchors_match =
        diagnostic.has_previous_table &&
        have_compatible_column_anchors(cursor.table_column_anchor_x_points,
                                       source.column_anchor_x_points);
    if (!diagnostic.previous_has_repeating_header ||
        !diagnostic.source_has_repeating_header) {
        diagnostic.header_match_kind =
            PdfTableContinuationHeaderMatchKind::not_required;
    } else {
        diagnostic.header_match_kind = row_cell_texts_match_kind(
            cursor.table_header_cell_texts, source.rows.front());
    }
    diagnostic.header_matches_previous =
        diagnostic.header_match_kind != PdfTableContinuationHeaderMatchKind::none;

    if (!diagnostic.has_previous_table) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::no_previous_table;
    } else if (!diagnostic.is_first_block_on_page) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker =
            PdfTableContinuationBlocker::not_first_block_on_page;
    } else if (!diagnostic.is_near_page_top) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::not_near_page_top;
    } else if (!diagnostic.source_rows_consistent) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::inconsistent_source_rows;
    } else if (!diagnostic.column_count_matches) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::column_count_mismatch;
    } else if (!diagnostic.column_anchors_match) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::column_anchors_mismatch;
    } else if (diagnostic.previous_has_repeating_header &&
               diagnostic.source_has_repeating_header &&
               !diagnostic.header_matches_previous) {
        diagnostic.disposition = PdfTableContinuationDisposition::created_new_table;
        diagnostic.blocker = PdfTableContinuationBlocker::repeated_header_mismatch;
    } else {
        diagnostic.disposition =
            PdfTableContinuationDisposition::merged_with_previous_table;
        diagnostic.source_row_offset =
            diagnostic.previous_has_repeating_header &&
                    diagnostic.source_has_repeating_header &&
                    diagnostic.header_matches_previous
                ? 1U
                : 0U;
        diagnostic.skipped_repeating_header =
            diagnostic.source_row_offset > 0U;
        diagnostic.blocker = PdfTableContinuationBlocker::none;
    }

    diagnostic.continuation_confidence = continuation_confidence(diagnostic);
    if (diagnostic.disposition ==
            PdfTableContinuationDisposition::merged_with_previous_table &&
        diagnostic.continuation_confidence < min_continuation_confidence) {
        diagnostic.disposition =
            PdfTableContinuationDisposition::created_new_table;
        diagnostic.source_row_offset = 0U;
        diagnostic.skipped_repeating_header = false;
        diagnostic.blocker =
            PdfTableContinuationBlocker::continuation_confidence_below_threshold;
    }

    return diagnostic;
}

[[nodiscard]] bool append_rows_to_imported_table(
    featherdoc::Table &table, std::size_t start_row_index,
    std::size_t column_count, const PdfParsedTableCandidate &source,
    std::size_t source_row_offset, std::size_t &appended_row_count) {
    if (!has_consistent_row_widths(source, column_count)) {
        return false;
    }

    if (source_row_offset > source.rows.size()) {
        return false;
    }

    appended_row_count = 0U;
    for (std::size_t row_index = source_row_offset; row_index < source.rows.size();
         ++row_index) {
        auto appended_row = table.append_row(column_count);
        if (!appended_row.has_next()) {
            return false;
        }

        const auto &row = source.rows[row_index];
        for (const auto &cell : row.cells) {
            if (cell.has_text &&
                !table.set_cell_text_by_grid_column(
                    start_row_index + appended_row_count, cell.column_index,
                    cell.text)) {
                return false;
            }
        }

        if (!appended_row.set_height_twips(
                table_row_height_twips(source, row),
                featherdoc::row_height_rule::exact)) {
            return false;
        }

        if (!apply_imported_table_column_spans(
                table, start_row_index + appended_row_count, row)) {
            return false;
        }

        ++appended_row_count;
    }

    if (!apply_imported_table_row_span_column_spans(
            table, start_row_index, source, source_row_offset,
            appended_row_count)) {
        return false;
    }

    if (!apply_imported_table_row_spans(table, start_row_index, source,
                                        source_row_offset, appended_row_count)) {
        return false;
    }

    return true;
}

[[nodiscard]] bool append_imported_paragraph(ImportCursor &cursor,
                                             const std::string &text) {
    if (!cursor.has_written_block) {
        if (!cursor.paragraph.has_next()) {
            return false;
        }
        if (!cursor.paragraph.set_text(text)) {
            return false;
        }
        cursor.has_written_block = true;
        cursor.table.reset();
        cursor.table_row_count = 0U;
        cursor.table_column_count = 0U;
        cursor.table_column_anchor_x_points.clear();
        cursor.table_header_cell_texts.clear();
        return true;
    }

    if (cursor.table.has_value()) {
        cursor.paragraph = cursor.table->insert_paragraph_after(text);
    } else {
        cursor.paragraph = cursor.paragraph.insert_paragraph_after(text);
    }

    if (!cursor.paragraph.has_next()) {
        return false;
    }
    cursor.table.reset();
    cursor.table_row_count = 0U;
    cursor.table_column_count = 0U;
    cursor.table_column_anchor_x_points.clear();
    cursor.table_header_cell_texts.clear();
    return true;
}

[[nodiscard]] TableAppendResult append_imported_table(
    featherdoc::Document &document, ImportCursor &cursor,
    const PdfParsedTableCandidate &source,
    const PdfTableContinuationDiagnostic &continuation) {
    const auto column_count = table_column_count(source);
    const bool is_first_written_block = !cursor.has_written_block;
    if (!has_consistent_row_widths(source, column_count)) {
        return TableAppendResult::failed;
    }

    if (continuation.disposition ==
        PdfTableContinuationDisposition::merged_with_previous_table) {
        std::size_t appended_row_count = 0U;
        if (!append_rows_to_imported_table(*cursor.table, cursor.table_row_count,
                                           column_count, source,
                                           continuation.source_row_offset,
                                           appended_row_count)) {
            return TableAppendResult::failed;
        }

        cursor.table_row_count += appended_row_count;
        cursor.has_written_block = true;
        return TableAppendResult::merged;
    }

    featherdoc::Table table;
    if (!cursor.has_written_block) {
        table = document.append_table(source.rows.size(), column_count);
    } else if (cursor.table.has_value()) {
        table = cursor.table->insert_table_after(source.rows.size(), column_count);
    } else {
        table = document.append_table(source.rows.size(), column_count);
    }

    if (!configure_imported_table(table, source)) {
        return TableAppendResult::failed;
    }

    cursor.table = std::move(table);
    cursor.table_row_count = source.rows.size();
    cursor.table_column_count = column_count;
    cursor.table_column_anchor_x_points = source.column_anchor_x_points;
    cursor.table_header_cell_texts =
        should_mark_repeating_header_row(source)
            ? collect_row_cell_texts(source.rows.front())
            : std::vector<std::string>{};
    cursor.has_written_block = true;

    if (is_first_written_block) {
        // 首个导入块如果是表格，删除 create_empty() 带来的占位段落，
        // 避免 body 头部残留一个空白段落。
        if (!cursor.paragraph.remove()) {
            return TableAppendResult::failed;
        }
    }

    return TableAppendResult::created;
}

[[nodiscard]] PdfDocumentImportResult
populate_document_from_parsed_pdf(featherdoc::Document &document,
                                  PdfParsedDocument parsed_document,
                                  const PdfDocumentImportOptions &options) {
    PdfDocumentImportResult result;
    result.parsed_document = std::move(parsed_document);

    if (has_table_candidates(result.parsed_document) &&
        !options.import_table_candidates_as_tables) {
        result.failure_kind =
            PdfDocumentImportFailureKind::table_candidates_detected;
        result.error_message =
            "PDF text import detected table-like structure candidates; "
            "enable table-candidate import to import them as DOCX tables";
        return result;
    }

    const auto create_error = document.create_empty();
    if (create_error) {
        result.failure_kind = PdfDocumentImportFailureKind::document_create_failed;
        result.error_message =
            "Unable to create output Document: " + create_error.message();
        return result;
    }

    ImportCursor cursor{
        document.paragraphs(), std::nullopt, 0U, 0U, {}, {}, false};

    for (std::size_t page_index = 0U;
         page_index < result.parsed_document.pages.size(); ++page_index) {
        const auto &page = result.parsed_document.pages[page_index];
        const auto blocks =
            build_import_blocks(page,
                                options.import_table_candidates_as_tables);
        for (std::size_t block_index = 0U; block_index < blocks.size();
             ++block_index) {
            const auto &block = blocks[block_index];
            if (block.kind == ImportBlock::Kind::paragraph) {
                if (block.paragraph_text.empty() ||
                    !append_imported_paragraph(cursor, block.paragraph_text)) {
                    result.failure_kind =
                        PdfDocumentImportFailureKind::document_population_failed;
                    result.error_message =
                        "Unable to append parsed PDF text paragraph to Document";
                    return result;
                }
                ++result.paragraphs_imported;
                continue;
            }

            if (block.table == nullptr) {
                result.failure_kind =
                    PdfDocumentImportFailureKind::document_population_failed;
                result.error_message =
                    "Unable to append parsed PDF table candidate to Document";
                return result;
            }

            const auto min_confidence =
                options.min_table_continuation_confidence;
            result.table_continuation_diagnostics.push_back(
                evaluate_table_continuation(cursor, page_index, block_index,
                                            block.top_points,
                                            page.size.height_points,
                                            *block.table, min_confidence));
            const auto &continuation =
                result.table_continuation_diagnostics.back();
            const auto append_result =
                append_imported_table(document, cursor, *block.table,
                                      continuation);
            if (append_result == TableAppendResult::failed) {
                result.failure_kind =
                    PdfDocumentImportFailureKind::document_population_failed;
                result.error_message =
                    "Unable to append parsed PDF table candidate to Document";
                return result;
            }
            if (append_result == TableAppendResult::created) {
                ++result.tables_imported;
            }
        }
    }

    if (result.paragraphs_imported == 0U && result.tables_imported == 0U) {
        result.failure_kind = PdfDocumentImportFailureKind::no_text_paragraphs;
        result.error_message =
            "PDF import currently supports text paragraphs only; no text "
            "paragraphs were detected";
        return result;
    }

    result.success = true;
    return result;
}

}  // namespace

PdfDocumentImportResult PdfDocumentImporter::import_text(
    const std::filesystem::path &input_path, featherdoc::Document &document,
    const PdfDocumentImportOptions &options) {
    if (!options.parse_options.extract_text) {
        PdfDocumentImportResult result;
        result.failure_kind = PdfDocumentImportFailureKind::extract_text_disabled;
        result.error_message =
            "PDF text import requires PdfParseOptions::extract_text=true";
        return result;
    }
    if (!options.parse_options.extract_geometry) {
        PdfDocumentImportResult result;
        result.failure_kind =
            PdfDocumentImportFailureKind::extract_geometry_disabled;
        result.error_message =
            "PDF text import requires PdfParseOptions::extract_geometry=true "
            "to group text into paragraphs";
        return result;
    }

    PdfiumParser parser;
    const auto parse_result = parser.parse(input_path, options.parse_options);
    if (!parse_result.success) {
        PdfDocumentImportResult result;
        result.failure_kind = PdfDocumentImportFailureKind::parse_failed;
        result.error_message = parse_result.error_message;
        return result;
    }

    return populate_document_from_parsed_pdf(document, parse_result.document,
                                             options);
}

PdfDocumentImportResult
import_pdf_text_document(const std::filesystem::path &input_path,
                         featherdoc::Document &document,
                         const PdfDocumentImportOptions &options) {
    PdfDocumentImporter importer;
    return importer.import_text(input_path, document, options);
}

}  // namespace featherdoc::pdf
