#include "featherdoc_cli_template_table_text_support.hpp"

#include "featherdoc_cli_table_row_summary.hpp"

#include <cstddef>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {

auto report_template_table_text_failure(std::string_view command,
                                        std::string_view summary,
                                        std::string detail,
                                        const selected_template_part &selected,
                                        bool json_output, std::errc code)
    -> bool {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(code);
    error_info.detail = std::move(detail);
    error_info.entry_name = std::string(selected.part.entry_name());
    return report_operation_failure(command, "mutate", summary, error_info,
                                    json_output);
}

auto validate_template_table_row_text_range(
    std::string_view command, template_table_text_context &context,
    std::size_t start_row_index,
    const std::vector<std::vector<std::string>> &rows, bool json_output)
    -> bool {
    const auto row_summaries = collect_table_row_summaries(context.table);
    if (start_row_index >= row_summaries.size()) {
        report_template_table_text_failure(
            command, "row index is out of range",
            "row index '" + std::to_string(start_row_index) +
                "' is out of range for table index '" +
                std::to_string(context.table_index) + "'",
            context.selected, json_output);
        return false;
    }

    if (rows.size() > row_summaries.size() - start_row_index) {
        report_template_table_text_failure(
            command, "row range exceeds table bounds",
            "row range starting at row index '" +
                std::to_string(start_row_index) + "' with count '" +
                std::to_string(rows.size()) + "' exceeds table index '" +
                std::to_string(context.table_index) + "' row count '" +
                std::to_string(row_summaries.size()) + "'",
            context.selected, json_output);
        return false;
    }

    for (std::size_t offset = 0U; offset < rows.size(); ++offset) {
        const auto target_row_index = start_row_index + offset;
        const auto replacement_cell_count = rows[offset].size();
        const auto target_cell_count = row_summaries[target_row_index].cell_count;
        if (replacement_cell_count != target_cell_count) {
            report_template_table_text_failure(
                command, "replacement row cell count does not match target row",
                "replacement row at offset '" + std::to_string(offset) +
                    "' contains '" + std::to_string(replacement_cell_count) +
                    "' cells but target row index '" +
                    std::to_string(target_row_index) + "' contains '" +
                    std::to_string(target_cell_count) + "' cells",
                context.selected, json_output);
            return false;
        }
    }

    return true;
}

auto validate_template_table_cell_block_text_range(
    std::string_view command, template_table_text_context &context,
    std::size_t start_row_index, std::size_t start_cell_index,
    const std::vector<std::vector<std::string>> &rows, bool json_output)
    -> bool {
    const auto row_summaries = collect_table_row_summaries(context.table);
    if (start_row_index >= row_summaries.size()) {
        report_template_table_text_failure(
            command, "row index is out of range",
            "row index '" + std::to_string(start_row_index) +
                "' is out of range for table index '" +
                std::to_string(context.table_index) + "'",
            context.selected, json_output);
        return false;
    }

    if (rows.size() > row_summaries.size() - start_row_index) {
        report_template_table_text_failure(
            command, "row range exceeds table bounds",
            "row range starting at row index '" +
                std::to_string(start_row_index) + "' with count '" +
                std::to_string(rows.size()) + "' exceeds table index '" +
                std::to_string(context.table_index) + "' row count '" +
                std::to_string(row_summaries.size()) + "'",
            context.selected, json_output);
        return false;
    }

    for (std::size_t offset = 0U; offset < rows.size(); ++offset) {
        const auto target_row_index = start_row_index + offset;
        const auto replacement_cell_count = rows[offset].size();
        const auto target_cell_count = row_summaries[target_row_index].cell_count;
        if (start_cell_index >= target_cell_count ||
            replacement_cell_count > target_cell_count - start_cell_index) {
            report_template_table_text_failure(
                command, "cell block exceeds target row bounds",
                "replacement row at offset '" + std::to_string(offset) +
                    "' starting at cell index '" +
                    std::to_string(start_cell_index) + "' with count '" +
                    std::to_string(replacement_cell_count) +
                    "' exceeds target row index '" +
                    std::to_string(target_row_index) + "' cell count '" +
                    std::to_string(target_cell_count) + "'",
                context.selected, json_output);
            return false;
        }
    }

    return true;
}

} // namespace featherdoc_cli
