#include "featherdoc_cli_template_table_inspect_commands_detail.hpp"

#include "featherdoc_cli_errors.hpp"

#include <system_error>
#include <utility>

namespace featherdoc_cli {

auto report_template_table_range_failure(
    std::string_view command, std::string_view summary, std::string detail,
    const selected_template_part &selected, bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, "inspect", summary, error_info,
                                    json_output);
}

} // namespace featherdoc_cli
