#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class template_schema_target_mode_parse_result {
    ignored,
    parsed,
    failed,
};

[[nodiscard]] auto parse_template_schema_path_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    std::string_view option_name,
    std::optional<std::filesystem::path> &target,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_schema_target_mode_option(
    std::string_view argument, bool &section_targets,
    bool &resolved_section_targets, std::string &error_message)
    -> template_schema_target_mode_parse_result;

} // namespace featherdoc_cli
