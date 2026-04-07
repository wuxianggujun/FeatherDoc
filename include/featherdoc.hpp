/*
 * FeatherDoc
 * Original upstream author: Amir Mohamadi (@amiremohamadi)
 * Current fork branding, licensing, and maintenance notes: see README,
 * LICENSE, and LICENSE.upstream-mit.
 * Licensing: see LICENSE and LICENSE.upstream-mit.
 */

#ifndef FEATHERDOC_HPP
#define FEATHERDOC_HPP

#include <cstddef>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <unordered_set>
#include <vector>

#include <constants.hpp>
#include <featherdoc_iterator.hpp>
#include <pugixml.hpp>
#include <zip.h>

namespace featherdoc {
class Document;

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

    Run &next();
    [[nodiscard]] bool has_next() const;
};

// Paragraph contains a paragraph
// and stores runs
class Paragraph {
  private:
    friend class IteratorHelper;
    friend class Document;
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

    Run &runs();
    Run add_run(const std::string &,
                featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Run add_run(const char *,
                featherdoc::formatting_flag = featherdoc::formatting_flag::none);
    Paragraph insert_paragraph_after(const std::string &,
                                     featherdoc::formatting_flag =
                                         featherdoc::formatting_flag::none);
};

// TableCell contains one or more paragraphs
class TableCell {
  private:
    friend class IteratorHelper;
    pugi::xml_node parent;
    pugi::xml_node current;

    Paragraph paragraph;

  public:
    TableCell();
    TableCell(pugi::xml_node, pugi::xml_node);

    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    Paragraph &paragraphs();
    [[nodiscard]] std::optional<std::uint32_t> width_twips() const;
    [[nodiscard]] bool set_width_twips(std::uint32_t width_twips);
    [[nodiscard]] bool clear_width();
    [[nodiscard]] std::size_t column_span() const;
    [[nodiscard]] bool merge_right(std::size_t additional_cells = 1U);
    [[nodiscard]] bool merge_down(std::size_t additional_rows = 1U);
    [[nodiscard]] std::optional<featherdoc::cell_vertical_alignment>
    vertical_alignment() const;
    [[nodiscard]] bool set_vertical_alignment(
        featherdoc::cell_vertical_alignment alignment);
    [[nodiscard]] bool clear_vertical_alignment();
    [[nodiscard]] std::optional<std::string> fill_color() const;
    [[nodiscard]] bool set_fill_color(std::string_view fill_color);
    [[nodiscard]] bool clear_fill_color();
    [[nodiscard]] std::optional<std::uint32_t> margin_twips(
        featherdoc::cell_margin_edge edge) const;
    [[nodiscard]] bool set_margin_twips(featherdoc::cell_margin_edge edge,
                                        std::uint32_t margin_twips);
    [[nodiscard]] bool clear_margin(featherdoc::cell_margin_edge edge);
    [[nodiscard]] bool set_border(featherdoc::cell_border_edge edge,
                                  featherdoc::border_definition border);
    [[nodiscard]] bool clear_border(featherdoc::cell_border_edge edge);

    TableCell &next();
    [[nodiscard]] bool has_next() const;
};

// TableRow consists of one or more TableCells
class TableRow {
    friend class IteratorHelper;
    pugi::xml_node parent;
    pugi::xml_node current;

    TableCell cell;

  public:
    TableRow();
    TableRow(pugi::xml_node, pugi::xml_node);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    TableCell &cells();
    [[nodiscard]] std::optional<std::uint32_t> height_twips() const;
    [[nodiscard]] std::optional<featherdoc::row_height_rule> height_rule() const;
    [[nodiscard]] bool set_height_twips(std::uint32_t height_twips,
                                        featherdoc::row_height_rule height_rule);
    [[nodiscard]] bool clear_height();
    [[nodiscard]] bool cant_split() const;
    [[nodiscard]] bool set_cant_split();
    [[nodiscard]] bool clear_cant_split();
    [[nodiscard]] bool repeats_header() const;
    [[nodiscard]] bool set_repeats_header();
    [[nodiscard]] bool clear_repeats_header();
    TableCell append_cell();

    [[nodiscard]] bool has_next() const;
    TableRow &next();
};

// Table consists of one or more TableRow objects
class Table {
  private:
    friend class IteratorHelper;
    Document *owner{nullptr};
    pugi::xml_node parent;
    pugi::xml_node current;

    TableRow row;

  public:
    Table();
    Table(pugi::xml_node, pugi::xml_node);
    void set_owner(Document *);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    Table &next();
    [[nodiscard]] bool has_next() const;

    TableRow &rows();
    [[nodiscard]] std::optional<std::uint32_t> width_twips() const;
    [[nodiscard]] bool set_width_twips(std::uint32_t width_twips);
    [[nodiscard]] bool clear_width();
    [[nodiscard]] std::optional<featherdoc::table_layout_mode> layout_mode() const;
    [[nodiscard]] bool set_layout_mode(featherdoc::table_layout_mode layout_mode);
    [[nodiscard]] bool clear_layout_mode();
    [[nodiscard]] std::optional<featherdoc::table_alignment> alignment() const;
    [[nodiscard]] bool set_alignment(featherdoc::table_alignment alignment);
    [[nodiscard]] bool clear_alignment();
    [[nodiscard]] std::optional<std::uint32_t> indent_twips() const;
    [[nodiscard]] bool set_indent_twips(std::uint32_t indent_twips);
    [[nodiscard]] bool clear_indent();
    [[nodiscard]] std::optional<std::uint32_t> cell_margin_twips(
        featherdoc::cell_margin_edge edge) const;
    [[nodiscard]] bool set_cell_margin_twips(featherdoc::cell_margin_edge edge,
                                             std::uint32_t margin_twips);
    [[nodiscard]] bool clear_cell_margin(featherdoc::cell_margin_edge edge);
    [[nodiscard]] std::optional<std::string> style_id() const;
    [[nodiscard]] bool set_style_id(std::string_view style_id);
    [[nodiscard]] bool clear_style_id();
    [[nodiscard]] bool set_border(featherdoc::table_border_edge edge,
                                  featherdoc::border_definition border);
    [[nodiscard]] bool clear_border(featherdoc::table_border_edge edge);
    TableRow append_row(std::size_t cell_count = 1U);
};

struct document_error_info {
    std::error_code code{};
    std::string detail;
    std::string entry_name;
    std::optional<std::ptrdiff_t> xml_offset;

    explicit operator bool() const noexcept {
        return static_cast<bool>(this->code);
    }

    void clear() {
        this->code.clear();
        this->detail.clear();
        this->entry_name.clear();
        this->xml_offset.reset();
    }
};

struct bookmark_text_binding {
    std::string bookmark_name;
    std::string text;
};

struct bookmark_fill_result {
    std::size_t requested{};
    std::size_t matched{};
    std::size_t replaced{};
    std::vector<std::string> missing_bookmarks;

    explicit operator bool() const noexcept {
        return this->missing_bookmarks.empty();
    }
};

struct bookmark_block_visibility_binding {
    std::string bookmark_name;
    bool visible{true};
};

struct bookmark_block_visibility_result {
    std::size_t requested{};
    std::size_t matched{};
    std::size_t kept{};
    std::size_t removed{};
    std::vector<std::string> missing_bookmarks;

    explicit operator bool() const noexcept {
        return this->missing_bookmarks.empty();
    }
};

class TemplatePart {
  private:
    friend class Document;
    Document *owner{nullptr};
    pugi::xml_document *xml_document{nullptr};
    document_error_info *last_error_info{nullptr};
    std::string entry_name_storage;

    TemplatePart(Document *owner, pugi::xml_document *xml_document,
                 document_error_info *last_error_info, std::string entry_name);

  public:
    TemplatePart();

    explicit operator bool() const noexcept;
    [[nodiscard]] std::string_view entry_name() const noexcept;

    [[nodiscard]] std::size_t replace_bookmark_text(const std::string &bookmark_name,
                                                    const std::string &replacement);
    [[nodiscard]] std::size_t replace_bookmark_text(const char *bookmark_name,
                                                    const char *replacement);
    [[nodiscard]] bookmark_fill_result fill_bookmarks(
        std::span<const bookmark_text_binding> bindings);
    [[nodiscard]] bookmark_fill_result fill_bookmarks(
        std::initializer_list<bookmark_text_binding> bindings);
    [[nodiscard]] std::size_t replace_bookmark_with_paragraphs(
        std::string_view bookmark_name, const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_bookmark_with_table_rows(
        std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_table(
        std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_bookmark_with_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] std::size_t set_bookmark_block_visibility(
        std::string_view bookmark_name, bool visible);
    [[nodiscard]] bookmark_block_visibility_result apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding> bindings);
    [[nodiscard]] bookmark_block_visibility_result apply_bookmark_block_visibility(
        std::initializer_list<bookmark_block_visibility_binding> bindings);
};

// Document contains whole the docx file
// and stores paragraphs
class Document {
  private:
    friend class Table;

    struct xml_part_state {
        std::string relationship_id;
        std::string entry_name;
        std::string relationships_entry_name;
        pugi::xml_document xml;
        pugi::xml_document relationships;
        Paragraph paragraph;
        bool has_relationships_part{false};
        bool relationships_dirty{false};
    };

    struct image_part_state {
        std::string owner_entry_name;
        std::string entry_name;
        std::string content_type;
        std::string data;
    };

    [[nodiscard]] std::error_code ensure_content_types_loaded();
    [[nodiscard]] std::error_code ensure_settings_loaded();
    [[nodiscard]] std::error_code ensure_settings_part_attached();
    [[nodiscard]] std::error_code ensure_numbering_loaded();
    [[nodiscard]] std::error_code ensure_numbering_part_attached();
    [[nodiscard]] std::error_code ensure_styles_loaded();
    [[nodiscard]] std::error_code ensure_styles_part_attached();
    [[nodiscard]] std::error_code ensure_even_and_odd_headers_enabled();
    [[nodiscard]] pugi::xml_node section_properties(std::size_t section_index) const;
    [[nodiscard]] pugi::xml_node ensure_section_properties(std::size_t section_index);
    [[nodiscard]] bool ensure_title_page_enabled(pugi::xml_node section_properties);
    Paragraph &related_part_paragraphs_by_relationship_id(
        std::string_view relationship_id, std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name);
    Paragraph &section_related_part_paragraphs(
        std::size_t section_index, featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name,
        const char *part_root_name);
    [[nodiscard]] xml_part_state *section_related_part_state(
        std::size_t section_index, featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *reference_name);
    Paragraph &ensure_section_related_part_paragraphs(
        std::size_t section_index, featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
        const char *reference_name, const char *relationship_type, const char *content_type);
    Paragraph &assign_section_related_part_paragraphs(
        std::size_t section_index, std::size_t part_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
        const char *reference_name);
    [[nodiscard]] bool remove_section_related_part_reference(
        std::size_t section_index, featherdoc::section_reference_kind reference_kind,
        const char *reference_name);
    [[nodiscard]] std::error_code ensure_removal_prerequisites_loaded();
    void cleanup_first_page_section_markers();
    [[nodiscard]] std::error_code cleanup_even_and_odd_headers_setting();
    [[nodiscard]] bool remove_related_part(
        std::size_t part_index, std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *reference_name);
    [[nodiscard]] bool copy_section_related_part_references(
        std::size_t source_section_index, std::size_t target_section_index,
        const char *reference_name);
    Paragraph &ensure_related_part_paragraphs(
        std::vector<std::unique_ptr<xml_part_state>> &parts, const char *part_root_name,
        const char *reference_name, const char *relationship_type, const char *content_type);
    [[nodiscard]] xml_part_state *find_related_part_state(std::string_view entry_name);
    [[nodiscard]] std::uint32_t next_drawing_object_id() const;
    [[nodiscard]] bool append_inline_image_part(
        std::string image_data, std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] bool append_inline_image_part(
        pugi::xml_node parent, pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type, std::string display_name,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] bool append_inline_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document, std::string_view relationships_entry_name,
        bool &has_relationships_part, bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data, std::string extension,
        std::string content_type, std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px);
    [[nodiscard]] std::size_t replace_bookmark_with_image_in_part(
        pugi::xml_document &xml_document, std::string_view entry_name,
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions);

    friend class IteratorHelper;
    friend class TemplatePart;
    std::filesystem::path document_path;
    Paragraph paragraph;
    Paragraph detached_paragraph;
    Table table;
    pugi::xml_document document;
    pugi::xml_document document_relationships;
    pugi::xml_document content_types;
    pugi::xml_document settings;
    pugi::xml_document numbering;
    pugi::xml_document styles;
    std::vector<std::unique_ptr<xml_part_state>> header_parts;
    std::vector<std::unique_ptr<xml_part_state>> footer_parts;
    std::vector<image_part_state> image_parts;
    bool flag_is_open{false};
    bool has_source_archive{false};
    bool has_document_relationships_part{false};
    bool has_settings_part{false};
    bool has_numbering_part{false};
    bool has_styles_part{false};
    bool document_relationships_dirty{false};
    bool content_types_loaded{false};
    bool content_types_dirty{false};
    bool settings_loaded{false};
    bool settings_dirty{false};
    bool numbering_loaded{false};
    bool numbering_dirty{false};
    bool styles_loaded{false};
    bool styles_dirty{false};
    mutable std::unordered_set<std::string> removed_related_part_entries;
    mutable document_error_info last_error_info;

  public:
    Document();
    explicit Document(std::filesystem::path);
    [[nodiscard]] std::error_code create_empty();
    [[nodiscard]] bool append_section(bool inherit_header_footer = true);
    [[nodiscard]] bool insert_section(std::size_t section_index,
                                      bool inherit_header_footer = true);
    [[nodiscard]] bool remove_section(std::size_t section_index);
    [[nodiscard]] bool move_section(std::size_t source_section_index,
                                    std::size_t target_section_index);
    void set_path(std::filesystem::path);
    [[nodiscard]] const std::filesystem::path &path() const;
    [[nodiscard]] std::error_code open();
    [[nodiscard]] std::error_code save() const;
    [[nodiscard]] std::error_code save_as(std::filesystem::path) const;
    [[nodiscard]] bool is_open() const;
    [[nodiscard]] const document_error_info &last_error() const noexcept;
    [[nodiscard]] std::size_t section_count() const noexcept;
    [[nodiscard]] std::size_t header_count() const noexcept;
    [[nodiscard]] std::size_t footer_count() const noexcept;
    [[nodiscard]] TemplatePart body_template();
    [[nodiscard]] TemplatePart header_template(std::size_t index = 0U);
    [[nodiscard]] TemplatePart footer_template(std::size_t index = 0U);
    [[nodiscard]] TemplatePart section_header_template(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] TemplatePart section_footer_template(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &header_paragraphs(std::size_t index = 0U);
    Paragraph &footer_paragraphs(std::size_t index = 0U);
    Paragraph &section_header_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &section_footer_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &ensure_section_header_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &ensure_section_footer_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &assign_section_header_paragraphs(
        std::size_t section_index, std::size_t header_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &assign_section_footer_paragraphs(
        std::size_t section_index, std::size_t footer_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] bool remove_section_header_reference(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] bool remove_section_footer_reference(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] bool remove_header_part(std::size_t index);
    [[nodiscard]] bool remove_footer_part(std::size_t index);
    [[nodiscard]] bool copy_section_header_references(std::size_t source_section_index,
                                                      std::size_t target_section_index);
    [[nodiscard]] bool copy_section_footer_references(std::size_t source_section_index,
                                                      std::size_t target_section_index);
    [[nodiscard]] bool replace_section_header_text(
        std::size_t section_index, std::string_view replacement_text,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    [[nodiscard]] bool replace_section_footer_text(
        std::size_t section_index, std::string_view replacement_text,
        featherdoc::section_reference_kind reference_kind =
            featherdoc::section_reference_kind::default_reference);
    Paragraph &ensure_header_paragraphs();
    Paragraph &ensure_footer_paragraphs();
    [[nodiscard]] std::size_t replace_bookmark_text(const std::string &bookmark_name,
                                                    const std::string &replacement);
    [[nodiscard]] std::size_t replace_bookmark_text(const char *bookmark_name,
                                                    const char *replacement);
    [[nodiscard]] bookmark_fill_result fill_bookmarks(
        std::span<const bookmark_text_binding> bindings);
    [[nodiscard]] bookmark_fill_result fill_bookmarks(
        std::initializer_list<bookmark_text_binding> bindings);
    [[nodiscard]] std::size_t replace_bookmark_with_paragraphs(
        std::string_view bookmark_name, const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_bookmark_with_table_rows(
        std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_table(
        std::string_view bookmark_name, const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_bookmark_with_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] std::size_t set_bookmark_block_visibility(
        std::string_view bookmark_name, bool visible);
    [[nodiscard]] bookmark_block_visibility_result apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding> bindings);
    [[nodiscard]] bookmark_block_visibility_result apply_bookmark_block_visibility(
        std::initializer_list<bookmark_block_visibility_binding> bindings);
    [[nodiscard]] bool set_paragraph_list(
        Paragraph paragraph, featherdoc::list_kind kind, std::uint32_t level = 0U);
    [[nodiscard]] bool clear_paragraph_list(Paragraph paragraph);
    [[nodiscard]] bool set_paragraph_style(Paragraph paragraph, std::string_view style_id);
    [[nodiscard]] bool clear_paragraph_style(Paragraph paragraph);
    [[nodiscard]] bool set_run_style(Run run, std::string_view style_id);
    [[nodiscard]] bool clear_run_style(Run run);

    Paragraph &paragraphs();
    Table &tables();
    Table append_table(std::size_t row_count = 1U, std::size_t column_count = 1U);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path,
                                    std::uint32_t width_px,
                                    std::uint32_t height_px);
};
} // namespace featherdoc

#endif
