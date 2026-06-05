#include "featherdoc_cli_numbering_catalog_patch_apply.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc_cli {

namespace {

auto find_numbering_catalog_patch_definition(
    featherdoc::numbering_catalog &catalog, std::string_view definition_name,
    featherdoc::numbering_catalog_definition *&definition,
    std::string &error_message) -> bool {
    definition = nullptr;
    for (auto &candidate : catalog.definitions) {
        if (candidate.definition.name != definition_name) {
            continue;
        }
        if (definition != nullptr) {
            error_message = "numbering catalog contains duplicate definition name: " +
                            std::string(definition_name);
            return false;
        }
        definition = &candidate;
    }

    if (definition == nullptr) {
        error_message = "numbering catalog definition was not found: " +
                        std::string(definition_name);
        return false;
    }

    return true;
}

auto find_numbering_catalog_patch_instance(
    featherdoc::numbering_catalog_definition &definition,
    const numbering_catalog_override_patch &patch,
    featherdoc::numbering_instance_summary *&instance,
    std::string &error_message) -> bool {
    instance = nullptr;
    if (patch.instance_index.has_value()) {
        if (*patch.instance_index >= definition.instances.size()) {
            error_message = "numbering catalog instance index is out of range for "
                            "definition: " +
                            patch.definition_name;
            return false;
        }
        instance = &definition.instances[*patch.instance_index];
        return true;
    }

    for (auto &candidate : definition.instances) {
        if (candidate.instance_id != *patch.instance_id) {
            continue;
        }
        if (instance != nullptr) {
            error_message = "numbering catalog contains duplicate instance id for "
                            "definition: " +
                            patch.definition_name;
            return false;
        }
        instance = &candidate;
    }

    if (instance == nullptr) {
        error_message = "numbering catalog instance id was not found for definition: " +
                        patch.definition_name;
        return false;
    }

    return true;
}

void sort_numbering_catalog_patch_levels(
    featherdoc::numbering_catalog_definition &definition) {
    std::sort(definition.definition.levels.begin(),
              definition.definition.levels.end(), [](const auto &lhs,
                                                     const auto &rhs) {
                  return lhs.level < rhs.level;
              });
}

void sort_numbering_catalog_patch_overrides(
    featherdoc::numbering_instance_summary &instance) {
    std::sort(instance.level_overrides.begin(), instance.level_overrides.end(),
              [](const auto &lhs, const auto &rhs) {
                  return lhs.level < rhs.level;
              });
}

auto apply_numbering_catalog_level_upsert(
    featherdoc::numbering_catalog &catalog,
    const numbering_catalog_level_patch &patch,
    numbering_catalog_patch_summary &summary, std::string &error_message) -> bool {
    featherdoc::numbering_catalog_definition *definition = nullptr;
    if (!find_numbering_catalog_patch_definition(catalog, patch.definition_name,
                                                 definition, error_message)) {
        return false;
    }

    auto &levels = definition->definition.levels;
    auto level_it = std::find_if(
        levels.begin(), levels.end(), [&patch](const auto &level) {
            return level.level == patch.level_definition.level;
        });
    if (level_it == levels.end()) {
        levels.push_back(patch.level_definition);
    } else {
        *level_it = patch.level_definition;
    }

    sort_numbering_catalog_patch_levels(*definition);
    ++summary.upserted_level_count;
    return true;
}

auto apply_numbering_catalog_override_upsert(
    featherdoc::numbering_catalog &catalog,
    const numbering_catalog_override_patch &patch,
    numbering_catalog_patch_summary &summary, std::string &error_message) -> bool {
    featherdoc::numbering_catalog_definition *definition = nullptr;
    if (!find_numbering_catalog_patch_definition(catalog, patch.definition_name,
                                                 definition, error_message)) {
        return false;
    }

    featherdoc::numbering_instance_summary *instance = nullptr;
    if (!find_numbering_catalog_patch_instance(*definition, patch, instance,
                                               error_message)) {
        return false;
    }

    auto override_it = std::find_if(
        instance->level_overrides.begin(), instance->level_overrides.end(),
        [&patch](const auto &level_override) {
            return level_override.level == patch.level;
        });
    if (override_it == instance->level_overrides.end()) {
        auto level_override = featherdoc::numbering_level_override_summary{};
        level_override.level = patch.level;
        if (patch.saw_start_override) {
            level_override.start_override = patch.start_override;
        }
        if (patch.saw_level_definition) {
            level_override.level_definition = patch.level_definition;
        }
        instance->level_overrides.push_back(std::move(level_override));
    } else {
        if (patch.saw_start_override) {
            override_it->start_override = patch.start_override;
        }
        if (patch.saw_level_definition) {
            override_it->level_definition = patch.level_definition;
        }
    }

    sort_numbering_catalog_patch_overrides(*instance);
    ++summary.upserted_override_count;
    return true;
}

auto apply_numbering_catalog_override_remove(
    featherdoc::numbering_catalog &catalog,
    const numbering_catalog_override_patch &patch,
    numbering_catalog_patch_summary &summary, std::string &error_message) -> bool {
    featherdoc::numbering_catalog_definition *definition = nullptr;
    if (!find_numbering_catalog_patch_definition(catalog, patch.definition_name,
                                                 definition, error_message)) {
        return false;
    }

    featherdoc::numbering_instance_summary *instance = nullptr;
    if (!find_numbering_catalog_patch_instance(*definition, patch, instance,
                                               error_message)) {
        return false;
    }

    const auto original_size = instance->level_overrides.size();
    instance->level_overrides.erase(
        std::remove_if(instance->level_overrides.begin(),
                       instance->level_overrides.end(),
                       [&patch](const auto &level_override) {
                           return level_override.level == patch.level;
                       }),
        instance->level_overrides.end());
    if (instance->level_overrides.size() == original_size) {
        ++summary.missing_override_count;
    } else {
        ++summary.removed_override_count;
    }

    return true;
}

} // namespace

auto apply_numbering_catalog_patch(
    featherdoc::numbering_catalog &catalog,
    const numbering_catalog_patch_document &patch,
    numbering_catalog_patch_summary &summary, std::string &error_message) -> bool {
    for (const auto &level_patch : patch.upsert_levels) {
        if (!apply_numbering_catalog_level_upsert(catalog, level_patch, summary,
                                                  error_message)) {
            return false;
        }
    }
    for (const auto &override_patch : patch.upsert_overrides) {
        if (!apply_numbering_catalog_override_upsert(catalog, override_patch,
                                                     summary, error_message)) {
            return false;
        }
    }
    for (const auto &override_patch : patch.remove_overrides) {
        if (!apply_numbering_catalog_override_remove(catalog, override_patch,
                                                     summary, error_message)) {
            return false;
        }
    }

    return true;
}

} // namespace featherdoc_cli
