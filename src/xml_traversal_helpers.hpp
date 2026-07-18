#pragma once

#include <pugixml.hpp>

namespace featherdoc::detail {

[[nodiscard]] pugi::xml_node
next_xml_node_preorder(pugi::xml_node root, pugi::xml_node current,
                       bool descend_into_children = true);

} // namespace featherdoc::detail
