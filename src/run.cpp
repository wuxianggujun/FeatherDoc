#include "featherdoc.hpp"
#include "xml_helpers.hpp"

namespace featherdoc {
namespace {

auto read_font_family_attribute(pugi::xml_node run_fonts, const char *attribute_name)
    -> std::optional<std::string> {
    if (run_fonts == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = run_fonts.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return std::nullopt;
    }

    return std::string{attribute.value()};
}

auto read_language_attribute(pugi::xml_node run_language, const char *attribute_name)
    -> std::optional<std::string> {
    if (run_language == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto attribute = run_language.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{} || attribute.value()[0] == '\0') {
        return std::nullopt;
    }

    return std::string{attribute.value()};
}

void set_xml_attribute(pugi::xml_node node, const char *attribute_name,
                       std::string_view value) {
    auto attribute = node.attribute(attribute_name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(attribute_name);
    }

    const std::string copied_value{value};
    attribute.set_value(copied_value.c_str());
}

auto ensure_run_fonts_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts != pugi::xml_node{}) {
        return run_fonts;
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rFonts", run_style);
    }

    if (const auto first_child = run_properties.first_child(); first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:rFonts", first_child);
    }

    return run_properties.append_child("w:rFonts");
}

auto ensure_run_language_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language != pugi::xml_node{}) {
        return run_language;
    }

    if (const auto run_fonts = run_properties.child("w:rFonts");
        run_fonts != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:lang", run_fonts);
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:lang", run_style);
    }

    if (const auto first_child = run_properties.first_child(); first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:lang", first_child);
    }

    return run_properties.append_child("w:lang");
}

void remove_empty_run_fonts_node(pugi::xml_node run_properties) {
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    const auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts == pugi::xml_node{}) {
        return;
    }

    if (run_fonts.first_child() == pugi::xml_node{} &&
        run_fonts.first_attribute() == pugi::xml_attribute{}) {
        run_properties.remove_child(run_fonts);
    }
}

void remove_empty_run_language_node(pugi::xml_node run_properties) {
    if (run_properties == pugi::xml_node{}) {
        return;
    }

    const auto run_language = run_properties.child("w:lang");
    if (run_language == pugi::xml_node{}) {
        return;
    }

    if (run_language.first_child() == pugi::xml_node{} &&
        run_language.first_attribute() == pugi::xml_attribute{}) {
        run_properties.remove_child(run_language);
    }
}

} // namespace

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

std::optional<std::string> Run::font_family() const {
    const auto run_fonts = this->current.child("w:rPr").child("w:rFonts");
    if (const auto ascii_font = read_font_family_attribute(run_fonts, "w:ascii")) {
        return ascii_font;
    }
    if (const auto hansi_font = read_font_family_attribute(run_fonts, "w:hAnsi")) {
        return hansi_font;
    }
    return read_font_family_attribute(run_fonts, "w:cs");
}

std::optional<std::string> Run::east_asia_font_family() const {
    return read_font_family_attribute(this->current.child("w:rPr").child("w:rFonts"),
                                      "w:eastAsia");
}

std::optional<std::string> Run::language() const {
    return read_language_attribute(this->current.child("w:rPr").child("w:lang"), "w:val");
}

std::optional<std::string> Run::east_asia_language() const {
    return read_language_attribute(this->current.child("w:rPr").child("w:lang"),
                                   "w:eastAsia");
}

std::optional<std::string> Run::bidi_language() const {
    return read_language_attribute(this->current.child("w:rPr").child("w:lang"), "w:bidi");
}

bool Run::set_font_family(std::string_view font_family) const {
    if (this->current == pugi::xml_node{} || font_family.empty()) {
        return false;
    }

    const auto run_properties = detail::ensure_run_properties_node(this->current);
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        return false;
    }

    set_xml_attribute(run_fonts, "w:ascii", font_family);
    set_xml_attribute(run_fonts, "w:hAnsi", font_family);
    set_xml_attribute(run_fonts, "w:cs", font_family);
    return true;
}

bool Run::set_east_asia_font_family(std::string_view font_family) const {
    if (this->current == pugi::xml_node{} || font_family.empty()) {
        return false;
    }

    const auto run_properties = detail::ensure_run_properties_node(this->current);
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    const auto run_fonts = ensure_run_fonts_node(run_properties);
    if (run_fonts == pugi::xml_node{}) {
        return false;
    }

    set_xml_attribute(run_fonts, "w:eastAsia", font_family);
    return true;
}

bool Run::set_language(std::string_view language) const {
    if (this->current == pugi::xml_node{} || language.empty()) {
        return false;
    }

    const auto run_properties = detail::ensure_run_properties_node(this->current);
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        return false;
    }

    set_xml_attribute(run_language, "w:val", language);
    return true;
}

bool Run::set_east_asia_language(std::string_view language) const {
    if (this->current == pugi::xml_node{} || language.empty()) {
        return false;
    }

    const auto run_properties = detail::ensure_run_properties_node(this->current);
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        return false;
    }

    set_xml_attribute(run_language, "w:eastAsia", language);
    return true;
}

bool Run::set_bidi_language(std::string_view language) const {
    if (this->current == pugi::xml_node{} || language.empty()) {
        return false;
    }

    const auto run_properties = detail::ensure_run_properties_node(this->current);
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    const auto run_language = ensure_run_language_node(run_properties);
    if (run_language == pugi::xml_node{}) {
        return false;
    }

    set_xml_attribute(run_language, "w:bidi", language);
    return true;
}

bool Run::clear_font_family() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto run_properties = this->current.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return true;
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts != pugi::xml_node{}) {
        run_fonts.remove_attribute("w:ascii");
        run_fonts.remove_attribute("w:hAnsi");
        run_fonts.remove_attribute("w:cs");
        run_fonts.remove_attribute("w:eastAsia");
        remove_empty_run_fonts_node(run_properties);
    }

    detail::remove_empty_run_properties(this->current);
    return true;
}

bool Run::clear_language() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto run_properties = this->current.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return true;
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language != pugi::xml_node{}) {
        run_language.remove_attribute("w:val");
        run_language.remove_attribute("w:eastAsia");
        run_language.remove_attribute("w:bidi");
        remove_empty_run_language_node(run_properties);
    }

    detail::remove_empty_run_properties(this->current);
    return true;
}

Run &Run::next() {
    this->current = detail::next_named_sibling(this->current, "w:r");
    return *this;
}

bool Run::has_next() const { return this->current != pugi::xml_node{}; }

} // namespace featherdoc
