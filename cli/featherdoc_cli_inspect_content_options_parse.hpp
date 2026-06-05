#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_paragraphs_options {
    std::optional<std::size_t> paragraph_index;
    bool json_output = false;
};

struct inspect_tables_options {
    std::optional<std::size_t> table_index;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_paragraphs_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_inspect_tables_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_tables_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
