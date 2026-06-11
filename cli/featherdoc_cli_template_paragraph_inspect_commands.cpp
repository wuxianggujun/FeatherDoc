#include "featherdoc_cli_template_paragraph_inspect_commands.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_paragraph_inspect_commands_detail.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto is_template_paragraph_inspect_command(std::string_view command) -> bool {
    return command == "inspect-template-paragraphs" ||
           command == "inspect-template-runs";
}

auto run_template_paragraph_inspect_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "inspect-template-paragraphs") {
        return run_inspect_template_paragraphs_command(command, arguments);
    }
    if (command == "inspect-template-runs") {
        return run_inspect_template_runs_command(command, arguments);
    }

    print_parse_error(command,
                      "unsupported template paragraph inspect command: " +
                          std::string(command),
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
