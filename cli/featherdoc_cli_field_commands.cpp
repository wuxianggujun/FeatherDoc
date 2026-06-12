#include "featherdoc_cli_field_commands.hpp"

#include "featherdoc_cli_field_append_command.hpp"
#include "featherdoc_cli_field_parse.hpp"
#include "featherdoc_cli_field_update_commands.hpp"

namespace featherdoc_cli {

auto is_field_command(std::string_view command) -> bool {
    return is_append_field_command(command) ||
           command == "inspect-update-fields-on-open" ||
           command == "set-update-fields-on-open";
}

auto run_field_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-update-fields-on-open") {
        return run_inspect_update_fields_on_open_command(command, arguments,
                                                        doc);
    }

    if (command == "set-update-fields-on-open") {
        return run_set_update_fields_on_open_command(command, arguments, doc);
    }

    if (is_append_field_command(command)) {
        return run_append_field_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
