#pragma once

#include "featherdoc_cli_template_schema_patch_parse_detail.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli::detail {

struct template_schema_patch_remove_slot_members {
    std::optional<std::string> part_value;
    std::optional<std::size_t> index_value;
    std::optional<std::size_t> part_index_value;
    std::optional<std::size_t> section_value;
    std::optional<featherdoc::section_reference_kind> kind_value;
    std::optional<std::size_t> resolved_from_section_value;
    std::optional<bool> linked_to_previous_value;
    std::optional<std::string> bookmark_value;
    std::optional<std::string> bookmark_name_value;
    std::optional<std::string> content_control_tag_value;
    std::optional<std::string> content_control_alias_value;
};

[[nodiscard]] auto parse_template_schema_patch_remove_slot_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_patch_remove_slot_members &members,
    std::string &error_message) -> bool;
[[nodiscard]] auto parse_template_schema_patch_remove_slot_members(
    std::string_view content, std::size_t &index,
    template_schema_patch_remove_slot_members &members,
    std::string &error_message) -> bool;
[[nodiscard]] auto finalize_template_schema_patch_remove_slot(
    const template_schema_patch_remove_slot_members &members,
    template_schema_patch_remove_slot &operation,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli::detail
