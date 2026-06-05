#include "featherdoc_cli_template_table_json_patch_apply.hpp"

#include "featherdoc_cli_errors.hpp"

#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto format_template_table_json_batch_operation_label(
    std::size_t operation_index) -> std::string {
    return "operation[" + std::to_string(operation_index) + "]";
}

auto collect_template_table_row_cell_counts(featherdoc::Table &table)
    -> std::vector<std::size_t> {
    std::vector<std::size_t> row_cell_counts;
    auto row = table.rows();
    while (row.has_next()) {
        std::size_t cell_count = 0U;
        auto cell = row.cells();
        while (cell.has_next()) {
            ++cell_count;
            cell.next();
        }

        row_cell_counts.push_back(cell_count);
        row.next();
    }

    return row_cell_counts;
}

auto make_template_table_patch_error(std::string_view entry_name,
                                     std::string detail)
    -> featherdoc::document_error_info {
    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = std::move(detail);
    error_info.entry_name = std::string(entry_name);
    return error_info;
}

} // namespace

void annotate_template_table_json_batch_error(
    std::optional<std::size_t> operation_index,
    featherdoc::document_error_info &error_info) {
    if (!operation_index.has_value()) {
        return;
    }

    const auto label =
        format_template_table_json_batch_operation_label(*operation_index);
    if (error_info.detail.empty()) {
        error_info.detail = label;
    } else {
        error_info.detail = label + ": " + error_info.detail;
    }
}

auto apply_template_table_json_patch(
    std::string_view command, featherdoc::Table &table,
    std::string_view entry_name, std::size_t resolved_table_index,
    const template_table_json_patch &patch, bool json_output,
    std::optional<std::size_t> operation_index) -> bool {
    const auto row_cell_counts = collect_template_table_row_cell_counts(table);
    if (patch.start_row_index >= row_cell_counts.size()) {
        auto error_info = make_template_table_patch_error(
            entry_name,
            "row index '" + std::to_string(patch.start_row_index) +
                "' is out of range for table index '" +
                std::to_string(resolved_table_index) + "'");
        annotate_template_table_json_batch_error(operation_index, error_info);
        report_operation_failure(command, "mutate", "row index is out of range",
                                 error_info, json_output);
        return false;
    }

    if (patch.rows.size() > row_cell_counts.size() - patch.start_row_index) {
        auto error_info = make_template_table_patch_error(
            entry_name,
            "row range starting at row index '" +
                std::to_string(patch.start_row_index) + "' with count '" +
                std::to_string(patch.rows.size()) + "' exceeds table index '" +
                std::to_string(resolved_table_index) + "' row count '" +
                std::to_string(row_cell_counts.size()) + "'");
        annotate_template_table_json_batch_error(operation_index, error_info);
        report_operation_failure(command, "mutate",
                                 "row range exceeds table bounds", error_info,
                                 json_output);
        return false;
    }

    if (patch.mode == template_table_json_patch_mode::rows) {
        for (std::size_t offset = 0U; offset < patch.rows.size(); ++offset) {
            const auto target_row_index = patch.start_row_index + offset;
            const auto replacement_cell_count = patch.rows[offset].size();
            const auto target_cell_count = row_cell_counts[target_row_index];
            if (replacement_cell_count != target_cell_count) {
                auto error_info = make_template_table_patch_error(
                    entry_name,
                    "replacement row at offset '" + std::to_string(offset) +
                        "' contains '" +
                        std::to_string(replacement_cell_count) +
                        "' cells but target row index '" +
                        std::to_string(target_row_index) + "' contains '" +
                        std::to_string(target_cell_count) + "' cells");
                annotate_template_table_json_batch_error(operation_index,
                                                         error_info);
                report_operation_failure(
                    command, "mutate",
                    "replacement row cell count does not match target row",
                    error_info, json_output);
                return false;
            }
        }

        if (!table.set_rows_texts(patch.start_row_index, patch.rows)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail =
                "failed to set text for the requested table row range";
            error_info.entry_name = std::string(entry_name);
            annotate_template_table_json_batch_error(operation_index, error_info);
            report_operation_failure(command, "mutate",
                                     "failed to set template table row texts",
                                     error_info, json_output);
            return false;
        }

        return true;
    }

    for (std::size_t offset = 0U; offset < patch.rows.size(); ++offset) {
        const auto target_row_index = patch.start_row_index + offset;
        const auto replacement_cell_count = patch.rows[offset].size();
        const auto target_cell_count = row_cell_counts[target_row_index];
        if (patch.start_cell_index >= target_cell_count ||
            replacement_cell_count >
                target_cell_count - patch.start_cell_index) {
            auto error_info = make_template_table_patch_error(
                entry_name,
                "replacement row at offset '" + std::to_string(offset) +
                    "' starting at cell index '" +
                    std::to_string(patch.start_cell_index) + "' with count '" +
                    std::to_string(replacement_cell_count) +
                    "' exceeds target row index '" +
                    std::to_string(target_row_index) + "' cell count '" +
                    std::to_string(target_cell_count) + "'");
            annotate_template_table_json_batch_error(operation_index, error_info);
            report_operation_failure(command, "mutate",
                                     "cell block exceeds target row bounds",
                                     error_info, json_output);
            return false;
        }
    }

    if (!table.set_cell_block_texts(patch.start_row_index, patch.start_cell_index,
                                    patch.rows)) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::io_error);
        error_info.detail =
            "failed to set text for the requested table cell block";
        error_info.entry_name = std::string(entry_name);
        annotate_template_table_json_batch_error(operation_index, error_info);
        report_operation_failure(command, "mutate",
                                 "failed to set template table cell block texts",
                                 error_info, json_output);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
