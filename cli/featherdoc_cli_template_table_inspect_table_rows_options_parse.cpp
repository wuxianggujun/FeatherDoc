#include "featherdoc_cli_template_inspect_options_parse.hpp"

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
