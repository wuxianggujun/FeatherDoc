#include "document_section_xml_helpers.hpp"

#include "xml_helpers.hpp"

#include <cstddef>
#include <string>
#include <utility>

namespace featherdoc::detail {

auto find_section_reference(pugi::xml_node section_properties, const char *reference_name,
                            std::string_view xml_reference_type) -> pugi::xml_node {
    for (auto reference = section_properties.child(reference_name);
         reference != pugi::xml_node{};
         reference = reference.next_sibling(reference_name)) {
        if (std::string_view{reference.attribute("w:type").value()} == xml_reference_type) {
            return reference;
        }
    }

    return {};
}

auto read_on_off_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return true;
    }

    const auto value = std::string_view{attribute.value()};
    return value != "0" && value != "false" && value != "off";
}

auto append_section_reference(pugi::xml_node section_properties, const char *reference_name)
    -> pugi::xml_node {
    const auto reference_name_view = std::string_view{reference_name};
    auto insertion_anchor = pugi::xml_node{};

    for (auto child = section_properties.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};

        if (reference_name_view == "w:headerReference") {
            if (child_name == "w:footerReference") {
                insertion_anchor = child;
                break;
            }

            if (child_name != "w:headerReference") {
                insertion_anchor = child;
                break;
            }

            continue;
        }

        if (child_name != "w:headerReference" && child_name != "w:footerReference") {
            insertion_anchor = child;
            break;
        }
    }

    return insertion_anchor == pugi::xml_node{}
               ? section_properties.append_child(reference_name)
               : section_properties.insert_child_before(reference_name, insertion_anchor);
}

void clear_section_header_footer_references(pugi::xml_node section_properties) {
    if (section_properties == pugi::xml_node{}) {
        return;
    }

    for (auto reference = section_properties.child("w:headerReference");
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling("w:headerReference");
        section_properties.remove_child(reference);
        reference = next;
    }

    for (auto reference = section_properties.child("w:footerReference");
         reference != pugi::xml_node{};) {
        const auto next = reference.next_sibling("w:footerReference");
        section_properties.remove_child(reference);
        reference = next;
    }

    section_properties.remove_child("w:titlePg");
}

section_body_snapshot::section_body_snapshot() {
    this->root = this->xml.append_child("section");
    this->content_root = this->root.append_child("content");
    this->properties_root = this->root.append_child("properties");
}

auto section_body_snapshot::section_properties() const -> pugi::xml_node {
    return this->properties_root.child("w:sectPr");
}

auto node_has_attributes(pugi::xml_node node) -> bool {
    return node.first_attribute() != pugi::xml_attribute{};
}

void remove_empty_paragraph_properties(pugi::xml_node paragraph) {
    if (paragraph == pugi::xml_node{}) {
        return;
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return;
    }

    if (paragraph_properties.first_child() == pugi::xml_node{} &&
        !node_has_attributes(paragraph_properties)) {
        paragraph.remove_child(paragraph_properties);
    }
}

void remove_empty_paragraph(pugi::xml_node paragraph) {
    if (paragraph == pugi::xml_node{}) {
        return;
    }

    remove_empty_paragraph_properties(paragraph);
    if (paragraph.first_child() != pugi::xml_node{} || node_has_attributes(paragraph)) {
        return;
    }

    if (auto parent = paragraph.parent(); parent != pugi::xml_node{}) {
        parent.remove_child(paragraph);
    }
}

auto ensure_on_off_node_enabled(pugi::xml_node parent, const char *child_name)
    -> pugi::xml_node {
    auto child = parent.child(child_name);
    if (child == pugi::xml_node{}) {
        child = parent.append_child(child_name);
    }
    if (child == pugi::xml_node{}) {
        return {};
    }

    auto value_attribute = child.attribute("w:val");
    if (value_attribute == pugi::xml_attribute{}) {
        value_attribute = child.append_attribute("w:val");
    }
    value_attribute.set_value("1");
    return child;
}

auto ensure_section_title_page_node(pugi::xml_node section_properties) -> pugi::xml_node {
    auto title_page = section_properties.child("w:titlePg");
    if (title_page == pugi::xml_node{}) {
        auto insertion_anchor = pugi::xml_node{};
        for (auto child = section_properties.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            const auto child_name = std::string_view{child.name()};
            if (child_name == "w:textDirection" || child_name == "w:bidi" ||
                child_name == "w:rtlGutter" || child_name == "w:docGrid" ||
                child_name == "w:printerSettings" || child_name == "w:sectPrChange") {
                insertion_anchor = child;
                break;
            }
        }

        title_page = insertion_anchor == pugi::xml_node{}
                         ? section_properties.append_child("w:titlePg")
                         : section_properties.insert_child_before("w:titlePg",
                                                                  insertion_anchor);
    }

    if (title_page == pugi::xml_node{}) {
        return {};
    }

    auto value_attribute = title_page.attribute("w:val");
    if (value_attribute == pugi::xml_attribute{}) {
        value_attribute = title_page.append_attribute("w:val");
    }
    value_attribute.set_value("1");
    return title_page;
}

auto append_section_property_node(pugi::xml_node section_properties, const char *child_name)
    -> pugi::xml_node {
    const auto child_name_view = std::string_view{child_name};
    auto insertion_anchor = pugi::xml_node{};

    auto should_insert_before = [child_name_view](std::string_view existing_name) {
        if (child_name_view == "w:pgSz") {
            return existing_name == "w:pgMar" || existing_name == "w:paperSrc" ||
                   existing_name == "w:pgBorders" || existing_name == "w:lnNumType" ||
                   existing_name == "w:pgNumType" || existing_name == "w:cols" ||
                   existing_name == "w:formProt" || existing_name == "w:vAlign" ||
                   existing_name == "w:noEndnote" || existing_name == "w:titlePg" ||
                   existing_name == "w:textDirection" || existing_name == "w:bidi" ||
                   existing_name == "w:rtlGutter" || existing_name == "w:docGrid" ||
                   existing_name == "w:printerSettings" ||
                   existing_name == "w:sectPrChange";
        }

        if (child_name_view == "w:pgMar") {
            return existing_name == "w:paperSrc" || existing_name == "w:pgBorders" ||
                   existing_name == "w:lnNumType" || existing_name == "w:pgNumType" ||
                   existing_name == "w:cols" || existing_name == "w:formProt" ||
                   existing_name == "w:vAlign" || existing_name == "w:noEndnote" ||
                   existing_name == "w:titlePg" ||
                   existing_name == "w:textDirection" || existing_name == "w:bidi" ||
                   existing_name == "w:rtlGutter" || existing_name == "w:docGrid" ||
                   existing_name == "w:printerSettings" ||
                   existing_name == "w:sectPrChange";
        }

        if (child_name_view == "w:pgNumType") {
            return existing_name == "w:cols" || existing_name == "w:formProt" ||
                   existing_name == "w:vAlign" || existing_name == "w:noEndnote" ||
                   existing_name == "w:titlePg" ||
                   existing_name == "w:textDirection" || existing_name == "w:bidi" ||
                   existing_name == "w:rtlGutter" || existing_name == "w:docGrid" ||
                   existing_name == "w:printerSettings" ||
                   existing_name == "w:sectPrChange";
        }

        return false;
    };

    for (auto child = section_properties.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (should_insert_before(std::string_view{child.name()})) {
            insertion_anchor = child;
            break;
        }
    }

    return insertion_anchor == pugi::xml_node{}
               ? section_properties.append_child(child_name)
               : section_properties.insert_child_before(child_name, insertion_anchor);
}

auto ensure_section_property_node(pugi::xml_node section_properties, const char *child_name)
    -> pugi::xml_node {
    auto child = section_properties.child(child_name);
    if (child != pugi::xml_node{}) {
        return child;
    }

    return append_section_property_node(section_properties, child_name);
}

auto ensure_xml_uint32_attribute(pugi::xml_node node, const char *attribute_name,
                                 std::uint32_t value) -> bool {
    if (node == pugi::xml_node{}) {
        return false;
    }

    auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(attribute_name);
    }
    if (attribute == pugi::xml_attribute{}) {
        return false;
    }

    const auto text = std::to_string(value);
    return attribute.set_value(text.c_str());
}

void remove_empty_node(pugi::xml_node node) {
    if (node == pugi::xml_node{}) {
        return;
    }

    if (node.first_child() != pugi::xml_node{} || node_has_attributes(node)) {
        return;
    }

    if (auto parent = node.parent(); parent != pugi::xml_node{}) {
        parent.remove_child(node);
    }
}

auto section_break_paragraph_for(pugi::xml_node section_properties) -> pugi::xml_node {
    if (section_properties == pugi::xml_node{}) {
        return {};
    }

    const auto paragraph_properties = section_properties.parent();
    if (paragraph_properties == pugi::xml_node{} ||
        std::string_view{paragraph_properties.name()} != "w:pPr") {
        return {};
    }

    const auto paragraph = paragraph_properties.parent();
    if (paragraph == pugi::xml_node{} || std::string_view{paragraph.name()} != "w:p") {
        return {};
    }

    return paragraph;
}

auto replace_section_properties_contents(pugi::xml_node target_section_properties,
                                         pugi::xml_node source_section_properties) -> bool {
    if (target_section_properties == pugi::xml_node{} ||
        source_section_properties == pugi::xml_node{}) {
        return false;
    }

    for (auto attribute = target_section_properties.first_attribute();
         attribute != pugi::xml_attribute{};) {
        const auto next = attribute.next_attribute();
        target_section_properties.remove_attribute(attribute);
        attribute = next;
    }

    for (auto child = target_section_properties.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        target_section_properties.remove_child(child);
        child = next;
    }

    for (auto attribute = source_section_properties.first_attribute();
         attribute != pugi::xml_attribute{}; attribute = attribute.next_attribute()) {
        auto copied_attribute = target_section_properties.append_attribute(attribute.name());
        if (copied_attribute == pugi::xml_attribute{}) {
            return false;
        }
        copied_attribute.set_value(attribute.value());
    }

    for (auto child = source_section_properties.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (target_section_properties.append_copy(child) == pugi::xml_node{}) {
            return false;
        }
    }

    return true;
}

auto ensure_paragraph_properties_node(pugi::xml_node paragraph) -> pugi::xml_node {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    const auto first_child = paragraph.first_child();
    return first_child == pugi::xml_node{}
               ? paragraph.append_child("w:pPr")
               : paragraph.insert_child_before("w:pPr", first_child);
}

auto capture_section_snapshot(pugi::xml_node first_child, pugi::xml_node end_exclusive,
                              pugi::xml_node section_properties,
                              bool strip_last_paragraph_section_properties)
    -> std::unique_ptr<section_body_snapshot> {
    auto snapshot = std::make_unique<section_body_snapshot>();
    if (snapshot->root == pugi::xml_node{} || snapshot->content_root == pugi::xml_node{} ||
        snapshot->properties_root == pugi::xml_node{}) {
        return nullptr;
    }

    for (auto child = first_child; child != pugi::xml_node{} && child != end_exclusive;
         child = child.next_sibling()) {
        if (snapshot->content_root.append_copy(child) == pugi::xml_node{}) {
            return nullptr;
        }
    }

    if (section_properties != pugi::xml_node{}) {
        if (snapshot->properties_root.append_copy(section_properties) == pugi::xml_node{}) {
            return nullptr;
        }
    } else if (snapshot->properties_root.append_child("w:sectPr") == pugi::xml_node{}) {
        return nullptr;
    }

    if (strip_last_paragraph_section_properties) {
        auto last_copied_child = snapshot->content_root.last_child();
        if (last_copied_child != pugi::xml_node{} &&
            std::string_view{last_copied_child.name()} == "w:p") {
            if (auto paragraph_properties = last_copied_child.child("w:pPr");
                paragraph_properties != pugi::xml_node{}) {
                paragraph_properties.remove_child("w:sectPr");
                remove_empty_paragraph_properties(last_copied_child);
            }
        }
    }

    return snapshot;
}

auto collect_section_snapshots(pugi::xml_node body)
    -> std::optional<std::vector<std::unique_ptr<section_body_snapshot>>> {
    if (body == pugi::xml_node{}) {
        return std::nullopt;
    }

    std::vector<std::unique_ptr<section_body_snapshot>> snapshots;
    auto body_section_properties = body.child("w:sectPr");
    auto current_section_first_child = body.first_child();

    for (auto child = body.first_child();
         child != pugi::xml_node{} && child != body_section_properties;
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        auto paragraph_section_properties = child.child("w:pPr").child("w:sectPr");
        if (paragraph_section_properties == pugi::xml_node{}) {
            continue;
        }

        auto snapshot = capture_section_snapshot(current_section_first_child,
                                                 child.next_sibling(),
                                                 paragraph_section_properties, true);
        if (snapshot == nullptr) {
            return std::nullopt;
        }

        snapshots.push_back(std::move(snapshot));
        current_section_first_child = child.next_sibling();
    }

    auto final_snapshot = capture_section_snapshot(current_section_first_child,
                                                   body_section_properties,
                                                   body_section_properties, false);
    if (final_snapshot == nullptr) {
        return std::nullopt;
    }

    snapshots.push_back(std::move(final_snapshot));
    return snapshots;
}

auto rebuild_body_from_section_snapshots(
    pugi::xml_node body, const std::vector<std::unique_ptr<section_body_snapshot>> &snapshots)
    -> bool {
    if (body == pugi::xml_node{}) {
        return false;
    }

    for (auto child = body.first_child(); child != pugi::xml_node{};) {
        const auto next = child.next_sibling();
        body.remove_child(child);
        child = next;
    }

    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto &snapshot = snapshots[index];
        auto last_content_node = pugi::xml_node{};
        for (auto child = snapshot->content_root.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            last_content_node = body.append_copy(child);
            if (last_content_node == pugi::xml_node{}) {
                return false;
            }
        }

        const auto section_properties = snapshot->section_properties();
        if (index + 1U < snapshots.size()) {
            auto host_paragraph = pugi::xml_node{};
            if (last_content_node != pugi::xml_node{} &&
                std::string_view{last_content_node.name()} == "w:p") {
                host_paragraph = last_content_node;
            } else {
                host_paragraph = append_paragraph_node(body);
            }

            if (host_paragraph == pugi::xml_node{}) {
                return false;
            }

            auto paragraph_properties = ensure_paragraph_properties_node(host_paragraph);
            if (paragraph_properties == pugi::xml_node{}) {
                return false;
            }

            if (paragraph_properties.append_copy(section_properties) == pugi::xml_node{}) {
                return false;
            }
        } else if (body.append_copy(section_properties) == pugi::xml_node{}) {
            return false;
        }
    }

    return true;
}

auto section_has_reference_type(pugi::xml_node section_properties, const char *reference_name,
                                std::string_view xml_reference_type) -> bool {
    return find_section_reference(section_properties, reference_name, xml_reference_type) !=
           pugi::xml_node{};
}

auto document_has_reference_type(const pugi::xml_document &document, const char *reference_name,
                                 std::string_view xml_reference_type) -> bool {
    const auto body = document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return false;
    }

    const auto section_has_reference = [&](pugi::xml_node section_properties) {
        return section_has_reference_type(section_properties, reference_name, xml_reference_type);
    };

    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (const auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{} && section_has_reference(section_properties)) {
            return true;
        }
    }

    if (const auto section_properties = body.child("w:sectPr");
        section_properties != pugi::xml_node{} && section_has_reference(section_properties)) {
        return true;
    }

    return false;
}

} // namespace featherdoc::detail
