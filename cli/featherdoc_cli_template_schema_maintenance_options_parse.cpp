#include "featherdoc_cli_template_schema_options_parse.hpp"

#include "featherdoc_cli_template_schema_options_parse_support.hpp"

namespace featherdoc_cli {

auto parse_normalize_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    normalize_template_schema_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

auto parse_lint_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    lint_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_repair_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    repair_template_schema_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

auto parse_merge_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    merge_template_schema_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
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

        if (argument.starts_with("--")) {
            error_message = "unknown option: " + std::string(argument);
            return false;
        }

        options.schema_paths.emplace_back(std::string(argument));
    }

    if (options.schema_paths.size() < 2U) {
        error_message = "merge-template-schema expects at least two schema paths";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
