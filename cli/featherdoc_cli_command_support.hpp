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

auto save_document(featherdoc::Document &doc,
                   const std::optional<path_type> &output_path,
                   std::string_view command = {},
                   bool json_output = false) -> bool;

auto open_document(const path_type &input_path, featherdoc::Document &doc,
                   std::string_view command = {},
                   bool json_output = false) -> bool;

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
