#pragma once

#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <string>

namespace featherdoc_cli {

struct numbering_catalog_patch_summary {
    std::size_t upserted_level_count{};
    std::size_t upserted_override_count{};
    std::size_t removed_override_count{};
    std::size_t missing_override_count{};
};

[[nodiscard]] auto apply_numbering_catalog_patch(
    featherdoc::numbering_catalog &catalog,
    const numbering_catalog_patch_document &patch,
    numbering_catalog_patch_summary &summary, std::string &error_message) -> bool;

} // namespace featherdoc_cli