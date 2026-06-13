#include "featherdoc_cli_numbering_options_parse.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_ensure_numbering_definition_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    ensure_numbering_definition_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--definition-name") {
            if (options.definition_name.has_value()) {
                error_message = "duplicate --definition-name option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --definition-name";
                return false;
            }

            options.definition_name = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--numbering-level") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --numbering-level";
                return false;
            }

            featherdoc::numbering_level_definition definition;
            if (!parse_numbering_level_definition(arguments[index + 1U], definition,
                                                  error_message)) {
                return false;
            }

            options.levels.push_back(std::move(definition));
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

    if (!options.definition_name.has_value()) {
        error_message = "missing --definition-name <name>";
        return false;
    }
    if (options.levels.empty()) {
        error_message =
            "expected at least one --numbering-level "
            "<level>:<kind>:<start>:<text-pattern>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
