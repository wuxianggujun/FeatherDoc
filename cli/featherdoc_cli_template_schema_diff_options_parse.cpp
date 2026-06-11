#include "featherdoc_cli_template_schema_options_parse.hpp"

namespace featherdoc_cli {

auto parse_diff_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    diff_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--fail-on-diff") {
            options.fail_on_diff = true;
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
