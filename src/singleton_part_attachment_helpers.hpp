#pragma once

#include "package_content_types_xml_helpers.hpp"
#include "package_path_helpers.hpp"
#include "package_relationships_xml_helpers.hpp"
#include "xml_document_clone_helpers.hpp"
#include "xml_document_initialization_helpers.hpp"
#include "xml_namespace_helpers.hpp"

#include <featherdoc/document_core.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include <pugixml.hpp>

namespace featherdoc::detail {

struct singleton_part_attachment_options final {
    std::string_view relationship_type;
    std::string_view relationship_target;
    std::string_view override_part_name;
    std::string_view content_type;
    std::string_view relationships_entry_name;
    std::string_view content_types_entry_name;
    document_errc relationships_parse_error{
        document_errc::relationships_xml_parse_failed};
    document_errc content_types_parse_error{
        document_errc::content_types_xml_parse_failed};
};

struct singleton_part_attachment_work final {
    pugi::xml_document relationships;
    pugi::xml_document content_types;
    bool relationships_changed{false};
    bool content_types_changed{false};
};

namespace singleton_part_attachment_detail {

inline constexpr auto package_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/package/2006/relationships"};
inline constexpr auto package_content_types_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/package/2006/content-types"};

[[nodiscard]] inline auto
set_error(document_error_info &error_info, std::error_code code,
          std::string_view detail, std::string_view entry_name) noexcept
    -> std::error_code {
    error_info.code = code;
    error_info.detail.clear();
    error_info.entry_name.clear();
    error_info.xml_offset.reset();
    try {
        error_info.detail.assign(detail);
        error_info.entry_name.assign(entry_name);
    } catch (...) {
        error_info.detail.clear();
        error_info.entry_name.clear();
    }
    return code;
}

[[nodiscard]] inline auto set_error(document_error_info &error_info,
                                    document_errc code, std::string_view detail,
                                    std::string_view entry_name) noexcept
    -> std::error_code {
    return set_error(error_info, make_error_code(code), detail, entry_name);
}

[[nodiscard]] inline auto
find_relationship_by_type(pugi::xml_node relationships,
                          std::string_view relationship_type)
    -> pugi::xml_node {
    for (auto relationship = first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship = next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} ==
            relationship_type) {
            return relationship;
        }
    }
    return {};
}

[[nodiscard]] inline auto find_override_by_part_name(pugi::xml_node types,
                                                     std::string_view part_name)
    -> pugi::xml_node {
    for (auto override_node = first_package_content_type_override(types);
         override_node != pugi::xml_node{};
         override_node = next_package_content_type_override(override_node)) {
        if (content_type_part_names_equivalent(
                override_node.attribute("PartName").value(), part_name)) {
            return override_node;
        }
    }
    return {};
}

[[nodiscard]] inline auto next_relationship_id(pugi::xml_node relationships)
    -> std::string {
    std::unordered_set<std::string> used_ids;
    for (auto relationship = first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship = next_package_relationship(relationship)) {
        const auto id = std::string_view{relationship.attribute("Id").value()};
        if (!id.empty()) {
            used_ids.emplace(id);
        }
    }

    for (std::size_t index = 1U;; ++index) {
        auto candidate = std::string{"rId"} + std::to_string(index);
        if (!used_ids.contains(candidate)) {
            return candidate;
        }
    }
}

} // namespace singleton_part_attachment_detail

// Build both package metadata edits in isolated DOMs. No live document state is
// changed until publish_checked_singleton_part_attachment() is called.
[[nodiscard]] inline auto prepare_checked_singleton_part_attachment(
    const pugi::xml_document &source_relationships,
    const pugi::xml_document &source_content_types,
    const singleton_part_attachment_options &options,
    document_error_info &error_info, singleton_part_attachment_work &work)
    -> std::error_code {
    auto allocation_failure_entry = options.relationships_entry_name;
    try {
        singleton_part_attachment_work candidate;
        if (checked_clone_xml_document(source_relationships,
                                       candidate.relationships) !=
            xml_document_clone_status::success) {
            return set_xml_document_clone_allocation_failure(
                error_info, options.relationships_entry_name);
        }
        allocation_failure_entry = options.content_types_entry_name;
        if (checked_clone_xml_document(source_content_types,
                                       candidate.content_types) !=
            xml_document_clone_status::success) {
            return set_xml_document_clone_allocation_failure(
                error_info, options.content_types_entry_name);
        }

        allocation_failure_entry = options.relationships_entry_name;
        const auto relationships_state =
            inspect_package_relationships_document(candidate.relationships);
        if (relationships_state ==
            package_relationships_document_state::invalid) {
            return singleton_part_attachment_detail::set_error(
                error_info, options.relationships_parse_error,
                "relationships part has an invalid Relationships root",
                options.relationships_entry_name);
        }
        if (relationships_state ==
            package_relationships_document_state::missing) {
            const auto initialization = initialize_empty_relationships_document(
                candidate.relationships);
            if (!initialization) {
                return set_fixed_xml_initialization_failure(
                    error_info, initialization,
                    options.relationships_parse_error,
                    options.relationships_entry_name);
            }
            candidate.relationships_changed = true;
        }

        auto relationships =
            package_relationships_root(candidate.relationships);
        if (relationships == pugi::xml_node{}) {
            return singleton_part_attachment_detail::set_error(
                error_info, options.relationships_parse_error,
                "relationships part does not contain a Relationships root",
                options.relationships_entry_name);
        }
        allocation_failure_entry = options.content_types_entry_name;
        const auto content_types_inspection =
            inspect_package_content_types_document_structure(
                candidate.content_types);
        if (content_types_inspection.state !=
            package_content_types_document_state::valid) {
            return singleton_part_attachment_detail::set_error(
                error_info, options.content_types_parse_error,
                content_types_inspection.detail.empty()
                    ? std::string_view{"[Content_Types].xml has an invalid "
                                       "Types root"}
                    : content_types_inspection.detail,
                options.content_types_entry_name);
        }
        auto types = package_content_types_root(candidate.content_types);

        allocation_failure_entry = options.relationships_entry_name;
        if (singleton_part_attachment_detail::find_relationship_by_type(
                relationships, options.relationship_type) == pugi::xml_node{}) {
            const auto relationship_id =
                singleton_part_attachment_detail::next_relationship_id(
                    relationships);
            auto relationship = append_package_relationship(relationships);
            if (relationship == pugi::xml_node{} ||
                !xml_element_has_expanded_name(
                    relationship, "Relationship",
                    singleton_part_attachment_detail::
                        package_relationships_namespace_uri) ||
                !checked_set_xml_attribute_value(relationship, "Id",
                                                 relationship_id) ||
                !checked_set_xml_attribute_value(relationship, "Type",
                                                 options.relationship_type) ||
                !checked_set_xml_attribute_value(relationship, "Target",
                                                 options.relationship_target)) {
                return set_xml_document_mutation_allocation_failure(
                    error_info, options.relationships_entry_name);
            }
            candidate.relationships_changed = true;
        }

        allocation_failure_entry = options.content_types_entry_name;
        auto override_node =
            singleton_part_attachment_detail::find_override_by_part_name(
                types, options.override_part_name);
        if (override_node == pugi::xml_node{}) {
            override_node = append_package_content_type_override(types);
            candidate.content_types_changed = true;
        }
        if (override_node == pugi::xml_node{} ||
            !xml_element_has_expanded_name(
                override_node, "Override",
                singleton_part_attachment_detail::
                    package_content_types_namespace_uri)) {
            return set_xml_document_mutation_allocation_failure(
                error_info, options.content_types_entry_name);
        }

        const auto part_name_changed =
            std::string_view{override_node.attribute("PartName").value()} !=
            options.override_part_name;
        const auto content_type_changed =
            std::string_view{override_node.attribute("ContentType").value()} !=
            options.content_type;
        if (!checked_set_xml_attribute_value(override_node, "PartName",
                                             options.override_part_name) ||
            !checked_set_xml_attribute_value(override_node, "ContentType",
                                             options.content_type)) {
            return set_xml_document_mutation_allocation_failure(
                error_info, options.content_types_entry_name);
        }
        candidate.content_types_changed = candidate.content_types_changed ||
                                          part_name_changed ||
                                          content_type_changed;

        static_assert(
            std::is_nothrow_move_assignable_v<singleton_part_attachment_work>);
        work = std::move(candidate);
        return {};
    } catch (...) {
        return set_xml_document_mutation_allocation_failure(
            error_info, allocation_failure_entry);
    }
}

// Compose multiple singleton-part attachments against one logical work state.
// Each step clones the result of the previous step, and the changed flags are
// accumulated so a later no-op attachment cannot hide an earlier edit.
[[nodiscard]] inline auto prepare_checked_singleton_part_attachments(
    const pugi::xml_document &source_relationships,
    const pugi::xml_document &source_content_types,
    std::span<const singleton_part_attachment_options> options,
    document_error_info &error_info, singleton_part_attachment_work &work)
    -> std::error_code {
    if (options.empty()) {
        return singleton_part_attachment_detail::set_error(
            error_info, std::make_error_code(std::errc::invalid_argument),
            "expected at least one singleton part attachment", {});
    }

    auto accumulated = singleton_part_attachment_work{};
    bool has_accumulated_work = false;
    for (const auto &option : options) {
        auto candidate = singleton_part_attachment_work{};
        const auto &relationships_source =
            has_accumulated_work ? accumulated.relationships
                                 : source_relationships;
        const auto &content_types_source =
            has_accumulated_work ? accumulated.content_types
                                 : source_content_types;
        if (const auto error = prepare_checked_singleton_part_attachment(
                relationships_source, content_types_source, option, error_info,
                candidate)) {
            return error;
        }

        if (has_accumulated_work) {
            candidate.relationships_changed =
                candidate.relationships_changed ||
                accumulated.relationships_changed;
            candidate.content_types_changed =
                candidate.content_types_changed ||
                accumulated.content_types_changed;
        }
        accumulated = std::move(candidate);
        has_accumulated_work = true;
    }

    static_assert(
        std::is_nothrow_move_assignable_v<singleton_part_attachment_work>);
    work = std::move(accumulated);
    return {};
}

inline void publish_checked_singleton_part_attachment(
    singleton_part_attachment_work &&work,
    pugi::xml_document &live_relationships,
    pugi::xml_document &live_content_types, bool &has_relationships_part,
    bool &relationships_dirty, bool &content_types_dirty) noexcept {
    static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
    if (work.relationships_changed) {
        live_relationships = std::move(work.relationships);
        relationships_dirty = true;
    }
    if (work.content_types_changed) {
        live_content_types = std::move(work.content_types);
        content_types_dirty = true;
    }
    has_relationships_part = true;
}

} // namespace featherdoc::detail
