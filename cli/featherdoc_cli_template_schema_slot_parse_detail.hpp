#pragma once

#include "featherdoc_cli_template_schema_slot_parse.hpp"

#include <optional>
#include <string>

namespace featherdoc_cli {

struct template_schema_json_slot_members {
    std::optional<std::string> bookmark_value;
    std::optional<std::string> bookmark_name_value;
    std::optional<std::string> content_control_tag_value;
    std::optional<std::string> content_control_alias_value;
    std::optional<std::string> kind_value;
    std::optional<bool> required_value;
    std::optional<std::size_t> count_value;
    std::optional<std::size_t> min_value;
    std::optional<std::size_t> max_value;
};

[[nodiscard]] auto parse_template_schema_json_slot_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_json_slot_members &members, std::string &error_message)
    -> bool;

[[nodiscard]] auto finalize_template_schema_json_slot(
    const template_schema_json_slot_members &members,
    featherdoc::template_slot_requirement &requirement,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
