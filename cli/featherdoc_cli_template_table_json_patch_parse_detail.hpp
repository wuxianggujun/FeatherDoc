#pragma once

#include "featherdoc_cli_template_table_options_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_json_patch_cell_text(std::string_view text,
                                              std::size_t &index,
                                              std::string &cell_text,
                                              std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_json_patch_rows_matrix(
    std::string_view text, std::size_t &index,
    std::vector<std::vector<std::string>> &rows, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_json_patch_text_array(
    std::string_view text, std::size_t &index, std::vector<std::string> &values,
    std::string_view member_name, std::string &error_message) -> bool;

[[nodiscard]] auto parse_json_patch_part_value(std::string_view text,
                                               std::size_t &index,
                                               validation_part_family &part,
                                               std::string &error_message)
    -> bool;

[[nodiscard]] auto finalize_template_table_json_patch(
    const std::optional<std::string> &mode_value,
    const std::optional<std::size_t> &start_row_value,
    const std::optional<std::size_t> &start_row_index_value,
    const std::optional<std::size_t> &start_cell_value,
    const std::optional<std::size_t> &start_cell_index_value,
    std::vector<std::vector<std::string>> rows, bool saw_rows,
    template_table_json_patch &patch, std::string &error_message) -> bool;

} // namespace featherdoc_cli
