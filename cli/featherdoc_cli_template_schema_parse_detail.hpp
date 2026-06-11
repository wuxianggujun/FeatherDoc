#pragma once

#include "featherdoc_cli_template_schema_parse.hpp"

#include <optional>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct template_schema_json_target_members {
    std::optional<std::string> part_value;
    std::optional<std::size_t> index_value;
    std::optional<std::size_t> section_value;
    std::optional<featherdoc::section_reference_kind> kind_value;
    std::optional<std::size_t> resolved_from_section_value;
    std::optional<bool> linked_to_previous_value;
    std::optional<std::string> entry_name_value;
    bool saw_slots = false;
    std::vector<featherdoc::template_slot_requirement> slots;
};

[[nodiscard]] auto parse_template_schema_json_target_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_json_target_members &members,
    std::string &error_message) -> bool;

[[nodiscard]] auto finalize_template_schema_json_target(
    const template_schema_json_target_members &members,
    exported_template_schema_target &target, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
