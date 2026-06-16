#include "featherdoc_cli_bookmark_text_options_parse.hpp"

#include "featherdoc_cli_content_control_options_parse_support.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_inspect_content_controls_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_content_controls_options &options, std::string &error_message)
    -> bool {
    bool has_part = false;
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        const auto part_result = parse_content_control_part_selection_option(
            arguments, index, options.part, options.part_index,
            options.section_index, options.reference_kind, has_part,
            options.has_kind, false, error_message);
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

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index,
                                            options.has_kind, "inspection",
                                            error_message);
}

} // namespace featherdoc_cli
