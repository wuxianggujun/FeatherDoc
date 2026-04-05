#include "featherdoc.hpp"
#include "xml_helpers.hpp"

namespace featherdoc {

Run::Run() = default;

Run::Run(pugi::xml_node parent, pugi::xml_node current) {
    this->set_parent(parent);
    this->set_current(current);
}

void Run::set_parent(pugi::xml_node node) {
    this->parent = node;
    this->current = this->parent.child("w:r");
}

void Run::set_current(pugi::xml_node node) { this->current = node; }

std::string Run::get_text() const { return this->current.child("w:t").text().get(); }

bool Run::set_text(const std::string &text) const { return this->set_text(text.c_str()); }

bool Run::set_text(const char *text) const {
    if (text == nullptr) {
        return false;
    }

    auto text_node = this->current.child("w:t");
    if (text_node == pugi::xml_node{}) {
        return false;
    }

    detail::update_xml_space_attribute(text_node, text);
    return text_node.text().set(text);
}

Run &Run::next() {
    this->current = detail::next_named_sibling(this->current, "w:r");
    return *this;
}

bool Run::has_next() const { return this->current != pugi::xml_node{}; }

} // namespace featherdoc
