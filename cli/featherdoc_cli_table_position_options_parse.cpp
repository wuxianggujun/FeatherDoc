#include "featherdoc_cli_table_position_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_table_position_plan_build.hpp"

#include <cstdint>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

void apply_table_position_preset_defaults(table_position_options &options) {
    if (!options.preset.has_value()) {
        return;
    }

    const auto preset_position = make_table_position_preset(*options.preset);
    if (!options.has_horizontal_reference) {
        options.position.horizontal_reference = preset_position.horizontal_reference;
        options.has_horizontal_reference = true;
    }
    if (!options.has_horizontal_offset) {
        options.position.horizontal_offset_twips =
            preset_position.horizontal_offset_twips;
        options.has_horizontal_offset = true;
    }
    if (!options.has_horizontal_spec && preset_position.horizontal_spec.has_value()) {
        options.position.horizontal_spec = preset_position.horizontal_spec;
        options.has_horizontal_spec = true;
    }
    if (!options.has_vertical_reference) {
        options.position.vertical_reference = preset_position.vertical_reference;
        options.has_vertical_reference = true;
    }
    if (!options.has_vertical_offset) {
        options.position.vertical_offset_twips = preset_position.vertical_offset_twips;
        options.has_vertical_offset = true;
    }
    if (!options.has_vertical_spec && preset_position.vertical_spec.has_value()) {
        options.position.vertical_spec = preset_position.vertical_spec;
        options.has_vertical_spec = true;
    }
    if (!options.position.left_from_text_twips.has_value()) {
        options.position.left_from_text_twips =
            preset_position.left_from_text_twips;
    }
    if (!options.position.right_from_text_twips.has_value()) {
        options.position.right_from_text_twips =
            preset_position.right_from_text_twips;
    }
    if (!options.position.top_from_text_twips.has_value()) {
        options.position.top_from_text_twips = preset_position.top_from_text_twips;
    }
    if (!options.position.bottom_from_text_twips.has_value()) {
        options.position.bottom_from_text_twips =
            preset_position.bottom_from_text_twips;
    }
    if (!options.position.overlap.has_value()) {
        options.position.overlap = preset_position.overlap;
    }
}

auto parse_table_position_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_position_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--table") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --table";
                return false;
            }
            std::size_t table_index = 0U;
            if (!parse_index(arguments[index + 1U], table_index)) {
                error_message =
                    "invalid table index: " + std::string(arguments[index + 1U]);
                return false;
            }
            options.additional_table_indices.push_back(table_index);
            ++index;
            continue;
        }

        if (argument == "--preset") {
            if (options.preset.has_value()) {
                error_message = "duplicate --preset option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --preset";
                return false;
            }
            table_position_preset preset{};
            if (!parse_table_position_preset(arguments[index + 1U], preset)) {
                error_message = "invalid table position preset: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.preset = preset;
            ++index;
            continue;
        }

        if (argument == "--horizontal-reference") {
            if (options.has_horizontal_reference) {
                error_message = "duplicate --horizontal-reference option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --horizontal-reference";
                return false;
            }
            if (!parse_table_position_horizontal_reference(
                    arguments[index + 1U], options.position.horizontal_reference)) {
                error_message = "invalid horizontal reference: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.has_horizontal_reference = true;
            ++index;
            continue;
        }

        if (argument == "--horizontal-offset") {
            if (options.has_horizontal_offset) {
                error_message = "duplicate --horizontal-offset option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --horizontal-offset";
                return false;
            }
            if (!parse_int32(arguments[index + 1U],
                             options.position.horizontal_offset_twips)) {
                error_message = "invalid horizontal offset twips: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.has_horizontal_offset = true;
            ++index;
            continue;
        }

        if (argument == "--horizontal-spec") {
            if (options.has_horizontal_spec) {
                error_message = "duplicate --horizontal-spec option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --horizontal-spec";
                return false;
            }
            featherdoc::table_position_horizontal_spec spec{};
            if (!parse_table_position_horizontal_spec(arguments[index + 1U], spec)) {
                error_message = "invalid horizontal spec: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.position.horizontal_spec = spec;
            options.has_horizontal_spec = true;
            ++index;
            continue;
        }

        if (argument == "--vertical-reference") {
            if (options.has_vertical_reference) {
                error_message = "duplicate --vertical-reference option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --vertical-reference";
                return false;
            }
            if (!parse_table_position_vertical_reference(
                    arguments[index + 1U], options.position.vertical_reference)) {
                error_message = "invalid vertical reference: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.has_vertical_reference = true;
            ++index;
            continue;
        }

        if (argument == "--vertical-offset") {
            if (options.has_vertical_offset) {
                error_message = "duplicate --vertical-offset option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --vertical-offset";
                return false;
            }
            if (!parse_int32(arguments[index + 1U],
                             options.position.vertical_offset_twips)) {
                error_message = "invalid vertical offset twips: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.has_vertical_offset = true;
            ++index;
            continue;
        }

        if (argument == "--vertical-spec") {
            if (options.has_vertical_spec) {
                error_message = "duplicate --vertical-spec option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --vertical-spec";
                return false;
            }
            featherdoc::table_position_vertical_spec spec{};
            if (!parse_table_position_vertical_spec(arguments[index + 1U], spec)) {
                error_message = "invalid vertical spec: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.position.vertical_spec = spec;
            options.has_vertical_spec = true;
            ++index;
            continue;
        }

        if (argument == "--left-from-text") {
            if (options.position.left_from_text_twips.has_value()) {
                error_message = "duplicate --left-from-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --left-from-text";
                return false;
            }
            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid left-from-text twips: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.position.left_from_text_twips = value;
            ++index;
            continue;
        }

        if (argument == "--right-from-text") {
            if (options.position.right_from_text_twips.has_value()) {
                error_message = "duplicate --right-from-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --right-from-text";
                return false;
            }
            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid right-from-text twips: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.position.right_from_text_twips = value;
            ++index;
            continue;
        }

        if (argument == "--top-from-text") {
            if (options.position.top_from_text_twips.has_value()) {
                error_message = "duplicate --top-from-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --top-from-text";
                return false;
            }
            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid top-from-text twips: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.position.top_from_text_twips = value;
            ++index;
            continue;
        }

        if (argument == "--bottom-from-text") {
            if (options.position.bottom_from_text_twips.has_value()) {
                error_message = "duplicate --bottom-from-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --bottom-from-text";
                return false;
            }
            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid bottom-from-text twips: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.position.bottom_from_text_twips = value;
            ++index;
            continue;
        }

        if (argument == "--overlap") {
            if (options.position.overlap.has_value()) {
                error_message = "duplicate --overlap option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --overlap";
                return false;
            }
            featherdoc::table_overlap overlap{};
            if (!parse_table_overlap_text(arguments[index + 1U], overlap)) {
                error_message = "invalid table overlap: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.position.overlap = overlap;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }
            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    apply_table_position_preset_defaults(options);

    if (!options.has_horizontal_reference) {
        error_message =
            "missing required --horizontal-reference <margin|page|column> or --preset";
        return false;
    }
    if (!options.has_horizontal_offset) {
        error_message = "missing required --horizontal-offset <twips>";
        return false;
    }
    if (!options.has_vertical_reference) {
        error_message =
            "missing required --vertical-reference <margin|page|paragraph>";
        return false;
    }
    if (!options.has_vertical_offset) {
        error_message = "missing required --vertical-offset <twips>";
        return false;
    }

    return true;
}

auto parse_table_position_target_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_position_target_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--table") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --table";
                return false;
            }
            std::size_t table_index = 0U;
            if (!parse_index(arguments[index + 1U], table_index)) {
                error_message =
                    "invalid table index: " + std::string(arguments[index + 1U]);
                return false;
            }
            options.additional_table_indices.push_back(table_index);
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }
            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_apply_table_position_plan_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    apply_table_position_plan_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }
            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--dry-run") {
            options.dry_run = true;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_plan_table_position_presets_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    plan_table_position_presets_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--preset") {
            if (options.preset.has_value()) {
                error_message = "duplicate --preset option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --preset";
                return false;
            }
            table_position_preset preset{};
            if (!parse_table_position_preset(arguments[index + 1U], preset)) {
                error_message = "invalid table position preset: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.preset = preset;
            ++index;
            continue;
        }

        if (argument == "--replace-positioned") {
            options.replace_positioned = true;
            continue;
        }

        if (argument == "--fail-on-change") {
            options.fail_on_change = true;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }
            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--output-plan") {
            if (options.output_plan_path.has_value()) {
                error_message = "duplicate --output-plan option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output-plan";
                return false;
            }
            options.output_plan_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.preset.has_value()) {
        error_message =
            "missing required --preset <paragraph-callout|page-corner|margin-anchor>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
