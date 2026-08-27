#ifndef FEATHERDOC_PACKAGE_RELATIONSHIPS_XML_HELPERS_HPP
#define FEATHERDOC_PACKAGE_RELATIONSHIPS_XML_HELPERS_HPP

#include <cstdint>

#include <pugixml.hpp>

namespace featherdoc::detail {

enum class package_relationships_document_state : std::uint8_t {
    missing = 0U,
    valid,
    invalid,
};

[[nodiscard]] pugi::xml_node
package_relationships_root(const pugi::xml_document &relationships_document);
[[nodiscard]] pugi::xml_node
first_package_relationship(pugi::xml_node relationships_root);
[[nodiscard]] pugi::xml_node
next_package_relationship(pugi::xml_node relationship);
[[nodiscard]] pugi::xml_node
append_package_relationship(pugi::xml_node relationships_root);
[[nodiscard]] package_relationships_document_state
inspect_package_relationships_document(
    const pugi::xml_document &relationships_document,
    bool reject_duplicate_relationship_ids = true);
[[nodiscard]] package_relationships_document_state
inspect_package_relationships_document_structure(
    const pugi::xml_document &relationships_document);
[[nodiscard]] bool package_relationships_document_allows_mutation(
    const pugi::xml_document &relationships_document,
    bool has_relationships_part);
[[nodiscard]] bool package_relationships_document_is_invalid(
    const pugi::xml_document &relationships_document);

} // namespace featherdoc::detail

#endif
