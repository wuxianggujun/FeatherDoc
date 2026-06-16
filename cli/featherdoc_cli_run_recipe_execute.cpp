#include "featherdoc_cli_run_recipe_execution.hpp"

#include "featherdoc_cli_errors.hpp"

#include <iostream>
#include <string>
#include <unordered_map>

namespace featherdoc_cli {

auto report_run_recipe_execution_error(std::string_view stage,
                                       const std::string &message,
                                       bool json_output) -> int {
    if (json_output) {
        write_json_command_error(std::cerr, "run-recipe", stage, message);
    } else {
        std::cerr << message << '\n';
    }
    return 1;
}

auto execute_run_recipe(const run_recipe_options &options) -> int {
    std::string recipe_id;
    std::string error_message;
    if (!read_run_recipe_recipe_id(*options.recipe_path, recipe_id,
                                   error_message)) {
        return report_run_recipe_execution_error("input", error_message,
                                                 options.json_output);
    }

    std::unordered_map<std::string, std::string> inputs;
    if (!read_run_recipe_inputs_file(*options.inputs_path, inputs,
                                     error_message)) {
        return report_run_recipe_execution_error("input", error_message,
                                                 options.json_output);
    }

    if (recipe_id == "batch_replace") {
        run_recipe_batch_replace_result result;
        if (!execute_run_recipe_batch_replace(inputs, *options.output_dir, result,
                                              error_message)) {
            return report_run_recipe_execution_error("execute", error_message,
                                                     options.json_output);
        }

        if (options.json_output) {
            write_json_run_recipe_batch_replace_success(recipe_id, result);
        } else {
            print_run_recipe_batch_replace_success(result);
        }
        return 0;
    }

    return report_run_recipe_execution_error(
        "execute", "unsupported run-recipe recipe id: " + recipe_id,
        options.json_output);
}

} // namespace featherdoc_cli
