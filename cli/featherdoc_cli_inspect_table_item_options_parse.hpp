#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_table_cells_options {
    std::optional<std::size_t> row_index;
    std::optional<std::size_t> cell_index;
    std::optional<std::size_t> grid_column;
    bool json_output = false;
};

struct inspect_table_rows_options {
    std::optional<std::size_t> row_index;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_table_cells_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_table_cells_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_inspect_table_rows_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_table_rows_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
