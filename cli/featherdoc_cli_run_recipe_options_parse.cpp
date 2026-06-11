#include "featherdoc_cli_run_recipe_parse.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_run_recipe_options(const std::vector<std::string_view> &arguments,
                              run_recipe_options &options,
                              std::string &error_message) -> bool {
    options = {};

    for (std::size_t index = 1U; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        if (argument == "--recipe" || argument == "--input") {
            if (options.recipe_path.has_value()) {
                error_message =
                    "run-recipe recipe path was provided more than once";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = std::string(argument) + " expects a path";
                return false;
            }
            options.recipe_path = path_type(std::string(arguments[++index]));
            continue;
        }

        if (argument == "--inputs") {
            if (options.inputs_path.has_value()) {
                error_message =
                    "run-recipe inputs path was provided more than once";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "--inputs expects a path";
                return false;
            }
            options.inputs_path = path_type(std::string(arguments[++index]));
            continue;
        }

        if (argument == "--output" || argument == "--output-dir") {
            if (options.output_dir.has_value()) {
                error_message =
                    "run-recipe output directory was provided more than once";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = std::string(argument) + " expects a path";
                return false;
            }
            options.output_dir = path_type(std::string(arguments[++index]));
            continue;
        }

        error_message = "unknown run-recipe option: " + std::string(argument);
        return false;
    }

    if (!options.recipe_path.has_value()) {
        error_message = "run-recipe expects --recipe <recipe.json>";
        return false;
    }
    if (!options.inputs_path.has_value()) {
        error_message = "run-recipe expects --inputs <inputs.json>";
        return false;
    }
    if (!options.output_dir.has_value()) {
        error_message = "run-recipe expects --output <output-dir>";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
