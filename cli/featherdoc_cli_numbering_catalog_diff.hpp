#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct changed_numbering_catalog_level {
    featherdoc::numbering_level_definition left;
    featherdoc::numbering_level_definition right;
};

struct changed_numbering_catalog_override {
    featherdoc::numbering_level_override_summary left;
    featherdoc::numbering_level_override_summary right;
};

struct numbering_catalog_instance_diff_result {
    std::size_t instance_index{};
    std::vector<featherdoc::numbering_level_override_summary> added_overrides;
    std::vector<featherdoc::numbering_level_override_summary> removed_overrides;
    std::vector<changed_numbering_catalog_override> changed_overrides;

    [[nodiscard]] bool equal() const noexcept {
        return this->added_overrides.empty() && this->removed_overrides.empty() &&
               this->changed_overrides.empty();
    }
};

struct changed_numbering_catalog_definition {
    std::string name;
    std::vector<featherdoc::numbering_level_definition> added_levels;
    std::vector<featherdoc::numbering_level_definition> removed_levels;
    std::vector<changed_numbering_catalog_level> changed_levels;
    std::vector<featherdoc::numbering_instance_summary> added_instances;
    std::vector<featherdoc::numbering_instance_summary> removed_instances;
    std::vector<numbering_catalog_instance_diff_result> changed_instances;

    [[nodiscard]] bool equal() const noexcept {
        return this->added_levels.empty() && this->removed_levels.empty() &&
               this->changed_levels.empty() && this->added_instances.empty() &&
               this->removed_instances.empty() && this->changed_instances.empty();
    }
};

struct numbering_catalog_diff_result {
    std::vector<featherdoc::numbering_catalog_definition> added_definitions;
    std::vector<featherdoc::numbering_catalog_definition> removed_definitions;
    std::vector<changed_numbering_catalog_definition> changed_definitions;

    [[nodiscard]] bool equal() const noexcept {
        return this->added_definitions.empty() && this->removed_definitions.empty() &&
               this->changed_definitions.empty();
    }
};

[[nodiscard]] auto diff_numbering_catalogs(
    const featherdoc::numbering_catalog &left,
    const featherdoc::numbering_catalog &right) -> numbering_catalog_diff_result;

} // namespace featherdoc_cli
