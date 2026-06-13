#include "featherdoc_cli_table_row_summary.hpp"

namespace featherdoc_cli {

auto make_table_row_summary(featherdoc::TableRow &row, std::size_t row_index)
    -> table_row_inspection_summary {
    table_row_inspection_summary summary;
    summary.row_index = row_index;
    summary.height_twips = row.height_twips();
    summary.height_rule = row.height_rule();
    summary.cant_split = row.cant_split();
    summary.repeats_header = row.repeats_header();

    auto cell = row.cells();
    while (cell.has_next()) {
        summary.cell_texts.push_back(cell.get_text());
        ++summary.cell_count;
        cell.next();
    }

    return summary;
}

auto collect_table_row_summaries(featherdoc::Table &table)
    -> std::vector<table_row_inspection_summary> {
    std::vector<table_row_inspection_summary> summaries;
    auto row = table.rows();
    for (std::size_t row_index = 0U; row.has_next(); ++row_index) {
        summaries.push_back(make_table_row_summary(row, row_index));
        row.next();
    }

    return summaries;
}

} // namespace featherdoc_cli
