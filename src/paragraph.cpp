#include "featherdoc.hpp"
#include "xml_helpers.hpp"

namespace featherdoc {
namespace {

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

auto ensure_paragraph_properties_node(pugi::xml_node paragraph) -> pugi::xml_node {
    if (paragraph == pugi::xml_node{}) {
        return {};
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties != pugi::xml_node{}) {
        return paragraph_properties;
    }

    if (const auto first_child = paragraph.first_child(); first_child != pugi::xml_node{}) {
        return paragraph.insert_child_before("w:pPr", first_child);
    }

    return paragraph.append_child("w:pPr");
}

auto ensure_paragraph_bidi_node(pugi::xml_node paragraph_properties) -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto bidi = paragraph_properties.child("w:bidi");
    if (bidi != pugi::xml_node{}) {
        return bidi;
    }

    return paragraph_properties.append_child("w:bidi");
}

void set_on_off_value(pugi::xml_node node, bool enabled) {
    auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute("w:val");
    }
    attribute.set_value(enabled ? "1" : "0");
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
        paragraph_properties.first_attribute() == pugi::xml_attribute{}) {
        paragraph.remove_child(paragraph_properties);
    }
}

auto copy_paragraph_properties_without_section_break(pugi::xml_node source_paragraph,
                                                     pugi::xml_node target_paragraph)
    -> bool {
    if (source_paragraph == pugi::xml_node{} || target_paragraph == pugi::xml_node{}) {
        return false;
    }

    const auto source_properties = source_paragraph.child("w:pPr");
    if (source_properties == pugi::xml_node{}) {
        return true;
    }

    auto copied_properties = target_paragraph.append_copy(source_properties);
    if (copied_properties == pugi::xml_node{}) {
        return false;
    }

    if (const auto section_properties = copied_properties.child("w:sectPr");
        section_properties != pugi::xml_node{}) {
        copied_properties.remove_child(section_properties);
    }

    remove_empty_paragraph_properties(target_paragraph);
    return true;
}

auto insert_paragraph_like_node(pugi::xml_node parent, pugi::xml_node anchor,
                                bool insert_after) -> pugi::xml_node {
    if (parent == pugi::xml_node{} || anchor == pugi::xml_node{} ||
        anchor.parent() != parent) {
        return {};
    }

    const auto insert_before = insert_after ? anchor.next_sibling() : anchor;
    auto inserted_paragraph = detail::insert_paragraph_node(parent, insert_before);
    if (inserted_paragraph == pugi::xml_node{}) {
        return {};
    }

    if (!copy_paragraph_properties_without_section_break(anchor, inserted_paragraph)) {
        parent.remove_child(inserted_paragraph);
        return {};
    }

    return inserted_paragraph;
}

} // namespace

Paragraph::Paragraph() = default;

Paragraph::Paragraph(pugi::xml_node parent, pugi::xml_node current) {
    this->set_parent(parent);
    this->set_current(current);
}

void Paragraph::set_parent(pugi::xml_node node) {
    this->parent = node;
    this->current = this->parent.child("w:p");
    this->run.set_parent(this->current);
}

void Paragraph::set_current(pugi::xml_node node) { this->current = node; }

Paragraph &Paragraph::next() {
    this->current = detail::next_named_sibling(this->current, "w:p");
    this->run.set_parent(this->current);
    return *this;
}

bool Paragraph::has_next() const { return this->current != pugi::xml_node{}; }

std::optional<bool> Paragraph::bidi() const {
    return read_on_off_value(this->current.child("w:pPr").child("w:bidi"));
}

bool Paragraph::set_bidi(bool enabled) const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto paragraph_properties = ensure_paragraph_properties_node(this->current);
    if (paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    const auto bidi = ensure_paragraph_bidi_node(paragraph_properties);
    if (bidi == pugi::xml_node{}) {
        return false;
    }

    set_on_off_value(bidi, enabled);
    return true;
}

bool Paragraph::clear_bidi() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto paragraph_properties = this->current.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return true;
    }

    const auto bidi = paragraph_properties.child("w:bidi");
    if (bidi != pugi::xml_node{}) {
        paragraph_properties.remove_child(bidi);
    }

    remove_empty_paragraph_properties(this->current);
    return true;
}

bool Paragraph::set_text(const std::string &text) const { return this->set_text(text.c_str()); }

bool Paragraph::set_text(const char *text) const {
    if (this->current == pugi::xml_node{} || text == nullptr) {
        return false;
    }

    auto paragraph_node = this->current;
    for (auto child = paragraph_node.first_child(); child != pugi::xml_node{};) {
        const auto next_child = child.next_sibling();
        if (std::string_view{child.name()} != "w:pPr") {
            paragraph_node.remove_child(child);
        }
        child = next_child;
    }

    if (text[0] == '\0') {
        return true;
    }

    Paragraph updated_paragraph(this->parent, this->current);
    return updated_paragraph.add_run(text).has_next();
}

bool Paragraph::remove() {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return false;
    }

    if (this->current.child("w:pPr").child("w:sectPr") != pugi::xml_node{}) {
        return false;
    }

    if (detail::parent_requires_nonempty_block_content(this->parent) &&
        detail::count_remaining_block_children(this->parent, this->current) == 0U) {
        return false;
    }

    const auto next_paragraph = detail::next_named_sibling(this->current, "w:p");
    const auto previous_paragraph = detail::previous_named_sibling(this->current, "w:p");
    if (!this->parent.remove_child(this->current)) {
        return false;
    }

    this->current =
        next_paragraph != pugi::xml_node{} ? next_paragraph : previous_paragraph;
    this->run.set_parent(this->current);
    return true;
}

Run &Paragraph::runs() {
    this->run.set_parent(this->current);
    return this->run;
}

Run Paragraph::add_run(const std::string &text, featherdoc::formatting_flag f) {
    return this->add_run(text.c_str(), f);
}

Run Paragraph::add_run(const char *text, featherdoc::formatting_flag f) {
    if (this->current == pugi::xml_node{} && this->parent != pugi::xml_node{}) {
        this->current = detail::append_paragraph_node(this->parent);
    }

    pugi::xml_node new_run = this->current.append_child("w:r");
    pugi::xml_node meta = new_run.append_child("w:rPr");

    if (featherdoc::has_flag(f, featherdoc::formatting_flag::bold)) {
        meta.append_child("w:b");
    }

    if (featherdoc::has_flag(f, featherdoc::formatting_flag::italic)) {
        meta.append_child("w:i");
    }

    if (featherdoc::has_flag(f, featherdoc::formatting_flag::underline)) {
        meta.append_child("w:u").append_attribute("w:val").set_value("single");
    }

    if (featherdoc::has_flag(f, featherdoc::formatting_flag::strikethrough)) {
        meta.append_child("w:strike").append_attribute("w:val").set_value("true");
    }

    if (featherdoc::has_flag(f, featherdoc::formatting_flag::superscript)) {
        meta.append_child("w:vertAlign")
            .append_attribute("w:val")
            .set_value("superscript");
    } else if (featherdoc::has_flag(f, featherdoc::formatting_flag::subscript)) {
        meta.append_child("w:vertAlign")
            .append_attribute("w:val")
            .set_value("subscript");
    }

    if (featherdoc::has_flag(f, featherdoc::formatting_flag::smallcaps)) {
        meta.append_child("w:smallCaps").append_attribute("w:val").set_value("true");
    }

    if (featherdoc::has_flag(f, featherdoc::formatting_flag::shadow)) {
        meta.append_child("w:shadow").append_attribute("w:val").set_value("true");
    }

    pugi::xml_node new_run_text = new_run.append_child("w:t");
    detail::update_xml_space_attribute(new_run_text, text);
    new_run_text.text().set(text);

    return Run(this->current, new_run);
}

Paragraph Paragraph::insert_paragraph_before(const std::string &text,
                                             featherdoc::formatting_flag f) {
    const auto new_para = detail::insert_paragraph_node(this->parent, this->current);
    Paragraph paragraph(this->parent, new_para);
    paragraph.add_run(text, f);
    return paragraph;
}

Paragraph Paragraph::insert_paragraph_after(const std::string &text,
                                           featherdoc::formatting_flag f) {
    pugi::xml_node new_para;
    if (this->current == pugi::xml_node{}) {
        new_para = detail::append_paragraph_node(this->parent);
    } else {
        new_para = this->parent.insert_child_after("w:p", this->current);
    }

    Paragraph paragraph(this->parent, new_para);
    paragraph.add_run(text, f);
    return paragraph;
}

Paragraph Paragraph::insert_paragraph_like_before() {
    return Paragraph(this->parent, insert_paragraph_like_node(this->parent, this->current, false));
}

Paragraph Paragraph::insert_paragraph_like_after() {
    return Paragraph(this->parent, insert_paragraph_like_node(this->parent, this->current, true));
}

} // namespace featherdoc
