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

auto ensure_run_rtl_node(pugi::xml_node run_properties) -> pugi::xml_node {
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    auto rtl = run_properties.child("w:rtl");
    if (rtl != pugi::xml_node{}) {
        return rtl;
    }

    if (const auto run_language = run_properties.child("w:lang");
        run_language != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_language);
    }

    if (const auto run_fonts = run_properties.child("w:rFonts");
        run_fonts != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_fonts);
    }

    if (const auto run_style = run_properties.child("w:rStyle");
        run_style != pugi::xml_node{}) {
        return run_properties.insert_child_after("w:rtl", run_style);
    }

    if (const auto first_child = run_properties.first_child(); first_child != pugi::xml_node{}) {
        return run_properties.insert_child_before("w:rtl", first_child);
    }

    return run_properties.append_child("w:rtl");
}

void set_on_off_value(pugi::xml_node node, bool enabled) {
    auto attribute = node.attribute("w:val");
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute("w:val");
    }
    attribute.set_value(enabled ? "1" : "0");
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

auto clear_run_font_attribute(pugi::xml_node run_properties,
                              const char *attribute_name) -> bool {
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    auto run_fonts = run_properties.child("w:rFonts");
    if (run_fonts == pugi::xml_node{}) {
        return false;
    }

    const auto removed = run_fonts.remove_attribute(attribute_name);
    remove_empty_run_fonts_node(run_properties);
    return removed;
}

auto clear_run_language_attribute(pugi::xml_node run_properties,
                                  const char *attribute_name) -> bool {
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    auto run_language = run_properties.child("w:lang");
    if (run_language == pugi::xml_node{}) {
        return false;
    }

    const auto removed = run_language.remove_attribute(attribute_name);
    remove_empty_run_language_node(run_properties);
    return removed;
}

auto insert_run_node(pugi::xml_node parent, pugi::xml_node insert_before) -> pugi::xml_node {
    if (parent == pugi::xml_node{}) {
        return {};
    }

    if (insert_before != pugi::xml_node{}) {
        if (insert_before.parent() != parent) {
            return {};
        }
        return parent.insert_child_before("w:r", insert_before);
    }

    return parent.append_child("w:r");
}

auto insert_formatted_run(pugi::xml_node parent, pugi::xml_node insert_before,
                          const char *text, featherdoc::formatting_flag formatting)
    -> pugi::xml_node {
    if (text == nullptr) {
        return {};
    }

    auto run = insert_run_node(parent, insert_before);
    if (run == pugi::xml_node{}) {
        return {};
    }

    auto run_properties = run.append_child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return {};
    }

    if (featherdoc::has_flag(formatting, featherdoc::formatting_flag::bold)) {
        run_properties.append_child("w:b");
    }

    if (featherdoc::has_flag(formatting, featherdoc::formatting_flag::italic)) {
        run_properties.append_child("w:i");
    }

    if (featherdoc::has_flag(formatting, featherdoc::formatting_flag::underline)) {
        run_properties.append_child("w:u").append_attribute("w:val").set_value("single");
    }

    if (featherdoc::has_flag(formatting, featherdoc::formatting_flag::strikethrough)) {
        run_properties.append_child("w:strike").append_attribute("w:val").set_value("true");
    }

    if (featherdoc::has_flag(formatting, featherdoc::formatting_flag::superscript)) {
        run_properties.append_child("w:vertAlign").append_attribute("w:val").set_value(
            "superscript");
    } else if (featherdoc::has_flag(formatting, featherdoc::formatting_flag::subscript)) {
        run_properties.append_child("w:vertAlign").append_attribute("w:val").set_value(
            "subscript");
    }

    if (featherdoc::has_flag(formatting, featherdoc::formatting_flag::smallcaps)) {
        run_properties.append_child("w:smallCaps").append_attribute("w:val").set_value("true");
    }

    if (featherdoc::has_flag(formatting, featherdoc::formatting_flag::shadow)) {
        run_properties.append_child("w:shadow").append_attribute("w:val").set_value("true");
    }

    if (!detail::set_plain_text_run_content(run, text)) {
        return {};
    }

    return run;
}

auto copy_run_properties(pugi::xml_node source_run, pugi::xml_node target_run) -> bool {
    if (source_run == pugi::xml_node{} || target_run == pugi::xml_node{}) {
        return false;
    }

    const auto source_properties = source_run.child("w:rPr");
    if (source_properties == pugi::xml_node{}) {
        return true;
    }

    return target_run.append_copy(source_properties) != pugi::xml_node{};
}

auto insert_empty_run_like_node(pugi::xml_node parent, pugi::xml_node anchor,
                                bool insert_after) -> pugi::xml_node {
    if (parent == pugi::xml_node{} || anchor == pugi::xml_node{} ||
        anchor.parent() != parent) {
        return {};
    }

    const auto insert_before =
        insert_after ? detail::next_named_sibling(anchor, "w:r") : anchor;
    auto inserted_run = insert_run_node(parent, insert_before);
    if (inserted_run == pugi::xml_node{}) {
        return {};
    }

    if (!copy_run_properties(anchor, inserted_run)) {
        parent.remove_child(inserted_run);
        return {};
    }

    if (inserted_run.append_child("w:t") == pugi::xml_node{}) {
        parent.remove_child(inserted_run);
        return {};
    }

    return inserted_run;
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

std::string Run::get_text() const { return detail::collect_plain_text_from_xml(this->current); }

bool Run::set_text(const std::string &text) const { return this->set_text(text.c_str()); }

bool Run::set_text(const char *text) const {
    if (this->current == pugi::xml_node{} || text == nullptr) {
        return false;
    }

    return detail::set_plain_text_run_content(this->current, text);
}

std::optional<std::string> Run::style_id() const {
    return read_language_attribute(this->current.child("w:rPr").child("w:rStyle"), "w:val");
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

std::optional<bool> Run::rtl() const {
    return read_on_off_value(this->current.child("w:rPr").child("w:rtl"));
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

bool Run::set_rtl(bool enabled) const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto run_properties = detail::ensure_run_properties_node(this->current);
    if (run_properties == pugi::xml_node{}) {
        return false;
    }

    const auto rtl = ensure_run_rtl_node(run_properties);
    if (rtl == pugi::xml_node{}) {
        return false;
    }

    set_on_off_value(rtl, enabled);
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

bool Run::clear_east_asia_font_family() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto run_properties = this->current.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return true;
    }

    clear_run_font_attribute(run_properties, "w:eastAsia");
    detail::remove_empty_run_properties(this->current);
    return true;
}

bool Run::clear_primary_language() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto run_properties = this->current.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return true;
    }

    clear_run_language_attribute(run_properties, "w:val");
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

bool Run::clear_east_asia_language() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto run_properties = this->current.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return true;
    }

    clear_run_language_attribute(run_properties, "w:eastAsia");
    detail::remove_empty_run_properties(this->current);
    return true;
}

bool Run::clear_bidi_language() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    const auto run_properties = this->current.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return true;
    }

    clear_run_language_attribute(run_properties, "w:bidi");
    detail::remove_empty_run_properties(this->current);
    return true;
}

bool Run::clear_rtl() const {
    if (this->current == pugi::xml_node{}) {
        return false;
    }

    auto run_properties = this->current.child("w:rPr");
    if (run_properties == pugi::xml_node{}) {
        return true;
    }

    const auto rtl = run_properties.child("w:rtl");
    if (rtl != pugi::xml_node{}) {
        run_properties.remove_child(rtl);
    }

    detail::remove_empty_run_properties(this->current);
    return true;
}

bool Run::remove() {
    if (this->parent == pugi::xml_node{} || this->current == pugi::xml_node{}) {
        return false;
    }

    const auto next_run = detail::next_named_sibling(this->current, "w:r");
    const auto previous_run = detail::previous_named_sibling(this->current, "w:r");
    if (!this->parent.remove_child(this->current)) {
        return false;
    }

    this->current = next_run != pugi::xml_node{} ? next_run : previous_run;
    return true;
}

Run Run::insert_run_before(const std::string &text, featherdoc::formatting_flag formatting) {
    const auto inserted_run = insert_formatted_run(this->parent, this->current, text.c_str(),
                                                   formatting);
    return Run(this->parent, inserted_run);
}

Run Run::insert_run_after(const std::string &text, featherdoc::formatting_flag formatting) {
    const auto next_run = this->current == pugi::xml_node{}
                              ? pugi::xml_node{}
                              : detail::next_named_sibling(this->current, "w:r");
    const auto inserted_run = insert_formatted_run(this->parent, next_run, text.c_str(),
                                                   formatting);
    return Run(this->parent, inserted_run);
}

Run Run::insert_run_like_before() {
    return Run(this->parent, insert_empty_run_like_node(this->parent, this->current, false));
}

Run Run::insert_run_like_after() {
    return Run(this->parent, insert_empty_run_like_node(this->parent, this->current, true));
}

Run &Run::next() {
    this->current = detail::next_named_sibling(this->current, "w:r");
    return *this;
}

bool Run::has_next() const { return this->current != pugi::xml_node{}; }

} // namespace featherdoc
