#include "featherdoc_cli_bookmark_text_options_parse.hpp"

#include "featherdoc_cli_content_control_options_parse_support.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_sync_content_controls_from_custom_xml_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    sync_content_controls_from_custom_xml_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

    return true;
}

} // namespace featherdoc_cli
