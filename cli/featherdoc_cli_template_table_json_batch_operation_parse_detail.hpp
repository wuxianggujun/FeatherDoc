#pragma once

#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli::detail {

struct template_table_json_batch_operation_parse_state {
    std::optional<std::string> bookmark_value;
    std::optional<std::string> bookmark_name_value;
    std::optional<std::size_t> table_index_value;
    std::optional<std::string> after_text_value;
    std::optional<std::string> after_paragraph_text_value;
    std::vector<std::string> header_cells_value;
    std::vector<std::string> header_cell_texts_value;
    bool saw_header_cells = false;
    bool saw_header_cell_texts = false;
    std::optional<std::size_t> header_row_value;
    std::optional<std::size_t> header_row_index_value;
    std::optional<std::size_t> occurrence_value;
    std::optional<validation_part_family> part_value;
    std::optional<std::size_t> index_value;
    std::optional<std::size_t> part_index_value;
    std::optional<std::size_t> section_value;
    std::optional<std::size_t> section_index_value;
    std::optional<featherdoc::section_reference_kind> reference_kind_value;
    std::optional<std::string> mode_value;
    std::optional<std::size_t> start_row_value;
    std::optional<std::size_t> start_row_index_value;
    std::optional<std::size_t> start_cell_value;
    std::optional<std::size_t> start_cell_index_value;
    std::vector<std::vector<std::string>> rows;
    bool saw_rows = false;
};

[[nodiscard]] auto parse_template_table_json_batch_operation_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_table_json_batch_operation_parse_state &state,
    std::string &error_message) -> bool;

[[nodiscard]] auto finalize_template_table_json_batch_operation(
    template_table_json_batch_operation_parse_state &&state,
    template_table_json_batch_operation &operation, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli::detail
