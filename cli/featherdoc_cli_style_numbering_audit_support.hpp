#pragma once

#include "featherdoc_cli_style_numbering_audit.hpp"
#include "featherdoc_cli_style_numbering_repair_suggestions.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto find_numbering_definition_summary_by_id(
    const std::vector<featherdoc::numbering_definition_summary> &definitions,
    std::uint32_t definition_id)
    -> const featherdoc::numbering_definition_summary *;
[[nodiscard]] auto find_style_summary_by_id(
    const std::vector<featherdoc::style_summary> &styles,
    std::string_view style_id) -> const featherdoc::style_summary *;
[[nodiscard]] auto numbering_definition_has_level(
    const featherdoc::numbering_definition_summary &definition,
    std::uint32_t level) -> bool;
[[nodiscard]] auto find_unique_numbering_definition_summary_by_name_and_level(
    const std::vector<featherdoc::numbering_definition_summary> &definitions,
    std::string_view name, std::uint32_t level,
    std::optional<std::uint32_t> excluded_definition_id)
    -> const featherdoc::numbering_definition_summary *;
[[nodiscard]] auto numbering_instance_has_level_definition_override(
    const std::optional<featherdoc::numbering_instance_summary> &instance,
    std::uint32_t level) -> bool;
[[nodiscard]] auto numbering_definition_name_is_duplicate(
    const std::vector<featherdoc::numbering_definition_summary> &definitions,
    std::string_view name) -> bool;

void append_style_numbering_audit_issue(
    style_numbering_audit_result &result,
    style_numbering_audit_issue_kind kind,
    const featherdoc::style_summary &style, std::string message,
    const style_numbering_repair_target *target = nullptr);

} // namespace featherdoc_cli
