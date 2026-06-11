#include "featherdoc_cli_table_cell_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_append_table_row_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    append_table_row_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
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

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
