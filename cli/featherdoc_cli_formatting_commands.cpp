#include "featherdoc_cli_formatting_commands.hpp"

#include "featherdoc_cli_paragraph_numbering_commands.hpp"
#include "featherdoc_cli_paragraph_run_commands.hpp"
#include "featherdoc_cli_run_style_properties_commands.hpp"
#include "featherdoc_cli_style_ensure_commands.hpp"

namespace featherdoc_cli {

auto is_formatting_command(std::string_view command) -> bool {
    return command == "set-paragraph-style" ||
           command == "clear-paragraph-style" ||
           command == "set-run-style" ||
           command == "clear-run-style" ||
           command == "set-run-font-family" ||
           command == "clear-run-font-family" ||
           command == "set-run-language" ||
           command == "clear-run-language" ||
           command == "inspect-default-run-properties" ||
           command == "set-default-run-properties" ||
           command == "clear-default-run-properties" ||
           command == "inspect-style-run-properties" ||
           command == "inspect-paragraph-style-properties" ||
           command == "materialize-style-run-properties" ||
           command == "rebase-character-style-based-on" ||
           command == "rebase-paragraph-style-based-on" ||
           command == "set-paragraph-style-properties" ||
           command == "clear-paragraph-style-properties" ||
           command == "set-style-run-properties" ||
           command == "clear-style-run-properties" ||
           command == "ensure-style-linked-numbering" ||
           command == "set-paragraph-style-numbering" ||
           command == "clear-paragraph-style-numbering" ||
           command == "ensure-numbering-definition" ||
           command == "set-paragraph-numbering" ||
           command == "set-paragraph-list" ||
           command == "restart-paragraph-list" ||
           command == "clear-paragraph-list" ||
           command == "ensure-paragraph-style" ||
           command == "ensure-character-style";
}

auto run_formatting_command(
    std::string_view command,
    const std::vector<std::string_view> &arguments) -> int {
    if (command == "set-paragraph-style") {
        return run_set_paragraph_style_command(command, arguments);
    }
    if (command == "clear-paragraph-style") {
        return run_clear_paragraph_style_command(command, arguments);
    }
    if (command == "set-run-style") {
        return run_set_run_style_command(command, arguments);
    }
    if (command == "clear-run-style") {
        return run_clear_run_style_command(command, arguments);
    }
    if (command == "set-run-font-family") {
        return run_set_run_font_family_command(command, arguments);
    }
    if (command == "clear-run-font-family") {
        return run_clear_run_font_family_command(command, arguments);
    }
    if (command == "set-run-language") {
        return run_set_run_language_command(command, arguments);
    }
    if (command == "clear-run-language") {
        return run_clear_run_language_command(command, arguments);
    }
    if (command == "inspect-default-run-properties") {
        return run_inspect_default_run_properties_command(command, arguments);
    }
    if (command == "set-default-run-properties") {
        return run_set_default_run_properties_command(command, arguments);
    }
    if (command == "clear-default-run-properties") {
        return run_clear_default_run_properties_command(command, arguments);
    }
    if (command == "inspect-style-run-properties") {
        return run_inspect_style_run_properties_command(command, arguments);
    }
    if (command == "inspect-paragraph-style-properties") {
        return run_inspect_paragraph_style_properties_command(command,
                                                              arguments);
    }
    if (command == "materialize-style-run-properties") {
        return run_materialize_style_run_properties_command(command, arguments);
    }
    if (command == "rebase-character-style-based-on") {
        return run_rebase_character_style_based_on_command(command, arguments);
    }
    if (command == "rebase-paragraph-style-based-on") {
        return run_rebase_paragraph_style_based_on_command(command, arguments);
    }
    if (command == "set-paragraph-style-properties") {
        return run_set_paragraph_style_properties_command(command, arguments);
    }
    if (command == "clear-paragraph-style-properties") {
        return run_clear_paragraph_style_properties_command(command,
                                                           arguments);
    }
    if (command == "set-style-run-properties") {
        return run_set_style_run_properties_command(command, arguments);
    }
    if (command == "clear-style-run-properties") {
        return run_clear_style_run_properties_command(command, arguments);
    }
    if (command == "ensure-style-linked-numbering") {
        return run_ensure_style_linked_numbering_command(command, arguments);
    }
    if (command == "set-paragraph-style-numbering") {
        return run_set_paragraph_style_numbering_command(command, arguments);
    }
    if (command == "clear-paragraph-style-numbering") {
        return run_clear_paragraph_style_numbering_command(command, arguments);
    }
    if (command == "ensure-numbering-definition") {
        return run_ensure_numbering_definition_command(command, arguments);
    }
    if (command == "set-paragraph-numbering") {
        return run_set_paragraph_numbering_command(command, arguments);
    }
    if (command == "set-paragraph-list") {
        return run_set_paragraph_list_command(command, arguments);
    }
    if (command == "restart-paragraph-list") {
        return run_restart_paragraph_list_command(command, arguments);
    }
    if (command == "clear-paragraph-list") {
        return run_clear_paragraph_list_command(command, arguments);
    }
    if (command == "ensure-paragraph-style") {
        return run_ensure_paragraph_style_command(command, arguments);
    }
    if (command == "ensure-character-style") {
        return run_ensure_character_style_command(command, arguments);
    }

    return 2;
}

} // namespace featherdoc_cli
