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
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <vector>

#include <constants.hpp>
#include <featherdoc_iterator.hpp>
#include <pugixml.hpp>
#include <zip.h>

namespace featherdoc {
// Run contains runs in a paragraph
class Run {
  private:
    friend class IteratorHelper;
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

    [[nodiscard]] bool has_next() const;
    TableRow &next();
};

// Table consists of one or more TableRow objects
class Table {
  private:
    friend class IteratorHelper;
    pugi::xml_node parent;
    pugi::xml_node current;

    TableRow row;

  public:
    Table();
    Table(pugi::xml_node, pugi::xml_node);
    void set_parent(pugi::xml_node);
    void set_current(pugi::xml_node);

    Table &next();
    [[nodiscard]] bool has_next() const;

    TableRow &rows();
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

// Document contains whole the docx file
// and stores paragraphs
class Document {
  private:
    struct xml_part_state {
        std::string relationship_id;
        std::string entry_name;
        pugi::xml_document xml;
        Paragraph paragraph;
    };

    [[nodiscard]] std::error_code ensure_content_types_loaded();
    [[nodiscard]] std::error_code ensure_settings_loaded();
    [[nodiscard]] std::error_code ensure_settings_part_attached();
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

    friend class IteratorHelper;
    std::filesystem::path document_path;
    Paragraph paragraph;
    Paragraph detached_paragraph;
    Table table;
    pugi::xml_document document;
    pugi::xml_document document_relationships;
    pugi::xml_document content_types;
    pugi::xml_document settings;
    std::vector<std::unique_ptr<xml_part_state>> header_parts;
    std::vector<std::unique_ptr<xml_part_state>> footer_parts;
    bool flag_is_open{false};
    bool has_source_archive{false};
    bool has_document_relationships_part{false};
    bool has_settings_part{false};
    bool document_relationships_dirty{false};
    bool content_types_loaded{false};
    bool content_types_dirty{false};
    bool settings_loaded{false};
    bool settings_dirty{false};
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

    Paragraph &paragraphs();
    Table &tables();
};
} // namespace featherdoc

#endif
