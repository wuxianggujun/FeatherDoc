#pragma once

#include <featherdoc.hpp>

#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_export_pdf_command(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            featherdoc::Document &doc) -> int;

#if defined(FEATHERDOC_CLI_ENABLE_PDF_IMPORT)
auto run_import_pdf_command(std::string_view command,
                            const std::vector<std::string_view> &arguments,
                            featherdoc::Document &doc) -> int;
#endif

} // namespace featherdoc_cli
