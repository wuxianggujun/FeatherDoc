#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct numbering_catalog_override_patch {
    std::string definition_name;
    std::optional<std::size_t> instance_index;
    std::optional<std::uint32_t> instance_id;
    std::uint32_t level{};
    bool saw_start_override = false;
    std::optional<std::uint32_t> start_override;
    bool saw_level_definition = false;
    std::optional<featherdoc::numbering_level_definition> level_definition;
};

struct numbering_catalog_level_patch {
    std::string definition_name;
    featherdoc::numbering_level_definition level_definition;
};

struct numbering_catalog_patch_document {
    std::vector<numbering_catalog_level_patch> upsert_levels;
    std::vector<numbering_catalog_override_patch> upsert_overrides;
    std::vector<numbering_catalog_override_patch> remove_overrides;
};

[[nodiscard]] auto read_numbering_catalog_patch_file(
    const std::filesystem::path &patch_path,
    numbering_catalog_patch_document &patch, std::string &error_message) -> bool;

} // namespace featherdoc_cli