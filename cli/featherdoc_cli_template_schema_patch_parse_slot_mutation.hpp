#pragma once

#include "featherdoc_cli_template_schema_patch_parse_detail.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli::detail {

[[nodiscard]] auto parse_template_schema_patch_update_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_update_slot> &slots,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_template_schema_patch_rename_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_rename_slot> &slots,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli::detail