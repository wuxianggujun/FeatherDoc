#include "featherdoc_cli_table_position_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_text_size_array(std::ostream &stream,
                           const std::vector<std::size_t> &values) {
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        stream << values[index];
    }
    stream << ']';
}

void write_json_optional_table_overlap(
    std::ostream &stream,
    const std::optional<featherdoc::table_overlap> &overlap) {
    if (overlap.has_value()) {
        write_json_string(stream, table_overlap_name(*overlap));
    } else {
        stream << "null";
    }
}

void write_json_optional_table_position_horizontal_spec(
    std::ostream &stream,
    const std::optional<featherdoc::table_position_horizontal_spec> &spec) {
    if (spec.has_value()) {
        write_json_string(stream, table_position_horizontal_spec_name(*spec));
    } else {
        stream << "null";
    }
}

void write_json_optional_table_position_vertical_spec(
    std::ostream &stream,
    const std::optional<featherdoc::table_position_vertical_spec> &spec) {
    if (spec.has_value()) {
        write_json_string(stream, table_position_vertical_spec_name(*spec));
    } else {
        stream << "null";
    }
}

void write_json_table_position_table_fingerprint(
    std::ostream &stream, const table_position_table_fingerprint &fingerprint) {
    stream << "{\"table_index\":" << fingerprint.table_index
           << ",\"style_id\":";
    write_json_optional_string(stream, fingerprint.style_id);
    stream << ",\"width_twips\":";
    write_json_optional_u32(stream, fingerprint.width_twips);
    stream << ",\"row_count\":" << fingerprint.row_count
           << ",\"column_count\":" << fingerprint.column_count
           << ",\"column_widths\":[";
    for (std::size_t index = 0; index < fingerprint.column_widths.size();
         ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_optional_u32(stream, fingerprint.column_widths[index]);
    }
    stream << "],\"text\":";
    write_json_string(stream, fingerprint.text);
    stream << '}';
}

void write_json_table_position_table_fingerprints(
    std::ostream &stream,
    const std::vector<table_position_table_fingerprint> &fingerprints) {
    stream << '[';
    for (std::size_t index = 0; index < fingerprints.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_table_position_table_fingerprint(stream, fingerprints[index]);
    }
    stream << ']';
}

void write_json_table_position_preset_plan_item(
    std::ostream &stream, const table_position_preset_plan_item &item,
    table_position_preset preset) {
    stream << "{\"table_index\":" << item.table_index << ",\"action\":";
    write_json_string(stream, item.action);
    stream << ",\"automatic\":" << json_bool(item.automatic)
           << ",\"current_position\":";
    write_json_optional_table_position(stream, item.current_position);
    stream << ",\"target_preset\":";
    write_json_string(stream, table_position_preset_name(preset));
    stream << ",\"target_position\":";
    write_json_table_position(stream, item.target_position);
    stream << ",\"fingerprint\":";
    write_json_table_position_table_fingerprint(stream, item.fingerprint);
    stream << ",\"recommended_command\":";
    write_json_string(stream, item.recommended_command);
    if (item.resolved_output_path.has_value()) {
        stream << ",\"resolved_output_path\":";
        write_json_string(stream, item.resolved_output_path->string());
    }
    if (item.resolved_recommended_command.has_value()) {
        stream << ",\"resolved_recommended_command\":";
        write_json_string(stream, *item.resolved_recommended_command);
    }
    stream << '}';
}

} // namespace

void write_json_table_position(std::ostream &stream,
                               const featherdoc::table_position &position) {
    stream << "{\"horizontal_reference\":";
    write_json_string(
        stream,
        table_position_horizontal_reference_name(position.horizontal_reference));
    stream << ",\"horizontal_offset_twips\":"
           << position.horizontal_offset_twips << ",\"horizontal_spec\":";
    write_json_optional_table_position_horizontal_spec(stream,
                                                       position.horizontal_spec);
    stream << ",\"vertical_reference\":";
    write_json_string(stream,
                      table_position_vertical_reference_name(
                          position.vertical_reference));
    stream << ",\"vertical_offset_twips\":" << position.vertical_offset_twips
           << ",\"vertical_spec\":";
    write_json_optional_table_position_vertical_spec(stream,
                                                     position.vertical_spec);
    stream << ",\"left_from_text_twips\":";
    write_json_optional_u32_value(stream, position.left_from_text_twips);
    stream << ",\"right_from_text_twips\":";
    write_json_optional_u32_value(stream, position.right_from_text_twips);
    stream << ",\"top_from_text_twips\":";
    write_json_optional_u32_value(stream, position.top_from_text_twips);
    stream << ",\"bottom_from_text_twips\":";
    write_json_optional_u32_value(stream, position.bottom_from_text_twips);
    stream << ",\"overlap\":";
    write_json_optional_table_overlap(stream, position.overlap);
    stream << '}';
}

void write_json_optional_table_position(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position) {
    if (position.has_value()) {
        write_json_table_position(stream, *position);
    } else {
        stream << "null";
    }
}

void write_table_position_text(
    std::ostream &stream,
    const std::optional<featherdoc::table_position> &position) {
    if (!position.has_value()) {
        stream << "none";
        return;
    }

    stream << table_position_horizontal_reference_name(
                  position->horizontal_reference)
           << ':' << position->horizontal_offset_twips;
    if (position->horizontal_spec.has_value()) {
        stream << ':' << table_position_horizontal_spec_name(
                              *position->horizontal_spec);
    }
    stream << ','
           << table_position_vertical_reference_name(position->vertical_reference)
           << ':' << position->vertical_offset_twips;
    if (position->vertical_spec.has_value()) {
        stream << ':'
               << table_position_vertical_spec_name(*position->vertical_spec);
    }
    if (position->left_from_text_twips.has_value() ||
        position->right_from_text_twips.has_value() ||
        position->top_from_text_twips.has_value() ||
        position->bottom_from_text_twips.has_value()) {
        stream << " wrap=";
        write_json_optional_u32_value(stream, position->left_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->right_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->top_from_text_twips);
        stream << '/';
        write_json_optional_u32_value(stream, position->bottom_from_text_twips);
    }
    if (position->overlap.has_value()) {
        stream << " overlap=" << table_overlap_name(*position->overlap);
    }
}

void write_json_table_position_preset_plan(
    std::ostream &stream, const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options) {
    stream << "{\"command\":\"plan-table-position-presets\""
           << ",\"ok\":" << json_bool(plan.items.empty());
    if (options.input_path.has_value()) {
        stream << ",\"input_path\":";
        write_json_string(stream, options.input_path->string());
    }
    stream << ",\"preset\":";
    write_json_string(stream, table_position_preset_name(plan.preset));
    stream << ",\"replace_positioned\":"
           << json_bool(options.replace_positioned)
           << ",\"fail_on_change\":" << json_bool(options.fail_on_change);
    if (options.output_path.has_value()) {
        stream << ",\"output_path\":";
        write_json_string(stream, options.output_path->string());
    }
    if (options.output_plan_path.has_value()) {
        stream << ",\"output_plan_path\":";
        write_json_string(stream, options.output_plan_path->string());
    }
    stream << ",\"table_count\":" << plan.table_count
           << ",\"positioned_count\":" << plan.positioned_count
           << ",\"unpositioned_count\":" << plan.unpositioned_count
           << ",\"already_matching_count\":" << plan.already_matching_count
           << ",\"already_matching_table_indices\":";
    write_json_size_array(stream, plan.already_matching_table_indices);
    stream << ",\"set_count\":" << plan.set_count
           << ",\"replace_count\":" << plan.replace_count
           << ",\"review_count\":" << plan.review_count
           << ",\"review_table_indices\":";
    write_json_size_array(stream, plan.review_table_indices);
    stream << ",\"automatic_count\":" << plan.automatic_table_indices.size()
           << ",\"automatic_table_indices\":";
    write_json_size_array(stream, plan.automatic_table_indices);
    stream << ",\"recommended_batch_command\":";
    write_json_optional_string(stream, plan.recommended_batch_command);
    stream << ",\"resolved_output_path\":";
    if (plan.resolved_output_path.has_value()) {
        write_json_string(stream, plan.resolved_output_path->string());
    } else {
        stream << "null";
    }
    stream << ",\"resolved_recommended_batch_command\":";
    write_json_optional_string(stream, plan.resolved_recommended_batch_command);
    stream << ",\"plan_item_count\":" << plan.items.size()
           << ",\"target_position\":";
    write_json_table_position(stream, plan.target_position);
    stream << ",\"table_fingerprints\":";
    write_json_table_position_table_fingerprints(stream,
                                                 plan.table_fingerprints);
    stream << ",\"items\":[";
    for (std::size_t index = 0; index < plan.items.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_table_position_preset_plan_item(stream, plan.items[index],
                                                   plan.preset);
    }
    stream << "]}\n";
}

void write_text_table_position_preset_plan(
    const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options) {
    std::cout << "table_position_preset_plan: "
              << (plan.items.empty() ? "clean" : "changes") << '\n';
    if (options.input_path.has_value()) {
        std::cout << "input_path: " << options.input_path->string() << '\n';
    }
    std::cout << "preset: " << table_position_preset_name(plan.preset) << '\n'
              << "replace_positioned: "
              << (options.replace_positioned ? "true" : "false") << '\n'
              << "fail_on_change: "
              << (options.fail_on_change ? "true" : "false") << '\n'
              << "table_count: " << plan.table_count << '\n'
              << "positioned_count: " << plan.positioned_count << '\n'
              << "unpositioned_count: " << plan.unpositioned_count << '\n'
              << "already_matching_count: " << plan.already_matching_count
              << '\n'
              << "already_matching_table_indices: ";
    write_text_size_array(std::cout, plan.already_matching_table_indices);
    std::cout << '\n' << "set_count: " << plan.set_count << '\n'
              << "replace_count: " << plan.replace_count << '\n'
              << "review_count: " << plan.review_count << '\n'
              << "review_table_indices: ";
    write_text_size_array(std::cout, plan.review_table_indices);
    std::cout << '\n'
              << "automatic_count: " << plan.automatic_table_indices.size()
              << '\n'
              << "automatic_table_indices: ";
    write_text_size_array(std::cout, plan.automatic_table_indices);
    std::cout << "\nrecommended_batch_command: ";
    if (plan.recommended_batch_command.has_value()) {
        std::cout << *plan.recommended_batch_command;
    } else {
        std::cout << "none";
    }
    std::cout << "\nresolved_output_path: ";
    if (plan.resolved_output_path.has_value()) {
        std::cout << plan.resolved_output_path->string();
    } else {
        std::cout << "none";
    }
    std::cout << "\nresolved_recommended_batch_command: ";
    if (plan.resolved_recommended_batch_command.has_value()) {
        std::cout << *plan.resolved_recommended_batch_command;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "plan_item_count: " << plan.items.size() << '\n';
    if (options.output_path.has_value()) {
        std::cout << "output_path: " << options.output_path->string() << '\n';
    }
    if (options.output_plan_path.has_value()) {
        std::cout << "output_plan_path: " << options.output_plan_path->string()
                  << '\n';
    }
    for (std::size_t index = 0; index < plan.items.size(); ++index) {
        const auto &item = plan.items[index];
        std::cout << "item[" << index << "]: table=" << item.table_index
                  << " action=" << item.action
                  << " automatic=" << (item.automatic ? "true" : "false")
                  << " fingerprint_rows=" << item.fingerprint.row_count
                  << " fingerprint_columns=" << item.fingerprint.column_count
                  << " current=";
        write_table_position_text(std::cout, item.current_position);
        std::cout << " target_preset="
                  << table_position_preset_name(plan.preset)
                  << " recommended_command=\"" << item.recommended_command
                  << "\"";
        if (item.resolved_output_path.has_value()) {
            std::cout << " resolved_output_path=\""
                      << item.resolved_output_path->string() << "\"";
        }
        if (item.resolved_recommended_command.has_value()) {
            std::cout << " resolved_recommended_command=\""
                      << *item.resolved_recommended_command << "\"";
        }
        std::cout << '\n';
    }
}

auto write_table_position_preset_plan_file(
    const std::filesystem::path &output_path,
    const table_position_preset_plan &plan,
    const plan_table_position_presets_options &options,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open table position plan output path: " +
                        output_path.string();
        return false;
    }

    write_json_table_position_preset_plan(stream, plan, options);
    if (!stream.good()) {
        error_message = "failed to write table position plan output path: " +
                        output_path.string();
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
