#include "featherdoc_cli_bookmark_text_options_parse.hpp"

#include "featherdoc_cli_content_control_options_parse_support.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_replace_content_control_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    replace_content_control_text_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        const auto part_result = parse_content_control_part_selection_option(
            arguments, index, options.part, options.part_index,
            options.section_index, options.reference_kind, options.has_part,
            options.has_kind, true, error_message);
        if (part_result == option_parse_result::matched) {
            continue;
        }
        if (part_result == option_parse_result::error) {
            return false;
        }

        const auto selector_result = parse_content_control_selector_option(
            arguments, index, options.tag, options.alias, error_message);
        if (selector_result == option_parse_result::matched) {
            continue;
        }
        if (selector_result == option_parse_result::error) {
            return false;
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

        const auto output_result = parse_content_control_output_option(
            arguments, index, options.output_path, error_message);
        if (output_result == option_parse_result::matched) {
            continue;
        }
        if (output_result == option_parse_result::error) {
            return false;
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
                                            options.section_index,
                                            options.has_kind, "mutation",
                                            error_message);
}

} // namespace featherdoc_cli
