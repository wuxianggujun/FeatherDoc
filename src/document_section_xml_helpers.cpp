#include "document_section_xml_helpers.hpp"

#include "xml_document_clone_helpers.hpp"
#include "xml_helpers.hpp"

#include <cstddef>
#include <string>
#include <utility>

namespace featherdoc::detail {

namespace {

struct section_document_scratch_guard final {
    pugi::xml_node parent;
    pugi::xml_node scratch;

    section_document_scratch_guard(pugi::xml_node parent_node,
                                   pugi::xml_node scratch_node)
        : parent(parent_node), scratch(scratch_node) {}
    section_document_scratch_guard(const section_document_scratch_guard &) =
        delete;
    auto operator=(const section_document_scratch_guard &)
        -> section_document_scratch_guard & = delete;

    ~section_document_scratch_guard() {
        if (parent != pugi::xml_node{} && scratch != pugi::xml_node{}) {
            (void)parent.remove_child(scratch);
        }
    }
};

struct section_namespace_attribute_rollback final {
    pugi::xml_node root;
    pugi::xml_attribute attribute;
    bool active{false};

    section_namespace_attribute_rollback() = default;
    section_namespace_attribute_rollback(
        const section_namespace_attribute_rollback &) = delete;
    auto operator=(const section_namespace_attribute_rollback &)
        -> section_namespace_attribute_rollback & = delete;

    ~section_namespace_attribute_rollback() {
        if (active && root != pugi::xml_node{} &&
            attribute != pugi::xml_attribute{}) {
            (void)root.remove_attribute(attribute);
        }
    }

    void commit() noexcept { active = false; }
};

} // namespace

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

auto section_properties_at(pugi::xml_node body, std::size_t section_index)
    -> pugi::xml_node {
    if (body == pugi::xml_node{}) {
        return {};
    }

    std::size_t current_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        const auto paragraph_section_properties =
            child.child("w:pPr").child("w:sectPr");
        if (paragraph_section_properties == pugi::xml_node{}) {
            continue;
        }

        if (current_index == section_index) {
            return paragraph_section_properties;
        }
        ++current_index;
    }

    return current_index == section_index ? body.child("w:sectPr")
                                           : pugi::xml_node{};
}

auto publish_checked_section_document_update(
    pugi::xml_document &live_document,
    const pugi::xml_document &prepared_document, std::size_t section_index,
    bool document_changed) -> section_document_publish_result {
    if (!document_changed) {
        return section_document_publish_result::success;
    }

    auto live_root = live_document.child("w:document");
    auto live_body = live_root.child("w:body");
    const auto prepared_root = prepared_document.child("w:document");
    const auto prepared_body = prepared_root.child("w:body");
    const auto prepared_section =
        section_properties_at(prepared_body, section_index);
    if (live_root == pugi::xml_node{} || live_body == pugi::xml_node{} ||
        prepared_root == pugi::xml_node{} ||
        prepared_section == pugi::xml_node{}) {
        return section_document_publish_result::failure;
    }

    // A missing relationship namespace is the normal create_empty() case. It
    // can be appended with a rollback guard, preserving all body/paragraph
    // handles while retaining the strong failure guarantee. An already
    // present but different namespace cannot be changed atomically in place,
    // so only that malformed/legacy case requires publishing the full checked
    // document.
    auto namespace_rollback = section_namespace_attribute_rollback{};
    if (const auto prepared_namespace = prepared_root.attribute("xmlns:r");
        prepared_namespace != pugi::xml_attribute{}) {
        const auto live_namespace = live_root.attribute("xmlns:r");
        if (live_namespace == pugi::xml_attribute{}) {
            if (!checked_append_xml_attribute(live_root, "xmlns:r",
                                              prepared_namespace.value())) {
                return section_document_publish_result::failure;
            }
            namespace_rollback.root = live_root;
            namespace_rollback.attribute = live_root.attribute("xmlns:r");
            namespace_rollback.active = true;
        } else if (std::string_view{live_namespace.value()} !=
                   prepared_namespace.value()) {
            return section_document_publish_result::
                requires_full_document_publish;
        }
    }

    auto scratch = checked_append_xml_element(live_document,
                                              "featherdoc-transaction");
    if (scratch == pugi::xml_node{}) {
        return section_document_publish_result::failure;
    }
    const auto scratch_guard =
        section_document_scratch_guard{live_document, scratch};
    if (checked_append_copy_xml_node(prepared_section, scratch) !=
        xml_document_clone_status::success) {
        return section_document_publish_result::failure;
    }
    auto prepared_live_section = scratch.first_child();
    if (prepared_live_section == pugi::xml_node{}) {
        return section_document_publish_result::failure;
    }

    const auto live_section =
        section_properties_at(live_body, section_index);
    auto published_section = pugi::xml_node{};
    if (live_section == pugi::xml_node{}) {
        published_section = live_body.append_move(prepared_live_section);
    } else {
        auto live_parent = live_section.parent();
        published_section =
            live_parent.insert_move_before(prepared_live_section, live_section);
        if (published_section != pugi::xml_node{}) {
            (void)live_parent.remove_child(live_section);
        }
    }
    if (published_section == pugi::xml_node{}) {
        return section_document_publish_result::failure;
    }
    namespace_rollback.commit();
    return section_document_publish_result::success;
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

}

section_body_snapshot::section_body_snapshot() {
    this->root = checked_append_xml_element(this->xml, "section");
    this->content_root = checked_append_xml_element(this->root, "content");
    this->properties_root =
        checked_append_xml_element(this->root, "properties");
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
               ? checked_append_xml_element(section_properties, child_name)
               : checked_insert_xml_element_before(
                     section_properties, child_name, insertion_anchor);
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

    const auto text = std::to_string(value);
    return checked_set_xml_attribute_value(node, attribute_name, text);
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
               ? checked_append_xml_element(paragraph, "w:pPr")
               : checked_insert_xml_element_before(paragraph, "w:pPr",
                                                   first_child);
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
        if (checked_append_copy_xml_node(child, snapshot->content_root) !=
            xml_document_clone_status::success) {
            return nullptr;
        }
    }

    if (section_properties != pugi::xml_node{}) {
        if (checked_append_copy_xml_node(section_properties,
                                         snapshot->properties_root) !=
            xml_document_clone_status::success) {
            return nullptr;
        }
    } else if (checked_append_xml_element(snapshot->properties_root,
                                          "w:sectPr") == pugi::xml_node{}) {
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
            if (checked_append_copy_xml_node(child, body) !=
                xml_document_clone_status::success) {
                return false;
            }
            last_content_node = body.last_child();
        }

        const auto section_properties = snapshot->section_properties();
        if (index + 1U < snapshots.size()) {
            auto host_paragraph = pugi::xml_node{};
            if (last_content_node != pugi::xml_node{} &&
                std::string_view{last_content_node.name()} == "w:p") {
                host_paragraph = last_content_node;
            } else {
                host_paragraph = checked_append_xml_element(body, "w:p");
            }

            if (host_paragraph == pugi::xml_node{}) {
                return false;
            }

            auto paragraph_properties = ensure_paragraph_properties_node(host_paragraph);
            if (paragraph_properties == pugi::xml_node{}) {
                return false;
            }

            if (checked_append_copy_xml_node(section_properties,
                                             paragraph_properties) !=
                xml_document_clone_status::success) {
                return false;
            }
        } else if (checked_append_copy_xml_node(section_properties, body) !=
                   xml_document_clone_status::success) {
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

} // namespace featherdoc::detail
