#pragma once

#include "featherdoc_cli_template_table_options_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_template_table_json_batch_operations_array(
    std::string_view content, std::size_t &index,
    std::vector<template_table_json_batch_operation> &operations,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
