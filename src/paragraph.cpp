#include "featherdoc.hpp"
#include "xml_helpers.hpp"

namespace featherdoc {

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

} // namespace featherdoc
