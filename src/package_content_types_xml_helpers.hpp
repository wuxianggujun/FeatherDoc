#ifndef FEATHERDOC_PACKAGE_CONTENT_TYPES_XML_HELPERS_HPP
#define FEATHERDOC_PACKAGE_CONTENT_TYPES_XML_HELPERS_HPP

#include <cstdint>
#include <string_view>

#include <pugixml.hpp>

namespace featherdoc::detail {

enum class package_content_types_document_state : std::uint8_t {
    missing = 0U,
    valid,
    invalid,
};

struct package_content_types_document_inspection final {
    package_content_types_document_state state{
        package_content_types_document_state::missing};
    std::string_view detail;
};

[[nodiscard]] pugi::xml_node
package_content_types_root(const pugi::xml_document &content_types_document);
[[nodiscard]] pugi::xml_node
first_package_content_type_default(pugi::xml_node content_types_root);
[[nodiscard]] pugi::xml_node
next_package_content_type_default(pugi::xml_node default_node);
[[nodiscard]] pugi::xml_node
first_package_content_type_override(pugi::xml_node content_types_root);
[[nodiscard]] pugi::xml_node
next_package_content_type_override(pugi::xml_node override_node);
[[nodiscard]] pugi::xml_node
append_package_content_type_default(pugi::xml_node content_types_root);
[[nodiscard]] pugi::xml_node
append_package_content_type_override(pugi::xml_node content_types_root);
[[nodiscard]] package_content_types_document_inspection
inspect_package_content_types_document_structure(
    const pugi::xml_document &content_types_document);
[[nodiscard]] bool package_content_types_document_allows_mutation(
    const pugi::xml_document &content_types_document);

} // namespace featherdoc::detail

#endif
