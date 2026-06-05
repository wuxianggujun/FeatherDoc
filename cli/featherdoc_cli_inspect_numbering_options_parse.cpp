#include "featherdoc_cli_inspect_numbering_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

auto parse_inspect_numbering_options(const std::vector<std::string_view> &arguments,
                                     std::size_t start_index,
                                     inspect_numbering_options &options,
                                     std::string &error_message) -> bool {
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

            std::uint32_t definition_id = 0U;
            if (!parse_uint32(arguments[index + 1U], definition_id)) {
                error_message =
                    "invalid numbering definition id: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.definition_id = definition_id;
            ++index;
            continue;
        }

        if (argument == "--instance") {
            if (options.instance_id.has_value()) {
                error_message = "duplicate --instance option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --instance";
                return false;
            }

            std::uint32_t instance_id = 0U;
            if (!parse_uint32(arguments[index + 1U], instance_id)) {
                error_message =
                    "invalid numbering instance id: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.instance_id = instance_id;
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
