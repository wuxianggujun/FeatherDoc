#include "featherdoc_cli_table_position_options_parse.hpp"

#include "featherdoc_cli_table_position_options_parse_support.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_table_position_target_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    table_position_target_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (const auto result = parse_table_position_table_option(
                arguments, index, options.additional_table_indices,
                error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
            continue;
        }

        if (const auto result = parse_table_position_output_option(
                arguments, index, options.output_path, error_message);
            result != option_parse_result::not_matched) {
            if (result == option_parse_result::error) {
                return false;
            }
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

} // namespace featherdoc_cli
