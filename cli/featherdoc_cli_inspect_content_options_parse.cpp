#include "featherdoc_cli_inspect_content_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

auto parse_inspect_paragraphs_options(const std::vector<std::string_view> &arguments,
                                      std::size_t start_index,
                                      inspect_paragraphs_options &options,
                                      std::string &error_message) -> bool {
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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_inspect_tables_options(const std::vector<std::string_view> &arguments,
                                  std::size_t start_index,
                                  inspect_tables_options &options,
                                  std::string &error_message) -> bool {
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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
