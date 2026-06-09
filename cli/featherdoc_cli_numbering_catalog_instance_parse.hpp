#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto parse_numbering_catalog_instances(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::numbering_instance_summary> &instances,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
