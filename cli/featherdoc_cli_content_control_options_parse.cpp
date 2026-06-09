#include "featherdoc_cli_bookmark_text_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

#include <utility>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto filter_content_control_selector_arguments(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    std::optional<std::string> &tag, std::optional<std::string> &alias,
    std::vector<std::string_view> &filtered_arguments,
    std::string &error_message) -> bool {
    filtered_arguments.clear();
    filtered_arguments.reserve(arguments.size() - start_index);

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--tag") {
            if (tag.has_value()) {
                error_message = "duplicate --tag option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --tag";
                return false;
            }
            auto value = std::string(arguments[index + 1U]);
            if (value.empty()) {
                error_message = "--tag expects a non-empty value";
                return false;
            }
            tag = std::move(value);
            ++index;
            continue;
        }

        if (argument == "--alias") {
            if (alias.has_value()) {
                error_message = "duplicate --alias option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --alias";
                return false;
            }
            auto value = std::string(arguments[index + 1U]);
            if (value.empty()) {
                error_message = "--alias expects a non-empty value";
                return false;
            }
            alias = std::move(value);
            ++index;
            continue;
        }

        filtered_arguments.push_back(argument);
    }

    return true;
}

auto validate_content_control_selector(
    std::string_view command, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias, std::string &error_message) -> bool {
    if (!tag.has_value() && !alias.has_value()) {
        error_message = std::string(command) + " expects --tag or --alias";
        return false;
    }
    if (tag.has_value() && alias.has_value()) {
        error_message = "--tag cannot be combined with --alias";
        return false;
    }
    return true;
}

auto parse_inspect_content_controls_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_content_controls_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--part") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --part";
                return false;
            }
            if (!parse_validation_part(arguments[index + 1U], options.part)) {
                error_message = "invalid template part: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--index") {
            if (options.part_index.has_value()) {
                error_message = "duplicate --index option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --index";
                return false;
            }
            std::size_t part_index = 0U;
            if (!parse_index(arguments[index + 1U], part_index)) {
                error_message = "invalid part index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.part_index = part_index;
            ++index;
            continue;
        }

        if (argument == "--section") {
            if (options.section_index.has_value()) {
                error_message = "duplicate --section option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }
            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.section_index = section_index;
            ++index;
            continue;
        }

        if (argument == "--kind") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }
            if (!parse_reference_kind(arguments[index + 1U], options.reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.has_kind = true;
            ++index;
            continue;
        }

        if (argument == "--tag") {
            if (options.tag.has_value()) {
                error_message = "duplicate --tag option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --tag";
                return false;
            }
            auto tag = std::string(arguments[index + 1U]);
            if (tag.empty()) {
                error_message = "--tag expects a non-empty value";
                return false;
            }
            options.tag = std::move(tag);
            ++index;
            continue;
        }

        if (argument == "--alias") {
            if (options.alias.has_value()) {
                error_message = "duplicate --alias option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --alias";
                return false;
            }
            auto alias = std::string(arguments[index + 1U]);
            if (alias.empty()) {
                error_message = "--alias expects a non-empty value";
                return false;
            }
            options.alias = std::move(alias);
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

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

auto parse_replace_content_control_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_text_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--part") {
            if (options.has_part) {
                error_message = "duplicate --part option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --part";
                return false;
            }
            if (!parse_validation_part(arguments[index + 1U], options.part)) {
                error_message = "invalid template part: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.has_part = true;
            ++index;
            continue;
        }

        if (argument == "--index") {
            if (options.part_index.has_value()) {
                error_message = "duplicate --index option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --index";
                return false;
            }
            std::size_t part_index = 0U;
            if (!parse_index(arguments[index + 1U], part_index)) {
                error_message = "invalid part index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.part_index = part_index;
            ++index;
            continue;
        }

        if (argument == "--section") {
            if (options.section_index.has_value()) {
                error_message = "duplicate --section option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }
            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.section_index = section_index;
            ++index;
            continue;
        }

        if (argument == "--kind") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }
            if (!parse_reference_kind(arguments[index + 1U], options.reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.has_kind = true;
            ++index;
            continue;
        }

        if (argument == "--tag") {
            if (options.tag.has_value()) {
                error_message = "duplicate --tag option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --tag";
                return false;
            }
            auto tag = std::string(arguments[index + 1U]);
            if (tag.empty()) {
                error_message = "--tag expects a non-empty value";
                return false;
            }
            options.tag = std::move(tag);
            ++index;
            continue;
        }

        if (argument == "--alias") {
            if (options.alias.has_value()) {
                error_message = "duplicate --alias option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --alias";
                return false;
            }
            auto alias = std::string(arguments[index + 1U]);
            if (alias.empty()) {
                error_message = "--alias expects a non-empty value";
                return false;
            }
            options.alias = std::move(alias);
            ++index;
            continue;
        }

        if (argument == "--text") {
            if (options.has_text) {
                error_message = "duplicate --text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --text";
                return false;
            }
            options.text = std::string(arguments[index + 1U]);
            options.has_text = true;
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

    if (!options.tag.has_value() && !options.alias.has_value()) {
        error_message = "replace-content-control-text expects --tag or --alias";
        return false;
    }
    if (options.tag.has_value() && options.alias.has_value()) {
        error_message = "--tag cannot be combined with --alias";
        return false;
    }
    if (!options.has_text) {
        error_message = "expected --text <text>";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}

auto parse_sync_content_controls_from_custom_xml_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    sync_content_controls_from_custom_xml_options &options,
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

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}


auto parse_replace_content_control_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_paragraphs_options &options,
    std::string &error_message) -> bool {
    std::vector<std::string_view> filtered_arguments;
    if (!filter_content_control_selector_arguments(
            arguments, start_index, options.tag, options.alias,
            filtered_arguments, error_message)) {
        return false;
    }

    if (!parse_replace_bookmark_paragraphs_options(
            filtered_arguments, 0U, options, error_message)) {
        return false;
    }

    return validate_content_control_selector(
        "replace-content-control-paragraphs", options.tag, options.alias,
        error_message);
}

auto parse_content_control_table_replacement_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    content_control_table_replacement_options &options, bool allow_empty_rows,
    std::string &error_message) -> bool {
    std::vector<std::string_view> filtered_arguments;
    if (!filter_content_control_selector_arguments(
            arguments, start_index, options.tag, options.alias,
            filtered_arguments, error_message)) {
        return false;
    }

    if (!parse_bookmark_table_replacement_options(
            filtered_arguments, 0U, options, allow_empty_rows, error_message)) {
        return false;
    }

    return validate_content_control_selector(
        allow_empty_rows ? "replace-content-control-table-rows"
                         : "replace-content-control-table",
        options.tag, options.alias, error_message);
}

auto parse_replace_content_control_image_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_image_options &options,
    std::string &error_message) -> bool {
    std::vector<std::string_view> filtered_arguments;
    if (!filter_content_control_selector_arguments(
            arguments, start_index, options.tag, options.alias,
            filtered_arguments, error_message)) {
        return false;
    }

    if (!parse_bookmark_image_command_options(
            filtered_arguments, 0U, "replace-content-control-image", false,
            options, error_message)) {
        return false;
    }

    return validate_content_control_selector(
        "replace-content-control-image", options.tag, options.alias,
        error_message);
}

} // namespace featherdoc_cli
