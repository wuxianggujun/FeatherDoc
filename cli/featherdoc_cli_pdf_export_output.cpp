#include "featherdoc_cli_pdf_export_output.hpp"

#include "featherdoc_cli_json.hpp"

#include <featherdoc/detail/path.hpp>

#if defined(FEATHERDOC_CLI_ENABLE_PDF)
#include <fstream>
#include <iostream>
#endif

namespace featherdoc_cli {

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
        write_json_string(std::cout,
                          featherdoc::detail::path_to_utf8(output_path));
        std::cout << ",\"bytes_written\":" << result.bytes_written
                  << ",\"options\":";
        write_pdf_export_options_json(std::cout, options);
        std::cout << "}\n";
        return;
    }

    std::cout << "wrote " << featherdoc::detail::path_to_utf8(output_path)
              << " (" << result.bytes_written << " bytes)\n";
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
            "failed to open PDF export summary: " +
            featherdoc::detail::path_to_utf8(summary_path);
        return false;
    }

    output << "{\"command\":";
    write_json_string(output, command);
    output << ",\"ok\":true,\"output\":";
    write_json_string(output, featherdoc::detail::path_to_utf8(output_path));
    output << ",\"bytes_written\":" << result.bytes_written
           << ",\"options\":";
    write_pdf_export_options_json(output, options);
    output << "}\n";
    if (!output.good()) {
        error_message =
            "failed to write PDF export summary: " +
            featherdoc::detail::path_to_utf8(summary_path);
        return false;
    }

    return true;
}
#endif

} // namespace featherdoc_cli
