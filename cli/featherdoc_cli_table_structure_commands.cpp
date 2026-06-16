#include "featherdoc_cli_table_structure_commands.hpp"

#include "featherdoc_cli_table_column_width_commands.hpp"
#include "featherdoc_cli_table_insert_remove_commands.hpp"

namespace featherdoc_cli {

auto is_table_structure_command(std::string_view command) -> bool {
    return command == "set-table-column-width" ||
           command == "clear-table-column-width" ||
           command == "insert-table-before" ||
           command == "insert-table-after" ||
           command == "insert-table-like-before" ||
           command == "insert-table-like-after" ||
           command == "remove-table";
}

auto run_table_structure_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "set-table-column-width") {
        return run_set_table_column_width_command(command, arguments);
    }
    if (command == "clear-table-column-width") {
        return run_clear_table_column_width_command(command, arguments);
    }
    if (command == "insert-table-before") {
        return run_insert_table_command(command, arguments, true);
    }
    if (command == "insert-table-after") {
        return run_insert_table_command(command, arguments, false);
    }
    if (command == "insert-table-like-before") {
        return run_insert_table_like_command(command, arguments, true);
    }
    if (command == "insert-table-like-after") {
        return run_insert_table_like_command(command, arguments, false);
    }
    if (command == "remove-table") {
        return run_remove_table_command(command, arguments);
    }

    return 2;
}

} // namespace featherdoc_cli
