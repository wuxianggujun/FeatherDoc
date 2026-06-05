#pragma once

#include "featherdoc_cli_template_table_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

namespace featherdoc_cli {

void annotate_template_table_json_batch_error(
    std::optional<std::size_t> operation_index,
    featherdoc::document_error_info &error_info);

[[nodiscard]] auto apply_template_table_json_patch(
    std::string_view command, featherdoc::Table &table,
    std::string_view entry_name, std::size_t resolved_table_index,
    const template_table_json_patch &patch, bool json_output,
    std::optional<std::size_t> operation_index = std::nullopt) -> bool;

} // namespace featherdoc_cli
