#include "featherdoc_cli_table_commands.hpp"

#include "featherdoc_cli_table_cell_style_commands.hpp"
#include "featherdoc_cli_table_column_commands.hpp"
#include "featherdoc_cli_table_format_commands.hpp"
#include "featherdoc_cli_table_inspect_commands.hpp"
#include "featherdoc_cli_table_merge_commands.hpp"
#include "featherdoc_cli_table_position_commands.hpp"
#include "featherdoc_cli_table_row_commands.hpp"
#include "featherdoc_cli_table_structure_commands.hpp"
#include "featherdoc_cli_table_style_commands.hpp"
#include "featherdoc_cli_table_text_commands.hpp"

namespace featherdoc_cli {

auto is_table_command(std::string_view command) -> bool {
    return is_table_style_command(command) ||
           is_table_cell_style_command(command) ||
           is_table_format_command(command) ||
           is_table_position_command(command) ||
           is_table_structure_command(command) ||
           command == "inspect-tables" ||
           command == "inspect-table-rows" ||
           command == "inspect-table-cells" ||
           command == "set-table-cell-text" ||
           command == "merge-table-cells" ||
           command == "unmerge-table-cells" ||
           command == "append-table-row" ||
           command == "insert-table-row-before" ||
           command == "insert-table-row-after" ||
           command == "remove-table-row" ||
           command == "insert-paragraph-after-table" ||
           command == "insert-table-column-before" ||
           command == "insert-table-column-after" ||
           command == "remove-table-column" ||
           command == "set-table-row-height" ||
           command == "clear-table-row-height" ||
           command == "set-table-row-cant-split" ||
           command == "clear-table-row-cant-split" ||
           command == "set-table-row-repeat-header" ||
           command == "clear-table-row-repeat-header";
}

auto run_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (is_table_style_command(command)) {
        return run_table_style_command(command, arguments, doc);
    }
    if (is_table_cell_style_command(command)) {
        return run_table_cell_style_command(command, arguments, doc);
    }
    if (is_table_format_command(command)) {
        return run_table_format_command(command, arguments, doc);
    }
    if (is_table_position_command(command)) {
        return run_table_position_command(command, arguments);
    }
    if (command == "inspect-tables") {
        return run_inspect_tables_command(command, arguments);
    }
    if (command == "inspect-table-rows") {
        return run_inspect_table_rows_command(command, arguments);
    }
    if (command == "inspect-table-cells") {
        return run_inspect_table_cells_command(command, arguments);
    }
    if (command == "set-table-cell-text") {
        return run_set_table_cell_text_command(command, arguments);
    }
    if (command == "merge-table-cells") {
        return run_merge_table_cells_command(command, arguments);
    }
    if (command == "unmerge-table-cells") {
        return run_unmerge_table_cells_command(command, arguments);
    }
    if (is_table_structure_command(command)) {
        return run_table_structure_command(command, arguments);
    }
    if (command == "append-table-row") {
        return run_append_table_row_command(command, arguments);
    }
    if (command == "insert-table-row-before") {
        return run_insert_table_row_before_command(command, arguments);
    }
    if (command == "insert-table-row-after") {
        return run_insert_table_row_after_command(command, arguments);
    }
    if (command == "remove-table-row") {
        return run_remove_table_row_command(command, arguments);
    }
    if (command == "insert-paragraph-after-table") {
        return run_insert_paragraph_after_table_command(command, arguments);
    }
    if (command == "insert-table-column-before") {
        return run_insert_table_column_before_command(command, arguments);
    }
    if (command == "insert-table-column-after") {
        return run_insert_table_column_after_command(command, arguments);
    }
    if (command == "remove-table-column") {
        return run_remove_table_column_command(command, arguments);
    }
    if (command == "set-table-row-height") {
        return run_set_table_row_height_command(command, arguments);
    }
    if (command == "clear-table-row-height") {
        return run_clear_table_row_height_command(command, arguments);
    }
    if (command == "set-table-row-cant-split") {
        return run_set_table_row_cant_split_command(command, arguments);
    }
    if (command == "clear-table-row-cant-split") {
        return run_clear_table_row_cant_split_command(command, arguments);
    }
    if (command == "set-table-row-repeat-header") {
        return run_set_table_row_repeat_header_command(command, arguments);
    }
    if (command == "clear-table-row-repeat-header") {
        return run_clear_table_row_repeat_header_command(command, arguments);
    }

    return 2;
}

} // namespace featherdoc_cli
