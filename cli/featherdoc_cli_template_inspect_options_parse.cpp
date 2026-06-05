#include "featherdoc_cli_template_inspect_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

auto parse_template_part_selection_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    validation_part_family &part, std::optional<std::size_t> &part_index,
    std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind &reference_kind, bool &has_part,
    bool &has_kind, std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--part") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --part";
            return option_parse_result::error;
        }

        if (!parse_validation_part(arguments[index + 1U], part)) {
            error_message =
                "invalid template part: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        has_part = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--index") {
        if (part_index.has_value()) {
            error_message = "duplicate --index option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --index";
            return option_parse_result::error;
        }

        std::size_t parsed_part_index = 0U;
        if (!parse_index(arguments[index + 1U], parsed_part_index)) {
            error_message =
                "invalid part index: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        part_index = parsed_part_index;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--section") {
        if (section_index.has_value()) {
            error_message = "duplicate --section option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --section";
            return option_parse_result::error;
        }

        std::size_t parsed_section_index = 0U;
        if (!parse_index(arguments[index + 1U], parsed_section_index)) {
            error_message =
                "invalid section index: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        section_index = parsed_section_index;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--kind") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --kind";
            return option_parse_result::error;
        }

        if (!parse_reference_kind(arguments[index + 1U], reference_kind)) {
            error_message =
                "invalid reference kind: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        has_kind = true;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

auto parse_template_inspect_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_paragraphs_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--paragraph") {
            if (options.paragraph_index.has_value()) {
                error_message = "duplicate --paragraph option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --paragraph";
                return false;
            }

            std::size_t paragraph_index = 0U;
            if (!parse_index(arguments[index + 1U], paragraph_index)) {
                error_message =
                    "invalid paragraph index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.paragraph_index = paragraph_index;
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

auto parse_template_table_bookmark_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::optional<std::string> &bookmark_name, std::string &error_message)
    -> option_parse_result {
    const auto argument = arguments[index];
    if (argument != "--bookmark") {
        return option_parse_result::not_matched;
    }

    if (bookmark_name.has_value()) {
        error_message = "duplicate --bookmark option";
        return option_parse_result::error;
    }
    if (index + 1U >= arguments.size()) {
        error_message = "missing value after --bookmark";
        return option_parse_result::error;
    }

    const auto parsed_bookmark_name = std::string(arguments[index + 1U]);
    if (parsed_bookmark_name.empty()) {
        error_message = "bookmark name must not be empty";
        return option_parse_result::error;
    }

    bookmark_name = parsed_bookmark_name;
    ++index;
    return option_parse_result::matched;
}

auto template_table_selector_uses_text_matching(
    const featherdoc::template_table_selector &selector) -> bool {
    return selector.after_paragraph_text.has_value() ||
           !selector.header_cell_texts.empty();
}

auto validate_template_table_selector(
    const std::optional<std::size_t> &table_index,
    const std::optional<std::string> &bookmark_name, bool allow_none,
    std::string &error_message) -> bool {
    if (table_index.has_value() && bookmark_name.has_value()) {
        error_message = "cannot combine a table index with --bookmark";
        return false;
    }

    if (!allow_none && !table_index.has_value() && !bookmark_name.has_value()) {
        error_message = "expected a table index or --bookmark <name>";
        return false;
    }

    return true;
}

auto validate_template_table_selector(
    const featherdoc::template_table_selector &selector, bool allow_none,
    bool has_header_row_index, bool has_occurrence,
    std::string &error_message) -> bool {
    const auto uses_direct_target =
        selector.table_index.has_value() || selector.bookmark_name.has_value();
    const auto uses_text_target =
        template_table_selector_uses_text_matching(selector);

    if (selector.table_index.has_value() && selector.bookmark_name.has_value()) {
        error_message = "cannot combine a table index with --bookmark";
        return false;
    }

    if (selector.bookmark_name.has_value() && selector.bookmark_name->empty()) {
        error_message = "bookmark name must not be empty";
        return false;
    }

    if (selector.after_paragraph_text.has_value() &&
        selector.after_paragraph_text->empty()) {
        error_message = "--after-text must not be empty";
        return false;
    }

    if (uses_direct_target && uses_text_target) {
        error_message = "cannot combine a table index or --bookmark with "
                        "--after-text or --header-cell";
        return false;
    }

    if (!allow_none && !uses_direct_target && !uses_text_target) {
        error_message = "expected a table index, --bookmark <name>, "
                        "--after-text <text>, or --header-cell <text>";
        return false;
    }

    if (has_header_row_index && selector.header_cell_texts.empty()) {
        error_message = "--header-row requires at least one --header-cell";
        return false;
    }

    if (has_occurrence && !uses_text_target) {
        error_message = "--occurrence requires --after-text or --header-cell";
        return false;
    }

    return true;
}

auto parse_template_table_selector_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    featherdoc::template_table_selector &selector, bool &has_header_row_index,
    bool &has_occurrence, std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--bookmark") {
        return parse_template_table_bookmark_option(arguments, index,
                                                    selector.bookmark_name,
                                                    error_message);
    }

    if (argument == "--after-text") {
        if (selector.after_paragraph_text.has_value()) {
            error_message = "duplicate --after-text option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --after-text";
            return option_parse_result::error;
        }

        selector.after_paragraph_text = std::string(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--header-cell") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --header-cell";
            return option_parse_result::error;
        }

        selector.header_cell_texts.emplace_back(arguments[index + 1U]);
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--header-row") {
        if (has_header_row_index) {
            error_message = "duplicate --header-row option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --header-row";
            return option_parse_result::error;
        }

        std::size_t parsed_header_row_index = 0U;
        if (!parse_index(arguments[index + 1U], parsed_header_row_index)) {
            error_message =
                "invalid header row index: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        selector.header_row_index = parsed_header_row_index;
        has_header_row_index = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--occurrence") {
        if (has_occurrence) {
            error_message = "duplicate --occurrence option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --occurrence";
            return option_parse_result::error;
        }

        std::size_t parsed_occurrence = 0U;
        if (!parse_index(arguments[index + 1U], parsed_occurrence)) {
            error_message =
                "invalid occurrence index: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        selector.occurrence = parsed_occurrence;
        has_occurrence = true;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

auto parse_template_inspect_tables_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_tables_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--table") {
            if (options.table_index.has_value()) {
                error_message = "duplicate --table option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --table";
                return false;
            }

            std::size_t table_index = 0U;
            if (!parse_index(arguments[index + 1U], table_index)) {
                error_message =
                    "invalid table index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.table_index = table_index;
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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!validate_template_table_selector(std::nullopt, options.bookmark_name,
                                          true, error_message)) {
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

auto parse_template_inspect_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_table_cells_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--row") {
            if (options.row_index.has_value()) {
                error_message = "duplicate --row option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --row";
                return false;
            }

            std::size_t row_index = 0U;
            if (!parse_index(arguments[index + 1U], row_index)) {
                error_message =
                    "invalid row index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.row_index = row_index;
            ++index;
            continue;
        }

        if (argument == "--cell") {
            if (options.cell_index.has_value()) {
                error_message = "duplicate --cell option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --cell";
                return false;
            }

            std::size_t cell_index = 0U;
            if (!parse_index(arguments[index + 1U], cell_index)) {
                error_message =
                    "invalid cell index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.cell_index = cell_index;
            ++index;
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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.cell_index.has_value() && !options.row_index.has_value()) {
        error_message = "--cell requires --row";
        return false;
    }
    if (options.grid_column.has_value() && !options.row_index.has_value()) {
        error_message = "--grid-column requires --row";
        return false;
    }
    if (options.cell_index.has_value() && options.grid_column.has_value()) {
        error_message = "--cell and --grid-column are mutually exclusive";
        return false;
    }

    if (!validate_template_table_selector(std::nullopt, options.bookmark_name,
                                          true, error_message)) {
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

auto parse_template_inspect_table_rows_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_table_rows_options &options,
    std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--row") {
            if (options.row_index.has_value()) {
                error_message = "duplicate --row option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --row";
                return false;
            }

            std::size_t row_index = 0U;
            if (!parse_index(arguments[index + 1U], row_index)) {
                error_message =
                    "invalid row index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.row_index = row_index;
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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!validate_template_table_selector(options.selector, true,
                                          options.has_header_row_index,
                                          options.has_occurrence,
                                          error_message)) {
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

auto parse_template_inspect_runs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_runs_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--run") {
            if (options.run_index.has_value()) {
                error_message = "duplicate --run option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --run";
                return false;
            }

            std::size_t run_index = 0U;
            if (!parse_index(arguments[index + 1U], run_index)) {
                error_message =
                    "invalid run index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.run_index = run_index;
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

} // namespace featherdoc_cli
