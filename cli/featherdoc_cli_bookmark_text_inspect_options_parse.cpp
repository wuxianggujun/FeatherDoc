#include "featherdoc_cli_bookmark_text_options_parse.hpp"

#include "featherdoc_cli_bookmark_text_options_parse_support.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"

namespace featherdoc_cli {

auto parse_inspect_bookmarks_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_bookmarks_options &options, std::string &error_message) -> bool {
    bool has_part = false;
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        const auto part_result = parse_bookmark_text_part_selection_option(
            arguments, index, options.part, options.part_index,
            options.section_index, options.reference_kind, has_part,
            options.has_kind, false, error_message);
        if (part_result == option_parse_result::matched) {
            continue;
        }
        if (part_result == option_parse_result::error) {
            return false;
        }

        if (argument == "--bookmark") {
            if (options.bookmark_name.has_value()) {
                error_message = "duplicate --bookmark option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --bookmark";
                return false;
            }

            options.bookmark_name = std::string(arguments[index + 1U]);
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

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index,
                                            options.has_kind, "inspection",
                                            error_message);
}

} // namespace featherdoc_cli
