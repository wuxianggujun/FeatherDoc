#pragma once

#include <featherdoc.hpp>

#include <cstdint>
#include <iosfwd>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_paragraph_numbering_index_argument(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::uint32_t &paragraph_index, bool json_output) -> bool;

[[nodiscard]] auto resolve_paragraph_numbering_body_paragraph(
    std::string_view command, featherdoc::Document &doc,
    std::uint32_t paragraph_index, featherdoc::Paragraph &paragraph,
    bool json_output) -> bool;

void write_json_numbering_definition_levels(
    std::ostream &stream,
    const std::vector<featherdoc::numbering_level_definition> &levels);

void write_json_paragraph_style_numbering_links(
    std::ostream &stream,
    const std::vector<featherdoc::paragraph_style_numbering_link> &style_links);

[[nodiscard]] auto open_paragraph_numbering_document_for_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc, bool json_output) -> bool;

} // namespace featherdoc_cli
