#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include <pugixml.hpp>

namespace featherdoc::detail {

struct section_body_snapshot final {
    pugi::xml_document xml;
    pugi::xml_node root;
    pugi::xml_node content_root;
    pugi::xml_node properties_root;

    section_body_snapshot();

    [[nodiscard]] auto section_properties() const -> pugi::xml_node;
};

[[nodiscard]] auto find_section_reference(pugi::xml_node section_properties,
                                          const char *reference_name,
                                          std::string_view xml_reference_type)
    -> pugi::xml_node;
[[nodiscard]] auto section_properties_at(pugi::xml_node body,
                                         std::size_t section_index)
    -> pugi::xml_node;
enum class section_document_publish_result {
    success,
    requires_full_document_publish,
    failure,
};
[[nodiscard]] auto publish_checked_section_document_update(
    pugi::xml_document &live_document,
    const pugi::xml_document &prepared_document, std::size_t section_index,
    bool document_changed) -> section_document_publish_result;
[[nodiscard]] auto read_on_off_value(pugi::xml_node node) -> std::optional<bool>;
void clear_section_header_footer_references(pugi::xml_node section_properties);
void remove_empty_paragraph(pugi::xml_node paragraph);
[[nodiscard]] auto ensure_section_property_node(pugi::xml_node section_properties,
                                                const char *child_name)
    -> pugi::xml_node;
[[nodiscard]] auto ensure_xml_uint32_attribute(pugi::xml_node node,
                                               const char *attribute_name,
                                               std::uint32_t value) -> bool;
void remove_empty_node(pugi::xml_node node);
[[nodiscard]] auto section_break_paragraph_for(pugi::xml_node section_properties)
    -> pugi::xml_node;
[[nodiscard]] auto replace_section_properties_contents(
    pugi::xml_node target_section_properties,
    pugi::xml_node source_section_properties) -> bool;
[[nodiscard]] auto collect_section_snapshots(pugi::xml_node body)
    -> std::optional<std::vector<std::unique_ptr<section_body_snapshot>>>;
[[nodiscard]] auto rebuild_body_from_section_snapshots(
    pugi::xml_node body,
    const std::vector<std::unique_ptr<section_body_snapshot>> &snapshots)
    -> bool;
[[nodiscard]] auto section_has_reference_type(pugi::xml_node section_properties,
                                              const char *reference_name,
                                              std::string_view xml_reference_type)
    -> bool;

} // namespace featherdoc::detail
