#include "featherdoc_cli_template_table_commands.hpp"

#include "featherdoc_cli_template_table_column_commands.hpp"
#include "featherdoc_cli_template_table_inspect_commands.hpp"
#include "featherdoc_cli_template_table_json_patch_commands.hpp"
#include "featherdoc_cli_template_table_merge_commands.hpp"
#include "featherdoc_cli_template_table_row_commands.hpp"
#include "featherdoc_cli_template_table_text_commands.hpp"

namespace featherdoc_cli {

auto is_template_table_command(std::string_view command) -> bool {
    return command == "inspect-template-tables" ||
           command == "inspect-template-table-rows" ||
           command == "inspect-template-table-cells" ||
           command == "set-template-table-cell-text" ||
           command == "set-template-table-row-texts" ||
           command == "set-template-table-cell-block-texts" ||
           command == "set-template-table-from-json" ||
           command == "set-template-tables-from-json" ||
           command == "insert-template-table-column-before" ||
           command == "insert-template-table-column-after" ||
           command == "remove-template-table-column" ||
           command == "append-template-table-row" ||
           command == "insert-template-table-row-before" ||
           command == "insert-template-table-row-after" ||
           command == "remove-template-table-row" ||
           command == "merge-template-table-cells" ||
           command == "unmerge-template-table-cells";
}

auto run_template_table_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "inspect-template-tables") {
        return run_inspect_template_tables_command(command, arguments);
    }
    if (command == "inspect-template-table-rows") {
        return run_inspect_template_table_rows_command(command, arguments);
    }
    if (command == "inspect-template-table-cells") {
        return run_inspect_template_table_cells_command(command, arguments);
    }
    if (command == "set-template-table-cell-text") {
        return run_set_template_table_cell_text_command(command, arguments);
    }
    if (command == "set-template-table-row-texts") {
        return run_set_template_table_row_texts_command(command, arguments);
    }
    if (command == "set-template-table-cell-block-texts") {
        return run_set_template_table_cell_block_texts_command(command,
                                                               arguments);
    }
    if (command == "set-template-table-from-json") {
        return run_set_template_table_from_json_command(command, arguments);
    }
    if (command == "set-template-tables-from-json") {
        return run_set_template_tables_from_json_command(command, arguments);
    }
    if (command == "insert-template-table-column-before") {
        return run_insert_template_table_column_before_command(command, arguments);
    }
    if (command == "insert-template-table-column-after") {
        return run_insert_template_table_column_after_command(command, arguments);
    }
    if (command == "remove-template-table-column") {
        return run_remove_template_table_column_command(command, arguments);
    }
    if (command == "append-template-table-row") {
        return run_append_template_table_row_command(command, arguments);
    }
    if (command == "insert-template-table-row-before") {
        return run_insert_template_table_row_before_command(command, arguments);
    }
    if (command == "insert-template-table-row-after") {
        return run_insert_template_table_row_after_command(command, arguments);
    }
    if (command == "remove-template-table-row") {
        return run_remove_template_table_row_command(command, arguments);
    }
    if (command == "merge-template-table-cells") {
        return run_merge_template_table_cells_command(command, arguments);
    }
    if (command == "unmerge-template-table-cells") {
        return run_unmerge_template_table_cells_command(command, arguments);
    }

    return 2;
}

} // namespace featherdoc_cli
