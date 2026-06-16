#include "featherdoc_cli_template_schema_options_parse_support.hpp"

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

} // namespace

auto parse_template_schema_path_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::string_view option_name,
    std::optional<std::filesystem::path> &target,
    std::string &error_message) -> bool {
    const auto option_text = std::string(option_name);
    if (target.has_value()) {
        error_message = "duplicate " + option_text + " option";
        return false;
    }
    if (index + 1U >= arguments.size()) {
        error_message = "missing path after " + option_text;
        return false;
    }

    target = path_type(std::string(arguments[index + 1U]));
    ++index;
    return true;
}

auto parse_template_schema_target_mode_option(
    std::string_view argument, bool &section_targets,
    bool &resolved_section_targets, std::string &error_message)
    -> template_schema_target_mode_parse_result {
    if (argument == "--section-targets") {
        if (resolved_section_targets) {
            error_message =
                "--section-targets conflicts with --resolved-section-targets";
            return template_schema_target_mode_parse_result::failed;
        }
        section_targets = true;
        return template_schema_target_mode_parse_result::parsed;
    }

    if (argument == "--resolved-section-targets") {
        if (section_targets) {
            error_message =
                "--resolved-section-targets conflicts with --section-targets";
            return template_schema_target_mode_parse_result::failed;
        }
        resolved_section_targets = true;
        return template_schema_target_mode_parse_result::parsed;
    }

    return template_schema_target_mode_parse_result::ignored;
}

} // namespace featherdoc_cli
