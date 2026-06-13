#include "featherdoc_cli_numbering_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_set_paragraph_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--definition") {
            if (options.definition_id.has_value()) {
                error_message = "duplicate --definition option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --definition";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid numbering definition id: " +
                    std::string(arguments[index + 1U]);
                return false;
            }

            options.definition_id = value;
            ++index;
            continue;
        }

        if (argument == "--level") {
            if (options.level.has_value()) {
                error_message = "duplicate --level option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --level";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid numbering level: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.level = value;
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

    if (!options.definition_id.has_value()) {
        error_message = "missing --definition <id>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
