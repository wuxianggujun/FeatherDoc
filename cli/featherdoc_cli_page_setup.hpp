#pragma once

#include "featherdoc_cli_page_setup_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iosfwd>
#include <string_view>

namespace featherdoc_cli {

void write_json_section_page_setup(
    std::ostream &stream, const featherdoc::section_page_setup &page_setup);

[[nodiscard]] auto inspect_page_setup(featherdoc::Document &doc,
                                      std::size_t section_index,
                                      std::string_view command,
                                      bool json_output) -> bool;
[[nodiscard]] auto inspect_page_setups(featherdoc::Document &doc,
                                       std::string_view command,
                                       bool json_output) -> bool;

[[nodiscard]] auto resolve_section_page_setup(
    featherdoc::Document &doc, std::size_t section_index,
    const set_section_page_setup_options &options, std::string_view command,
    bool json_output, featherdoc::section_page_setup &setup) -> bool;

} // namespace featherdoc_cli
