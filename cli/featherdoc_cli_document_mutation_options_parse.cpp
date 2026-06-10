#include "featherdoc_cli_document_mutation_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_simple_document_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    simple_document_mutation_options &options, std::string &error_message)
    -> bool {
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

} // namespace featherdoc_cli
