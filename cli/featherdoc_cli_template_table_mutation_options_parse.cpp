#include "featherdoc_cli_template_table_mutation_options_parse.hpp"

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_template_table_cell_text_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_cell_text_options &options, std::string &error_message) -> bool {
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

        if (argument == "--grid-column") {
            if (options.grid_column.has_value()) {
                error_message = "duplicate --grid-column option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --grid-column";
                return false;
            }

            std::size_t grid_column = 0U;
            if (!parse_index(arguments[index + 1U], grid_column)) {
                error_message = "invalid grid column: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.grid_column = grid_column;
            ++index;
            continue;
        }

        const auto bookmark_parse_result = parse_template_table_bookmark_option(
            arguments, index, options.bookmark_name, error_message);
        if (bookmark_parse_result == option_parse_result::matched) {
            continue;
        }
        if (bookmark_parse_result == option_parse_result::error) {
            return false;
        }

        if (argument == "--text") {
            if (options.text.has_value() || options.text_file.has_value()) {
                error_message = "text source was already specified";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --text";
                return false;
            }

            options.text = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--text-file") {
            if (options.text.has_value() || options.text_file.has_value()) {
                error_message = "text source was already specified";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --text-file";
                return false;
            }

            options.text_file = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.text.has_value() && !options.text_file.has_value()) {
        error_message = "expected --text <text> or --text-file <path>";
        return false;
    }

    if (!validate_template_table_selector(std::nullopt, options.bookmark_name,
                                          true, error_message)) {
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}

auto parse_template_table_cell_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_cell_mutation_options &options,
    std::string &error_message) -> bool {
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

        const auto bookmark_parse_result = parse_template_table_bookmark_option(
            arguments, index, options.bookmark_name, error_message);
        if (bookmark_parse_result == option_parse_result::matched) {
            continue;
        }
        if (bookmark_parse_result == option_parse_result::error) {
            return false;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!validate_template_table_selector(std::nullopt, options.bookmark_name,
                                          true, error_message)) {
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}

} // namespace featherdoc_cli
