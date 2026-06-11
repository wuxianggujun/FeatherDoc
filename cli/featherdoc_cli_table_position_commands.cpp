#include "featherdoc_cli_table_position_commands.hpp"

#include "featherdoc_cli_table_position_commands_detail.hpp"

#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto is_table_position_command(std::string_view command) -> bool {
    return command == "plan-table-position-presets" ||
           command == "apply-table-position-plan" ||
           command == "set-table-position" ||
           command == "clear-table-position";
}

auto run_table_position_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "plan-table-position-presets") {
        return run_plan_table_position_presets_command(command, arguments);
    }
    if (command == "apply-table-position-plan") {
        return run_apply_table_position_plan_command(command, arguments);
    }
    if (command == "set-table-position") {
        return run_set_table_position_command(command, arguments);
    }
    if (command == "clear-table-position") {
        return run_clear_table_position_command(command, arguments);
    }

    return 2;
}

} // namespace featherdoc_cli
