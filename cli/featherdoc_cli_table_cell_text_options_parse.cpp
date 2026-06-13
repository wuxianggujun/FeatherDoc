#include "featherdoc_cli_table_cell_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_table_cell_text_options(const std::vector<std::string_view> &arguments,
                                   std::size_t start_index,
                                   table_cell_text_options &options,
                                   std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
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

    return true;
}

} // namespace featherdoc_cli
