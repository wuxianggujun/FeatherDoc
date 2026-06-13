#include "featherdoc_cli_template_table_row_commands_detail.hpp"

#include <system_error>
#include <utility>

namespace featherdoc_cli {

auto validate_target_selector(std::string_view command, bool json_output,
                              parsed_template_table_selector_target &target,
                              std::string &error_message) -> bool {
    if (!validate_template_table_selector(target.selector, false,
                                          target.has_header_row_index,
                                          target.has_occurrence,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return false;
    }

    return true;
}

auto report_template_row_failure(std::string_view command,
                                 std::string_view summary, std::string detail,
                                 const selected_template_part &selected,
                                 bool json_output) -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

} // namespace featherdoc_cli
