#include "featherdoc_cli_style_refactor_commands.hpp"
#include "featherdoc_cli_style_refactor_commands_detail.hpp"

namespace featherdoc_cli {

auto is_style_refactor_command(std::string_view command) -> bool {
    return command == "plan-style-refactor" ||
           command == "suggest-style-merges" ||
           command == "apply-style-refactor" ||
           command == "restore-style-merge" || command == "rename-style" ||
           command == "merge-style" ||
           command == "plan-prune-unused-styles" ||
           command == "prune-unused-styles";
}

auto run_style_refactor_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "plan-style-refactor") {
        return run_plan_style_refactor_command(command, arguments, doc);
    }
    if (command == "suggest-style-merges") {
        return run_suggest_style_merges_command(command, arguments, doc);
    }
    if (command == "apply-style-refactor") {
        return run_apply_style_refactor_command(command, arguments, doc);
    }
    if (command == "restore-style-merge") {
        return run_restore_style_merge_command(command, arguments, doc);
    }
    if (command == "rename-style") {
        return run_rename_style_command(command, arguments, doc);
    }
    if (command == "merge-style") {
        return run_merge_style_command(command, arguments, doc);
    }
    if (command == "plan-prune-unused-styles") {
        return run_plan_prune_unused_styles_command(command, arguments, doc);
    }
    if (command == "prune-unused-styles") {
        return run_prune_unused_styles_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
