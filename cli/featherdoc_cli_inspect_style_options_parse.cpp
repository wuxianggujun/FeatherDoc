#include "featherdoc_cli_inspect_style_options_parse.hpp"

namespace featherdoc_cli {

auto parse_inspect_options(const std::vector<std::string_view> &arguments,
                           std::size_t start_index, inspect_options &options,
                           std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (arguments[index] == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(arguments[index]);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
