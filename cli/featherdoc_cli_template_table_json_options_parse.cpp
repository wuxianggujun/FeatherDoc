#include "featherdoc_cli_template_table_options_parse.hpp"

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_template_table_json_patch_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_json_patch_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto parse_result = parse_template_part_selection_option(
            arguments, index, options.part, options.part_index,
            options.section_index, options.reference_kind, options.has_part,
            options.has_kind, error_message);
        if (parse_result == option_parse_result::matched) {
            continue;
        }
        if (parse_result == option_parse_result::error) {
            return false;
        }

        const auto argument = arguments[index];
        if (argument == "--patch-file") {
            if (options.patch_file.has_value()) {
                error_message = "duplicate --patch-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --patch-file";
                return false;
            }

            options.patch_file = path_type(std::string(arguments[index + 1U]));
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

        const auto selector_parse_result = parse_template_table_selector_option(
            arguments, index, options.selector, options.has_header_row_index,
            options.has_occurrence, error_message);
        if (selector_parse_result == option_parse_result::matched) {
            continue;
        }
        if (selector_parse_result == option_parse_result::error) {
            return false;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.patch_file.has_value()) {
        error_message = "missing --patch-file <path>";
        return false;
    }

    if (!validate_template_table_selector(options.selector, true,
                                          options.has_header_row_index,
                                          options.has_occurrence,
                                          error_message)) {
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index,
                                            options.has_kind, "mutation",
                                            error_message);
}

auto parse_template_table_json_batch_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_json_batch_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto parse_result = parse_template_part_selection_option(
            arguments, index, options.part, options.part_index,
            options.section_index, options.reference_kind, options.has_part,
            options.has_kind, error_message);
        if (parse_result == option_parse_result::matched) {
            continue;
        }
        if (parse_result == option_parse_result::error) {
            return false;
        }

        const auto argument = arguments[index];
        if (argument == "--patch-file") {
            if (options.patch_file.has_value()) {
                error_message = "duplicate --patch-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --patch-file";
                return false;
            }

            options.patch_file = path_type(std::string(arguments[index + 1U]));
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

    if (!options.patch_file.has_value()) {
        error_message = "missing --patch-file <path>";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index,
                                            options.has_kind, "mutation",
                                            error_message);
}

} // namespace featherdoc_cli
