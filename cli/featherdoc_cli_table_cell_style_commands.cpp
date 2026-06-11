#include "featherdoc_cli_table_cell_style_commands.hpp"

#include "featherdoc_cli_table_cell_style_commands_detail.hpp"

#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto is_table_cell_style_command(std::string_view command) -> bool {
    return command == "set-table-cell-fill" ||
           command == "clear-table-cell-fill" ||
           command == "set-table-cell-vertical-alignment" ||
           command == "clear-table-cell-vertical-alignment" ||
           command == "set-table-cell-text-direction" ||
           command == "clear-table-cell-text-direction" ||
           command == "set-table-cell-width" ||
           command == "clear-table-cell-width";
}

auto run_table_cell_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "set-table-cell-fill") {
        return run_set_table_cell_fill_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-fill") {
        return run_clear_table_cell_fill_command(command, arguments, doc);
    }
    if (command == "set-table-cell-vertical-alignment") {
        return run_set_table_cell_vertical_alignment_command(command, arguments,
                                                            doc);
    }
    if (command == "clear-table-cell-vertical-alignment") {
        return run_clear_table_cell_vertical_alignment_command(command, arguments,
                                                              doc);
    }
    if (command == "set-table-cell-text-direction") {
        return run_set_table_cell_text_direction_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-text-direction") {
        return run_clear_table_cell_text_direction_command(command, arguments,
                                                          doc);
    }
    if (command == "set-table-cell-width") {
        return run_set_table_cell_width_command(command, arguments, doc);
    }
    if (command == "clear-table-cell-width") {
        return run_clear_table_cell_width_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
