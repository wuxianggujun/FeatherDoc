#include "featherdoc.hpp"
#include "xml_helpers.hpp"

#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>

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

void remove_empty_paragraph_properties(pugi::xml_node paragraph);

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

auto ensure_paragraph_alignment_node(pugi::xml_node paragraph_properties)
    -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto alignment = paragraph_properties.child("w:jc");
    if (alignment != pugi::xml_node{}) {
        return alignment;
    }

    return paragraph_properties.append_child("w:jc");
}

auto ensure_paragraph_indentation_node(pugi::xml_node paragraph_properties)
    -> pugi::xml_node {
    if (paragraph_properties == pugi::xml_node{}) {
        return {};
    }

    auto indentation = paragraph_properties.child("w:ind");
    if (indentation != pugi::xml_node{}) {
        return indentation;
    }

    if (const auto alignment = paragraph_properties.child("w:jc");
        alignment != pugi::xml_node{}) {
        return paragraph_properties.insert_child_before("w:ind", alignment);
    }

    return paragraph_properties.append_child("w:ind");
}

auto parse_u32_attribute_value(const char *text)
    -> std::optional<std::uint32_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' ||
        value > static_cast<unsigned long>(
                    std::numeric_limits<std::uint32_t>::max())) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(value);
}

auto read_paragraph_alignment_node(pugi::xml_node node)
    -> std::optional<featherdoc::paragraph_alignment> {
    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value == "left" || value == "start") {
        return featherdoc::paragraph_alignment::left;
    }
    if (value == "center") {
        return featherdoc::paragraph_alignment::center;
    }
    if (value == "right" || value == "end") {
        return featherdoc::paragraph_alignment::right;
    }
    if (value == "both" || value == "mediumKashida" ||
        value == "highKashida" || value == "lowKashida") {
        return featherdoc::paragraph_alignment::justified;
    }
    if (value == "distribute") {
        return featherdoc::paragraph_alignment::distribute;
    }

    return std::nullopt;
}

auto paragraph_alignment_value(featherdoc::paragraph_alignment alignment)
    -> const char * {
    switch (alignment) {
    case featherdoc::paragraph_alignment::left:
        return "left";
    case featherdoc::paragraph_alignment::center:
        return "center";
    case featherdoc::paragraph_alignment::right:
        return "right";
    case featherdoc::paragraph_alignment::justified:
        return "both";
    case featherdoc::paragraph_alignment::distribute:
        return "distribute";
    }

    return "left";
}

void set_attribute_value(pugi::xml_node node, const char *attribute_name,
                         const char *value) {
    auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(attribute_name);
    }
    attribute.set_value(value);
}

void remove_empty_paragraph_indentation(pugi::xml_node paragraph) {
    if (paragraph == pugi::xml_node{}) {
        return;
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return;
    }

    auto indentation = paragraph_properties.child("w:ind");
    if (indentation != pugi::xml_node{} &&
        indentation.first_child() == pugi::xml_node{} &&
        indentation.first_attribute() == pugi::xml_attribute{}) {
        paragraph_properties.remove_child(indentation);
    }

    remove_empty_paragraph_properties(paragraph);
}

auto paragraph_indentation_attribute(pugi::xml_node paragraph,
                                     const char *attribute_name)
    -> std::optional<std::uint32_t> {
    return parse_u32_attribute_value(
        paragraph.child("w:pPr").child("w:ind").attribute(attribute_name).value());
}

bool set_paragraph_indentation_attribute(pugi::xml_node paragraph,
                                         const char *attribute_name,
                                         std::uint32_t indent_twips) {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    auto paragraph_properties = ensure_paragraph_properties_node(paragraph);
    if (paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    auto indentation = ensure_paragraph_indentation_node(paragraph_properties);
    if (indentation == pugi::xml_node{}) {
        return false;
    }

    const auto value = std::to_string(indent_twips);
    set_attribute_value(indentation, attribute_name, value.c_str());
    if (std::string_view{attribute_name} == "w:firstLine") {
        indentation.remove_attribute("w:hanging");
    } else if (std::string_view{attribute_name} == "w:hanging") {
        indentation.remove_attribute("w:firstLine");
    }
    return true;
}

bool clear_paragraph_indentation_attribute(pugi::xml_node paragraph,
                                           const char *attribute_name) {
    if (paragraph == pugi::xml_node{}) {
        return false;
    }

    auto paragraph_properties = paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return true;
    }

    auto indentation = paragraph_properties.child("w:ind");
    if (indentation != pugi::xml_node{}) {
        indentation.remove_attribute(attribute_name);
    }

    remove_empty_paragraph_indentation(paragraph);
    return true;
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

std::optional<featherdoc::paragraph_alignment> Paragraph::alignment() const {
    return read_paragraph_alignment_node(
        this->current.child("w:pPr").child("w:jc"));
}

bool Paragraph::set_alignment(featherdoc::paragraph_alignment alignment) const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto paragraph_properties = ensure_paragraph_properties_node(this->current);
    if (paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    const auto alignment_node =
        ensure_paragraph_alignment_node(paragraph_properties);
    if (alignment_node == pugi::xml_node{}) {
        return false;
    }

    set_attribute_value(alignment_node, "w:val",
                        paragraph_alignment_value(alignment));
    return true;
}

bool Paragraph::clear_alignment() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto paragraph_properties = this->current.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        return true;
    }

    const auto alignment = paragraph_properties.child("w:jc");
    if (alignment != pugi::xml_node{}) {
        paragraph_properties.remove_child(alignment);
    }

    remove_empty_paragraph_properties(this->current);
    return true;
}

std::optional<std::uint32_t> Paragraph::indent_left_twips() const {
    return paragraph_indentation_attribute(this->current, "w:left");
}

std::optional<std::uint32_t> Paragraph::indent_right_twips() const {
    return paragraph_indentation_attribute(this->current, "w:right");
}

std::optional<std::uint32_t> Paragraph::first_line_indent_twips() const {
    return paragraph_indentation_attribute(this->current, "w:firstLine");
}

std::optional<std::uint32_t> Paragraph::hanging_indent_twips() const {
    return paragraph_indentation_attribute(this->current, "w:hanging");
}

bool Paragraph::set_indent_left_twips(std::uint32_t indent_twips) const {
    return set_paragraph_indentation_attribute(this->current, "w:left",
                                               indent_twips);
}

bool Paragraph::set_indent_right_twips(std::uint32_t indent_twips) const {
    return set_paragraph_indentation_attribute(this->current, "w:right",
                                               indent_twips);
}

bool Paragraph::set_first_line_indent_twips(std::uint32_t indent_twips) const {
    return set_paragraph_indentation_attribute(this->current, "w:firstLine",
                                               indent_twips);
}

bool Paragraph::set_hanging_indent_twips(std::uint32_t indent_twips) const {
    return set_paragraph_indentation_attribute(this->current, "w:hanging",
                                               indent_twips);
}

bool Paragraph::clear_indent_left() const {
    return clear_paragraph_indentation_attribute(this->current, "w:left");
}

bool Paragraph::clear_indent_right() const {
    return clear_paragraph_indentation_attribute(this->current, "w:right");
}

bool Paragraph::clear_first_line_indent() const {
    return clear_paragraph_indentation_attribute(this->current, "w:firstLine");
}

bool Paragraph::clear_hanging_indent() const {
    return clear_paragraph_indentation_attribute(this->current, "w:hanging");
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

    if (!detail::set_plain_text_run_content(new_run, text)) {
        return Run{};
    }

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
