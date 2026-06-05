#include "featherdoc_cli_template_table_mutation_options_parse.hpp"

#include <utility>

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

auto parse_template_table_row_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_row_mutation_options &options,
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

auto parse_template_table_row_texts_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_table_row_texts_options &options, std::string &error_message) -> bool {
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
        if (argument == "--row") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --row";
                return false;
            }

            options.rows.push_back({std::string(arguments[index + 1U])});
            ++index;
            continue;
        }

        if (argument == "--cell") {
            if (options.rows.empty()) {
                error_message = "--cell requires --row";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --cell";
                return false;
            }

            options.rows.back().push_back(std::string(arguments[index + 1U]));
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

            options.output_path =
                path_type(std::string(arguments[index + 1U]));
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

    if (options.rows.empty()) {
        error_message = "expected at least one --row <text>";
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

auto parse_template_append_table_row_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_append_table_row_options &options, std::string &error_message)
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
        if (argument == "--cell-count") {
            if (options.cell_count.has_value()) {
                error_message = "duplicate --cell-count option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --cell-count";
                return false;
            }

            std::size_t cell_count = 0U;
            if (!parse_index(arguments[index + 1U], cell_count)) {
                error_message =
                    "invalid cell count: " + std::string(arguments[index + 1U]);
                return false;
            }
            if (cell_count == 0U) {
                error_message = "cell count must be greater than 0";
                return false;
            }

            options.cell_count = cell_count;
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

auto parse_template_merge_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_merge_table_cells_options &options, std::string &error_message)
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
        if (argument == "--direction") {
            if (options.has_direction) {
                error_message = "duplicate --direction option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --direction";
                return false;
            }

            if (!parse_table_merge_direction(arguments[index + 1U], options.direction)) {
                error_message = "invalid merge direction: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_direction = true;
            ++index;
            continue;
        }

        if (argument == "--count") {
            if (options.has_count) {
                error_message = "duplicate --count option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --count";
                return false;
            }

            std::size_t count = 0U;
            if (!parse_index(arguments[index + 1U], count)) {
                error_message =
                    "invalid merge count: " + std::string(arguments[index + 1U]);
                return false;
            }
            if (count == 0U) {
                error_message = "merge count must be greater than zero";
                return false;
            }

            options.count = count;
            options.has_count = true;
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

    if (!options.has_direction) {
        error_message = "missing required --direction <right|down>";
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

auto parse_template_unmerge_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_unmerge_table_cells_options &options, std::string &error_message)
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
        if (argument == "--direction") {
            if (options.has_direction) {
                error_message = "duplicate --direction option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --direction";
                return false;
            }

            if (!parse_table_merge_direction(arguments[index + 1U], options.direction)) {
                error_message = "invalid merge direction: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_direction = true;
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

    if (!options.has_direction) {
        error_message = "missing required --direction <right|down>";
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
