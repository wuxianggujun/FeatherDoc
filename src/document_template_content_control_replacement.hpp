#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <pugixml.hpp>

#include <featherdoc/document_core.hpp>

namespace featherdoc {

[[nodiscard]] auto replace_content_control_with_paragraphs_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::string> &paragraphs,
    bool match_tag) -> std::size_t;

[[nodiscard]] auto replace_content_control_with_table_rows_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::vector<std::string>> &rows,
    bool match_tag) -> std::size_t;

[[nodiscard]] auto replace_content_control_with_table_by_tag_or_alias_in_part(
    featherdoc::document_error_info &last_error_info,
    pugi::xml_document &document, std::string_view entry_name,
    std::string_view value, const std::vector<std::vector<std::string>> &rows,
    bool match_tag) -> std::size_t;

} // namespace featherdoc
