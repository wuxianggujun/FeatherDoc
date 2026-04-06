#ifndef FEATHERDOC_XML_HELPERS_HPP
#define FEATHERDOC_XML_HELPERS_HPP

#include <string_view>

#include <pugixml.hpp>

namespace featherdoc::detail {

void update_xml_space_attribute(pugi::xml_node text_node, const char *text);

[[nodiscard]] pugi::xml_node next_named_sibling(pugi::xml_node node,
                                                std::string_view node_name);

[[nodiscard]] pugi::xml_node append_paragraph_node(pugi::xml_node parent);
[[nodiscard]] pugi::xml_node append_table_node(pugi::xml_node parent);

} // namespace featherdoc::detail

#endif
