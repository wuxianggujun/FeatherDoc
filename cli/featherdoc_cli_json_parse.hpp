#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

void skip_json_patch_whitespace(std::string_view text, std::size_t &index);

[[nodiscard]] auto report_json_input_error(std::string_view input_label,
                                           std::size_t offset,
                                           std::string_view detail,
                                           std::string &error_message) -> bool;
[[nodiscard]] auto report_json_patch_error(std::size_t offset,
                                           std::string_view detail,
                                           std::string &error_message) -> bool;

[[nodiscard]] auto parse_json_patch_string(std::string_view text,
                                           std::size_t &index,
                                           std::string &value,
                                           std::string &error_message) -> bool;
[[nodiscard]] auto parse_json_patch_number(std::string_view text,
                                           std::size_t &index,
                                           std::string &value,
                                           std::string &error_message) -> bool;
[[nodiscard]] auto skip_json_patch_value(std::string_view text,
                                         std::size_t &index,
                                         std::string &error_message) -> bool;
[[nodiscard]] auto parse_json_patch_index_value(std::string_view text,
                                                std::size_t &index,
                                                std::size_t &value,
                                                std::string_view member_name,
                                                std::string &error_message)
    -> bool;
[[nodiscard]] auto resolve_json_patch_string_member(
    const std::optional<std::string> &primary_value,
    const std::optional<std::string> &alias_value,
    std::string_view primary_name, std::string_view alias_name,
    std::optional<std::string> &resolved_value,
    std::string &error_message) -> bool;
[[nodiscard]] auto resolve_json_patch_index_member(
    std::optional<std::size_t> primary_value,
    std::optional<std::size_t> alias_value, std::string_view primary_name,
    std::string_view alias_name, std::optional<std::size_t> &resolved_value,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_json_patch_bool_member_value(
    std::string_view text, std::size_t &index, bool &value,
    std::string_view member_name, std::string &error_message) -> bool;
[[nodiscard]] auto read_template_table_json_content(
    const std::filesystem::path &patch_path, std::string &content,
    std::size_t &index, std::string &error_message) -> bool;

} // namespace featherdoc_cli
