#include "featherdoc_cli_run_recipe_commands.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_run_recipe_execution.hpp"
#include "featherdoc_cli_run_recipe_parse.hpp"

namespace featherdoc_cli {

auto run_recipe_command(std::string_view command,
                        const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);

    run_recipe_options options;
    std::string error_message;
    if (!parse_run_recipe_options(arguments, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    return execute_run_recipe(options);
}

} // namespace featherdoc_cli
