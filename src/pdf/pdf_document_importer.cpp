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

#include "pdf_document_importer_blocks.inc"
#include "pdf_document_importer_table_shape.inc"
#include "pdf_document_importer_table_append.inc"

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

    return import_parsed_document(std::move(parse_result.document), document,
                                  options);
}

PdfDocumentImportResult PdfDocumentImporter::import_parsed_document(
    PdfParsedDocument parsed_document, featherdoc::Document &document,
    const PdfDocumentImportOptions &options) {
    return populate_document_from_parsed_pdf(document, std::move(parsed_document),
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
