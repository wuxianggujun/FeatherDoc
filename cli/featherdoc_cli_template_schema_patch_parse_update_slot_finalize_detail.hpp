#pragma once

#include "featherdoc_cli_template_schema_patch_parse_update_slot_state.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli::detail {

[[nodiscard]] auto resolve_json_patch_bool_alias(
    const std::optional<bool> &primary, const std::optional<bool> &alias,
    std::string_view primary_name, std::string_view alias_name,
    std::optional<bool> &resolved, std::string &error_message) -> bool;

[[nodiscard]] auto resolve_update_slot_selector_name(
    const template_schema_patch_update_slot_parse_state &state,
    featherdoc::template_slot_source_kind &source, std::string &slot_name,
    std::string &error_message) -> bool;

[[nodiscard]] auto resolve_update_slot_occurrence_aliases(
    const template_schema_patch_update_slot_parse_state &state,
    std::optional<std::size_t> &resolved_min,
    std::optional<std::size_t> &resolved_max,
    std::optional<bool> &resolved_clear_min,
    std::optional<bool> &resolved_clear_max, std::string &error_message)
    -> bool;

[[nodiscard]] auto validate_update_slot_occurrence_options(
    const template_schema_patch_update_slot_parse_state &state,
    const std::optional<std::size_t> &resolved_min,
    const std::optional<std::size_t> &resolved_max,
    const std::optional<bool> &resolved_clear_min,
    const std::optional<bool> &resolved_clear_max,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli::detail
