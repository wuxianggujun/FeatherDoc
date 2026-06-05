#pragma once

#include "featherdoc_cli_template_table_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto prefix_template_table_json_batch_message(
    std::optional<std::size_t> operation_index, std::string_view message)
    -> std::string;

[[nodiscard]] auto read_template_table_json_patch(
    const std::filesystem::path &patch_path, template_table_json_patch &patch,
    std::string &error_message) -> bool;

[[nodiscard]] auto read_template_table_json_batch(
    const std::filesystem::path &patch_path,
    std::vector<template_table_json_batch_operation> &operations,
    std::string &error_message) -> bool;

[[nodiscard]] auto resolve_template_table_json_batch_operation_selection(
    const template_table_json_batch_options &options,
    const template_table_json_batch_operation &operation,
    validation_part_family &part, std::optional<std::size_t> &part_index,
    std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind &reference_kind, bool &has_kind,
    featherdoc::template_table_selector &selector, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
