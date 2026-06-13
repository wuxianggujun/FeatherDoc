#include "featherdoc_cli_content_control_commands.hpp"

#include "featherdoc_cli_content_control_image_commands.hpp"
#include "featherdoc_cli_content_control_inspect_commands.hpp"
#include "featherdoc_cli_content_control_paragraph_commands.hpp"
#include "featherdoc_cli_content_control_sync_commands.hpp"
#include "featherdoc_cli_content_control_table_commands.hpp"
#include "featherdoc_cli_content_control_text_commands.hpp"

namespace featherdoc_cli {

auto is_content_control_command(std::string_view command) -> bool {
    return is_content_control_inspect_command(command) ||
           command == "replace-content-control-text" ||
           command == "set-content-control-form-state" ||
           command == "sync-content-controls-from-custom-xml" ||
           command == "replace-content-control-paragraphs" ||
           command == "replace-content-control-table" ||
           command == "replace-content-control-table-rows" ||
           command == "replace-content-control-image";
}

auto run_content_control_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (is_content_control_inspect_command(command)) {
        return run_content_control_inspect_command(command, arguments, doc);
    }

    if (command == "replace-content-control-text") {
        return run_replace_content_control_text_command(command, arguments, doc);
    }
    if (command == "set-content-control-form-state") {
        return run_set_content_control_form_state_command(command, arguments,
                                                          doc);
    }
    if (command == "sync-content-controls-from-custom-xml") {
        return run_sync_content_controls_from_custom_xml_command(command,
                                                                 arguments, doc);
    }
    if (command == "replace-content-control-paragraphs") {
        return run_replace_content_control_paragraphs_command(command, arguments,
                                                              doc);
    }
    if (command == "replace-content-control-table" ||
        command == "replace-content-control-table-rows") {
        return run_replace_content_control_table_command(command, arguments, doc);
    }
    if (command == "replace-content-control-image") {
        return run_replace_content_control_image_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
