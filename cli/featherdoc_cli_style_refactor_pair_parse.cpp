#include "featherdoc_cli_style_refactor_pair_parse.hpp"

namespace featherdoc_cli {

auto parse_style_refactor_pair(
    std::string_view text, featherdoc::style_refactor_action action,
    featherdoc::style_refactor_request &request, std::string &error_message)
    -> bool {
    const auto separator = text.find(':');
    if (separator == std::string_view::npos) {
        error_message = action == featherdoc::style_refactor_action::rename
                            ? "invalid --rename value: expected <old-style-id>:<new-style-id>"
                            : "invalid --merge value: expected <source-style-id>:<target-style-id>";
        return false;
    }

    const auto source = text.substr(0U, separator);
    const auto target = text.substr(separator + 1U);
    if (source.empty()) {
        error_message = action == featherdoc::style_refactor_action::rename
                            ? "invalid --rename value: old style id must not be empty"
                            : "invalid --merge value: source style id must not be empty";
        return false;
    }
    if (target.empty()) {
        error_message = action == featherdoc::style_refactor_action::rename
                            ? "invalid --rename value: new style id must not be empty"
                            : "invalid --merge value: target style id must not be empty";
        return false;
    }

    request.action = action;
    request.source_style_id = std::string(source);
    request.target_style_id = std::string(target);
    return true;
}

} // namespace featherdoc_cli
