#include "featherdoc_cli_inspect_style_options_parse.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_repair_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    repair_style_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--plan-only") {
            options.plan_only = true;
            continue;
        }

        if (argument == "--apply") {
            options.apply = true;
            continue;
        }

        if (argument == "--catalog-file") {
            if (options.catalog_path.has_value()) {
                error_message = "duplicate --catalog-file option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --catalog-file";
                return false;
            }

            options.catalog_path = path_type(std::string(arguments[index + 1U]));
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

    if (options.plan_only && options.apply) {
        error_message = "--plan-only and --apply are mutually exclusive";
        return false;
    }
    if (!options.apply) {
        options.plan_only = true;
    }
    if (options.plan_only && options.output_path.has_value()) {
        error_message = "--output requires --apply";
        return false;
    }
    if (options.catalog_path.has_value() && !options.apply) {
        error_message = "--catalog-file requires --apply";
        return false;
    }
    if (options.apply && !options.output_path.has_value()) {
        error_message = "repair-style-numbering --apply requires --output <path>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
