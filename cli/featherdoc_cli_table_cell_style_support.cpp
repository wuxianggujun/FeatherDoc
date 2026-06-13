#include "featherdoc_cli_table_cell_style_commands_detail.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {

auto open_table_cell_for_style(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc, std::size_t table_index,
    std::size_t row_index, std::size_t cell_index,
    const table_cell_style_options &options, featherdoc::TableCell &cell)
    -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    return resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                   command, options.json_output);
}

auto report_invalid_table_cell_target(std::string_view command,
                                      std::string_view summary,
                                      std::string detail,
                                      bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

} // namespace featherdoc_cli
