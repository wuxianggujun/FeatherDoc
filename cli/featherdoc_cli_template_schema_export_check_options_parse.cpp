#include "featherdoc_cli_template_schema_options_parse.hpp"

#include "featherdoc_cli_template_schema_options_parse_support.hpp"

namespace featherdoc_cli {

auto parse_export_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    export_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        const auto target_result = parse_template_schema_target_mode_option(
            argument, options.section_targets,
            options.resolved_section_targets, error_message);
        if (target_result == template_schema_target_mode_parse_result::failed) {
            return false;
        }
        if (target_result == template_schema_target_mode_parse_result::parsed) {
            continue;
        }

        if (argument == "--output") {
            if (!parse_template_schema_path_option(arguments, index, "--output",
                                                  options.output_path,
                                                  error_message)) {
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

auto parse_check_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    check_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        const auto target_result = parse_template_schema_target_mode_option(
            argument, options.section_targets,
            options.resolved_section_targets, error_message);
        if (target_result == template_schema_target_mode_parse_result::failed) {
            return false;
        }
        if (target_result == template_schema_target_mode_parse_result::parsed) {
            continue;
        }

        if (argument == "--schema-file") {
            if (!parse_template_schema_path_option(
                    arguments, index, "--schema-file", options.schema_path,
                    error_message)) {
                return false;
            }
            continue;
        }

        if (argument == "--output") {
            if (!parse_template_schema_path_option(arguments, index, "--output",
                                                  options.output_path,
                                                  error_message)) {
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

    if (!options.schema_path.has_value()) {
        error_message = "missing required --schema-file <path> option";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
