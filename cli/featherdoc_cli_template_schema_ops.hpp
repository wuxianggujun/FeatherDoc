#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto compare_optional_index(
    const std::optional<std::size_t> &left,
    const std::optional<std::size_t> &right) -> int;
[[nodiscard]] auto compare_optional_reference_kind(
    const std::optional<featherdoc::section_reference_kind> &left,
    const std::optional<featherdoc::section_reference_kind> &right) -> int;
[[nodiscard]] auto compare_template_slot_requirement(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right) -> int;
[[nodiscard]] auto make_template_schema_slot_update(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right)
    -> featherdoc::template_schema_slot_update;
[[nodiscard]] auto compare_template_slot_requirement_shape(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right) -> int;
[[nodiscard]] auto compare_template_schema_target_selector(
    const exported_template_schema_target &left,
    const exported_template_schema_target &right) -> int;
[[nodiscard]] auto compare_template_schema_target(
    const exported_template_schema_target &left,
    const exported_template_schema_target &right) -> int;
[[nodiscard]] auto compare_template_schema_target_identity(
    const exported_template_schema_target &left,
    const exported_template_schema_target &right) -> int;
[[nodiscard]] auto make_template_schema_patch_target_selector(
    const exported_template_schema_target &target)
    -> template_schema_patch_target_selector;
[[nodiscard]] auto template_schema_lint_issue_name(
    template_schema_lint_issue_kind kind) -> std::string_view;
[[nodiscard]] auto lint_template_schema(
    const exported_template_schema_result &result) -> template_schema_lint_result;

} // namespace featherdoc_cli
