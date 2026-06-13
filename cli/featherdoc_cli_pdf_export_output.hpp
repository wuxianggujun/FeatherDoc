#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_pdf_parse.hpp"

#if defined(FEATHERDOC_CLI_ENABLE_PDF)
#include <featherdoc/pdf/pdf_writer.hpp>
#endif

#include <iosfwd>
#include <string>
#include <string_view>

namespace featherdoc_cli {

#if defined(FEATHERDOC_CLI_ENABLE_PDF)
void write_pdf_export_options_json(std::ostream &stream,
                                   const export_pdf_options &options);

void print_pdf_export_result(std::string_view command, const path_type &output_path,
                             const export_pdf_options &options,
                             const featherdoc::pdf::PdfWriteResult &result,
                             bool json_output);

[[nodiscard]] auto write_pdf_export_summary_json(
    std::string_view command, const path_type &summary_path,
    const path_type &output_path, const export_pdf_options &options,
    const featherdoc::pdf::PdfWriteResult &result,
    std::string &error_message) -> bool;
#endif

} // namespace featherdoc_cli
