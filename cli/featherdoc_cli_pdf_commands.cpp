#include "featherdoc_cli_pdf_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_pdf_parse.hpp"
#include "featherdoc_cli_text.hpp"

#if defined(FEATHERDOC_CLI_ENABLE_PDF)
#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>
#endif
#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
#include <featherdoc/pdf/pdf_document_importer.hpp>
#endif

#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <utility>

namespace featherdoc_cli {
namespace {

#if defined(FEATHERDOC_CLI_ENABLE_PDF)
void write_pdf_export_options_json(std::ostream &stream,
                                   const export_pdf_options &options) {
    stream << "{\"render_headers_and_footers\":"
           << json_bool(options.render_headers_and_footers)
           << ",\"render_inline_images\":"
           << json_bool(options.render_inline_images)
           << ",\"expand_header_footer_page_placeholders\":"
           << json_bool(options.expand_header_footer_page_placeholders)
           << ",\"subset_unicode_fonts\":"
           << json_bool(options.subset_unicode_fonts)
           << ",\"use_system_font_fallbacks\":"
           << json_bool(options.use_system_font_fallbacks) << '}';
}

void print_pdf_export_result(std::string_view command, const path_type &output_path,
                             const export_pdf_options &options,
                             const featherdoc::pdf::PdfWriteResult &result,
                             bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":";
        write_json_string(std::cout, command);
        std::cout << ",\"ok\":true,\"output\":";
        write_json_string(std::cout, output_path.string());
        std::cout << ",\"bytes_written\":" << result.bytes_written
                  << ",\"options\":";
        write_pdf_export_options_json(std::cout, options);
        std::cout << "}\n";
        return;
    }

    std::cout << "wrote " << output_path.string() << " (" << result.bytes_written
              << " bytes)\n";
}

auto write_pdf_export_summary_json(std::string_view command,
                                   const path_type &summary_path,
                                   const path_type &output_path,
                                   const export_pdf_options &options,
                                   const featherdoc::pdf::PdfWriteResult &result,
                                   std::string &error_message) -> bool {
    std::ofstream output(summary_path, std::ios::binary);
    if (!output.good()) {
        error_message =
            "failed to open PDF export summary: " + summary_path.string();
        return false;
    }

    output << "{\"command\":";
    write_json_string(output, command);
    output << ",\"ok\":true,\"output\":";
    write_json_string(output, output_path.string());
    output << ",\"bytes_written\":" << result.bytes_written
           << ",\"options\":";
    write_pdf_export_options_json(output, options);
    output << "}\n";
    if (!output.good()) {
        error_message =
            "failed to write PDF export summary: " + summary_path.string();
        return false;
    }

    return true;
}
#endif

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
[[nodiscard]] auto pdf_import_failure_kind_name(
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

} // namespace

auto run_export_pdf_command(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "export-pdf expects an input path",
                          json_output);
        return 2;
    }

    export_pdf_options options;
    std::string error_message;
    if (!parse_export_pdf_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

#if defined(FEATHERDOC_CLI_ENABLE_PDF)
    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions adapter_options;
    adapter_options.metadata.title = options.title.value_or(
        input_path.filename().empty() ? std::string{"FeatherDoc PDF export"}
                                      : input_path.filename().string());
    adapter_options.metadata.creator =
        options.creator.value_or(std::string{"FeatherDoc"});
    adapter_options.render_headers_and_footers =
        options.render_headers_and_footers;
    adapter_options.render_inline_images = options.render_inline_images;
    adapter_options.expand_header_footer_page_placeholders =
        options.expand_header_footer_page_placeholders;
    adapter_options.use_system_font_fallbacks =
        options.use_system_font_fallbacks;
    if (options.font_file_path.has_value()) {
        adapter_options.font_file_path = *options.font_file_path;
    }
    if (options.cjk_font_file_path.has_value()) {
        adapter_options.cjk_font_file_path = *options.cjk_font_file_path;
    }
    for (const auto &mapping : options.font_mappings) {
        adapter_options.font_mappings.push_back(
            featherdoc::pdf::PdfFontMapping{mapping.first, mapping.second});
    }

    auto layout =
        featherdoc::pdf::layout_document_paragraphs(doc, adapter_options);

    featherdoc::pdf::PdfWriterOptions writer_options;
    writer_options.page_size = adapter_options.page_size;
    writer_options.title = adapter_options.metadata.title;
    writer_options.creator = adapter_options.metadata.creator;
    writer_options.subset_unicode_fonts = options.subset_unicode_fonts;

    const auto result = featherdoc::pdf::write_pdfio_document(
        *options.output_path, layout, writer_options);
    if (!result) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail = result.error_message;
        report_operation_failure(command, "export",
                                 "failed to write PDF output", error_info,
                                 options.json_output);
        return 1;
    }

    if (options.summary_json_path.has_value()) {
        std::string summary_error;
        if (!write_pdf_export_summary_json(
                command, *options.summary_json_path, *options.output_path,
                options, result, summary_error)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(summary_error);
            report_operation_failure(command, "summary",
                                     "failed to write PDF export summary",
                                     error_info, options.json_output);
            return 1;
        }
    }

    print_pdf_export_result(command, *options.output_path, options, result,
                            options.json_output);
    return 0;
#else
    featherdoc::document_error_info error_info{};
    error_info.detail =
        "PDF export requires configuring with -DFEATHERDOC_BUILD_PDF=ON";
    if (options.json_output) {
        write_json_command_error(std::cerr, command, "export",
                                 "Operation not supported", &error_info);
        return 1;
    }
    error_info.code = std::make_error_code(std::errc::not_supported);
    report_operation_failure(command, "export",
                             "PDF export is not enabled in this build",
                             error_info, options.json_output);
    return 1;
#endif
}

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
auto run_import_pdf_command(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "import-pdf expects an input path",
                          json_output);
        return 2;
    }

    import_pdf_options options;
    std::string error_message;
    if (!parse_import_pdf_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::pdf::PdfDocumentImportOptions import_options;
    import_options.import_table_candidates_as_tables =
        options.import_table_candidates_as_tables;
    if (options.min_table_continuation_confidence.has_value()) {
        import_options.min_table_continuation_confidence =
            *options.min_table_continuation_confidence;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    const auto import_result = featherdoc::pdf::import_pdf_text_document(
        input_path, doc, import_options);
    if (!import_result) {
        print_pdf_import_failure(command, input_path, *options.output_path,
                                 import_result, options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command,
                       options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&input_path, &options, &import_result](std::ostream &stream) {
                stream << ",\"input\":";
                write_json_string(stream, input_path.string());
                stream << ",\"output\":";
                write_json_string(stream, options.output_path->string());
                stream << ",\"paragraphs_imported\":"
                       << import_result.paragraphs_imported;
                stream << ",\"tables_imported\":"
                       << import_result.tables_imported;
                stream << ",\"table_continuation_diagnostics_count\":"
                       << import_result.table_continuation_diagnostics.size();
                stream << ",\"table_continuation_diagnostics\":";
                write_pdf_table_continuation_diagnostics_json(
                    stream, import_result.table_continuation_diagnostics);
                stream << ",\"import_table_candidates_as_tables\":"
                       << json_bool(options.import_table_candidates_as_tables);
                if (options.min_table_continuation_confidence.has_value()) {
                    stream << ",\"min_table_continuation_confidence\":"
                           << *options.min_table_continuation_confidence;
                }
            });
    } else {
        std::cout << "imported " << input_path.string() << " -> "
                  << options.output_path->string() << " ("
                  << import_result.paragraphs_imported << " paragraphs, "
                  << import_result.tables_imported << " tables)\n";
    }

    return 0;
}
#endif

} // namespace featherdoc_cli
