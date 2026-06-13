#pragma once

#include <featherdoc.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_numbering_catalog_definitions(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::numbering_catalog_definition> &definitions,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
