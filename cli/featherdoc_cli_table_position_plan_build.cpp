#include "featherdoc_cli_table_position_plan_build.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto describe_table_position_optional_string(
    const std::optional<std::string> &value) -> std::string {
    return value.has_value() ? *value : std::string{"null"};
}

auto describe_table_position_optional_u32(
    const std::optional<std::uint32_t> &value) -> std::string {
    return value.has_value() ? std::to_string(*value) : std::string{"null"};
}

auto describe_table_position_column_widths(
    const std::vector<std::optional<std::uint32_t>> &values) -> std::string {
    auto stream = std::ostringstream{};
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        stream << describe_table_position_optional_u32(values[index]);
    }
    stream << ']';
    return stream.str();
}

auto make_table_position_plan_output_path(
    const std::optional<std::filesystem::path> &input_path,
    table_position_preset preset) -> std::optional<std::filesystem::path> {
    if (!input_path.has_value()) {
        return std::nullopt;
    }

    auto output_path = *input_path;
    const auto stem = output_path.stem().string();
    const auto extension = output_path.extension().string();
    output_path.replace_filename(
        stem + "-table-position-" + std::string(table_position_preset_name(preset)) +
        (extension.empty() ? std::string{".docx"} : extension));
    return output_path;
}

auto make_table_position_preset_batch_command(
    const std::vector<std::size_t> &table_indices, std::size_t table_count,
    table_position_preset preset) -> std::optional<std::string> {
    if (table_indices.empty()) {
        return std::nullopt;
    }

    auto command = std::string{"set-table-position <input.docx> "};
    if (table_indices.size() == table_count && table_count > 0U) {
        command += "all";
    } else {
        command += std::to_string(table_indices.front());
        for (std::size_t index = 1U; index < table_indices.size(); ++index) {
            command += " --table ";
            command += std::to_string(table_indices[index]);
        }
    }
    command += " --preset ";
    command += std::string(table_position_preset_name(preset));
    return command;
}

} // namespace

auto make_table_position_preset(table_position_preset preset)
    -> featherdoc::table_position {
    featherdoc::table_position position{};
    switch (preset) {
    case table_position_preset::paragraph_callout:
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::column;
        position.horizontal_offset_twips = 0;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::paragraph;
        position.vertical_offset_twips = 0;
        position.left_from_text_twips = 144U;
        position.right_from_text_twips = 144U;
        position.top_from_text_twips = 72U;
        position.bottom_from_text_twips = 72U;
        position.overlap = featherdoc::table_overlap::never;
        return position;
    case table_position_preset::page_corner:
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::page;
        position.horizontal_offset_twips = 720;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 720;
        position.left_from_text_twips = 144U;
        position.right_from_text_twips = 144U;
        position.top_from_text_twips = 144U;
        position.bottom_from_text_twips = 144U;
        position.overlap = featherdoc::table_overlap::never;
        return position;
    case table_position_preset::margin_anchor:
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::margin;
        position.horizontal_offset_twips = 0;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::paragraph;
        position.vertical_offset_twips = 0;
        position.left_from_text_twips = 144U;
        position.right_from_text_twips = 144U;
        position.top_from_text_twips = 72U;
        position.bottom_from_text_twips = 72U;
        position.overlap = featherdoc::table_overlap::never;
        return position;
    }

    return position;
}

auto make_table_position_table_fingerprint(
    const featherdoc::table_inspection_summary &table)
    -> table_position_table_fingerprint {
    auto fingerprint = table_position_table_fingerprint{};
    fingerprint.table_index = table.index;
    fingerprint.style_id = table.style_id;
    fingerprint.width_twips = table.width_twips;
    fingerprint.row_count = table.row_count;
    fingerprint.column_count = table.column_count;
    fingerprint.column_widths = table.column_widths;
    fingerprint.text = table.text;
    return fingerprint;
}

auto table_position_table_fingerprints_equal(
    const table_position_table_fingerprint &left,
    const table_position_table_fingerprint &right) -> bool {
    return left.table_index == right.table_index &&
           left.style_id == right.style_id &&
           left.width_twips == right.width_twips &&
           left.row_count == right.row_count &&
           left.column_count == right.column_count &&
           left.column_widths == right.column_widths && left.text == right.text;
}

auto describe_table_position_fingerprint_differences(
    const table_position_table_fingerprint &expected,
    const table_position_table_fingerprint &current) -> std::string {
    auto detail = std::string{"table fingerprint changed since plan was generated: table "} +
                  std::to_string(expected.table_index);
    auto difference_count = std::size_t{0U};
    const auto append_difference = [&detail, &difference_count](
                                       std::string_view field,
                                       const std::string &expected_value,
                                       const std::string &current_value) {
        detail += difference_count == 0U ? "; changed fields: " : "; ";
        detail += std::string(field) + " expected=" + expected_value +
                  " current=" + current_value;
        ++difference_count;
    };

    if (expected.table_index != current.table_index) {
        append_difference("table_index", std::to_string(expected.table_index),
                          std::to_string(current.table_index));
    }
    if (expected.style_id != current.style_id) {
        append_difference("style_id",
                          describe_table_position_optional_string(expected.style_id),
                          describe_table_position_optional_string(current.style_id));
    }
    if (expected.width_twips != current.width_twips) {
        append_difference("width_twips",
                          describe_table_position_optional_u32(expected.width_twips),
                          describe_table_position_optional_u32(current.width_twips));
    }
    if (expected.row_count != current.row_count) {
        append_difference("row_count", std::to_string(expected.row_count),
                          std::to_string(current.row_count));
    }
    if (expected.column_count != current.column_count) {
        append_difference("column_count", std::to_string(expected.column_count),
                          std::to_string(current.column_count));
    }
    if (expected.column_widths != current.column_widths) {
        append_difference("column_widths",
                          describe_table_position_column_widths(expected.column_widths),
                          describe_table_position_column_widths(current.column_widths));
    }
    if (expected.text != current.text) {
        append_difference("text", json_escape(expected.text), json_escape(current.text));
    }
    return detail;
}

auto table_positions_equal(const featherdoc::table_position &left,
                           const featherdoc::table_position &right) -> bool {
    return left.horizontal_reference == right.horizontal_reference &&
           left.horizontal_offset_twips == right.horizontal_offset_twips &&
           left.horizontal_spec == right.horizontal_spec &&
           left.vertical_reference == right.vertical_reference &&
           left.vertical_offset_twips == right.vertical_offset_twips &&
           left.vertical_spec == right.vertical_spec &&
           left.left_from_text_twips == right.left_from_text_twips &&
           left.right_from_text_twips == right.right_from_text_twips &&
           left.top_from_text_twips == right.top_from_text_twips &&
           left.bottom_from_text_twips == right.bottom_from_text_twips &&
           left.overlap == right.overlap;
}

auto build_table_position_preset_plan(
    const std::vector<featherdoc::table_inspection_summary> &tables,
    table_position_preset preset, bool replace_positioned,
    const std::optional<std::filesystem::path> &input_path,
    const std::optional<std::filesystem::path> &output_path)
    -> table_position_preset_plan {
    auto plan = table_position_preset_plan{};
    plan.preset = preset;
    plan.target_position = make_table_position_preset(preset);
    plan.table_count = tables.size();
    const auto input_argument = quote_cli_argument(
        input_path.has_value() ? input_path->string() : std::string{"<input.docx>"});
    const auto resolved_output_path = output_path.has_value()
                                          ? output_path
                                          : make_table_position_plan_output_path(
                                                input_path, preset);
    const auto output_argument = quote_cli_argument(
        resolved_output_path.has_value() ? resolved_output_path->string()
                                         : std::string{"<output.docx>"});

    for (const auto &table : tables) {
        const auto fingerprint = make_table_position_table_fingerprint(table);
        plan.table_fingerprints.push_back(fingerprint);
        if (table.position.has_value()) {
            ++plan.positioned_count;
            if (table_positions_equal(*table.position, plan.target_position)) {
                ++plan.already_matching_count;
                plan.already_matching_table_indices.push_back(table.index);
                continue;
            }
        } else {
            ++plan.unpositioned_count;
        }

        auto item = table_position_preset_plan_item{};
        item.table_index = table.index;
        item.current_position = table.position;
        item.target_position = plan.target_position;
        item.fingerprint = fingerprint;
        if (!table.position.has_value()) {
            item.action = "set-preset";
            item.automatic = true;
            item.recommended_command =
                "set-table-position <input.docx> " + std::to_string(table.index) +
                " --preset " + std::string(table_position_preset_name(preset));
            item.resolved_output_path = resolved_output_path;
            item.resolved_recommended_command =
                "set-table-position " + input_argument + " " +
                std::to_string(table.index) + " --preset " +
                std::string(table_position_preset_name(preset)) +
                " --output " + output_argument;
            plan.automatic_table_indices.push_back(table.index);
            ++plan.set_count;
        } else if (replace_positioned) {
            item.action = "replace-preset";
            item.automatic = true;
            item.recommended_command =
                "set-table-position <input.docx> " + std::to_string(table.index) +
                " --preset " + std::string(table_position_preset_name(preset));
            item.resolved_output_path = resolved_output_path;
            item.resolved_recommended_command =
                "set-table-position " + input_argument + " " +
                std::to_string(table.index) + " --preset " +
                std::string(table_position_preset_name(preset)) +
                " --output " + output_argument;
            plan.automatic_table_indices.push_back(table.index);
            ++plan.replace_count;
        } else {
            item.action = "review-existing-position";
            item.automatic = false;
            item.recommended_command =
                "inspect-tables <input.docx> --table " +
                std::to_string(table.index) + " --json";
            item.resolved_recommended_command =
                "inspect-tables " + input_argument + " --table " +
                std::to_string(table.index) + " --json";
            plan.review_table_indices.push_back(table.index);
            ++plan.review_count;
        }
        plan.items.push_back(std::move(item));
    }

    plan.recommended_batch_command = make_table_position_preset_batch_command(
        plan.automatic_table_indices, plan.table_count, preset);
    if (plan.recommended_batch_command.has_value()) {
        plan.resolved_output_path = resolved_output_path;
        plan.resolved_recommended_batch_command = *plan.recommended_batch_command;
        const auto placeholder = std::string{"<input.docx>"};
        const auto placeholder_position =
            plan.resolved_recommended_batch_command->find(placeholder);
        if (placeholder_position != std::string::npos) {
            plan.resolved_recommended_batch_command->replace(
                placeholder_position, placeholder.size(), input_argument);
        }
        *plan.resolved_recommended_batch_command += " --output " + output_argument;
    }
    return plan;
}

} // namespace featherdoc_cli
