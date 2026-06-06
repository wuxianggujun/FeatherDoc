#include "featherdoc_cli_template_table_resolve.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_template_table_json_patch_apply.hpp"

#include <system_error>

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

auto resolve_template_table(selected_template_part &selected,
                            std::size_t table_index, featherdoc::Table &table,
                            std::string_view command, bool json_output,
                            std::string_view stage) -> bool {
    table = selected.part.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next();
         ++current_index) {
        table.next();
    }

    if (!table.has_next()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "table index '" + std::to_string(table_index) + "' is out of range";
        error_info.entry_name = std::string(selected.part.entry_name());
        return report_operation_failure(command, stage,
                                        "table index is out of range", error_info,
                                        json_output);
    }

    return true;
}

auto resolve_template_table_index(featherdoc::Document &doc,
                                  selected_template_part &selected,
                                  const std::optional<std::size_t> &table_index,
                                  const std::optional<std::string> &bookmark_name,
                                  std::size_t &resolved_table_index,
                                  std::string_view command, bool json_output,
                                  std::string_view stage) -> bool {
    if (table_index.has_value()) {
        resolved_table_index = *table_index;
        return true;
    }

    if (!bookmark_name.has_value()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "either a table index or --bookmark <name> is required";
        error_info.entry_name = std::string(selected.part.entry_name());
        return report_operation_failure(command, stage,
                                        "template table target was not specified",
                                        error_info, json_output);
    }

    const auto resolved = selected.part.find_table_index_by_bookmark(*bookmark_name);
    if (!resolved.has_value()) {
        return report_document_error(command, stage, doc.last_error(), json_output);
    }

    resolved_table_index = *resolved;
    return true;
}

auto resolve_template_table_index(
    featherdoc::Document &doc, selected_template_part &selected,
    const featherdoc::template_table_selector &selector,
    std::size_t &resolved_table_index, std::string_view command,
    bool json_output, std::string_view stage) -> bool {
    const auto resolved = selected.part.find_table_index(selector);
    if (!resolved.has_value()) {
        return report_document_error(command, stage, doc.last_error(), json_output);
    }

    resolved_table_index = *resolved;
    return true;
}

auto resolve_template_table_index_for_batch(
    featherdoc::Document &doc, selected_template_part &selected,
    const std::optional<std::size_t> &table_index,
    const std::optional<std::string> &bookmark_name,
    std::size_t &resolved_table_index, std::size_t operation_index,
    std::string_view command, bool json_output,
    std::string_view stage) -> bool {
    if (table_index.has_value()) {
        resolved_table_index = *table_index;
        return true;
    }

    if (!bookmark_name.has_value()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "either a table index or --bookmark <name> is required";
        error_info.entry_name = std::string(selected.part.entry_name());
        annotate_template_table_json_batch_error(operation_index, error_info);
        return report_operation_failure(
            command, stage, "template table target was not specified", error_info,
            json_output);
    }

    const auto resolved = selected.part.find_table_index_by_bookmark(*bookmark_name);
    if (!resolved.has_value()) {
        auto error_info = doc.last_error();
        annotate_template_table_json_batch_error(operation_index, error_info);
        return report_document_error(command, stage, error_info, json_output);
    }

    resolved_table_index = *resolved;
    return true;
}

auto resolve_template_table_index_for_batch(
    featherdoc::Document &doc, selected_template_part &selected,
    const featherdoc::template_table_selector &selector,
    std::size_t &resolved_table_index, std::size_t operation_index,
    std::string_view command, bool json_output,
    std::string_view stage) -> bool {
    const auto resolved = selected.part.find_table_index(selector);
    if (!resolved.has_value()) {
        auto error_info = doc.last_error();
        annotate_template_table_json_batch_error(operation_index, error_info);
        return report_document_error(command, stage, error_info, json_output);
    }

    resolved_table_index = *resolved;
    return true;
}

auto resolve_template_table_for_batch(
    selected_template_part &selected, std::size_t table_index,
    featherdoc::Table &table, std::size_t operation_index,
    std::string_view command, bool json_output,
    std::string_view stage) -> bool {
    table = selected.part.tables();
    for (std::size_t current_index = 0;
         current_index < table_index && table.has_next();
         ++current_index) {
        table.next();
    }

    if (!table.has_next()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "table index '" + std::to_string(table_index) + "' is out of range";
        error_info.entry_name = std::string(selected.part.entry_name());
        annotate_template_table_json_batch_error(operation_index, error_info);
        return report_operation_failure(command, stage,
                                        "table index is out of range", error_info,
                                        json_output);
    }

    return true;
}

auto resolve_template_table_row(selected_template_part &selected,
                                std::size_t table_index, std::size_t row_index,
                                featherdoc::TableRow &row,
                                std::string_view command, bool json_output,
                                std::string_view stage) -> bool {
    featherdoc::Table table;
    if (!resolve_template_table(selected, table_index, table, command,
                                json_output, stage)) {
        return false;
    }

    row = table.rows();
    for (std::size_t current_index = 0;
         current_index < row_index && row.has_next();
         ++current_index) {
        row.next();
    }

    if (!row.has_next()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'";
        error_info.entry_name = std::string(selected.part.entry_name());
        return report_operation_failure(command, stage,
                                        "row index is out of range", error_info,
                                        json_output);
    }

    return true;
}

auto resolve_template_table_cell(selected_template_part &selected,
                                 std::size_t table_index,
                                 std::size_t row_index,
                                 std::size_t cell_index,
                                 featherdoc::TableCell &cell,
                                 std::string_view command, bool json_output,
                                 std::string_view stage) -> bool {
    featherdoc::TableRow row;
    if (!resolve_template_table_row(selected, table_index, row_index, row, command,
                                    json_output, stage)) {
        return false;
    }

    cell = row.cells();
    for (std::size_t current_index = 0;
         current_index < cell_index && cell.has_next();
         ++current_index) {
        cell.next();
    }

    if (cell.has_next()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "cell index '" + std::to_string(cell_index) +
        "' is out of range for row index '" + std::to_string(row_index) +
        "' in table index '" + std::to_string(table_index) + "'";
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, stage,
                                    "cell index is out of range", error_info,
                                    json_output);
}

auto resolve_template_table_cell_by_grid_column(selected_template_part &selected,
                                                std::size_t table_index,
                                                std::size_t row_index,
                                                std::size_t grid_column,
                                                featherdoc::TableCell &cell,
                                                std::string_view command,
                                                bool json_output,
                                                std::string_view stage) -> bool {
    featherdoc::TableRow row;
    if (!resolve_template_table_row(selected, table_index, row_index, row, command,
                                    json_output, stage)) {
        return false;
    }

    auto resolved_cell = row.find_cell_by_grid_column(grid_column);
    if (resolved_cell.has_value()) {
        cell = *resolved_cell;
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "grid column '" + std::to_string(grid_column) +
        "' is out of range for row index '" + std::to_string(row_index) +
        "' in table index '" + std::to_string(table_index) + "'";
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, stage,
                                    "grid column is out of range", error_info,
                                    json_output);
}

auto load_template_table_summary(featherdoc::Document &doc,
                                 selected_template_part &selected,
                                 std::size_t table_index,
                                 featherdoc::table_inspection_summary &table,
                                 std::string_view command, bool json_output,
                                 std::string_view stage) -> bool {
    const auto inspected_table = selected.part.inspect_table(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, stage, error_info, json_output);
        return false;
    }

    if (inspected_table.has_value()) {
        table = *inspected_table;
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "table index '" + std::to_string(table_index) + "' is out of range";
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, stage,
                                    "table index is out of range", error_info,
                                    json_output);
}

auto load_template_table_cell_summary(
    featherdoc::Document &doc, selected_template_part &selected,
    std::size_t table_index, std::size_t row_index, std::size_t cell_index,
    featherdoc::table_cell_inspection_summary &cell, std::string_view command,
    bool json_output, std::string_view stage) -> bool {
    featherdoc::table_inspection_summary table{};
    if (!load_template_table_summary(doc, selected, table_index, table, command,
                                     json_output, stage)) {
        return false;
    }

    const auto inspected_cell =
        selected.part.inspect_table_cell(table_index, row_index, cell_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, stage, error_info, json_output);
        return false;
    }

    if (inspected_cell.has_value()) {
        cell = *inspected_cell;
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    if (row_index >= table.row_count) {
        error_info.detail =
            "row index '" + std::to_string(row_index) +
            "' is out of range for table index '" + std::to_string(table_index) +
            "'";
        error_info.entry_name = std::string(selected.part.entry_name());
        return report_operation_failure(command, stage,
                                        "row index is out of range", error_info,
                                        json_output);
    }

    error_info.detail =
        "cell index '" + std::to_string(cell_index) +
        "' is out of range for row index '" + std::to_string(row_index) +
        "' in table index '" + std::to_string(table_index) + "'";
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, stage,
                                    "cell index is out of range", error_info,
                                    json_output);
}

auto load_template_table_cells_summary(
    featherdoc::Document &doc, selected_template_part &selected,
    std::size_t table_index,
    std::vector<featherdoc::table_cell_inspection_summary> &cells,
    std::string_view command, bool json_output,
    std::string_view stage) -> bool {
    featherdoc::table_inspection_summary table{};
    if (!load_template_table_summary(doc, selected, table_index, table, command,
                                     json_output, stage)) {
        return false;
    }

    cells = selected.part.inspect_table_cells(table_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, stage, error_info, json_output);
        return false;
    }

    return true;
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
