#pragma once

#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_numbering_catalog_override_patch(
    std::string_view content, std::size_t &index,
    numbering_catalog_override_patch &patch, bool allow_override_values,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
