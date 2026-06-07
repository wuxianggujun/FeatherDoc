#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli::detail {

void rewrite_template_schema_patch_error(std::string &error_message);

[[nodiscard]] auto finalize_template_schema_patch_selector(
    const std::optional<std::string> &part_value,
    const std::optional<std::size_t> &index_value,
    const std::optional<std::size_t> &part_index_value,
    const std::optional<std::size_t> &section_value,
    const std::optional<featherdoc::section_reference_kind> &kind_value,
    const std::optional<std::size_t> &resolved_from_section_value,
    const std::optional<bool> &linked_to_previous_value,
    template_schema_patch_target_selector &selector,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_schema_patch_target_selector(
    std::string_view content, std::size_t &index,
    template_schema_patch_target_selector &selector,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_schema_patch_target_selectors_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_target_selector> &selectors,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_schema_patch_remove_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_remove_slot> &slots,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli::detail
