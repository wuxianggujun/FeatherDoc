#include <featherdoc/pdf/pdf_document_importer.hpp>

#include <featherdoc/pdf/pdf_parser.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <utility>
#include <vector>

namespace featherdoc::pdf {
namespace {

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
    return std::any_of(page.table_candidates.begin(),
                       page.table_candidates.end(),
                       [&paragraph](const PdfParsedTableCandidate &table) {
                           return rects_intersect(paragraph.bounds, table.bounds);
                       });
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
    const PdfParsedParagraph *paragraph{nullptr};
    const PdfParsedTableCandidate *table{nullptr};
};

struct ImportCursor {
    featherdoc::Paragraph paragraph;
    std::optional<featherdoc::Table> table;
    bool has_written_block{false};
};

[[nodiscard]] std::vector<ImportBlock> build_import_blocks(
    const PdfParsedPage &page, bool import_table_candidates_as_tables) {
    std::vector<ImportBlock> blocks;
    blocks.reserve(page.paragraphs.size() + page.table_candidates.size());

    for (const auto &paragraph : page.paragraphs) {
        if (!has_importable_text(paragraph) ||
            (import_table_candidates_as_tables &&
             overlaps_table_candidate(paragraph, page))) {
            continue;
        }
        blocks.push_back(ImportBlock{
            ImportBlock::Kind::paragraph, rect_top(paragraph.bounds),
            &paragraph, nullptr});
    }

    if (import_table_candidates_as_tables) {
        for (const auto &table : page.table_candidates) {
            blocks.push_back(ImportBlock{
                ImportBlock::Kind::table, rect_top(table.bounds), nullptr,
                &table});
        }
    }

    std::stable_sort(blocks.begin(), blocks.end(),
                     [](const ImportBlock &left, const ImportBlock &right) {
                         return left.top_points > right.top_points;
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

[[nodiscard]] bool append_imported_table(featherdoc::Document &document,
                                         const PdfParsedTableCandidate &source,
                                         featherdoc::Table *target_table = nullptr) {
    if (source.rows.empty() || source.rows.front().cells.empty()) {
        return false;
    }

    const auto row_count = source.rows.size();
    const auto column_count = source.rows.front().cells.size();
    auto table = target_table == nullptr ? document.append_table(row_count, column_count)
                                        : std::move(*target_table);
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
                !table.set_cell_text(row_index, cell.column_index, cell.text)) {
                return false;
            }
        }
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
    return true;
}

[[nodiscard]] bool append_imported_table(featherdoc::Document &document,
                                         ImportCursor &cursor,
                                         const PdfParsedTableCandidate &source) {
    const bool is_first_written_block = !cursor.has_written_block;
    featherdoc::Table table;
    if (!cursor.has_written_block) {
        table = document.append_table(source.rows.size(),
                                      source.rows.empty()
                                          ? 1U
                                          : source.rows.front().cells.size());
    } else if (cursor.table.has_value()) {
        table = cursor.table->insert_table_after(
            source.rows.size(),
            source.rows.empty() ? 1U : source.rows.front().cells.size());
    } else {
        table = document.append_table(source.rows.size(),
                                      source.rows.empty()
                                          ? 1U
                                          : source.rows.front().cells.size());
    }

    if (!append_imported_table(document, source, &table)) {
        return false;
    }

    cursor.table = std::move(table);
    cursor.has_written_block = true;

    if (is_first_written_block) {
        // 首个导入块如果是表格，删除 create_empty() 带来的占位段落，
        // 避免 body 头部残留一个空白段落。
        if (!cursor.paragraph.remove()) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] PdfDocumentImportResult
populate_document_from_parsed_pdf(featherdoc::Document &document,
                                  PdfParsedDocument parsed_document,
                                  bool import_table_candidates_as_tables) {
    PdfDocumentImportResult result;
    result.parsed_document = std::move(parsed_document);

    if (has_table_candidates(result.parsed_document) &&
        !import_table_candidates_as_tables) {
        result.failure_kind =
            PdfDocumentImportFailureKind::table_candidates_detected;
        result.error_message =
            "PDF text import detected table-like structure candidates; "
            "table import is not supported yet";
        return result;
    }

    const auto create_error = document.create_empty();
    if (create_error) {
        result.failure_kind = PdfDocumentImportFailureKind::document_create_failed;
        result.error_message =
            "Unable to create output Document: " + create_error.message();
        return result;
    }

    ImportCursor cursor{document.paragraphs(), std::nullopt, false};

    for (const auto &page : result.parsed_document.pages) {
        for (const auto &block :
             build_import_blocks(page, import_table_candidates_as_tables)) {
            if (block.kind == ImportBlock::Kind::paragraph) {
                if (block.paragraph == nullptr ||
                    !append_imported_paragraph(cursor, block.paragraph->text)) {
                    result.failure_kind =
                        PdfDocumentImportFailureKind::document_population_failed;
                    result.error_message =
                        "Unable to append parsed PDF text paragraph to Document";
                    return result;
                }
                ++result.paragraphs_imported;
                continue;
            }

            if (block.table == nullptr ||
                !append_imported_table(document, cursor, *block.table)) {
                result.failure_kind =
                    PdfDocumentImportFailureKind::document_population_failed;
                result.error_message =
                    "Unable to append parsed PDF table candidate to Document";
                return result;
            }
            ++result.tables_imported;
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

    return populate_document_from_parsed_pdf(
        document, parse_result.document,
        options.import_table_candidates_as_tables);
}

PdfDocumentImportResult
import_pdf_text_document(const std::filesystem::path &input_path,
                         featherdoc::Document &document,
                         const PdfDocumentImportOptions &options) {
    PdfDocumentImporter importer;
    return importer.import_text(input_path, document, options);
}

}  // namespace featherdoc::pdf
