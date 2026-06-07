#pragma once

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <ostream>
#include <string_view>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

struct cli_text_source_options;
struct section_text_options;
struct table_cell_text_options;

auto save_document(featherdoc::Document &doc,
                   const std::optional<path_type> &output_path,
                   std::string_view command = {},
                   bool json_output = false) -> bool;

auto open_document(const path_type &input_path, featherdoc::Document &doc,
                   std::string_view command = {},
                   bool json_output = false) -> bool;

auto resolve_body_table(featherdoc::Document &doc, std::size_t table_index,
                        featherdoc::Table &table, std::string_view command,
                        bool json_output,
                        std::string_view stage = "mutate") -> bool;

auto resolve_body_table_row(featherdoc::Document &doc, std::size_t table_index,
                            std::size_t row_index, featherdoc::TableRow &row,
                            std::string_view command, bool json_output,
                            std::string_view stage = "mutate") -> bool;

auto resolve_body_table_cell(featherdoc::Document &doc, std::size_t table_index,
                             std::size_t row_index, std::size_t cell_index,
                             featherdoc::TableCell &cell,
                             std::string_view command, bool json_output) -> bool;

[[nodiscard]] auto read_text_source(const cli_text_source_options &options,
                                    std::string &text,
                                    std::string &error_message) -> bool;

[[nodiscard]] auto read_text_source(const section_text_options &options,
                                    std::string &text,
                                    std::string &error_message) -> bool;

[[nodiscard]] auto read_text_source(const table_cell_text_options &options,
                                    std::string &text,
                                    std::string &error_message) -> bool;

template <typename ExtraWriter>
void write_json_mutation_result(std::string_view command, featherdoc::Document &doc,
                                const std::optional<path_type> &output_path,
                                ExtraWriter &&write_extra) {
    std::cout << "{\"command\":";
    write_json_string(std::cout, command);
    std::cout << ",\"ok\":true"
              << ",\"in_place\":" << json_bool(!output_path.has_value())
              << ",\"sections\":" << doc.section_count()
              << ",\"headers\":" << doc.header_count()
              << ",\"footers\":" << doc.footer_count();
    write_extra(std::cout);
    std::cout << "}\n";
}

void write_json_mutation_result(std::string_view command, featherdoc::Document &doc,
                                const std::optional<path_type> &output_path);

} // namespace featherdoc_cli
