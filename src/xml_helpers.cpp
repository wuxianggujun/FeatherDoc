#include "xml_helpers.hpp"

#include <cctype>
#include <cstring>
#include <string>

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

auto node_has_attributes(pugi::xml_node node) -> bool {
    return node.first_attribute() != pugi::xml_attribute{};
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

pugi::xml_node ensure_run_properties_node(pugi::xml_node run) {
    if (run == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = run.child("w:rPr");
    if (run_properties != pugi::xml_node{}) {
        return run_properties;
    }

    if (const auto first_child = run.first_child(); first_child != pugi::xml_node{}) {
        return run.insert_child_before("w:rPr", first_child);
    }

    return run.append_child("w:rPr");
}

void remove_empty_run_properties(pugi::xml_node run) {
    if (run == pugi::xml_node{}) {
        return;
    }

    auto run_properties = run.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    if (run_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(run_properties)) {
        run.remove_child(run_properties);
    }
}

pugi::xml_node insert_paragraph_node(pugi::xml_node parent, pugi::xml_node insert_before) {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    if (insert_before != pugi::xml_node{}) {
        if (insert_before.parent() != parent) {
            return {};
        }
        return parent.insert_child_before("w:p", insert_before);
    }

    if (std::string_view{parent.name()} == "w:body") {
        if (const auto section_properties = parent.child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            return parent.insert_child_before("w:p", section_properties);
        }
    }

    return parent.append_child("w:p");
}

pugi::xml_node append_paragraph_node(pugi::xml_node parent) {
    return insert_paragraph_node(parent, {});
}

bool insert_plain_text_paragraph(pugi::xml_node parent, pugi::xml_node insert_before,
                                 std::string_view text) {
    auto paragraph = insert_paragraph_node(parent, insert_before);
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    if (text.empty()) {
        return true;
    }

    auto run = paragraph.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    auto text_node = run.append_child("w:t");
    if (text_node == pugi::xml_node{}) {
        return false;
    }

    const std::string paragraph_text{text};
    update_xml_space_attribute(text_node, paragraph_text.c_str());
    return text_node.text().set(paragraph_text.c_str());
}

bool append_plain_text_paragraph(pugi::xml_node parent, std::string_view text) {
    return insert_plain_text_paragraph(parent, {}, text);
}

pugi::xml_node append_table_node(pugi::xml_node parent) {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    if (std::string_view{parent.name()} == "w:body") {
        if (const auto section_properties = parent.child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            return parent.insert_child_before("w:tbl", section_properties);
        }
    }

    return parent.append_child("w:tbl");
}

} // namespace featherdoc::detail
