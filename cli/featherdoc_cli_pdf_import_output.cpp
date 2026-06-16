#include "featherdoc_cli_pdf_import_output.hpp"

#include "featherdoc_cli_json.hpp"

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
#include <cstddef>
#include <iostream>
#endif

namespace featherdoc_cli {

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
auto pdf_import_failure_kind_name(
    featherdoc::pdf::PdfDocumentImportFailureKind failure_kind) -> std::string_view {
    using featherdoc::pdf::PdfDocumentImportFailureKind;
    switch (failure_kind) {
    case PdfDocumentImportFailureKind::none:
        return "none";
    case PdfDocumentImportFailureKind::parse_failed:
        return "parse_failed";
    case PdfDocumentImportFailureKind::document_create_failed:
        return "document_create_failed";
    case PdfDocumentImportFailureKind::document_population_failed:
        return "document_population_failed";
    case PdfDocumentImportFailureKind::extract_text_disabled:
        return "extract_text_disabled";
    case PdfDocumentImportFailureKind::extract_geometry_disabled:
        return "extract_geometry_disabled";
    case PdfDocumentImportFailureKind::table_candidates_detected:
        return "table_candidates_detected";
    case PdfDocumentImportFailureKind::no_text_paragraphs:
        return "no_text_paragraphs";
    }

    return "unknown";
}

namespace {

[[nodiscard]] auto pdf_table_continuation_disposition_name(
    featherdoc::pdf::PdfTableContinuationDisposition disposition)
    -> std::string_view {
    switch (disposition) {
    case featherdoc::pdf::PdfTableContinuationDisposition::none:
        return "none";
    case featherdoc::pdf::PdfTableContinuationDisposition::created_new_table:
        return "created_new_table";
    case featherdoc::pdf::PdfTableContinuationDisposition::
        merged_with_previous_table:
        return "merged_with_previous_table";
    }

    return "unknown";
}

[[nodiscard]] auto pdf_table_continuation_blocker_name(
    featherdoc::pdf::PdfTableContinuationBlocker blocker) -> std::string_view {
    switch (blocker) {
    case featherdoc::pdf::PdfTableContinuationBlocker::none:
        return "none";
    case featherdoc::pdf::PdfTableContinuationBlocker::no_previous_table:
        return "no_previous_table";
    case featherdoc::pdf::PdfTableContinuationBlocker::not_first_block_on_page:
        return "not_first_block_on_page";
    case featherdoc::pdf::PdfTableContinuationBlocker::not_near_page_top:
        return "not_near_page_top";
    case featherdoc::pdf::PdfTableContinuationBlocker::inconsistent_source_rows:
        return "inconsistent_source_rows";
    case featherdoc::pdf::PdfTableContinuationBlocker::column_count_mismatch:
        return "column_count_mismatch";
    case featherdoc::pdf::PdfTableContinuationBlocker::column_anchors_mismatch:
        return "column_anchors_mismatch";
    case featherdoc::pdf::PdfTableContinuationBlocker::repeated_header_mismatch:
        return "repeated_header_mismatch";
    case featherdoc::pdf::PdfTableContinuationBlocker::
        continuation_confidence_below_threshold:
        return "continuation_confidence_below_threshold";
    }

    return "unknown";
}

[[nodiscard]] auto pdf_table_continuation_header_match_kind_name(
    featherdoc::pdf::PdfTableContinuationHeaderMatchKind kind)
    -> std::string_view {
    switch (kind) {
    case featherdoc::pdf::PdfTableContinuationHeaderMatchKind::none:
        return "none";
    case featherdoc::pdf::PdfTableContinuationHeaderMatchKind::not_required:
        return "not_required";
    case featherdoc::pdf::PdfTableContinuationHeaderMatchKind::exact:
        return "exact";
    case featherdoc::pdf::PdfTableContinuationHeaderMatchKind::normalized_text:
        return "normalized_text";
    case featherdoc::pdf::PdfTableContinuationHeaderMatchKind::plural_variant:
        return "plural_variant";
    case featherdoc::pdf::PdfTableContinuationHeaderMatchKind::canonical_text:
        return "canonical_text";
    case featherdoc::pdf::PdfTableContinuationHeaderMatchKind::token_set:
        return "token_set";
    }

    return "unknown";
}

} // namespace

void write_pdf_table_continuation_diagnostics_json(
    std::ostream &stream,
    const std::vector<featherdoc::pdf::PdfTableContinuationDiagnostic>
        &diagnostics) {
    stream << '[';
    for (std::size_t index = 0U; index < diagnostics.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        const auto &diagnostic = diagnostics[index];
        stream << "{\"page_index\":" << diagnostic.page_index
               << ",\"block_index\":" << diagnostic.block_index
               << ",\"source_row_offset\":" << diagnostic.source_row_offset
               << ",\"continuation_confidence\":"
               << diagnostic.continuation_confidence
               << ",\"minimum_continuation_confidence\":"
               << diagnostic.minimum_continuation_confidence
               << ",\"has_previous_table\":"
               << json_bool(diagnostic.has_previous_table)
               << ",\"is_first_block_on_page\":"
               << json_bool(diagnostic.is_first_block_on_page)
               << ",\"is_near_page_top\":"
               << json_bool(diagnostic.is_near_page_top)
               << ",\"source_rows_consistent\":"
               << json_bool(diagnostic.source_rows_consistent)
               << ",\"column_count_matches\":"
               << json_bool(diagnostic.column_count_matches)
               << ",\"column_anchors_match\":"
               << json_bool(diagnostic.column_anchors_match)
               << ",\"previous_has_repeating_header\":"
               << json_bool(diagnostic.previous_has_repeating_header)
               << ",\"source_has_repeating_header\":"
               << json_bool(diagnostic.source_has_repeating_header)
               << ",\"header_matches_previous\":"
               << json_bool(diagnostic.header_matches_previous)
               << ",\"header_match_kind\":";
        write_json_string(
            stream, pdf_table_continuation_header_match_kind_name(
                        diagnostic.header_match_kind));
        stream << ",\"skipped_repeating_header\":"
               << json_bool(diagnostic.skipped_repeating_header)
               << ",\"disposition\":";
        write_json_string(stream, pdf_table_continuation_disposition_name(
                                      diagnostic.disposition));
        stream << ",\"blocker\":";
        write_json_string(stream,
                          pdf_table_continuation_blocker_name(
                              diagnostic.blocker));
        stream << '}';
    }
    stream << ']';
}

void print_pdf_import_failure(
    std::string_view command, const path_type &input_path,
    const path_type &output_path,
    const featherdoc::pdf::PdfDocumentImportResult &result, bool json_output) {
    if (json_output) {
        std::cerr << "{\"command\":";
        write_json_string(std::cerr, command);
        std::cerr << ",\"ok\":false,\"stage\":\"import\",\"failure_kind\":";
        write_json_string(std::cerr, pdf_import_failure_kind_name(result.failure_kind));
        std::cerr << ",\"message\":";
        write_json_string(std::cerr, result.error_message);
        std::cerr << ",\"input\":";
        write_json_string(std::cerr, input_path.string());
        std::cerr << ",\"output\":";
        write_json_string(std::cerr, output_path.string());
        std::cerr << "}\n";
        return;
    }

    std::cerr << "PDF import failed ("
              << pdf_import_failure_kind_name(result.failure_kind) << ")";
    if (!result.error_message.empty()) {
        std::cerr << ": " << result.error_message;
    }
    std::cerr << '\n';
}
#endif

} // namespace featherdoc_cli
