#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli::detail {

void rewrite_template_schema_patch_error(std::string &error_message);

[[nodiscard]] auto parse_template_schema_patch_target_selectors_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_target_selector> &selectors,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_schema_patch_remove_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_remove_slot> &slots,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_schema_patch_update_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_update_slot> &slots,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_schema_patch_rename_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_rename_slot> &slots,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli::detail
