#include "xml_helpers.hpp"

#include <cctype>
#include <cstring>

namespace featherdoc::detail {
namespace {

auto should_preserve_xml_space(const char *text) -> bool {
    if (text == nullptr || *text == '\0') {
        return false;
    }

    const auto text_length = std::strlen(text);
    return std::isspace(static_cast<unsigned char>(text[0])) != 0 ||
           std::isspace(static_cast<unsigned char>(text[text_length - 1])) != 0;
}

} // namespace

void update_xml_space_attribute(pugi::xml_node text_node, const char *text) {
    if (text_node == pugi::xml_node{}) {
        return;
    }

    if (should_preserve_xml_space(text)) {
        auto xml_space = text_node.attribute("xml:space");
        if (xml_space == pugi::xml_attribute{}) {
            xml_space = text_node.append_attribute("xml:space");
        }
        xml_space.set_value("preserve");
        return;
    }

    text_node.remove_attribute("xml:space");
}

pugi::xml_node next_named_sibling(pugi::xml_node node, std::string_view node_name) {
    for (auto sibling = node.next_sibling(); sibling != pugi::xml_node{};
         sibling = sibling.next_sibling()) {
        if (std::string_view{sibling.name()} == node_name) {
            return sibling;
        }
    }

    return {};
}

pugi::xml_node append_paragraph_node(pugi::xml_node parent) {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    if (std::string_view{parent.name()} == "w:body") {
        if (const auto section_properties = parent.child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            return parent.insert_child_before("w:p", section_properties);
        }
    }

    return parent.append_child("w:p");
}

} // namespace featherdoc::detail
