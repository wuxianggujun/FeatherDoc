#pragma once

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_index_arg(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t argument_index, std::string_view label, std::size_t &value,
    bool json_output) -> bool;

[[nodiscard]] auto parse_table_index(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t &table_index, bool json_output) -> bool;

[[nodiscard]] auto parse_table_cell_location(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t &table_index, std::size_t &row_index,
    std::size_t &cell_index, bool json_output) -> bool;

[[nodiscard]] auto parse_uint32_arg(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t argument_index, std::string_view label, std::uint32_t &value,
    bool json_output) -> bool;

[[nodiscard]] auto parse_style_options(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t option_start, table_cell_style_options &options,
    bool json_output) -> bool;

[[nodiscard]] auto parse_border_options(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t option_start, table_cell_border_options &options,
    bool json_output) -> bool;

[[nodiscard]] auto make_border_definition(
    const table_cell_border_options &options) -> featherdoc::border_definition;

auto report_invalid_table_target(std::string_view command,
                                 std::string_view summary, bool json_output)
    -> bool;

auto report_invalid_table_cell_target(std::string_view command,
                                      std::string_view summary,
                                      bool json_output) -> bool;

void write_json_table_location(std::ostream &stream, std::size_t table_index);
void write_json_table_cell_location(std::ostream &stream,
                                    std::size_t table_index,
                                    std::size_t row_index,
                                    std::size_t cell_index);
void write_json_uint_field(std::ostream &stream, std::string_view field_name,
                           std::uint32_t value);
void write_json_border_definition(
    std::ostream &stream, const featherdoc::border_definition &border);

template <typename Options>
auto open_table_for_format(std::string_view command,
                           const std::vector<std::string_view> &arguments,
                           featherdoc::Document &doc,
                           std::size_t table_index, const Options &options,
                           featherdoc::Table &table) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    return resolve_body_table(doc, table_index, table, command,
                              options.json_output);
}

template <typename Options>
auto open_table_cell_for_format(std::string_view command,
                                const std::vector<std::string_view> &arguments,
                                featherdoc::Document &doc,
                                std::size_t table_index,
                                std::size_t row_index,
                                std::size_t cell_index, const Options &options,
                                featherdoc::TableCell &cell) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    return resolve_body_table_cell(doc, table_index, row_index, cell_index, cell,
                                   command, options.json_output);
}

} // namespace featherdoc_cli
