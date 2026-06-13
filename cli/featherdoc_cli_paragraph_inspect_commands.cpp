#include "featherdoc_cli_paragraph_inspect_commands.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_paragraph_inspect_commands_detail.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_paragraph_inspect_commands.hpp"

#include <string>

namespace featherdoc_cli {

auto is_paragraph_inspect_command(std::string_view command) -> bool {
    return command == "inspect-paragraphs" || command == "inspect-runs" ||
           is_template_paragraph_inspect_command(command);
}

auto run_paragraph_inspect_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "inspect-paragraphs") {
        return run_inspect_paragraphs_command(command, arguments);
    }
    if (command == "inspect-runs") {
        return run_inspect_runs_command(command, arguments);
    }
    if (is_template_paragraph_inspect_command(command)) {
        return run_template_paragraph_inspect_command(command, arguments);
    }

    print_parse_error(command,
                      "unsupported paragraph inspect command: " +
                          std::string(command),
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
