#include "featherdoc_cli_table_border_margin_commands.hpp"

#include "featherdoc_cli_table_border_margin_commands_detail.hpp"

#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto is_table_border_margin_command(std::string_view command) -> bool {
    return command == "set-table-default-cell-margin" ||
           command == "clear-table-default-cell-margin" ||
           command == "set-table-border" ||
           command == "clear-table-border" ||
           command == "set-table-cell-margin" ||
           command == "clear-table-cell-margin" ||
           command == "set-table-cell-border" ||
           command == "clear-table-cell-border";
}

auto run_table_border_margin_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "set-table-default-cell-margin") {
        return run_set_table_default_cell_margin_command(command, arguments, doc);
    }
    if (command == "clear-table-default-cell-margin") {
        return run_clear_table_default_cell_margin_command(command, arguments,
                                                          doc);
    }
    if (command == "set-table-border") {
        return run_set_table_border_command(command, arguments, doc);
    }
    if (command == "clear-table-border") {
        return run_clear_table_border_command(command, arguments, doc);
    }
    if (command == "set-table-cell-margin") {
        return run_set_table_cell_margin_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-margin") {
        return run_clear_table_cell_margin_command(command, arguments, doc);
    }
    if (command == "set-table-cell-border") {
        return run_set_table_cell_border_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-border") {
        return run_clear_table_cell_border_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
