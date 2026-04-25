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

auto append_plain_text_node(pugi::xml_node run, std::string_view text) -> bool {
    auto text_node = run.append_child("w:t");
    if (text_node == pugi::xml_node{}) {
        return false;
    }

    const std::string text_buffer{text};
    update_xml_space_attribute(text_node, text_buffer.c_str());
    return text_node.text().set(text_buffer.c_str());
}

auto append_plain_text_run_content(pugi::xml_node run, std::string_view text) -> bool {
    if (run == pugi::xml_node{}) {
        return false;
    }

    if (text.empty()) {
        return append_plain_text_node(run, {});
    }

    std::size_t segment_start = 0U;
    std::size_t offset = 0U;
    while (offset < text.size()) {
        const auto current = text[offset];
        if (current != '\n' && current != '\r') {
            ++offset;
            continue;
        }

        if (offset > segment_start &&
            !append_plain_text_node(run, text.substr(segment_start, offset - segment_start))) {
            return false;
        }

        if (run.append_child("w:br") == pugi::xml_node{}) {
            return false;
        }

        if (current == '\r' && offset + 1U < text.size() && text[offset + 1U] == '\n') {
            ++offset;
        }

        ++offset;
        segment_start = offset;
    }

    if (segment_start < text.size()) {
        return append_plain_text_node(run, text.substr(segment_start));
    }

    return true;
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

void append_plain_text_from_xml(std::string &text, pugi::xml_node node) {
    if (node == pugi::xml_node{}) {
        return;
    }

    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:t" || name == "w:delText") {
            text += child.text().get();
            continue;
        }

        if (name == "w:tab") {
            text.push_back('\t');
            continue;
        }

        if (name == "w:br" || name == "w:cr") {
            text.push_back('\n');
            continue;
        }

        append_plain_text_from_xml(text, child);
    }
}

std::string collect_plain_text_from_xml(pugi::xml_node node) {
    std::string text;
    append_plain_text_from_xml(text, node);
    return text;
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

pugi::xml_node previous_named_sibling(pugi::xml_node node, std::string_view node_name) {
    for (auto sibling = node.previous_sibling(); sibling != pugi::xml_node{};
         sibling = sibling.previous_sibling()) {
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

bool set_plain_text_run_content(pugi::xml_node run, std::string_view text) {
    if (run == pugi::xml_node{}) {
        return false;
    }

    for (auto child = run.first_child(); child != pugi::xml_node{};) {
        const auto next_child = child.next_sibling();
        if (std::string_view{child.name()} != "w:rPr") {
            run.remove_child(child);
        }
        child = next_child;
    }

    return append_plain_text_run_content(run, text);
}

bool append_plain_text_run(pugi::xml_node parent, std::string_view text) {
    auto run = parent.append_child("w:r");
    if (run == pugi::xml_node{}) {
        return false;
    }

    return set_plain_text_run_content(run, text);
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

    return append_plain_text_run(paragraph, text);
}

bool append_plain_text_paragraph(pugi::xml_node parent, std::string_view text) {
    return insert_plain_text_paragraph(parent, {}, text);
}

pugi::xml_node insert_table_node(pugi::xml_node parent, pugi::xml_node insert_before) {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    if (insert_before != pugi::xml_node{}) {
        if (insert_before.parent() != parent) {
            return {};
        }
        return parent.insert_child_before("w:tbl", insert_before);
    }

    if (std::string_view{parent.name()} == "w:body") {
        if (const auto section_properties = parent.child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            return parent.insert_child_before("w:tbl", section_properties);
        }
    }

    return parent.append_child("w:tbl");
}

pugi::xml_node append_table_node(pugi::xml_node parent) {
    return insert_table_node(parent, {});
}

std::size_t count_remaining_block_children(pugi::xml_node parent,
                                           pugi::xml_node skipped_child) {
    std::size_t count = 0U;
    const auto parent_name = std::string_view{parent.name()};

    for (auto child = parent.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (child == skipped_child || child.type() != pugi::node_element) {
            continue;
        }

        const auto child_name = std::string_view{child.name()};
        if ((parent_name == "w:body" && child_name == "w:sectPr") ||
            (parent_name == "w:tc" && child_name == "w:tcPr")) {
            continue;
        }

        ++count;
    }

    return count;
}

bool parent_requires_nonempty_block_content(pugi::xml_node parent) {
    const auto parent_name = std::string_view{parent.name()};
    return parent_name == "w:body" || parent_name == "w:hdr" ||
           parent_name == "w:ftr" || parent_name == "w:tc";
}

} // namespace featherdoc::detail
