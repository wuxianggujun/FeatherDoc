#include "featherdoc_cli_pdf_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_pdf_export_output.hpp"
#include "featherdoc_cli_pdf_parse.hpp"

#if defined(FEATHERDOC_CLI_ENABLE_PDF)
#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>
#endif

#include <iostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
