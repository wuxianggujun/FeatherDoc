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
#include <optional>
#include <string>
#include <system_error>

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
    friend class IteratorHelper;
    std::filesystem::path document_path;
    Paragraph paragraph;
    Table table;
    pugi::xml_document document;
    bool flag_is_open{false};
    bool has_source_archive{false};
    mutable document_error_info last_error_info;

  public:
    Document();
    explicit Document(std::filesystem::path);
    [[nodiscard]] std::error_code create_empty();
    void set_path(std::filesystem::path);
    [[nodiscard]] const std::filesystem::path &path() const;
    [[nodiscard]] std::error_code open();
    [[nodiscard]] std::error_code save() const;
    [[nodiscard]] std::error_code save_as(std::filesystem::path) const;
    [[nodiscard]] bool is_open() const;
    [[nodiscard]] const document_error_info &last_error() const noexcept;

    Paragraph &paragraphs();
    Table &tables();
};
} // namespace featherdoc

#endif
