#pragma once

#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_numbering_catalog_level_patch_array(
    std::string_view content, std::size_t &index,
    std::vector<numbering_catalog_level_patch> &patches,
    std::string_view member_name, std::string &error_message) -> bool;

} // namespace featherdoc_cli
