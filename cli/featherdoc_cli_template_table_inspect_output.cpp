#include "featherdoc_cli_template_table_inspect_output.hpp"

#include "featherdoc_cli_table_inspect_output.hpp"
#include "featherdoc_cli_table_inspect_output_detail.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <vector>

namespace featherdoc_cli {

void inspect_template_tables(
    const selected_template_part &selected,
    const std::vector<featherdoc::table_inspection_summary> &tables,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"count\":" << tables.size() << ",\"tables\":[";
        for (std::size_t index = 0; index < tables.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_summary(std::cout, tables[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "tables: " << tables.size() << '\n';
    for (const auto &table : tables) {
        print_table_summary(std::cout, table);
        std::cout << '\n';
    }
}

void inspect_template_table(const selected_template_part &selected,
                            const featherdoc::table_inspection_summary &table,
                            bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table\":";
        write_json_table_summary(std::cout, table);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table(table, false);
}

void inspect_template_table_cells(
    const selected_template_part &selected, std::size_t table_index,
    std::optional<std::size_t> row_index,
    const std::vector<featherdoc::table_cell_inspection_summary> &cells,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table_index\":" << table_index;
        if (row_index.has_value()) {
            std::cout << ",\"row_index\":" << *row_index;
        }
        std::cout << ",\"count\":" << cells.size() << ",\"cells\":[";
        for (std::size_t index = 0; index < cells.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_cell_summary(std::cout, cells[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table_cells(table_index, row_index, cells, false);
}

void inspect_template_table_cell(
    const selected_template_part &selected, std::size_t table_index,
    const featherdoc::table_cell_inspection_summary &cell, bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table_index\":" << table_index << ",\"cell\":";
        write_json_table_cell_summary(std::cout, cell);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table_cell(table_index, cell, false);
}

void inspect_template_table_rows(
    const selected_template_part &selected, std::size_t table_index,
    const std::vector<table_row_inspection_summary> &rows, bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table_index\":" << table_index
                  << ",\"count\":" << rows.size() << ",\"rows\":[";
        for (std::size_t index = 0; index < rows.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_table_row_summary(std::cout, rows[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table_rows(table_index, rows, false);
}

void inspect_template_table_row(const selected_template_part &selected,
                                std::size_t table_index,
                                const table_row_inspection_summary &row,
                                bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"table_index\":" << table_index << ",\"row\":";
        write_json_table_row_summary(std::cout, row);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    inspect_table_row(table_index, row, false);
}

} // namespace featherdoc_cli
