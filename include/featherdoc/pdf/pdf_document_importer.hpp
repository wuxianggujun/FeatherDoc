/*
 * FeatherDoc experimental PDF-to-Document importer facade.
 *
 * This is intentionally conservative: the first implementation imports only
 * text paragraphs from the PDFium-backed parse result into a plain DOCX model.
 * Geometry extraction is required because paragraph and table grouping depend
 * on span bounds; geometry-free parsing is rejected instead of degraded.
 */

#ifndef FEATHERDOC_PDF_PDF_DOCUMENT_IMPORTER_HPP
#define FEATHERDOC_PDF_PDF_DOCUMENT_IMPORTER_HPP

#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_interfaces.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace featherdoc::pdf {

enum class PdfDocumentImportFailureKind {
    none,
    parse_failed,
    document_create_failed,
    document_population_failed,
    extract_text_disabled,
    extract_geometry_disabled,
    table_candidates_detected,
    no_text_paragraphs,
};

struct PdfDocumentImportOptions {
    PdfParseOptions parse_options{};
    bool import_table_candidates_as_tables{false};
    std::uint32_t min_table_continuation_confidence{0U};
};

enum class PdfTableContinuationDisposition {
    none,
    created_new_table,
    merged_with_previous_table,
};

enum class PdfTableContinuationBlocker {
    none,
    no_previous_table,
    not_first_block_on_page,
    not_near_page_top,
    inconsistent_source_rows,
    column_count_mismatch,
    column_anchors_mismatch,
    repeated_header_mismatch,
    continuation_confidence_below_threshold,
};

enum class PdfTableContinuationHeaderMatchKind {
    none,
    not_required,
    exact,
    normalized_text,
    plural_variant,
    canonical_text,
    token_set,
};

struct PdfTableContinuationDiagnostic {
    std::size_t page_index{0U};
    std::size_t block_index{0U};
    std::size_t source_row_offset{0U};
    std::uint32_t continuation_confidence{0U};
    std::uint32_t minimum_continuation_confidence{0U};
    bool has_previous_table{false};
    bool is_first_block_on_page{false};
    bool is_near_page_top{false};
    bool source_rows_consistent{false};
    bool column_count_matches{false};
    bool column_anchors_match{false};
    bool previous_has_repeating_header{false};
    bool source_has_repeating_header{false};
    bool header_matches_previous{false};
    PdfTableContinuationHeaderMatchKind header_match_kind{
        PdfTableContinuationHeaderMatchKind::none};
    bool skipped_repeating_header{false};
    PdfTableContinuationDisposition disposition{
        PdfTableContinuationDisposition::none};
    PdfTableContinuationBlocker blocker{PdfTableContinuationBlocker::none};
};

struct PdfDocumentImportResult {
    bool success{false};
    PdfDocumentImportFailureKind failure_kind{
        PdfDocumentImportFailureKind::none};
    std::string error_message;
    std::size_t paragraphs_imported{0U};
    std::size_t tables_imported{0U};
    std::vector<PdfTableContinuationDiagnostic> table_continuation_diagnostics;
    PdfParsedDocument parsed_document;

    [[nodiscard]] explicit operator bool() const noexcept {
        return success;
    }
};

class PdfDocumentImporter {
public:
    [[nodiscard]] PdfDocumentImportResult
    import_text(const std::filesystem::path &input_path,
                featherdoc::Document &document,
                const PdfDocumentImportOptions &options = {});
};

[[nodiscard]] PdfDocumentImportResult
import_pdf_text_document(const std::filesystem::path &input_path,
                         featherdoc::Document &document,
                         const PdfDocumentImportOptions &options = {});

}  // namespace featherdoc::pdf

#endif  // FEATHERDOC_PDF_PDF_DOCUMENT_IMPORTER_HPP
