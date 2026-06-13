#pragma once

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_ensure_style_linked_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_set_paragraph_style_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_clear_paragraph_style_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_ensure_numbering_definition_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_set_paragraph_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_set_paragraph_list_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_restart_paragraph_list_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

[[nodiscard]] auto run_clear_paragraph_list_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int;

} // namespace featherdoc_cli
