#include "featherdoc_cli_style_refactor_plan_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <string>
#include <string_view>

namespace featherdoc_cli {

auto parse_style_refactor_plan_action(std::string_view content,
                                      std::size_t &index,
                                      featherdoc::style_refactor_action &action,
                                      std::string &error_message) -> bool {
    std::string action_name;
    if (!parse_json_patch_string(content, index, action_name, error_message)) {
        return false;
    }

    if (action_name == "rename") {
        action = featherdoc::style_refactor_action::rename;
        return true;
    }
    if (action_name == "merge") {
        action = featherdoc::style_refactor_action::merge;
        return true;
    }

    return report_json_input_error("style refactor plan file", index,
                                   "operation action must be 'rename' or 'merge'",
                                   error_message);
}
} // namespace featherdoc_cli
