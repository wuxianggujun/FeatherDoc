#include "featherdoc_cli_section_part_support.hpp"

#include <string>

namespace featherdoc_cli {

auto section_part_name(section_part_family family) -> const char * {
    return family == section_part_family::header ? "header" : "footer";
}

auto part_family_for_command(std::string_view command) -> section_part_family {
    return command.find("header") != std::string_view::npos
               ? section_part_family::header
               : section_part_family::footer;
}

auto parse_section_part_command_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    bool allow_no_inherit, section_part_command_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (allow_no_inherit && argument == "--no-inherit") {
            options.inherit_header_footer = false;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1 >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1]));
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
