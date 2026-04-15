#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <featherdoc.hpp>

namespace {
namespace fs = std::filesystem;

auto cli_binary_name() -> const char * {
#if defined(_WIN32)
    return "featherdoc_cli.exe";
#else
    return "featherdoc_cli";
#endif
}

auto shell_quote(std::string_view value) -> std::string {
#if defined(_WIN32)
    std::string quoted = "\"";
    for (const char ch : value) {
        if (ch == '"') {
            quoted += "\\\"";
        } else {
            quoted += ch;
        }
    }
    quoted += '"';
    return quoted;
#else
    std::string quoted = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += '\'';
    return quoted;
#endif
}

auto json_quote(std::string_view value) -> std::string {
    std::string quoted = "\"";
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            quoted += "\\\\";
            break;
        case '"':
            quoted += "\\\"";
            break;
        case '\b':
            quoted += "\\b";
            break;
        case '\f':
            quoted += "\\f";
            break;
        case '\n':
            quoted += "\\n";
            break;
        case '\r':
            quoted += "\\r";
            break;
        case '\t':
            quoted += "\\t";
            break;
        default:
            quoted += ch;
            break;
        }
    }
    quoted += '"';
    return quoted;
}

auto normalize_system_status(int status) -> int {
#if defined(_WIN32)
    return status;
#else
    if (status == -1) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return status;
#endif
}

auto cli_binary_path() -> fs::path {
    return fs::current_path() / cli_binary_name();
}

void remove_if_exists(const fs::path &path) {
    std::error_code error;
    fs::remove(path, error);
}

auto read_text_file(const fs::path &path) -> std::string {
    std::ifstream stream(path);
    REQUIRE(stream.good());
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

auto read_binary_file(const fs::path &path) -> std::string {
    std::ifstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

void write_binary_file(const fs::path &path, const std::string &data) {
    std::ofstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    REQUIRE(stream.good());
}

auto tiny_png_data() -> std::string {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes), sizeof(tiny_png_bytes)};
}

auto tiny_gif_data() -> std::string {
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U,
        0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U,
        0xF9U, 0x04U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U,
        0x00U, 0x00U, 0x01U, 0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U,
        0x01U, 0x00U, 0x3BU,
    };

    return {reinterpret_cast<const char *>(tiny_gif_bytes), sizeof(tiny_gif_bytes)};
}

auto read_docx_entry(const fs::path &path, const char *entry_name) -> std::string {
    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE_EQ(zip_entry_open(archive, entry_name), 0);

    void *buffer = nullptr;
    size_t buffer_size = 0;
    REQUIRE_GE(zip_entry_read(archive, &buffer, &buffer_size), 0);
    REQUIRE_EQ(zip_entry_close(archive), 0);
    zip_close(archive);

    std::string content(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);
    return content;
}

auto find_style_xml_node(pugi::xml_node styles_root, std::string_view style_id)
    -> pugi::xml_node {
    for (auto style = styles_root.child("w:style"); style != pugi::xml_node{};
         style = style.next_sibling("w:style")) {
        if (std::string_view{style.attribute("w:styleId").value()} == style_id) {
            return style;
        }
    }

    return {};
}

auto find_numbering_abstract_xml_node(pugi::xml_node numbering_root,
                                      std::string_view definition_name)
    -> pugi::xml_node {
    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        const auto name = abstract_num.child("w:name");
        if (name != pugi::xml_node{} &&
            std::string_view{name.attribute("w:val").value()} == definition_name) {
            return abstract_num;
        }
    }

    return {};
}

auto find_numbering_level_xml_node(pugi::xml_node abstract_num, std::string_view level)
    -> pugi::xml_node {
    for (auto numbering_level = abstract_num.child("w:lvl");
         numbering_level != pugi::xml_node{};
         numbering_level = numbering_level.next_sibling("w:lvl")) {
        if (std::string_view{numbering_level.attribute("w:ilvl").value()} == level) {
            return numbering_level;
        }
    }

    return {};
}

auto collect_part_lines(featherdoc::Paragraph paragraph) -> std::vector<std::string> {
    std::vector<std::string> lines;
    for (; paragraph.has_next(); paragraph.next()) {
        std::string text;
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            text += run.get_text();
        }
        lines.push_back(std::move(text));
    }
    return lines;
}

auto collect_non_empty_document_text(featherdoc::Document &document) -> std::string {
    std::ostringstream stream;
    for (auto paragraph = document.paragraphs(); paragraph.has_next(); paragraph.next()) {
        std::string text;
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            text += run.get_text();
        }

        if (!text.empty()) {
            stream << text << '\n';
        }
    }

    return stream.str();
}

auto find_header_index_by_text(featherdoc::Document &document, std::string_view text)
    -> std::size_t {
    for (std::size_t index = 0; index < document.header_count(); ++index) {
        const auto lines = collect_part_lines(document.header_paragraphs(index));
        if (!lines.empty() && lines.front() == text) {
            return index;
        }
    }

    REQUIRE(false);
    return 0;
}

auto find_footer_index_by_text(featherdoc::Document &document, std::string_view text)
    -> std::size_t {
    for (std::size_t index = 0; index < document.footer_count(); ++index) {
        const auto lines = collect_part_lines(document.footer_paragraphs(index));
        if (!lines.empty() && lines.front() == text) {
            return index;
        }
    }

    REQUIRE(false);
    return 0;
}

void append_body_paragraph(featherdoc::Document &document, const char *text) {
    auto paragraph = document.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    const auto inserted = paragraph.insert_paragraph_after(text);
    REQUIRE(inserted.has_next());
}

void create_cli_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    REQUIRE(document.paragraphs().add_run("section 0 body").has_next());

    auto section0_header = document.ensure_section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK(section0_header.add_run("section 0 header").has_next());

    CHECK(document.append_section(false));
    append_body_paragraph(document, "section 1 body");

    auto section1_header = document.ensure_section_header_paragraphs(1);
    REQUIRE(section1_header.has_next());
    CHECK(section1_header.add_run("section 1 header").has_next());

    auto section1_first_footer = document.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(document.append_section(false));
    append_body_paragraph(document, "section 2 body");

    CHECK_EQ(document.section_count(), 3);
    REQUIRE_FALSE(document.save());
}

void create_cli_reference_fixture(const fs::path &path) {
    create_cli_fixture(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());

    auto section0_footer = document.ensure_section_footer_paragraphs(0);
    REQUIRE(section0_footer.has_next());
    CHECK(section0_footer.add_run("section 0 footer").has_next());

    REQUIRE_FALSE(document.save());
}

void create_cli_part_inspect_fixture(const fs::path &path) {
    create_cli_reference_fixture(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());

    const auto header_index = find_header_index_by_text(document, "section 0 header");
    const auto footer_index = find_footer_index_by_text(document, "section 0 footer");

    auto even_header = document.assign_section_header_paragraphs(
        2, header_index, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    auto default_footer = document.assign_section_footer_paragraphs(2, footer_index);
    REQUIRE(default_footer.has_next());

    REQUIRE_FALSE(document.save());
}

bool write_archive_entry(zip_t *archive, const char *entry_name,
                         std::string_view content) {
    if (zip_entry_open(archive, entry_name) != 0) {
        return false;
    }
    if (zip_entry_write(archive, content.data(), content.size()) < 0) {
        zip_entry_close(archive);
        return false;
    }
    return zip_entry_close(archive) == 0;
}

void create_cli_body_template_validation_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer A: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>Alice</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Customer B: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="customer"/>
      <w:r><w:t>Bob</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:p>
      <w:r><w:t>Summary prefix </w:t></w:r>
      <w:bookmarkStart w:id="2" w:name="summary_block"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="2"/>
      <w:r><w:t> suffix</w:t></w:r>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}

void create_cli_part_template_validation_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>CLI Template Validation Fixture</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)";
    constexpr auto header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Header title: </w:t></w:r>
    <w:bookmarkStart w:id="0" w:name="header_title"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_note"/>
    <w:r><w:t>standalone header note</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
  <w:tbl>
    <w:tblGrid>
      <w:gridCol w:w="2400"/>
      <w:gridCol w:w="2400"/>
    </w:tblGrid>
    <w:tr>
      <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
      <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
    </w:tr>
    <w:tr>
      <w:tc>
        <w:p>
          <w:bookmarkStart w:id="2" w:name="header_rows"/>
          <w:r><w:t>row placeholder</w:t></w:r>
          <w:bookmarkEnd w:id="2"/>
        </w:p>
      </w:tc>
      <w:tc><w:p><w:r><w:t>0</w:t></w:r></w:p></w:tc>
    </w:tr>
  </w:tbl>
</w:hdr>
)";
    constexpr auto footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Footer company A: </w:t></w:r>
    <w:bookmarkStart w:id="3" w:name="footer_company"/>
    <w:r><w:t>placeholder A</w:t></w:r>
    <w:bookmarkEnd w:id="3"/>
  </w:p>
  <w:p>
    <w:r><w:t>Footer company B: </w:t></w:r>
    <w:bookmarkStart w:id="4" w:name="footer_company"/>
    <w:r><w:t>placeholder B</w:t></w:r>
    <w:bookmarkEnd w:id="4"/>
  </w:p>
  <w:p>
    <w:r><w:t>Summary: prefix </w:t></w:r>
    <w:bookmarkStart w:id="5" w:name="footer_summary"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="5"/>
    <w:r><w:t> suffix</w:t></w:r>
  </w:p>
</w:ftr>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/header1.xml", header_xml));
    REQUIRE(write_archive_entry(archive, "word/footer1.xml", footer_xml));
    zip_close(archive);
}

void create_cli_bookmark_image_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="body_logo"/>
      <w:r><w:t>body placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
</Relationships>
)";
    constexpr auto header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>header before</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo"/>
    <w:r><w:t>header placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/header1.xml", header_xml));
    zip_close(archive);
}

void create_cli_style_defaults_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE_FALSE(document.save());
}

void create_cli_paragraph_style_fixture(const fs::path &path,
                                        std::string_view style_id,
                                        std::string_view style_name,
                                        std::string_view based_on,
                                        bool paragraph_bidi) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto style = featherdoc::paragraph_style_definition{};
    style.name = std::string(style_name);
    style.based_on = std::string(based_on);
    style.is_quick_format = true;
    style.paragraph_bidi = paragraph_bidi;
    REQUIRE(document.ensure_paragraph_style(std::string(style_id), style));
    REQUIRE_FALSE(document.save());
}

void create_cli_style_existing_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="CustomBody" w:customStyle="1">
    <w:name w:val="Custom Body"/>
    <w:basedOn w:val="Normal"/>
    <w:qFormat/>
  </w:style>
  <w:style w:type="numbering" w:styleId="NumberedStyle">
    <w:name w:val="Numbered"/>
    <w:semiHidden/>
    <w:unhideWhenUsed/>
  </w:style>
  <w:style w:type="mystery" w:styleId="MysteryStyle">
    <w:name w:val="Mystery"/>
  </w:style>
</w:styles>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/styles.xml", styles_xml));
    zip_close(archive);
}

void create_cli_style_usage_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr><w:pStyle w:val="CustomBody"/></w:pPr>
      <w:r><w:rPr><w:rStyle w:val="Strong"/></w:rPr><w:t>alpha</w:t></w:r>
    </w:p>
    <w:p>
      <w:pPr><w:pStyle w:val="CustomBody"/></w:pPr>
      <w:r><w:t>beta</w:t></w:r>
      <w:r><w:rPr><w:rStyle w:val="Strong"/></w:rPr><w:t>gamma</w:t></w:r>
    </w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="ReportTable"/>
        <w:tblW w:w="0" w:type="auto"/>
      </w:tblPr>
      <w:tr><w:tc><w:p><w:r><w:t>cell</w:t></w:r></w:p></w:tc></w:tr>
    </w:tbl>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId3"/>
      <w:footerReference w:type="default" r:id="rId4"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="CustomBody" w:customStyle="1">
    <w:name w:val="Custom Body"/>
    <w:basedOn w:val="Normal"/>
    <w:qFormat/>
  </w:style>
  <w:style w:type="character" w:styleId="Strong">
    <w:name w:val="Strong"/>
  </w:style>
  <w:style w:type="table" w:styleId="ReportTable">
    <w:name w:val="Report Table"/>
  </w:style>
</w:styles>
)";
    constexpr auto header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:pPr><w:pStyle w:val="CustomBody"/></w:pPr>
    <w:r><w:t>header paragraph</w:t></w:r>
  </w:p>
  <w:tbl>
    <w:tblPr>
      <w:tblStyle w:val="ReportTable"/>
      <w:tblW w:w="0" w:type="auto"/>
    </w:tblPr>
    <w:tr><w:tc><w:p><w:r><w:t>header cell</w:t></w:r></w:p></w:tc></w:tr>
  </w:tbl>
</w:hdr>
)";
    constexpr auto footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:rPr><w:rStyle w:val="Strong"/></w:rPr><w:t>footer run</w:t></w:r>
  </w:p>
</w:ftr>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/styles.xml", styles_xml));
    REQUIRE(write_archive_entry(archive, "word/header1.xml", header_xml));
    REQUIRE(write_archive_entry(archive, "word/footer1.xml", footer_xml));
    zip_close(archive);
}

void create_cli_page_setup_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:pgSz w:w="12240" w:h="15840"/>
          <w:pgMar w:top="1440" w:right="1800" w:bottom="1440" w:left="1800" w:header="720" w:footer="720"/>
          <w:pgNumType w:start="5"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>portrait section</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>landscape section</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/>
      <w:pgMar w:top="720" w:right="1440" w:bottom="1080" w:left="1440" w:header="360" w:footer="540"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}

void create_cli_empty_document_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());
    REQUIRE_FALSE(document.save());
}

void create_cli_image_fixture(const fs::path &path) {
    remove_if_exists(path);

    const auto image_path =
        path.parent_path() / (path.stem().string() + "_fixture_image.png");
    remove_if_exists(image_path);
    write_binary_file(image_path, tiny_png_data());

    featherdoc::floating_image_options body_options;
    body_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    body_options.horizontal_offset_px = 24;
    body_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    body_options.vertical_offset_px = -8;
    body_options.behind_text = true;
    body_options.allow_overlap = false;
    body_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    body_options.wrap_distance_left_px = 8U;
    body_options.wrap_distance_right_px = 10U;
    body_options.wrap_distance_top_px = 6U;
    body_options.wrap_distance_bottom_px = 12U;
    body_options.crop = featherdoc::floating_image_crop{15U, 25U, 35U, 45U};

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_image(image_path, 20U, 10U));
    CHECK(body_template.append_floating_image(image_path, 20U, 10U, body_options));

    auto &header = document.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.add_run("cli image header").has_next());
    REQUIRE_FALSE(document.save());

    featherdoc::floating_image_options header_options;
    header_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    header_options.horizontal_offset_px = 40;
    header_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    header_options.vertical_offset_px = 12;
    header_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    header_options.wrap_distance_left_px = 5U;
    header_options.wrap_distance_right_px = 7U;
    header_options.crop = featherdoc::floating_image_crop{10U, 20U, 30U, 40U};

    featherdoc::Document reopened(path);
    REQUIRE_FALSE(reopened.open());

    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.append_floating_image(image_path, 30U, 15U, header_options));
    REQUIRE_FALSE(reopened.save());

    remove_if_exists(image_path);
}

auto run_cli(const std::vector<std::string> &arguments,
             const std::optional<fs::path> &captured_output = std::nullopt) -> int {
    const fs::path executable_path = cli_binary_path();
    REQUIRE(fs::exists(executable_path));

#if defined(_WIN32)
    std::string command = "\"";
    command += shell_quote(executable_path.string());
#else
    std::string command = shell_quote(executable_path.string());
#endif
    for (const auto &argument : arguments) {
        command += ' ';
        command += shell_quote(argument);
    }

    if (captured_output.has_value()) {
        remove_if_exists(*captured_output);
        command += " > ";
        command += shell_quote(captured_output->string());
        command += " 2>&1";
    }

#if defined(_WIN32)
    command += '"';
#endif

    return normalize_system_status(std::system(command.c_str()));
}
} // namespace

TEST_CASE("cli section commands modify layout end to end") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_sections_source.docx";
    const fs::path inserted = working_directory / "cli_sections_inserted.docx";
    const fs::path copied = working_directory / "cli_sections_copied.docx";
    const fs::path moved = working_directory / "cli_sections_moved.docx";
    const fs::path removed = working_directory / "cli_sections_removed.docx";
    const fs::path inspect_output = working_directory / "cli_sections_inspect.txt";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(copied);
    remove_if_exists(moved);
    remove_if_exists(removed);
    remove_if_exists(inspect_output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"inspect-sections", source.string()}, inspect_output), 0);

    const auto inspect_text = read_text_file(inspect_output);
    CHECK_NE(inspect_text.find("sections: 3"), std::string::npos);
    CHECK_NE(inspect_text.find("headers: 2"), std::string::npos);
    CHECK_NE(inspect_text.find("footers: 1"), std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[0]: header(default=yes, first=no, even=no) "
                 "footer(default=no, first=no, even=no)"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[1]: header(default=yes, first=no, even=no) "
                 "footer(default=no, first=yes, even=no)"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[2]: header(default=no, first=no, even=no) "
                 "footer(default=no, first=no, even=no)"),
             std::string::npos);

    CHECK_EQ(run_cli({"insert-section", source.string(), "1", "--no-inherit", "--output",
                      inserted.string()}),
             0);

    featherdoc::Document unchanged_source(source);
    REQUIRE_FALSE(unchanged_source.open());
    CHECK_EQ(unchanged_source.section_count(), 3);

    featherdoc::Document inserted_doc(inserted);
    REQUIRE_FALSE(inserted_doc.open());
    CHECK_EQ(inserted_doc.section_count(), 4);
    CHECK_EQ(inserted_doc.header_count(), 2);
    CHECK_EQ(inserted_doc.footer_count(), 1);
    CHECK_FALSE(inserted_doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(inserted_doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());

    CHECK_EQ(run_cli({"copy-section-layout", inserted.string(), "1", "2", "--output",
                      copied.string()}),
             0);

    featherdoc::Document copied_doc(copied);
    REQUIRE_FALSE(copied_doc.open());
    CHECK_EQ(copied_doc.section_count(), 4);
    CHECK_EQ(copied_doc.header_count(), 2);
    CHECK_EQ(copied_doc.footer_count(), 1);
    CHECK_EQ(copied_doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(copied_doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK_EQ(run_cli({"move-section", copied.string(), "3", "0", "--output",
                      moved.string()}),
             0);

    featherdoc::Document moved_doc(moved);
    REQUIRE_FALSE(moved_doc.open());
    CHECK_EQ(moved_doc.section_count(), 4);
    CHECK_EQ(moved_doc.header_count(), 2);
    CHECK_EQ(moved_doc.footer_count(), 1);
    CHECK_FALSE(moved_doc.section_header_paragraphs(0).has_next());
    CHECK_EQ(moved_doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(moved_doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(moved_doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_EQ(collect_non_empty_document_text(moved_doc),
             "section 2 body\nsection 0 body\nsection 1 body\n");

    CHECK_EQ(run_cli({"remove-section", moved.string(), "3", "--output",
                      removed.string()}),
             0);

    featherdoc::Document removed_doc(removed);
    REQUIRE_FALSE(removed_doc.open());
    CHECK_EQ(removed_doc.section_count(), 3);
    CHECK_EQ(removed_doc.header_count(), 2);
    CHECK_EQ(removed_doc.footer_count(), 1);
    CHECK_FALSE(removed_doc.section_header_paragraphs(0).has_next());
    CHECK_EQ(removed_doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(removed_doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(removed_doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_EQ(collect_non_empty_document_text(removed_doc),
             "section 2 body\nsection 0 body\nsection 1 body\n");

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(copied);
    remove_if_exists(moved);
    remove_if_exists(removed);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli can show and replace section header footer text") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_section_text_source.docx";
    const fs::path header_output = working_directory / "cli_section_header.txt";
    const fs::path text_source = working_directory / "cli_section_text_input.txt";
    const fs::path header_updated = working_directory / "cli_section_header_updated.docx";
    const fs::path shown_even_header = working_directory / "cli_section_even_header.txt";
    const fs::path footer_updated = working_directory / "cli_section_footer_updated.docx";
    const fs::path shown_footer = working_directory / "cli_section_footer.txt";

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(text_source);
    remove_if_exists(header_updated);
    remove_if_exists(shown_even_header);
    remove_if_exists(footer_updated);
    remove_if_exists(shown_footer);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"show-section-header", source.string(), "1"}, header_output), 0);
    CHECK_EQ(read_text_file(header_output), std::string("section 1 header\n"));

    {
        std::ofstream stream(text_source);
        REQUIRE(stream.good());
        stream << "alpha\nbeta\n";
    }

    CHECK_EQ(run_cli({"set-section-header", source.string(), "2", "--kind", "even",
                      "--text-file", text_source.string(), "--output",
                      header_updated.string()}),
             0);
    CHECK_EQ(run_cli({"show-section-header", header_updated.string(), "2", "--kind",
                      "even"},
                     shown_even_header),
             0);
    CHECK_EQ(read_text_file(shown_even_header), std::string("alpha\nbeta\n"));

    featherdoc::Document header_doc(header_updated);
    REQUIRE_FALSE(header_doc.open());
    CHECK_EQ(collect_part_lines(header_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"alpha", "beta"}));

    CHECK_EQ(run_cli({"set-section-footer", header_updated.string(), "0", "--text",
                      "front footer", "--output", footer_updated.string()}),
             0);
    CHECK_EQ(run_cli({"show-section-footer", footer_updated.string(), "0"},
                     shown_footer),
             0);
    CHECK_EQ(read_text_file(shown_footer), std::string("front footer\n"));

    featherdoc::Document footer_doc(footer_updated);
    REQUIRE_FALSE(footer_doc.open());
    CHECK_EQ(collect_part_lines(footer_doc.section_footer_paragraphs(0)),
             std::vector<std::string>({"front footer"}));
    CHECK_EQ(collect_part_lines(footer_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"alpha", "beta"}));

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(text_source);
    remove_if_exists(header_updated);
    remove_if_exists(shown_even_header);
    remove_if_exists(footer_updated);
    remove_if_exists(shown_footer);
}

TEST_CASE("cli inspect and show commands support json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_sections_json_source.docx";
    const fs::path inspect_output = working_directory / "cli_sections_json_inspect.txt";
    const fs::path shown_header = working_directory / "cli_sections_json_header.txt";
    const fs::path shown_missing_footer =
        working_directory / "cli_sections_json_missing_footer.txt";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(shown_header);
    remove_if_exists(shown_missing_footer);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"inspect-sections", source.string(), "--json"}, inspect_output), 0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"sections\":3,\"headers\":2,\"footers\":1,\"section_layouts\":["
            "{\"index\":0,\"header\":{\"default\":true,\"first\":false,\"even\":false},"
            "\"footer\":{\"default\":false,\"first\":false,\"even\":false}},"
            "{\"index\":1,\"header\":{\"default\":true,\"first\":false,\"even\":false},"
            "\"footer\":{\"default\":false,\"first\":true,\"even\":false}},"
            "{\"index\":2,\"header\":{\"default\":false,\"first\":false,\"even\":false},"
            "\"footer\":{\"default\":false,\"first\":false,\"even\":false}}]}\n"});

    CHECK_EQ(run_cli({"show-section-header", source.string(), "1", "--json"},
                     shown_header),
             0);
    CHECK_EQ(read_text_file(shown_header),
             std::string{
                 "{\"part\":\"header\",\"section\":1,\"kind\":\"default\","
                 "\"present\":true,\"paragraphs\":[\"section 1 header\"]}\n"});

    CHECK_EQ(run_cli({"show-section-footer", source.string(), "2", "--json"},
                     shown_missing_footer),
             0);
    CHECK_EQ(read_text_file(shown_missing_footer),
             std::string{
                 "{\"part\":\"footer\",\"section\":2,\"kind\":\"default\","
                 "\"present\":false,\"paragraphs\":[]}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(shown_header);
    remove_if_exists(shown_missing_footer);
}

TEST_CASE("cli inspect-styles lists the default catalog") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_defaults_source.docx";
    const fs::path output = working_directory / "cli_styles_defaults_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("styles: 9"), std::string::npos);
    CHECK_NE(inspect_text.find("id=Normal name=Normal type=paragraph kind=paragraph"),
             std::string::npos);
    CHECK_NE(inspect_text.find("numbering=none"), std::string::npos);
    CHECK_NE(inspect_text.find("id=DefaultParagraphFont"), std::string::npos);
    CHECK_NE(inspect_text.find("semi_hidden=yes unhide_when_used=yes"),
             std::string::npos);
    CHECK_NE(inspect_text.find("id=TableGrid"), std::string::npos);
    CHECK_NE(inspect_text.find("based_on=TableNormal"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-styles supports single-style json output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_existing_source.docx";
    const fs::path style_output = working_directory / "cli_styles_single.json";
    const fs::path missing_output = working_directory / "cli_styles_missing.json";

    remove_if_exists(source);
    remove_if_exists(style_output);
    remove_if_exists(missing_output);

    create_cli_style_existing_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "CustomBody",
                      "--json"},
                     style_output),
             0);
    CHECK_EQ(
        read_text_file(style_output),
        std::string{
            "{\"style\":{\"style_id\":\"CustomBody\",\"name\":\"Custom Body\","
            "\"based_on\":\"Normal\",\"kind\":\"paragraph\",\"type\":\"paragraph\","
            "\"numbering\":null,"
            "\"is_default\":false,\"is_custom\":true,\"is_semi_hidden\":false,"
            "\"is_unhide_when_used\":false,\"is_quick_format\":true}}\n"});

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "MissingStyle",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-styles\""), std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find("\"detail\":\"style id 'MissingStyle' was not found in word/styles.xml\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/styles.xml\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(style_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli inspect-styles reports style usage for a single style") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_usage_source.docx";
    const fs::path json_output = working_directory / "cli_styles_usage.json";
    const fs::path text_output = working_directory / "cli_styles_usage.txt";
    const fs::path table_output = working_directory / "cli_styles_usage_table.txt";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(table_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "CustomBody",
                      "--usage", "--json"},
                     json_output),
             0);
    CHECK_EQ(
        read_text_file(json_output),
        std::string{
            "{\"style\":{\"style_id\":\"CustomBody\",\"name\":\"Custom Body\","
            "\"based_on\":\"Normal\",\"kind\":\"paragraph\",\"type\":\"paragraph\","
            "\"numbering\":null,"
            "\"is_default\":false,\"is_custom\":true,\"is_semi_hidden\":false,"
            "\"is_unhide_when_used\":false,\"is_quick_format\":true},"
            "\"usage\":{\"style_id\":\"CustomBody\",\"paragraph_count\":3,"
            "\"run_count\":0,\"table_count\":0,\"total_count\":3,"
            "\"body\":{\"paragraph_count\":2,\"run_count\":0,\"table_count\":0,"
            "\"total_count\":2},"
            "\"header\":{\"paragraph_count\":1,\"run_count\":0,\"table_count\":0,"
            "\"total_count\":1},"
            "\"footer\":{\"paragraph_count\":0,\"run_count\":0,\"table_count\":0,"
            "\"total_count\":0},\"hits\":["
            "{\"part\":\"body\",\"entry_name\":\"word/document.xml\","
            "\"section_index\":0,\"ordinal\":1,\"kind\":\"paragraph\",\"references\":[]},"
            "{\"part\":\"body\",\"entry_name\":\"word/document.xml\","
            "\"section_index\":0,\"ordinal\":2,\"kind\":\"paragraph\",\"references\":[]},"
            "{\"part\":\"header\",\"entry_name\":\"word/header1.xml\","
            "\"section_index\":null,\"ordinal\":1,\"kind\":\"paragraph\",\"references\":["
            "{\"section_index\":0,\"reference_kind\":\"default\"}]}]}}\n"});

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "Strong",
                      "--usage"},
                     text_output),
             0);
    const auto usage_text = read_text_file(text_output);
    CHECK_NE(usage_text.find("style_id: Strong"), std::string::npos);
    CHECK_NE(usage_text.find("usage_paragraphs: 0"), std::string::npos);
    CHECK_NE(usage_text.find("usage_runs: 3"), std::string::npos);
    CHECK_NE(usage_text.find("usage_tables: 0"), std::string::npos);
    CHECK_NE(usage_text.find("usage_total: 3"), std::string::npos);
    CHECK_NE(usage_text.find("usage_body_runs: 2"), std::string::npos);
    CHECK_NE(usage_text.find("usage_body_total: 2"), std::string::npos);
    CHECK_NE(usage_text.find("usage_header_total: 0"), std::string::npos);
    CHECK_NE(usage_text.find("usage_footer_runs: 1"), std::string::npos);
    CHECK_NE(usage_text.find("usage_footer_total: 1"), std::string::npos);
    CHECK_NE(usage_text.find("usage_hits: 3"), std::string::npos);
    CHECK_NE(usage_text.find(
                 "usage_hit[0]: part=body entry_name=word/document.xml ordinal=1 section_index=0 kind=run references=0"),
             std::string::npos);
    CHECK_NE(usage_text.find(
                 "usage_hit[1]: part=body entry_name=word/document.xml ordinal=2 section_index=0 kind=run references=0"),
             std::string::npos);
    CHECK_NE(usage_text.find(
                 "usage_hit[2]: part=footer entry_name=word/footer1.xml ordinal=1 section_index=- kind=run references=1 ref[0]=section:0,kind:default"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--style", "ReportTable",
                      "--usage"},
                     table_output),
             0);
    const auto table_text = read_text_file(table_output);
    CHECK_NE(table_text.find("style_id: ReportTable"), std::string::npos);
    CHECK_NE(table_text.find("usage_paragraphs: 0"), std::string::npos);
    CHECK_NE(table_text.find("usage_runs: 0"), std::string::npos);
    CHECK_NE(table_text.find("usage_tables: 2"), std::string::npos);
    CHECK_NE(table_text.find("usage_total: 2"), std::string::npos);
    CHECK_NE(table_text.find("usage_body_tables: 1"), std::string::npos);
    CHECK_NE(table_text.find("usage_header_tables: 1"), std::string::npos);
    CHECK_NE(table_text.find("usage_footer_tables: 0"), std::string::npos);
    CHECK_NE(table_text.find("usage_header_total: 1"), std::string::npos);
    CHECK_NE(table_text.find("usage_footer_total: 0"), std::string::npos);
    CHECK_NE(table_text.find("usage_hits: 2"), std::string::npos);
    CHECK_NE(table_text.find(
                 "usage_hit[0]: part=body entry_name=word/document.xml ordinal=1 section_index=0 kind=table references=0"),
             std::string::npos);
    CHECK_NE(table_text.find(
                 "usage_hit[1]: part=header entry_name=word/header1.xml ordinal=1 section_index=- kind=table references=1 ref[0]=section:0,kind:default"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(table_output);
}

TEST_CASE("cli inspect-styles rejects usage without a single style target") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_usage_parse_source.docx";
    const fs::path output = working_directory / "cli_styles_usage_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--usage", "--json"}, output), 2);
    const auto parse_json = read_text_file(output);
    CHECK_NE(parse_json.find("\"command\":\"inspect-styles\""), std::string::npos);
    CHECK_NE(parse_json.find("\"message\":\"--usage requires --style\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-numbering lists custom numbering definitions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_source.docx";
    const fs::path output = working_directory / "cli_numbering_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "LegalOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 3U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = document.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(document.set_paragraph_numbering(paragraph, *numbering_id));
    REQUIRE(paragraph.add_run("legal heading").has_next());
    REQUIRE_FALSE(document.save());

    const auto numbering_xml = read_docx_entry(source, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});
    const auto numbering_instance = numbering_root.child("w:num");
    REQUIRE(numbering_instance != pugi::xml_node{});
    const auto num_id = std::string{numbering_instance.attribute("w:numId").value()};

    CHECK_EQ(run_cli({"inspect-numbering", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("definitions: 1"), std::string::npos);
    CHECK_NE(inspect_text.find("definition[0]: id=" + std::to_string(*numbering_id) +
                                   " name=LegalOutline levels=2 instances=" + num_id),
             std::string::npos);
    CHECK_NE(inspect_text.find("level[0]: kind=decimal start=3 text_pattern=%1."),
             std::string::npos);
    CHECK_NE(inspect_text.find("level[1]: kind=decimal start=1 text_pattern=%1.%2."),
             std::string::npos);
    CHECK_NE(inspect_text.find("instance[" + num_id + "]: override_count=0"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-numbering supports single-definition json output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_single_source.docx";
    const fs::path definition_output =
        working_directory / "cli_numbering_single_output.json";
    const fs::path missing_output =
        working_directory / "cli_numbering_missing_output.json";

    remove_if_exists(source);
    remove_if_exists(definition_output);
    remove_if_exists(missing_output);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "PolicyOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 5U, 0U, "(%1)"},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };

    const auto numbering_id = document.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(document.set_paragraph_numbering(paragraph, *numbering_id));
    REQUIRE(paragraph.add_run("policy item").has_next());
    REQUIRE_FALSE(document.save());

    const auto numbering_xml = read_docx_entry(source, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});
    const auto numbering_instance = numbering_root.child("w:num");
    REQUIRE(numbering_instance != pugi::xml_node{});
    const auto num_id = std::string{numbering_instance.attribute("w:numId").value()};

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--definition",
                      std::to_string(*numbering_id), "--json"},
                     definition_output),
             0);
    CHECK_EQ(
        read_text_file(definition_output),
        std::string{
            "{\"definition\":{\"definition_id\":"} +
            std::to_string(*numbering_id) +
            ",\"name\":\"PolicyOutline\",\"levels\":[{\"level\":0,\"kind\":\"decimal\","
            "\"start\":5,\"text_pattern\":\"(%1)\"},{\"level\":1,\"kind\":\"bullet\","
            "\"start\":1,\"text_pattern\":\"o\"}],\"instance_ids\":[" +
            num_id + "],\"instances\":[{\"instance_id\":" + num_id +
            ",\"level_overrides\":[]}]}}\n");

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--definition", "999",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-numbering\""), std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find(
                 "\"detail\":\"numbering definition id '999' was not found in "
                 "word/numbering.xml\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/numbering.xml\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(definition_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli inspect-numbering reports instance overrides for restarted lists") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_restart_source.docx";
    const fs::path list_output = working_directory / "cli_numbering_restart_output.txt";
    const fs::path json_output = working_directory / "cli_numbering_restart_output.json";

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(json_output);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.create_empty());

    auto first_item = document.paragraphs();
    REQUIRE(first_item.has_next());
    REQUIRE(document.set_paragraph_list(first_item, featherdoc::list_kind::decimal));
    REQUIRE(first_item.add_run("first item").has_next());

    auto spacer = first_item.insert_paragraph_after("restart here");
    REQUIRE(spacer.has_next());

    auto restarted_item = spacer.insert_paragraph_after("");
    REQUIRE(restarted_item.has_next());
    REQUIRE(document.restart_paragraph_list(restarted_item, featherdoc::list_kind::decimal));
    REQUIRE(restarted_item.add_run("restarted item").has_next());

    const auto definitions = document.list_numbering_definitions();
    REQUIRE_FALSE(document.last_error());
    const auto managed_definition =
        std::find_if(definitions.begin(), definitions.end(), [](const auto &summary) {
            return summary.name == "FeatherDocDecimalList";
        });
    REQUIRE(managed_definition != definitions.end());
    REQUIRE_EQ(managed_definition->instance_ids.size(), 2U);

    const auto definition_id = managed_definition->definition_id;
    const auto first_num_id = std::to_string(managed_definition->instance_ids[0]);
    const auto restarted_num_id = std::to_string(managed_definition->instance_ids[1]);

    REQUIRE_FALSE(document.save());

    CHECK_EQ(run_cli({"inspect-numbering", source.string()}, list_output), 0);
    const auto inspect_text = read_text_file(list_output);
    CHECK_NE(inspect_text.find("id=" + std::to_string(definition_id) +
                                   " name=FeatherDocDecimalList"),
             std::string::npos);
    CHECK_NE(inspect_text.find("instances=" + first_num_id + "," + restarted_num_id),
             std::string::npos);
    CHECK_NE(inspect_text.find("instance[" + first_num_id + "]: override_count=0"),
             std::string::npos);
    CHECK_NE(inspect_text.find("instance[" + restarted_num_id + "]: override_count=1"),
             std::string::npos);
    CHECK_NE(inspect_text.find("override[0]: level=0 start_override=1"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--definition",
                      std::to_string(definition_id), "--json"},
                     json_output),
             0);
    const auto inspect_json = read_text_file(json_output);
    CHECK_NE(inspect_json.find("\"definition_id\":" + std::to_string(definition_id)),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"instance_ids\":[" + first_num_id + "," + restarted_num_id +
                               "]"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"instances\":[{\"instance_id\":" + first_num_id +
                               ",\"level_overrides\":[]},{\"instance_id\":" +
                               restarted_num_id + ",\"level_overrides\":["),
             std::string::npos);
    CHECK_NE(inspect_json.find(
                 "{\"level\":0,\"start_override\":1,\"level_definition\":null}"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-numbering filters a single numbering instance") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_instance_source.docx";
    const fs::path text_output = working_directory / "cli_numbering_instance_output.txt";
    const fs::path json_output = working_directory / "cli_numbering_instance_output.json";
    const fs::path missing_output = working_directory / "cli_numbering_instance_missing.json";

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
    remove_if_exists(missing_output);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.create_empty());

    auto first_item = document.paragraphs();
    REQUIRE(first_item.has_next());
    REQUIRE(document.set_paragraph_list(first_item, featherdoc::list_kind::decimal));
    REQUIRE(first_item.add_run("first item").has_next());

    auto spacer = first_item.insert_paragraph_after("restart here");
    REQUIRE(spacer.has_next());

    auto restarted_item = spacer.insert_paragraph_after("");
    REQUIRE(restarted_item.has_next());
    REQUIRE(document.restart_paragraph_list(restarted_item, featherdoc::list_kind::decimal));
    REQUIRE(restarted_item.add_run("restarted item").has_next());

    const auto definitions = document.list_numbering_definitions();
    REQUIRE_FALSE(document.last_error());
    const auto managed_definition =
        std::find_if(definitions.begin(), definitions.end(), [](const auto &summary) {
            return summary.name == "FeatherDocDecimalList";
        });
    REQUIRE(managed_definition != definitions.end());
    REQUIRE_EQ(managed_definition->instance_ids.size(), 2U);

    const auto definition_id = std::to_string(managed_definition->definition_id);
    const auto restarted_num_id = std::to_string(managed_definition->instance_ids[1]);

    REQUIRE_FALSE(document.save());

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--instance", restarted_num_id},
                     text_output),
             0);
    const auto inspect_text = read_text_file(text_output);
    CHECK_NE(inspect_text.find("definition_id: " + definition_id), std::string::npos);
    CHECK_NE(inspect_text.find("name: FeatherDocDecimalList"), std::string::npos);
    CHECK_NE(inspect_text.find("instance[" + restarted_num_id + "]: override_count=1"),
             std::string::npos);
    CHECK_NE(inspect_text.find("override[0]: level=0 start_override=1"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--instance", restarted_num_id,
                      "--json"},
                     json_output),
             0);
    CHECK_EQ(
        read_text_file(json_output),
        std::string{
            "{\"instance\":{\"definition_id\":"} +
            definition_id +
            ",\"definition_name\":\"FeatherDocDecimalList\",\"instance\":{\"instance_id\":" +
            restarted_num_id +
            ",\"level_overrides\":[{\"level\":0,\"start_override\":1,"
            "\"level_definition\":null}]}}}\n");

    CHECK_EQ(run_cli({"inspect-numbering", source.string(), "--instance", "999", "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-numbering\""), std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find(
                 "\"detail\":\"numbering instance id '999' was not found in "
                 "word/numbering.xml\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/numbering.xml\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli set-paragraph-style-numbering links a custom numbering definition to a style") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_style_numbering_source.docx";
    const fs::path updated = working_directory / "cli_style_numbering_updated.docx";
    const fs::path output = working_directory / "cli_style_numbering_output.json";
    const fs::path inspect_output =
        working_directory / "cli_style_numbering_inspect_output.json";
    const fs::path inspect_text_output =
        working_directory / "cli_style_numbering_inspect_output.txt";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
    remove_if_exists(inspect_text_output);

    create_cli_paragraph_style_fixture(source, "LegalHeading", "Legal Heading",
                                       "Heading1", false);

    CHECK_EQ(run_cli({"set-paragraph-style-numbering",
                      source.string(),
                      "LegalHeading",
                      "--definition-name",
                      "LegalHeadingOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--numbering-level",
                      "1:decimal:1:%1.%2.",
                      "--style-level",
                      "1",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-paragraph-style-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"LegalHeading\",\"definition_name\":\"LegalHeadingOutline\","
            "\"style_level\":1,\"definition_levels\":[{\"level\":0,\"kind\":\"decimal\","
            "\"start\":1,\"text_pattern\":\"%1.\"},{\"level\":1,\"kind\":\"decimal\","
            "\"start\":1,\"text_pattern\":\"%1.%2.\"}]}\n"});

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto style = find_style_xml_node(styles_root, "LegalHeading");
    REQUIRE(style != pugi::xml_node{});
    const auto style_num_pr = style.child("w:pPr").child("w:numPr");
    REQUIRE(style_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{style_num_pr.child("w:ilvl").attribute("w:val").value()},
             "1");
    CHECK_NE(std::string_view{style_num_pr.child("w:numId").attribute("w:val").value()},
             "");

    const auto numbering_xml = read_docx_entry(updated, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});

    const auto abstract_num =
        find_numbering_abstract_xml_node(numbering_root, "LegalHeadingOutline");
    REQUIRE(abstract_num != pugi::xml_node{});

    const auto level_zero = find_numbering_level_xml_node(abstract_num, "0");
    REQUIRE(level_zero != pugi::xml_node{});
    CHECK_EQ(std::string_view{level_zero.child("w:numFmt").attribute("w:val").value()},
             "decimal");
    CHECK_EQ(std::string_view{level_zero.child("w:start").attribute("w:val").value()},
             "1");
    CHECK_EQ(std::string_view{level_zero.child("w:lvlText").attribute("w:val").value()},
             "%1.");

    const auto level_one = find_numbering_level_xml_node(abstract_num, "1");
    REQUIRE(level_one != pugi::xml_node{});
    CHECK_EQ(std::string_view{level_one.child("w:numFmt").attribute("w:val").value()},
             "decimal");
    CHECK_EQ(std::string_view{level_one.child("w:start").attribute("w:val").value()},
             "1");
    CHECK_EQ(std::string_view{level_one.child("w:lvlText").attribute("w:val").value()},
             "%1.%2.");

    CHECK_EQ(run_cli({"inspect-styles", updated.string(), "--style", "LegalHeading", "--json"},
                     inspect_output),
             0);
    const auto style_num_id =
        std::string{style_num_pr.child("w:numId").attribute("w:val").value()};
    const auto definition_id = std::string{abstract_num.attribute("w:abstractNumId").value()};
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style\":{\"style_id\":\"LegalHeading\",\"name\":\"Legal Heading\","
            "\"based_on\":\"Heading1\",\"kind\":\"paragraph\",\"type\":\"paragraph\","
            "\"numbering\":{\"num_id\":"} +
            style_num_id +
            ",\"level\":1,\"definition_id\":" + definition_id +
            ",\"definition_name\":\"LegalHeadingOutline\",\"instance\":{\"instance_id\":" +
            style_num_id +
            ",\"level_overrides\":[]}},"
            "\"is_default\":false,\"is_custom\":true,\"is_semi_hidden\":false,"
            "\"is_unhide_when_used\":false,\"is_quick_format\":true}}\n");

    CHECK_EQ(run_cli({"inspect-styles", updated.string(), "--style", "LegalHeading"},
                     inspect_text_output),
             0);
    const auto inspect_text = read_text_file(inspect_text_output);
    CHECK_NE(
        inspect_text.find("numbering: (level=1, num_id=" + style_num_id +
                          ", definition_id=" + definition_id +
                          ", definition_name=LegalHeadingOutline, override_count=0)"),
        std::string::npos);
    CHECK_NE(inspect_text.find("numbering_instance_id: " + style_num_id),
             std::string::npos);
    CHECK_NE(inspect_text.find("numbering_override_count: 0"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
    remove_if_exists(inspect_text_output);
}

TEST_CASE("cli clear-paragraph-style-numbering removes style numbering and keeps bidi") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_clear_style_numbering_source.docx";
    const fs::path numbered =
        working_directory / "cli_clear_style_numbering_numbered.docx";
    const fs::path cleared =
        working_directory / "cli_clear_style_numbering_cleared.docx";
    const fs::path output =
        working_directory / "cli_clear_style_numbering_output.json";

    remove_if_exists(source);
    remove_if_exists(numbered);
    remove_if_exists(cleared);
    remove_if_exists(output);

    create_cli_paragraph_style_fixture(source, "BodyNumbered", "Body Numbered",
                                       "Normal", true);

    CHECK_EQ(run_cli({"set-paragraph-style-numbering",
                      source.string(),
                      "BodyNumbered",
                      "--definition-name",
                      "BodyOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--output",
                      numbered.string()},
                     std::nullopt),
             0);

    const auto numbered_styles_xml = read_docx_entry(numbered, "word/styles.xml");
    pugi::xml_document numbered_styles_document;
    REQUIRE(numbered_styles_document.load_string(numbered_styles_xml.c_str()));
    const auto numbered_style = find_style_xml_node(
        numbered_styles_document.child("w:styles"), "BodyNumbered");
    REQUIRE(numbered_style != pugi::xml_node{});
    REQUIRE(numbered_style.child("w:pPr").child("w:numPr") != pugi::xml_node{});

    CHECK_EQ(run_cli({"clear-paragraph-style-numbering",
                      numbered.string(),
                      "BodyNumbered",
                      "--output",
                      cleared.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"clear-paragraph-style-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"BodyNumbered\"}\n"});

    const auto cleared_styles_xml = read_docx_entry(cleared, "word/styles.xml");
    pugi::xml_document cleared_styles_document;
    REQUIRE(cleared_styles_document.load_string(cleared_styles_xml.c_str()));
    const auto cleared_style =
        find_style_xml_node(cleared_styles_document.child("w:styles"), "BodyNumbered");
    REQUIRE(cleared_style != pugi::xml_node{});
    CHECK(cleared_style.child("w:pPr").child("w:numPr") == pugi::xml_node{});
    CHECK(cleared_style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(numbered);
    remove_if_exists(cleared);
    remove_if_exists(output);
}

TEST_CASE("cli set-paragraph-style-numbering reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_numbering_parse_source.docx";
    const fs::path output =
        working_directory / "cli_style_numbering_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_paragraph_style_fixture(source, "LegalHeading", "Legal Heading",
                                       "Heading1", false);

    CHECK_EQ(run_cli({"set-paragraph-style-numbering",
                      source.string(),
                      "LegalHeading",
                      "--definition-name",
                      "LegalHeadingOutline",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-paragraph-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected at least one "
            "--numbering-level <level>:<kind>:<start>:<text-pattern>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-page-setup lists all sections in text mode") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_page_setup_source.docx";
    const fs::path output = working_directory / "cli_page_setup_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_page_setup_fixture(source);

    CHECK_EQ(run_cli({"inspect-page-setup", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("sections: 2"), std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[0]: present=yes orientation=portrait width_twips=12240 "
                 "height_twips=15840"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "margins(top=1440, bottom=1440, left=1800, right=1800, "
                 "header=720, footer=720) page_number_start=5"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[1]: present=yes orientation=landscape width_twips=15840 "
                 "height_twips=12240"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "margins(top=720, bottom=1080, left=1440, right=1440, "
                 "header=360, footer=540) page_number_start=none"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-page-setup supports single-section json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_page_setup_single_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_page_setup_single.json";
    const fs::path empty_source = working_directory / "cli_page_setup_empty.docx";
    const fs::path empty_output = working_directory / "cli_page_setup_empty.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(empty_source);
    remove_if_exists(empty_output);

    create_cli_page_setup_fixture(source);
    create_cli_style_defaults_fixture(empty_source);

    CHECK_EQ(run_cli({"inspect-page-setup", source.string(), "--section", "1",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"section\":1,\"present\":true,\"page_setup\":{"
            "\"orientation\":\"landscape\",\"width_twips\":15840,"
            "\"height_twips\":12240,\"margins\":{\"top_twips\":720,"
            "\"bottom_twips\":1080,\"left_twips\":1440,\"right_twips\":1440,"
            "\"header_twips\":360,\"footer_twips\":540},"
            "\"page_number_start\":null}}\n"});

    CHECK_EQ(run_cli({"inspect-page-setup", empty_source.string(), "--section",
                      "0", "--json"},
                     empty_output),
             0);
    CHECK_EQ(read_text_file(empty_output),
             std::string{"{\"section\":0,\"present\":false,\"page_setup\":null}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(empty_source);
    remove_if_exists(empty_output);
}

TEST_CASE("cli set-section-page-setup updates an existing section and clears page numbering") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_page_setup_mutate_source.docx";
    const fs::path updated = working_directory / "cli_page_setup_mutated.docx";
    const fs::path output = working_directory / "cli_page_setup_mutated.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_page_setup_fixture(source);

    CHECK_EQ(run_cli({"set-section-page-setup",
                      source.string(),
                      "0",
                      "--orientation",
                      "landscape",
                      "--width",
                      "15840",
                      "--height",
                      "12240",
                      "--margin-top",
                      "720",
                      "--clear-page-number-start",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-section-page-setup\",\"ok\":true,"
            "\"in_place\":false,\"sections\":2,\"headers\":0,\"footers\":0,"
            "\"section\":0,\"page_setup\":{"
            "\"orientation\":\"landscape\",\"width_twips\":15840,"
            "\"height_twips\":12240,\"margins\":{\"top_twips\":720,"
            "\"bottom_twips\":1440,\"left_twips\":1800,\"right_twips\":1800,"
            "\"header_twips\":720,\"footer_twips\":720},"
            "\"page_number_start\":null}}\n"});

    featherdoc::Document updated_doc(updated);
    REQUIRE_FALSE(updated_doc.open());
    const auto page_setup = updated_doc.get_section_page_setup(0);
    REQUIRE(page_setup.has_value());
    CHECK_EQ(page_setup->orientation, featherdoc::page_orientation::landscape);
    CHECK_EQ(page_setup->width_twips, 15840U);
    CHECK_EQ(page_setup->height_twips, 12240U);
    CHECK_EQ(page_setup->margins.top_twips, 720U);
    CHECK_EQ(page_setup->margins.bottom_twips, 1440U);
    CHECK_EQ(page_setup->margins.left_twips, 1800U);
    CHECK_EQ(page_setup->margins.right_twips, 1800U);
    CHECK_EQ(page_setup->margins.header_twips, 720U);
    CHECK_EQ(page_setup->margins.footer_twips, 720U);
    CHECK_FALSE(page_setup->page_number_start.has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli set-section-page-setup materializes an implicit final section") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_page_setup_implicit_source.docx";
    const fs::path updated = working_directory / "cli_page_setup_implicit.docx";
    const fs::path output = working_directory / "cli_page_setup_implicit.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-section-page-setup",
                      source.string(),
                      "0",
                      "--orientation",
                      "portrait",
                      "--width",
                      "12240",
                      "--height",
                      "15840",
                      "--margin-top",
                      "1440",
                      "--margin-bottom",
                      "1440",
                      "--margin-left",
                      "1800",
                      "--margin-right",
                      "1800",
                      "--margin-header",
                      "720",
                      "--margin-footer",
                      "720",
                      "--page-number-start",
                      "3",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-section-page-setup\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"section\":0,\"page_setup\":{"
            "\"orientation\":\"portrait\",\"width_twips\":12240,"
            "\"height_twips\":15840,\"margins\":{\"top_twips\":1440,"
            "\"bottom_twips\":1440,\"left_twips\":1800,\"right_twips\":1800,"
            "\"header_twips\":720,\"footer_twips\":720},"
            "\"page_number_start\":3}}\n"});

    featherdoc::Document updated_doc(updated);
    REQUIRE_FALSE(updated_doc.open());
    const auto page_setup = updated_doc.get_section_page_setup(0);
    REQUIRE(page_setup.has_value());
    CHECK_EQ(page_setup->orientation, featherdoc::page_orientation::portrait);
    CHECK_EQ(page_setup->width_twips, 12240U);
    CHECK_EQ(page_setup->height_twips, 15840U);
    CHECK_EQ(page_setup->margins.top_twips, 1440U);
    CHECK_EQ(page_setup->margins.bottom_twips, 1440U);
    CHECK_EQ(page_setup->margins.left_twips, 1800U);
    CHECK_EQ(page_setup->margins.right_twips, 1800U);
    CHECK_EQ(page_setup->margins.header_twips, 720U);
    CHECK_EQ(page_setup->margins.footer_twips, 720U);
    REQUIRE(page_setup->page_number_start.has_value());
    CHECK_EQ(*page_setup->page_number_start, 3U);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-bookmarks lists selected part bookmark summaries as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_bookmarks_parts_source.docx";
    const fs::path output = working_directory / "cli_bookmarks_header.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"inspect-bookmarks", source.string(), "--part", "header",
                      "--index", "0", "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"part\":\"header\",\"part_index\":0,\"entry_name\":\"word/header1.xml\","
            "\"count\":3,\"bookmarks\":[{\"bookmark_name\":\"header_title\","
            "\"occurrence_count\":1,\"kind\":\"text\",\"is_duplicate\":false},"
            "{\"bookmark_name\":\"header_note\",\"occurrence_count\":1,"
            "\"kind\":\"block\",\"is_duplicate\":false},"
            "{\"bookmark_name\":\"header_rows\",\"occurrence_count\":1,"
            "\"kind\":\"table_rows\",\"is_duplicate\":false}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-bookmarks supports single-bookmark text output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_bookmarks_body_source.docx";
    const fs::path bookmark_output =
        working_directory / "cli_bookmarks_customer.txt";
    const fs::path missing_output =
        working_directory / "cli_bookmarks_missing.json";

    remove_if_exists(source);
    remove_if_exists(bookmark_output);
    remove_if_exists(missing_output);

    create_cli_body_template_validation_fixture(source);

    CHECK_EQ(run_cli({"inspect-bookmarks", source.string(), "--bookmark", "customer"},
                     bookmark_output),
             0);
    CHECK_EQ(read_text_file(bookmark_output),
             std::string{"part: body\n"
                         "entry_name: word/document.xml\n"
                         "bookmark_name: customer\n"
                         "occurrence_count: 2\n"
                         "kind: text\n"
                         "duplicate: yes\n"});

    CHECK_EQ(run_cli({"inspect-bookmarks", source.string(), "--bookmark",
                      "missing_slot", "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-bookmarks\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find(
                 "\"detail\":\"bookmark name 'missing_slot' was not found in "
                 "word/document.xml\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(bookmark_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli inspect-images lists selected body drawing images as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_body_source.docx";
    const fs::path output = working_directory / "cli_images_body.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    CHECK_EQ(run_cli({"inspect-images", source.string(), "--json"}, output), 0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"entry_name\":\"word/document.xml\""), std::string::npos);
    CHECK_NE(json.find("\"count\":2"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"inline\""), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"floating_options\":null"), std::string::npos);
    CHECK_NE(json.find("\"wrap_mode\":\"square\""), std::string::npos);
    CHECK_NE(json.find("\"crop\":{\"left_per_mille\":15,\"top_per_mille\":25,"
                       "\"right_per_mille\":35,\"bottom_per_mille\":45}"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-images supports single image text output for header parts") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_header_source.docx";
    const fs::path output = working_directory / "cli_images_header.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    CHECK_EQ(run_cli({"inspect-images", source.string(), "--part", "header",
                      "--index", "0", "--image", "0"},
                     output),
             0);

    const auto text = read_text_file(output);
    CHECK_NE(text.find("part: header\n"), std::string::npos);
    CHECK_NE(text.find("part_index: 0\n"), std::string::npos);
    CHECK_NE(text.find("entry_name: word/header1.xml\n"), std::string::npos);
    CHECK_NE(text.find("image_index: 0\n"), std::string::npos);
    CHECK_NE(text.find("placement: anchored\n"), std::string::npos);
    CHECK_NE(text.find("wrap_mode: square\n"), std::string::npos);
    CHECK_NE(text.find("crop_left_per_mille: 10\n"), std::string::npos);
    CHECK_NE(text.find("crop_bottom_per_mille: 40\n"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-images filters images by relationship id and image entry name") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_filter_source.docx";
    const fs::path output = working_directory / "cli_images_filter.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    const auto &anchored_image = images[1];

    CHECK_EQ(run_cli({"inspect-images",
                      source.string(),
                      "--relationship-id",
                      anchored_image.relationship_id,
                      "--image-entry-name",
                      anchored_image.entry_name,
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"filters\":{\"relationship_id\":\"" +
                           anchored_image.relationship_id +
                           "\",\"image_entry_name\":\"" + anchored_image.entry_name +
                           "\"}"),
             std::string::npos);
    CHECK_NE(json.find("\"count\":1"), std::string::npos);
    CHECK_NE(json.find("\"index\":" + std::to_string(anchored_image.index)),
             std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-images reports filtered single image misses") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_filter_miss_source.docx";
    const fs::path output = working_directory / "cli_images_filter_miss.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_images = header_template.drawing_images();
    REQUIRE_EQ(header_images.size(), 1U);
    const auto entry_name = std::string(header_template.entry_name());

    CHECK_EQ(run_cli({"inspect-images",
                      source.string(),
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--image",
                      std::to_string(header_images[0].index),
                      "--relationship-id",
                      "missing-rid",
                      "--json"},
                     output),
             1);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"inspect-images\""), std::string::npos);
    CHECK_NE(json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(json.find("\"message\":\"result out of range\""), std::string::npos);
    CHECK_NE(json.find("\"detail\":\"drawing image index 0 was not found in " +
                           entry_name + " for relationship_id 'missing-rid'\""),
             std::string::npos);
    CHECK_NE(json.find("\"entry\":\"" + entry_name + "\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-images reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_images_parse_source.docx";
    const fs::path output = working_directory / "cli_images_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    CHECK_EQ(run_cli({"inspect-images", source.string(), "--part",
                      "section-header", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"inspect-images\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "inspection requires --section <index>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli extract-image exports a filtered anchored body image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_extract_image_source.docx";
    const fs::path extracted = working_directory / "cli_extract_image.png";
    const fs::path output = working_directory / "cli_extract_image.json";

    remove_if_exists(source);
    remove_if_exists(extracted);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    const auto &anchored_image = images[1];

    CHECK_EQ(run_cli({"extract-image",
                      source.string(),
                      extracted.string(),
                      "--relationship-id",
                      anchored_image.relationship_id,
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"extract-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"output_path\":" + json_quote(extracted.string())),
             std::string::npos);
    CHECK_NE(json.find("\"filters\":{\"relationship_id\":\"" +
                           anchored_image.relationship_id + "\"}"),
             std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/png\""), std::string::npos);
    CHECK_EQ(read_binary_file(extracted), tiny_png_data());

    remove_if_exists(source);
    remove_if_exists(extracted);
    remove_if_exists(output);
}

TEST_CASE("cli extract-image requires an explicit image selector") {
    const fs::path working_directory = fs::current_path();
    const fs::path output = working_directory / "cli_extract_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"extract-image", "missing.docx", "exported.png", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"extract-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"extract-image requires --image "
            "<index>, --relationship-id <id>, or --image-entry-name <path>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-image replaces a filtered anchored body image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_replace_image_source.docx";
    const fs::path replacement = working_directory / "cli_replace_image_replacement.gif";
    const fs::path updated = working_directory / "cli_replace_image_updated.docx";
    const fs::path output = working_directory / "cli_replace_image.json";

    remove_if_exists(source);
    remove_if_exists(replacement);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_image_fixture(source);
    write_binary_file(replacement, tiny_gif_data());

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    const auto &anchored_image = images[1];

    CHECK_EQ(run_cli({"replace-image",
                      source.string(),
                      replacement.string(),
                      "--relationship-id",
                      anchored_image.relationship_id,
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"replacement_path\":" + json_quote(replacement.string())),
             std::string::npos);
    CHECK_NE(json.find("\"filters\":{\"relationship_id\":\"" +
                           anchored_image.relationship_id + "\"}"),
             std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/gif\""), std::string::npos);

    featherdoc::Document reopened(updated);
    CHECK_FALSE(reopened.open());
    const auto updated_images = reopened.drawing_images();
    REQUIRE_EQ(updated_images.size(), 2U);
    CHECK_EQ(updated_images[1].index, anchored_image.index);
    CHECK_EQ(updated_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_images[1].width_px, anchored_image.width_px);
    CHECK_EQ(updated_images[1].height_px, anchored_image.height_px);
    CHECK_EQ(updated_images[1].content_type, "image/gif");
    CHECK(updated_images[1].entry_name.ends_with(".gif"));
    CHECK_EQ(read_docx_entry(updated, updated_images[1].entry_name.c_str()),
             tiny_gif_data());

    remove_if_exists(source);
    remove_if_exists(replacement);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-image requires an explicit image selector") {
    const fs::path working_directory = fs::current_path();
    const fs::path output = working_directory / "cli_replace_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-image", "missing.docx", "replacement.gif", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"replace-image requires --image "
            "<index>, --relationship-id <id>, or --image-entry-name <path>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli remove-image removes a filtered anchored body image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_remove_image_source.docx";
    const fs::path updated = working_directory / "cli_remove_image_updated.docx";
    const fs::path output = working_directory / "cli_remove_image.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_image_fixture(source);

    featherdoc::Document doc(source);
    REQUIRE_FALSE(doc.open());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 2U);
    const auto &anchored_image = images[1];

    CHECK_EQ(run_cli({"remove-image",
                      source.string(),
                      "--relationship-id",
                      anchored_image.relationship_id,
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"remove-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"filters\":{\"relationship_id\":\"" +
                           anchored_image.relationship_id + "\"}"),
             std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/png\""), std::string::npos);

    featherdoc::Document reopened(updated);
    CHECK_FALSE(reopened.open());
    auto updated_body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(updated_body_template));
    const auto updated_images = updated_body_template.drawing_images();
    REQUIRE_EQ(updated_images.size(), 1U);
    CHECK_EQ(updated_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(updated_images[0].content_type, "image/png");

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli remove-image requires an explicit image selector") {
    const fs::path working_directory = fs::current_path();
    const fs::path output = working_directory / "cli_remove_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"remove-image", "missing.docx", "--json"}, output), 2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"remove-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"remove-image requires --image "
            "<index>, --relationship-id <id>, or --image-entry-name <path>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli append-image appends a scaled inline body image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_append_image_source.docx";
    const fs::path image = working_directory / "cli_append_image_fixture_image.png";
    const fs::path updated = working_directory / "cli_append_image_updated.docx";
    const fs::path output = working_directory / "cli_append_image.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_empty_document_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(run_cli({"append-image",
                      source.string(),
                      image.string(),
                      "--width",
                      "32",
                      "--height",
                      "16",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"image_path\":" + json_quote(image.string())),
             std::string::npos);
    CHECK_NE(json.find("\"floating\":false"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"inline\""), std::string::npos);
    CHECK_NE(json.find("\"width_px\":32"), std::string::npos);
    CHECK_NE(json.find("\"height_px\":16"), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/png\""), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].placement, featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(images[0].width_px, 32U);
    CHECK_EQ(images[0].height_px, 16U);
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_FALSE(images[0].floating_options.has_value());

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append-image materializes a section header floating image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_section_header_source.docx";
    const fs::path image =
        working_directory / "cli_append_section_header_fixture_image.png";
    const fs::path updated =
        working_directory / "cli_append_section_header_updated.docx";
    const fs::path output =
        working_directory / "cli_append_section_header.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_empty_document_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(
        run_cli({"append-image",
                 source.string(),
                 image.string(),
                 "--part",
                 "section-header",
                 "--section",
                 "0",
                 "--floating",
                 "--width",
                 "30",
                 "--height",
                 "15",
                 "--horizontal-reference",
                 "page",
                 "--horizontal-offset",
                 "40",
                 "--vertical-reference",
                 "margin",
                 "--vertical-offset",
                 "12",
                 "--behind-text",
                 "true",
                 "--allow-overlap",
                 "false",
                 "--wrap-mode",
                 "square",
                 "--wrap-distance-left",
                 "5",
                 "--wrap-distance-right",
                 "7",
                 "--crop-left",
                 "10",
                 "--crop-top",
                 "20",
                 "--crop-right",
                 "30",
                 "--crop-bottom",
                 "40",
                 "--output",
                 updated.string(),
                 "--json"},
                output),
        0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-image\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"floating\":true"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_reference\":\"page\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_offset_px\":40"), std::string::npos);
    CHECK_NE(json.find("\"vertical_reference\":\"margin\""), std::string::npos);
    CHECK_NE(json.find("\"vertical_offset_px\":12"), std::string::npos);
    CHECK_NE(json.find("\"behind_text\":true"), std::string::npos);
    CHECK_NE(json.find("\"allow_overlap\":false"), std::string::npos);
    CHECK_NE(json.find("\"wrap_mode\":\"square\""), std::string::npos);
    CHECK_NE(json.find("\"crop\":{\"left_per_mille\":10,\"top_per_mille\":20,"
                       "\"right_per_mille\":30,\"bottom_per_mille\":40}"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto images = header_template.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(images[0].width_px, 30U);
    CHECK_EQ(images[0].height_px, 15U);
    REQUIRE(images[0].floating_options.has_value());
    CHECK_EQ(images[0].floating_options->horizontal_reference,
             featherdoc::floating_image_horizontal_reference::page);
    CHECK_EQ(images[0].floating_options->horizontal_offset_px, 40);
    CHECK_EQ(images[0].floating_options->vertical_reference,
             featherdoc::floating_image_vertical_reference::margin);
    CHECK_EQ(images[0].floating_options->vertical_offset_px, 12);
    CHECK(images[0].floating_options->behind_text);
    CHECK_FALSE(images[0].floating_options->allow_overlap);
    CHECK_EQ(images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(images[0].floating_options->wrap_distance_left_px, 5U);
    CHECK_EQ(images[0].floating_options->wrap_distance_right_px, 7U);
    REQUIRE(images[0].floating_options->crop.has_value());
    CHECK_EQ(images[0].floating_options->crop->left_per_mille, 10U);
    CHECK_EQ(images[0].floating_options->crop->top_per_mille, 20U);
    CHECK_EQ(images[0].floating_options->crop->right_per_mille, 30U);
    CHECK_EQ(images[0].floating_options->crop->bottom_per_mille, 40U);

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append-image requires width and height together") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_image_parse_source.docx";
    const fs::path image =
        working_directory / "cli_append_image_parse_fixture_image.png";
    const fs::path output =
        working_directory / "cli_append_image_parse.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(output);

    create_cli_empty_document_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(run_cli({"append-image",
                      source.string(),
                      image.string(),
                      "--width",
                      "32",
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{"{\"command\":\"append-image\",\"ok\":false,"
                         "\"stage\":\"parse\",\"message\":\"append-image "
                         "requires both --width <px> and --height <px> when "
                         "scaling\"}\n"});

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-image replaces a body bookmark with a scaled inline image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_image_source.docx";
    const fs::path image =
        working_directory / "cli_replace_bookmark_image_fixture.png";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_image_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_image.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(run_cli({"replace-bookmark-image",
                      source.string(),
                      "body_logo",
                      image.string(),
                      "--width",
                      "32",
                      "--height",
                      "16",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-image\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"image_path\":" + json_quote(image.string())),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"inline\""), std::string::npos);
    CHECK_NE(json.find("\"width_px\":32"), std::string::npos);
    CHECK_NE(json.find("\"height_px\":16"), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/png\""), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].placement, featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(images[0].width_px, 32U);
    CHECK_EQ(images[0].height_px, 16U);
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(read_docx_entry(updated, images[0].entry_name.c_str()), tiny_png_data());
    CHECK_FALSE(body_template.find_bookmark("body_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-image rejects floating layout options") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-image",
                      "missing.docx",
                      "logo",
                      "logo.png",
                      "--floating",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-bookmark-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"replace-bookmark-image does not "
            "accept floating layout options; use "
            "replace-bookmark-floating-image\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-floating-image replaces a section header bookmark with an anchored image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_floating_image_source.docx";
    const fs::path image =
        working_directory / "cli_replace_bookmark_floating_image_fixture.png";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_floating_image_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_floating_image.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(
        run_cli({"replace-bookmark-floating-image",
                 source.string(),
                 "header_logo",
                 image.string(),
                 "--part",
                 "section-header",
                 "--section",
                 "0",
                 "--width",
                 "30",
                 "--height",
                 "15",
                 "--horizontal-reference",
                 "page",
                 "--horizontal-offset",
                 "40",
                 "--vertical-reference",
                 "margin",
                 "--vertical-offset",
                 "12",
                 "--behind-text",
                 "true",
                 "--allow-overlap",
                 "false",
                 "--wrap-mode",
                 "square",
                 "--wrap-distance-left",
                 "5",
                 "--wrap-distance-right",
                 "7",
                 "--crop-left",
                 "10",
                 "--crop-top",
                 "20",
                 "--crop-right",
                 "30",
                 "--crop-bottom",
                 "40",
                 "--output",
                 updated.string(),
                 "--json"},
                output),
        0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-floating-image\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_reference\":\"page\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_offset_px\":40"), std::string::npos);
    CHECK_NE(json.find("\"vertical_reference\":\"margin\""), std::string::npos);
    CHECK_NE(json.find("\"vertical_offset_px\":12"), std::string::npos);
    CHECK_NE(json.find("\"behind_text\":true"), std::string::npos);
    CHECK_NE(json.find("\"allow_overlap\":false"), std::string::npos);
    CHECK_NE(json.find("\"wrap_mode\":\"square\""), std::string::npos);
    CHECK_NE(json.find("\"crop\":{\"left_per_mille\":10,\"top_per_mille\":20,"
                       "\"right_per_mille\":30,\"bottom_per_mille\":40}"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto images = header_template.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(images[0].width_px, 30U);
    CHECK_EQ(images[0].height_px, 15U);
    REQUIRE(images[0].floating_options.has_value());
    CHECK_EQ(images[0].floating_options->horizontal_reference,
             featherdoc::floating_image_horizontal_reference::page);
    CHECK_EQ(images[0].floating_options->horizontal_offset_px, 40);
    CHECK_EQ(images[0].floating_options->vertical_reference,
             featherdoc::floating_image_vertical_reference::margin);
    CHECK_EQ(images[0].floating_options->vertical_offset_px, 12);
    CHECK(images[0].floating_options->behind_text);
    CHECK_FALSE(images[0].floating_options->allow_overlap);
    CHECK_EQ(images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(images[0].floating_options->wrap_distance_left_px, 5U);
    CHECK_EQ(images[0].floating_options->wrap_distance_right_px, 7U);
    REQUIRE(images[0].floating_options->crop.has_value());
    CHECK_EQ(images[0].floating_options->crop->left_per_mille, 10U);
    CHECK_EQ(images[0].floating_options->crop->top_per_mille, 20U);
    CHECK_EQ(images[0].floating_options->crop->right_per_mille, 30U);
    CHECK_EQ(images[0].floating_options->crop->bottom_per_mille, 40U);
    CHECK_FALSE(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-floating-image requires width and height together") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_floating_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-floating-image",
                      "missing.docx",
                      "logo",
                      "logo.png",
                      "--width",
                      "30",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-bookmark-floating-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"replace-bookmark-floating-image "
            "requires both --width <px> and --height <px> when scaling\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli mutating commands support json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_mutation_json_source.docx";
    const fs::path inserted = working_directory / "cli_mutation_json_inserted.docx";
    const fs::path moved = working_directory / "cli_mutation_json_moved.docx";
    const fs::path header_updated =
        working_directory / "cli_mutation_json_header.docx";
    const fs::path insert_output = working_directory / "cli_mutation_insert.json";
    const fs::path move_output = working_directory / "cli_mutation_move.json";
    const fs::path set_header_output =
        working_directory / "cli_mutation_set_header.json";
    const fs::path parse_error_output =
        working_directory / "cli_mutation_parse_error.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(moved);
    remove_if_exists(header_updated);
    remove_if_exists(insert_output);
    remove_if_exists(move_output);
    remove_if_exists(set_header_output);
    remove_if_exists(parse_error_output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"insert-section", source.string(), "1", "--no-inherit", "--output",
                      inserted.string(), "--json"},
                     insert_output),
             0);
    CHECK_EQ(
        read_text_file(insert_output),
        std::string{
            "{\"command\":\"insert-section\",\"ok\":true,\"in_place\":false,"
            "\"sections\":4,\"headers\":2,\"footers\":1,\"section\":2,"
            "\"inherit_header_footer\":false}\n"});

    CHECK_EQ(run_cli({"move-section", inserted.string(), "3", "0", "--output",
                      moved.string(), "--json"},
                     move_output),
             0);
    CHECK_EQ(
        read_text_file(move_output),
        std::string{
            "{\"command\":\"move-section\",\"ok\":true,\"in_place\":false,"
            "\"sections\":4,\"headers\":2,\"footers\":1,\"source\":3,"
            "\"target\":0}\n"});

    CHECK_EQ(run_cli({"set-section-header", moved.string(), "2", "--kind", "even",
                      "--text", "json header", "--output",
                      header_updated.string(), "--json"},
                     set_header_output),
             0);
    CHECK_EQ(
        read_text_file(set_header_output),
        std::string{
            "{\"command\":\"set-section-header\",\"ok\":true,"
            "\"in_place\":false,\"sections\":4,\"headers\":3,\"footers\":1,"
            "\"part\":\"header\",\"section\":2,\"kind\":\"even\"}\n"});

    featherdoc::Document header_doc(header_updated);
    REQUIRE_FALSE(header_doc.open());
    CHECK_EQ(collect_part_lines(header_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"json header"}));

    CHECK_EQ(run_cli({"set-section-footer", source.string(), "0", "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-section-footer\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected --text <text> or "
            "--text-file <path>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(moved);
    remove_if_exists(header_updated);
    remove_if_exists(insert_output);
    remove_if_exists(move_output);
    remove_if_exists(set_header_output);
    remove_if_exists(parse_error_output);
}

TEST_CASE("cli can assign and remove section header footer references and parts") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_section_refs_source.docx";
    const fs::path header_assigned =
        working_directory / "cli_section_refs_header_assigned.docx";
    const fs::path footer_assigned =
        working_directory / "cli_section_refs_footer_assigned.docx";
    const fs::path header_detached =
        working_directory / "cli_section_refs_header_detached.docx";
    const fs::path footer_detached =
        working_directory / "cli_section_refs_footer_detached.docx";
    const fs::path header_pruned =
        working_directory / "cli_section_refs_header_pruned.docx";
    const fs::path footer_pruned =
        working_directory / "cli_section_refs_footer_pruned.docx";
    const fs::path assign_header_output =
        working_directory / "cli_section_refs_assign_header.json";
    const fs::path assign_footer_output =
        working_directory / "cli_section_refs_assign_footer.json";
    const fs::path remove_header_output =
        working_directory / "cli_section_refs_remove_header.json";
    const fs::path remove_footer_output =
        working_directory / "cli_section_refs_remove_footer.json";
    const fs::path remove_header_part_output =
        working_directory / "cli_section_refs_remove_header_part.json";
    const fs::path remove_footer_part_output =
        working_directory / "cli_section_refs_remove_footer_part.json";

    remove_if_exists(source);
    remove_if_exists(header_assigned);
    remove_if_exists(footer_assigned);
    remove_if_exists(header_detached);
    remove_if_exists(footer_detached);
    remove_if_exists(header_pruned);
    remove_if_exists(footer_pruned);
    remove_if_exists(assign_header_output);
    remove_if_exists(assign_footer_output);
    remove_if_exists(remove_header_output);
    remove_if_exists(remove_footer_output);
    remove_if_exists(remove_header_part_output);
    remove_if_exists(remove_footer_part_output);

    create_cli_reference_fixture(source);

    featherdoc::Document fixture_doc(source);
    REQUIRE_FALSE(fixture_doc.open());
    const auto shared_header_index =
        find_header_index_by_text(fixture_doc, "section 0 header");
    const auto removable_header_index =
        find_header_index_by_text(fixture_doc, "section 1 header");
    const auto shared_footer_index =
        find_footer_index_by_text(fixture_doc, "section 0 footer");
    const auto removable_footer_index =
        find_footer_index_by_text(fixture_doc, "section 1 first footer");

    CHECK_EQ(run_cli({"assign-section-header", source.string(), "2",
                      std::to_string(shared_header_index), "--kind", "even",
                      "--output", header_assigned.string(), "--json"},
                     assign_header_output),
             0);
    CHECK_EQ(
        read_text_file(assign_header_output),
        std::string{"{\"command\":\"assign-section-header\",\"ok\":true,"
                    "\"in_place\":false,\"sections\":3,\"headers\":2,"
                    "\"footers\":2,\"part\":\"header\",\"section\":2,"
                    "\"kind\":\"even\",\"part_index\":"} +
            std::to_string(shared_header_index) + "}\n");

    featherdoc::Document assigned_header_doc(header_assigned);
    REQUIRE_FALSE(assigned_header_doc.open());
    CHECK_EQ(assigned_header_doc.header_count(), 2);
    CHECK_EQ(collect_part_lines(assigned_header_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"section 0 header"}));

    CHECK_EQ(run_cli({"assign-section-footer", header_assigned.string(), "2",
                      std::to_string(shared_footer_index), "--output",
                      footer_assigned.string(), "--json"},
                     assign_footer_output),
             0);
    CHECK_EQ(
        read_text_file(assign_footer_output),
        std::string{"{\"command\":\"assign-section-footer\",\"ok\":true,"
                    "\"in_place\":false,\"sections\":3,\"headers\":2,"
                    "\"footers\":2,\"part\":\"footer\",\"section\":2,"
                    "\"kind\":\"default\",\"part_index\":"} +
            std::to_string(shared_footer_index) + "}\n");

    featherdoc::Document assigned_footer_doc(footer_assigned);
    REQUIRE_FALSE(assigned_footer_doc.open());
    CHECK_EQ(assigned_footer_doc.footer_count(), 2);
    CHECK_EQ(collect_part_lines(assigned_footer_doc.section_footer_paragraphs(2)),
             std::vector<std::string>({"section 0 footer"}));

    CHECK_EQ(run_cli({"remove-section-header", footer_assigned.string(), "2",
                      "--kind", "even", "--output", header_detached.string(),
                      "--json"},
                     remove_header_output),
             0);
    CHECK_EQ(
        read_text_file(remove_header_output),
        std::string{
            "{\"command\":\"remove-section-header\",\"ok\":true,"
            "\"in_place\":false,\"sections\":3,\"headers\":2,"
            "\"footers\":2,\"part\":\"header\",\"section\":2,"
            "\"kind\":\"even\"}\n"});

    featherdoc::Document detached_header_doc(header_detached);
    REQUIRE_FALSE(detached_header_doc.open());
    CHECK_FALSE(detached_header_doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());

    CHECK_EQ(run_cli({"remove-section-footer", header_detached.string(), "2",
                      "--output", footer_detached.string(), "--json"},
                     remove_footer_output),
             0);
    CHECK_EQ(
        read_text_file(remove_footer_output),
        std::string{
            "{\"command\":\"remove-section-footer\",\"ok\":true,"
            "\"in_place\":false,\"sections\":3,\"headers\":2,"
            "\"footers\":2,\"part\":\"footer\",\"section\":2,"
            "\"kind\":\"default\"}\n"});

    featherdoc::Document detached_footer_doc(footer_detached);
    REQUIRE_FALSE(detached_footer_doc.open());
    CHECK_FALSE(detached_footer_doc.section_footer_paragraphs(2).has_next());

    CHECK_EQ(run_cli({"remove-header-part", footer_detached.string(),
                      std::to_string(removable_header_index), "--output",
                      header_pruned.string(), "--json"},
                     remove_header_part_output),
             0);
    CHECK_EQ(
        read_text_file(remove_header_part_output),
        std::string{"{\"command\":\"remove-header-part\",\"ok\":true,"
                    "\"in_place\":false,\"sections\":3,\"headers\":1,"
                    "\"footers\":2,\"part\":\"header\",\"part_index\":"} +
            std::to_string(removable_header_index) + "}\n");

    featherdoc::Document pruned_header_doc(header_pruned);
    REQUIRE_FALSE(pruned_header_doc.open());
    CHECK_EQ(pruned_header_doc.header_count(), 1);
    CHECK_EQ(collect_part_lines(pruned_header_doc.section_header_paragraphs(0)),
             std::vector<std::string>({"section 0 header"}));
    CHECK_FALSE(pruned_header_doc.section_header_paragraphs(1).has_next());

    CHECK_EQ(run_cli({"remove-footer-part", header_pruned.string(),
                      std::to_string(removable_footer_index), "--output",
                      footer_pruned.string(), "--json"},
                     remove_footer_part_output),
             0);
    CHECK_EQ(
        read_text_file(remove_footer_part_output),
        std::string{"{\"command\":\"remove-footer-part\",\"ok\":true,"
                    "\"in_place\":false,\"sections\":3,\"headers\":1,"
                    "\"footers\":1,\"part\":\"footer\",\"part_index\":"} +
            std::to_string(removable_footer_index) + "}\n");

    featherdoc::Document pruned_footer_doc(footer_pruned);
    REQUIRE_FALSE(pruned_footer_doc.open());
    CHECK_EQ(pruned_footer_doc.footer_count(), 1);
    CHECK_EQ(collect_part_lines(pruned_footer_doc.section_footer_paragraphs(0)),
             std::vector<std::string>({"section 0 footer"}));
    CHECK_FALSE(pruned_footer_doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    remove_if_exists(source);
    remove_if_exists(header_assigned);
    remove_if_exists(footer_assigned);
    remove_if_exists(header_detached);
    remove_if_exists(footer_detached);
    remove_if_exists(header_pruned);
    remove_if_exists(footer_pruned);
    remove_if_exists(assign_header_output);
    remove_if_exists(assign_footer_output);
    remove_if_exists(remove_header_output);
    remove_if_exists(remove_footer_output);
    remove_if_exists(remove_header_part_output);
    remove_if_exists(remove_footer_part_output);
}

TEST_CASE("cli can inspect header and footer parts") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_inspect_parts_source.docx";
    const fs::path shown_headers = working_directory / "cli_inspect_headers.json";
    const fs::path shown_footers = working_directory / "cli_inspect_footers.txt";

    remove_if_exists(source);
    remove_if_exists(shown_headers);
    remove_if_exists(shown_footers);

    create_cli_part_inspect_fixture(source);

    featherdoc::Document fixture_doc(source);
    REQUIRE_FALSE(fixture_doc.open());
    const auto shared_header_index =
        find_header_index_by_text(fixture_doc, "section 0 header");
    const auto shared_footer_index =
        find_footer_index_by_text(fixture_doc, "section 0 footer");

    CHECK_EQ(run_cli({"inspect-header-parts", source.string(), "--json"},
                     shown_headers),
             0);
    const auto header_json = read_text_file(shown_headers);
    CHECK_NE(header_json.find("{\"part\":\"header\",\"count\":2,\"parts\":"),
             std::string::npos);
    CHECK_NE(header_json.find("\"index\":" + std::to_string(shared_header_index)),
             std::string::npos);
    CHECK_NE(header_json.find("\"entry\":\"word/header"), std::string::npos);
    CHECK_NE(header_json.find(
                 "\"references\":[{\"section\":0,\"kind\":\"default\"},"
                 "{\"section\":2,\"kind\":\"even\"}]"),
             std::string::npos);
    CHECK_NE(header_json.find("\"paragraphs\":[\"section 0 header\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-footer-parts", source.string()}, shown_footers), 0);
    const auto footer_text = read_text_file(shown_footers);
    CHECK_NE(footer_text.find("footer parts: 2"), std::string::npos);
    CHECK_NE(footer_text.find("part[" + std::to_string(shared_footer_index) +
                              "]: entry=word/footer"),
             std::string::npos);
    CHECK_NE(footer_text.find("references=section[0]:default, section[2]:default"),
             std::string::npos);
    CHECK_NE(footer_text.find("paragraph[0]: section 0 footer"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(shown_headers);
    remove_if_exists(shown_footers);
}

TEST_CASE("cli validate-template reports body schema mismatches as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_body_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_body_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_body_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part", "body", "--slot",
                      "customer:text", "--slot", "summary_block:block", "--slot",
                      "signature_image:image", "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"part\":\"body\",\"entry_name\":\"word/document.xml\","
                 "\"passed\":false,\"missing_required\":[\"signature_image\"],"
                 "\"duplicate_bookmarks\":[\"customer\"],"
                 "\"malformed_placeholders\":[\"summary_block\"]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template supports header and section footer targets") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_parts_source.docx";
    const fs::path header_output =
        working_directory / "cli_validate_template_header_output.json";
    const fs::path footer_output =
        working_directory / "cli_validate_template_footer_output.txt";

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(footer_output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part", "header",
                      "--index", "0", "--slot", "header_title:text", "--slot",
                      "header_note:block", "--slot", "header_rows:table_rows",
                      "--json"},
                     header_output),
             0);
    CHECK_EQ(read_text_file(header_output),
             std::string{
                 "{\"part\":\"header\",\"part_index\":0,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[]}\n"});

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part",
                      "section-footer", "--section", "0", "--kind", "default",
                      "--slot", "footer_company:text", "--slot",
                      "footer_summary:block", "--slot",
                      "footer_extra:text:optional"},
                     footer_output),
             0);
    CHECK_EQ(read_text_file(footer_output),
             std::string{
                 "part: section-footer\n"
                 "section: 0\n"
                 "kind: default\n"
                 "entry_name: word/footer1.xml\n"
                 "passed: no\n"
                 "missing_required: none\n"
                 "duplicate_bookmarks: footer_company\n"
                 "malformed_placeholders: footer_summary\n"});

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(footer_output);
}

TEST_CASE("cli validate-template reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_parse_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part",
                      "section-header", "--slot", "header_title:text", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "validation requires --section <index>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli append page number field updates an existing section header") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_page_number_field_source.docx";
    const fs::path updated =
        working_directory / "cli_append_page_number_field_updated.docx";
    const fs::path output =
        working_directory / "cli_append_page_number_field_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-page-number-field", source.string(), "--part",
                      "section-header", "--section", "1", "--output",
                      updated.string(), "--json"},
                     output),
             0);
    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-page-number-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":1"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"field\":\"page_number\""), std::string::npos);

    featherdoc::Document updated_doc(updated);
    REQUIRE_FALSE(updated_doc.open());
    const auto header_entry_name =
        std::string(updated_doc.section_header_template(1).entry_name());
    const auto header_xml = read_docx_entry(updated, header_entry_name.c_str());
    CHECK_NE(header_xml.find("w:fldSimple"), std::string::npos);
    CHECK_NE(header_xml.find("w:instr=\" PAGE \""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append total pages field materializes a missing section footer") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_total_pages_field_source.docx";
    const fs::path updated =
        working_directory / "cli_append_total_pages_field_updated.docx";
    const fs::path output =
        working_directory / "cli_append_total_pages_field_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"append-total-pages-field", source.string(), "--part",
                      "section-footer", "--section", "0", "--output",
                      updated.string(), "--json"},
                     output),
             0);
    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-total-pages-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-footer\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"field\":\"total_pages\""), std::string::npos);
    CHECK_NE(json.find("\"footers\":1"), std::string::npos);

    featherdoc::Document updated_doc(updated);
    REQUIRE_FALSE(updated_doc.open());
    CHECK_EQ(updated_doc.footer_count(), 1U);
    const auto footer_entry_name =
        std::string(updated_doc.section_footer_template(0).entry_name());
    const auto footer_xml = read_docx_entry(updated, footer_entry_name.c_str());
    CHECK_NE(footer_xml.find("w:fldSimple"), std::string::npos);
    CHECK_NE(footer_xml.find("w:instr=\" NUMPAGES \""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append page number field reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_page_number_field_parse_source.docx";
    const fs::path output =
        working_directory / "cli_append_page_number_field_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-page-number-field", source.string(), "--part",
                      "section-header", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-page-number-field\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}
