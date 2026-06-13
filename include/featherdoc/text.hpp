#pragma once

#include <constants.hpp>
#include <featherdoc_iterator.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <pugixml.hpp>

namespace featherdoc {

class Document;
class TemplatePart;

// Run contains runs in a paragraph
class Run {
  private:
    friend class IteratorHelper;
    friend class Document;
    // Store the parent node (a paragraph)
    pugi::xml_node parent;
    // And store current node also
    pugi::xml_node current;

  public:
    Run();
    Run(pugi::xml_node, pugi::xml_node);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    [[nodiscard]] std::string get_text() const;
    [[nodiscard]] bool set_text(const std::string &) const;
    [[nodiscard]] bool set_text(const char *) const;
    [[nodiscard]] std::optional<std::string> style_id() const;
    [[nodiscard]] std::optional<std::string> font_family() const;
    [[nodiscard]] std::optional<std::string> east_asia_font_family() const;
    [[nodiscard]] std::optional<std::string> language() const;
    [[nodiscard]] std::optional<std::string> east_asia_language() const;
    [[nodiscard]] std::optional<std::string> bidi_language() const;
    [[nodiscard]] std::optional<bool> rtl() const;
    [[nodiscard]] std::optional<std::string> text_color() const;
    [[nodiscard]] std::optional<bool> bold() const;
    [[nodiscard]] std::optional<bool> italic() const;
    [[nodiscard]] std::optional<bool> strikethrough() const;
    [[nodiscard]] std::optional<bool> underline() const;
    [[nodiscard]] std::optional<bool> superscript() const;
    [[nodiscard]] std::optional<bool> subscript() const;
    [[nodiscard]] std::optional<double> font_size_points() const;
    [[nodiscard]] bool set_font_family(std::string_view font_family) const;
    [[nodiscard]] bool
    set_east_asia_font_family(std::string_view font_family) const;
    [[nodiscard]] bool set_language(std::string_view language) const;
    [[nodiscard]] bool set_east_asia_language(std::string_view language) const;
    [[nodiscard]] bool set_bidi_language(std::string_view language) const;
    [[nodiscard]] bool set_rtl(bool enabled = true) const;
    [[nodiscard]] bool clear_font_family() const;
    [[nodiscard]] bool clear_east_asia_font_family() const;
    [[nodiscard]] bool clear_primary_language() const;
    [[nodiscard]] bool clear_language() const;
    [[nodiscard]] bool clear_east_asia_language() const;
    [[nodiscard]] bool clear_bidi_language() const;
    [[nodiscard]] bool clear_rtl() const;
    [[nodiscard]] bool remove();
    Run insert_run_before(
        const std::string &,
        featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Run insert_run_after(
        const std::string &,
        featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Run insert_run_like_before();
    Run insert_run_like_after();

    Run &next();
    [[nodiscard]] bool has_next() const;
};

// Paragraph contains a paragraph
// and stores runs
class Paragraph {
  private:
    friend class IteratorHelper;
    friend class Document;
    friend class TemplatePart;
    // Store parent node (usually the body node)
    pugi::xml_node parent;
    // And store current node also
    pugi::xml_node current;
    // A paragraph consists of runs
    Run run;

  public:
    Paragraph();
    Paragraph(pugi::xml_node, pugi::xml_node);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    Paragraph &next();
    [[nodiscard]] bool has_next() const;
    [[nodiscard]] std::optional<bool> bidi() const;
    [[nodiscard]] bool set_bidi(bool enabled = true) const;
    [[nodiscard]] bool clear_bidi() const;
    [[nodiscard]] std::optional<featherdoc::paragraph_alignment>
    alignment() const;
    [[nodiscard]] bool
    set_alignment(featherdoc::paragraph_alignment alignment) const;
    [[nodiscard]] bool clear_alignment() const;
    [[nodiscard]] std::optional<std::uint32_t> indent_left_twips() const;
    [[nodiscard]] std::optional<std::uint32_t> indent_right_twips() const;
    [[nodiscard]] std::optional<std::uint32_t>
    first_line_indent_twips() const;
    [[nodiscard]] std::optional<std::uint32_t> hanging_indent_twips() const;
    [[nodiscard]] bool set_indent_left_twips(std::uint32_t indent_twips) const;
    [[nodiscard]] bool set_indent_right_twips(std::uint32_t indent_twips) const;
    [[nodiscard]] bool
    set_first_line_indent_twips(std::uint32_t indent_twips) const;
    [[nodiscard]] bool set_hanging_indent_twips(std::uint32_t indent_twips) const;
    [[nodiscard]] bool clear_indent_left() const;
    [[nodiscard]] bool clear_indent_right() const;
    [[nodiscard]] bool clear_first_line_indent() const;
    [[nodiscard]] bool clear_hanging_indent() const;
    [[nodiscard]] bool set_text(const std::string &) const;
    [[nodiscard]] bool set_text(const char *) const;
    [[nodiscard]] bool remove();

    Run &runs();
    Run
    add_run(const std::string &,
            featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Run
    add_run(const char *,
            featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Paragraph insert_paragraph_before(
        const std::string &,
        featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Paragraph insert_paragraph_after(
        const std::string &,
        featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Paragraph insert_paragraph_like_before();
    Paragraph insert_paragraph_like_after();
};

} // namespace featherdoc
