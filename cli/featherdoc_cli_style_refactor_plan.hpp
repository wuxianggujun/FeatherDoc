#pragma once

#include <featherdoc.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto style_refactor_action_name(
    featherdoc::style_refactor_action action) -> const char *;
[[nodiscard]] auto style_refactor_command_template(
    const featherdoc::style_refactor_operation_plan &operation) -> std::string;
[[nodiscard]] auto style_refactor_rollback_command_template(
    const featherdoc::style_refactor_rollback_entry &entry) -> std::string;
[[nodiscard]] auto style_merge_suggestion_confidence_profile_min_confidence(
    std::string_view profile) -> std::optional<std::uint32_t>;
[[nodiscard]] auto style_merge_suggestion_confidence_profile_is_valid(
    std::string_view profile) -> bool;
[[nodiscard]] auto filter_style_refactor_plan_by_min_confidence(
    featherdoc::style_refactor_plan plan, std::uint32_t min_confidence)
    -> featherdoc::style_refactor_plan;
[[nodiscard]] auto filter_style_refactor_plan_by_style_ids(
    featherdoc::style_refactor_plan plan,
    const std::vector<std::string> &source_style_ids,
    const std::vector<std::string> &target_style_ids)
    -> featherdoc::style_refactor_plan;

} // namespace featherdoc_cli
