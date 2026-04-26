#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/wait.h>
#else
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

auto test_binary_directory() -> fs::path {
#if defined(_WIN32)
    std::wstring buffer(static_cast<std::size_t>(MAX_PATH), L'\0');
    while (true) {
        const DWORD copied =
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        REQUIRE(copied != 0);
        if (copied < buffer.size()) {
            buffer.resize(static_cast<std::size_t>(copied));
            return fs::path(buffer).parent_path();
        }

        buffer.resize(buffer.size() * 2U);
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    REQUIRE_EQ(_NSGetExecutablePath(nullptr, &size), -1);

    std::string buffer(static_cast<std::size_t>(size), '\0');
    REQUIRE_EQ(_NSGetExecutablePath(buffer.data(), &size), 0);
    return fs::weakly_canonical(fs::path(buffer.c_str())).parent_path();
#else
    std::error_code error;
    const fs::path executable_path = fs::read_symlink("/proc/self/exe", error);
    REQUIRE_FALSE(error);
    return executable_path.parent_path();
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

#if defined(_WIN32)
auto utf8_to_wide(std::string_view value) -> std::wstring {
    if (value.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8,
                                             0,
                                             value.data(),
                                             static_cast<int>(value.size()),
                                             nullptr,
                                             0);
    REQUIRE(required > 0);

    std::wstring converted(static_cast<std::size_t>(required), L'\0');
    const int written = MultiByteToWideChar(CP_UTF8,
                                            0,
                                            value.data(),
                                            static_cast<int>(value.size()),
                                            converted.data(),
                                            required);
    REQUIRE_EQ(written, required);
    return converted;
}

void append_windows_command_line_argument(std::wstring_view argument,
                                          std::wstring &command_line) {
    if (!command_line.empty()) {
        command_line += L' ';
    }

    const bool needs_quotes =
        argument.empty() ||
        argument.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
    if (!needs_quotes) {
        command_line.append(argument);
        return;
    }

    command_line += L'"';
    std::size_t trailing_backslashes = 0;
    for (const wchar_t ch : argument) {
        if (ch == L'\\') {
            ++trailing_backslashes;
            continue;
        }

        if (ch == L'"') {
            command_line.append(trailing_backslashes * 2 + 1, L'\\');
            command_line += L'"';
            trailing_backslashes = 0;
            continue;
        }

        command_line.append(trailing_backslashes, L'\\');
        trailing_backslashes = 0;
        command_line += ch;
    }

    command_line.append(trailing_backslashes * 2, L'\\');
    command_line += L'"';
}
#endif

auto cli_binary_path() -> fs::path {
    static const fs::path executable_path = []() {
        const fs::path binary_directory = test_binary_directory();
        const fs::path local_candidate = binary_directory / cli_binary_name();
        const fs::path parent_candidate = binary_directory.parent_path() / cli_binary_name();

        std::error_code local_error;
        const bool has_local = fs::exists(local_candidate, local_error) && !local_error;
        std::error_code parent_error;
        const bool has_parent =
            fs::exists(parent_candidate, parent_error) && !parent_error;

        if (!has_local) {
            return parent_candidate;
        }
        if (!has_parent) {
            return local_candidate;
        }

        std::error_code local_time_error;
        const auto local_time = fs::last_write_time(local_candidate, local_time_error);
        std::error_code parent_time_error;
        const auto parent_time =
            fs::last_write_time(parent_candidate, parent_time_error);
        if (!local_time_error && !parent_time_error && parent_time > local_time) {
            return parent_candidate;
        }
        return local_candidate;
    }();
    return executable_path;
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

auto json_escape_text(std::string_view text) -> std::string {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
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

auto find_body_paragraph_xml_node(pugi::xml_node document_root, std::size_t index)
    -> pugi::xml_node {
    auto body = document_root.child("w:body");
    if (body == pugi::xml_node{}) {
        return {};
    }

    std::size_t current_index = 0U;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (current_index == index) {
            return paragraph;
        }
        ++current_index;
    }

    return {};
}

auto find_run_xml_node(pugi::xml_node paragraph_node, std::size_t index)
    -> pugi::xml_node {
    std::size_t current_index = 0U;
    for (auto run = paragraph_node.child("w:r"); run != pugi::xml_node{};
         run = run.next_sibling("w:r")) {
        if (current_index == index) {
            return run;
        }
        ++current_index;
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

auto collect_paragraph_text(featherdoc::Paragraph paragraph) -> std::string {
    std::string text;
    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
        text += run.get_text();
    }
    return text;
}

auto find_body_paragraph_index_by_text(featherdoc::Document &document,
                                       std::string_view text) -> std::size_t {
    std::size_t index = 0U;
    for (auto paragraph = document.paragraphs(); paragraph.has_next();
         paragraph.next(), ++index) {
        if (collect_paragraph_text(paragraph) == text) {
            return index;
        }
    }

    REQUIRE(false);
    return 0U;
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

void create_cli_resolved_part_template_validation_fixture(const fs::path &path) {
    create_cli_part_template_validation_fixture(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.append_section(false));
    append_body_paragraph(document, "section 1 body");
    REQUIRE_EQ(document.section_count(), 2U);
    REQUIRE_FALSE(document.save());
}

void create_cli_schema_v2_template_validation_fixture(const fs::path &path) {
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
      <w:bookmarkStart w:id="0" w:name="summary_block"/>
      <w:r><w:t>standalone block placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Approver: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="approver"/>
      <w:r><w:t>Alice</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:p>
      <w:r><w:t>Extra: </w:t></w:r>
      <w:bookmarkStart w:id="2" w:name="extra_slot"/>
      <w:r><w:t>Keep me declared or report me</w:t></w:r>
      <w:bookmarkEnd w:id="2"/>
    </w:p>
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

void create_cli_bookmark_table_rows_fixture(const fs::path &path) {
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
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>template qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
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

void create_cli_template_table_bookmark_fixture(const fs::path &path) {
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
    <w:p><w:r><w:t>bookmark template intro</w:t></w:r></w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>keep-00</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>keep-01</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="target_before_table"/>
      <w:r><w:t>before target table</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="1" w:name="target_inside_table"/>
            <w:r><w:t>target-00</w:t></w:r>
            <w:bookmarkEnd w:id="1"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>target-01</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>target-10</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>target-11</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>bookmark template outro</w:t></w:r></w:p>
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

void create_cli_template_table_selector_fixture(const fs::path &path) {
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
    <w:p><w:r><w:t>selector intro</w:t></w:r></w:p>
    <w:p><w:r><w:t>selector target table</w:t></w:r></w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Region</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Amount</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>North</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>12</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>selector middle</w:t></w:r></w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Label</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Value</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Ignore</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Keep</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>selector target table</w:t></w:r></w:p>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Region</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Amount</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>South</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>9</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>24</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>selector outro</w:t></w:r></w:p>
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

void create_cli_bookmark_block_visibility_fixture(const fs::path &path) {
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
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="0" w:name="keep_block"/></w:p>
    <w:p><w:r><w:t>Keep me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="0"/></w:p>
    <w:p><w:r><w:t>middle</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="1" w:name="hide_block"/></w:p>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Secret Cell</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>Hide me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="1"/></w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
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

void create_cli_fill_bookmarks_fixture(const fs::path &path) {
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
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>old customer</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Invoice: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="invoice"/>
      <w:r><w:t>old invoice</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
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


void create_cli_style_numbering_audit_broken_fixture(const fs::path &path) {
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
  <w:style w:type="paragraph" w:styleId="MissingNumId" w:customStyle="1">
    <w:name w:val="Missing NumId"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:numPr><w:ilvl w:val="0"/></w:numPr>
    </w:pPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="OrphanNumId" w:customStyle="1">
    <w:name w:val="Orphan NumId"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:numPr><w:ilvl w:val="0"/><w:numId w:val="777"/></w:numPr>
    </w:pPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="PlainBody" w:customStyle="1">
    <w:name w:val="Plain Body"/>
    <w:basedOn w:val="Normal"/>
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


void create_cli_style_numbering_based_on_mismatch_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto base_style = featherdoc::paragraph_style_definition{};
    base_style.name = "Base Numbered";
    base_style.based_on = std::string{"Normal"};
    base_style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("BaseNumbered", base_style));

    auto child_style = featherdoc::paragraph_style_definition{};
    child_style.name = "Child Numbered";
    child_style.based_on = std::string{"BaseNumbered"};
    child_style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("ChildNumbered", child_style));

    auto base_definition = featherdoc::numbering_definition{};
    base_definition.name = "BaseOutline";
    base_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };
    const auto base_definition_id =
        document.ensure_numbering_definition(base_definition);
    REQUIRE(base_definition_id.has_value());

    auto child_definition = featherdoc::numbering_definition{};
    child_definition.name = "ChildOutline";
    child_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1)"},
    };
    const auto child_definition_id =
        document.ensure_numbering_definition(child_definition);
    REQUIRE(child_definition_id.has_value());

    REQUIRE(document.set_paragraph_style_numbering("BaseNumbered",
                                                   *base_definition_id, 0U));
    REQUIRE(document.set_paragraph_style_numbering("ChildNumbered",
                                                   *child_definition_id, 0U));
    REQUIRE_FALSE(document.save());
}


void create_cli_style_numbering_missing_level_relink_fixture(
    const fs::path &path) {
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
  <Override PartName="/word/numbering.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
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
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering"
                Target="numbering.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="NeedsRelink" w:customStyle="1">
    <w:name w:val="Needs Relink"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:numPr><w:ilvl w:val="1"/><w:numId w:val="7"/></w:numPr>
    </w:pPr>
  </w:style>
</w:styles>
)";
    constexpr auto numbering_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="1">
    <w:multiLevelType w:val="multilevel"/>
    <w:name w:val="SharedOutline"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
    </w:lvl>
  </w:abstractNum>
  <w:abstractNum w:abstractNumId="2">
    <w:multiLevelType w:val="multilevel"/>
    <w:name w:val="SharedOutline"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
    </w:lvl>
    <w:lvl w:ilvl="1">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1.%2."/>
    </w:lvl>
  </w:abstractNum>
  <w:num w:numId="7"><w:abstractNumId w:val="1"/></w:num>
  <w:num w:numId="8"><w:abstractNumId w:val="2"/></w:num>
</w:numbering>
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
    REQUIRE(write_archive_entry(archive, "word/numbering.xml", numbering_xml));
    zip_close(archive);
}


void create_cli_style_numbering_catalog_repair_fixture(const fs::path &path) {
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
  <Override PartName="/word/numbering.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
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
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering"
                Target="numbering.xml"/>
</Relationships>
)";
    constexpr auto styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="CatalogRepaired" w:customStyle="1">
    <w:name w:val="Catalog Repaired"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:numPr><w:ilvl w:val="1"/><w:numId w:val="7"/></w:numPr>
    </w:pPr>
  </w:style>
</w:styles>
)";
    constexpr auto numbering_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="1">
    <w:multiLevelType w:val="multilevel"/>
    <w:name w:val="CatalogOutline"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
    </w:lvl>
  </w:abstractNum>
  <w:num w:numId="7"><w:abstractNumId w:val="1"/></w:num>
</w:numbering>
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
    REQUIRE(write_archive_entry(archive, "word/numbering.xml", numbering_xml));
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

void create_cli_duplicate_style_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    featherdoc::paragraph_style_definition duplicate_a;
    duplicate_a.name = "Duplicate Body A";
    duplicate_a.based_on = "Normal";
    duplicate_a.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyA", duplicate_a));

    featherdoc::paragraph_style_definition duplicate_b;
    duplicate_b.name = "Duplicate Body B";
    duplicate_b.based_on = "Normal";
    duplicate_b.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyB", duplicate_b));

    auto first_paragraph = document.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first duplicate style reference").has_next());
    REQUIRE(document.set_paragraph_style(first_paragraph, "DuplicateBodyA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    REQUIRE(document.set_paragraph_style(second_paragraph, "DuplicateBodyA"));
    auto third_paragraph = second_paragraph.insert_paragraph_after("third");
    REQUIRE(document.set_paragraph_style(third_paragraph, "DuplicateBodyB"));

    REQUIRE_FALSE(document.save());
}

void create_cli_duplicate_style_profile_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    featherdoc::paragraph_style_definition duplicate_a;
    duplicate_a.name = "Duplicate Body A";
    duplicate_a.based_on = "Normal";
    duplicate_a.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyA", duplicate_a));

    featherdoc::paragraph_style_definition duplicate_b;
    duplicate_b.name = "Duplicate Body B";
    duplicate_b.based_on = "Normal";
    duplicate_b.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyB", duplicate_b));

    featherdoc::paragraph_style_definition duplicate_c;
    duplicate_c.name = "Duplicate Body C";
    duplicate_c.based_on = "Normal";
    duplicate_c.next_style = "Normal";
    duplicate_c.run_font_family = "Aptos";
    REQUIRE(document.ensure_paragraph_style("DuplicateBodyC", duplicate_c));

    auto first_paragraph = document.paragraphs();
    REQUIRE(first_paragraph.has_next());
    REQUIRE(first_paragraph.add_run("first duplicate style reference").has_next());
    REQUIRE(document.set_paragraph_style(first_paragraph, "DuplicateBodyA"));
    auto second_paragraph = first_paragraph.insert_paragraph_after("second");
    REQUIRE(document.set_paragraph_style(second_paragraph, "DuplicateBodyA"));
    auto third_paragraph = second_paragraph.insert_paragraph_after("third");
    REQUIRE(document.set_paragraph_style(third_paragraph, "DuplicateBodyB"));
    auto fourth_paragraph = third_paragraph.insert_paragraph_after("fourth");
    REQUIRE(document.set_paragraph_style(fourth_paragraph, "DuplicateBodyC"));

    REQUIRE_FALSE(document.save());
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

void create_cli_paragraph_list_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    REQUIRE(document.paragraphs().add_run("item 0").has_next());
    append_body_paragraph(document, "item 1");
    append_body_paragraph(document, "item 2");

    REQUIRE_FALSE(document.save());
}

void create_cli_inspect_paragraphs_fixture(const fs::path &path) {
    create_cli_fixture(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());

    featherdoc::paragraph_style_definition style;
    style.name = "Custom Body";
    style.based_on = "Normal";
    style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("CustomBody", style));

    const auto styled_index =
        find_body_paragraph_index_by_text(document, "section 0 body");
    auto styled_paragraph = document.paragraphs();
    for (std::size_t index = 0U; index < styled_index && styled_paragraph.has_next();
         ++index) {
        styled_paragraph.next();
    }
    REQUIRE(styled_paragraph.has_next());
    REQUIRE(document.set_paragraph_style(styled_paragraph, "CustomBody"));

    const auto numbered_index =
        find_body_paragraph_index_by_text(document, "section 1 body");
    auto numbered_paragraph = document.paragraphs();
    for (std::size_t index = 0U; index < numbered_index && numbered_paragraph.has_next();
         ++index) {
        numbered_paragraph.next();
    }
    REQUIRE(numbered_paragraph.has_next());
    REQUIRE(document.set_paragraph_list(numbered_paragraph,
                                        featherdoc::list_kind::decimal, 0U));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_inspection_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2, 2);
    REQUIRE(table.has_next());
    REQUIRE(table.set_width_twips(7200U));
    REQUIRE(table.set_style_id("TableGrid"));
    REQUIRE(table.set_column_width_twips(0U, 1800U));
    REQUIRE(table.set_column_width_twips(1U, 5400U));

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("r0c0"));
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("r0c1"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("r1c0"));
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("r1c1"));

    auto second_table = document.append_table(2, 3);
    REQUIRE(second_table.has_next());
    REQUIRE(second_table.set_column_width_twips(0U, 1200U));
    REQUIRE(second_table.set_column_width_twips(1U, 2200U));
    REQUIRE(second_table.set_column_width_twips(2U, 3200U));

    row = second_table.rows();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("merged-top"));
    REQUIRE(cell.merge_right());
    REQUIRE(cell.set_vertical_alignment(
        featherdoc::cell_vertical_alignment::center));
    REQUIRE(cell.set_text_direction(
        featherdoc::cell_text_direction::top_to_bottom_right_to_left));

    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("top-right"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("bottom-left"));

    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_width_twips(2222U));
    REQUIRE(cell.set_text("bottom-middle"));

    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("line1"));
    auto paragraph = cell.paragraphs();
    REQUIRE(paragraph.has_next());
    auto inserted_paragraph = paragraph.insert_paragraph_after("line2");
    REQUIRE(inserted_paragraph.has_next());

    REQUIRE_FALSE(document.save());
}

void create_cli_template_inspection_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "CliTemplateInspectOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id =
        document.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    CHECK(document.set_paragraph_style(body_paragraph, "Heading1"));
    CHECK(body_paragraph.set_bidi());
    REQUIRE(body_paragraph.add_run("Body heading").has_next());

    auto numbered_body_paragraph = body_paragraph.insert_paragraph_after("Body item");
    REQUIRE(numbered_body_paragraph.has_next());
    CHECK(document.set_paragraph_numbering(numbered_body_paragraph, *numbering_id));

    auto &header_paragraph = document.ensure_header_paragraphs();
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = document.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    auto header_table = header_template.append_table(2U, 2U);
    REQUIRE(header_table.has_next());
    CHECK(header_table.set_style_id("TableGrid"));
    CHECK(header_table.set_column_width_twips(0U, 1800U));
    CHECK(header_table.set_column_width_twips(1U, 3600U));

    auto header_row = header_table.rows();
    REQUIRE(header_row.has_next());
    auto header_cell = header_row.cells();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("h00"));
    header_cell.next();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("h01"));

    header_row.next();
    REQUIRE(header_row.has_next());
    header_cell = header_row.cells();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_text("h10"));
    header_cell.next();
    REQUIRE(header_cell.has_next());
    CHECK(header_cell.set_width_twips(2222U));
    CHECK(header_cell.set_text("h11"));

    auto &footer_paragraph = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer_paragraph.has_next());
    CHECK(footer_paragraph.set_text("Footer intro"));

    auto footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_inspected_paragraph = footer_template.append_paragraph();
    REQUIRE(footer_inspected_paragraph.has_next());
    auto footer_styled_run = footer_inspected_paragraph.add_run("Footer styled");
    REQUIRE(footer_styled_run.has_next());
    CHECK(document.set_run_style(footer_styled_run, "Strong"));
    CHECK(footer_styled_run.set_font_family("Segoe UI"));
    CHECK(footer_styled_run.set_language("en-US"));
    auto footer_plain_run = footer_inspected_paragraph.add_run(" tail");
    REQUIRE(footer_plain_run.has_next());

    REQUIRE_FALSE(document.save());
}

void populate_table_cells(featherdoc::Table table,
                          const std::vector<std::vector<std::string>> &rows) {
    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        REQUIRE(row.has_next());
        auto cell = row.cells();
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            REQUIRE(cell.has_next());
            REQUIRE(cell.set_text(rows[row_index][cell_index]));
            if (cell_index + 1U < rows[row_index].size()) {
                cell.next();
            }
        }
        if (row_index + 1U < rows.size()) {
            row.next();
        }
    }
}

void create_cli_template_table_merge_unmerge_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template merge fixture"));

    auto body_table = body_template.append_table(1U, 3U);
    REQUIRE(body_table.has_next());
    populate_table_cells(body_table, {{"A", "B", "C"}});
    auto body_cell = body_table.rows().cells();
    REQUIRE(body_cell.has_next());
    REQUIRE(body_cell.merge_right(1U));

    auto &header_paragraph = document.ensure_header_paragraphs();
    REQUIRE(header_paragraph.has_next());
    REQUIRE(header_paragraph.set_text("Header template merge fixture"));

    auto header_template = document.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    auto header_table = header_template.append_table(3U, 3U);
    REQUIRE(header_table.has_next());
    populate_table_cells(header_table,
                         {{"hA", "hB", "hC"},
                          {"hD", "hE", "hF"},
                          {"hG", "hH", "hI"}});

    auto &footer_paragraph = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer_paragraph.has_next());
    REQUIRE(footer_paragraph.set_text("Section footer merge fixture"));

    auto footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_table = footer_template.append_table(3U, 2U);
    REQUIRE(footer_table.has_next());
    populate_table_cells(footer_table,
                         {{"fA", "fB"},
                          {"fC", "fD"},
                          {"fE", "fF"}});
    auto footer_cell = footer_table.rows().cells();
    REQUIRE(footer_cell.has_next());
    REQUIRE(footer_cell.merge_down(2U));

    auto direct_footer_template = document.footer_template(0U);
    REQUIRE(static_cast<bool>(direct_footer_template));
    auto direct_footer_table = direct_footer_template.append_table(3U, 2U);
    REQUIRE(direct_footer_table.has_next());
    populate_table_cells(direct_footer_table,
                         {{"dfA", "dfB"},
                          {"dfC", "dfD"},
                          {"dfE", "dfF"}});
    auto direct_footer_cell = direct_footer_table.rows().cells();
    REQUIRE(direct_footer_cell.has_next());
    REQUIRE(direct_footer_cell.merge_down(2U));

    REQUIRE_FALSE(document.save());
}

void configure_cli_template_table_fixture(
    featherdoc::Table table, const std::vector<std::uint32_t> &column_widths,
    const std::vector<std::vector<std::string>> &rows) {
    REQUIRE(table.has_next());
    REQUIRE_FALSE(column_widths.empty());

    std::uint32_t total_width = 0U;
    for (const auto width : column_widths) {
        total_width += width;
    }

    CHECK(table.set_style_id("TableGrid"));
    CHECK(table.set_width_twips(total_width));
    CHECK(table.set_layout_mode(featherdoc::table_layout_mode::fixed));
    for (std::size_t column_index = 0; column_index < column_widths.size();
         ++column_index) {
        CHECK(table.set_column_width_twips(column_index,
                                           column_widths[column_index]));
    }

    populate_table_cells(table, rows);
}

void create_cli_template_table_column_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template column fixture"));
    auto body_table = body_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(body_table, {1200U, 2200U, 2800U},
                                         {{"b00", "b01", "b02"},
                                          {"b10", "b11", "b12"}});

    auto &header_paragraph = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header_paragraph.has_next());
    REQUIRE(header_paragraph.set_text("Section header template column fixture"));
    auto header_template = document.section_header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    auto header_table = header_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(header_table, {1800U, 3600U},
                                         {{"h00", "h01"},
                                          {"h10", "h11"}});

    auto &footer_paragraph = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer_paragraph.has_next());
    REQUIRE(footer_paragraph.set_text("Section footer template column fixture"));
    auto footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_table = footer_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(footer_table, {1600U, 2800U},
                                         {{"f00", "f01"},
                                          {"f10", "f11"}});

    REQUIRE_FALSE(document.save());
}

void create_cli_template_table_direct_column_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template direct column fixture"));
    auto body_table = body_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(body_table, {1200U, 2200U, 2800U},
                                         {{"b00", "b01", "b02"},
                                          {"b10", "b11", "b12"}});

    auto &header_paragraph = document.ensure_header_paragraphs();
    REQUIRE(header_paragraph.has_next());
    REQUIRE(header_paragraph.set_text("Direct header template column fixture"));
    auto header_template = document.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    auto header_table = header_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(header_table, {1800U, 3600U},
                                         {{"h00", "h01"},
                                          {"h10", "h11"}});

    auto &footer_paragraph = document.ensure_footer_paragraphs();
    REQUIRE(footer_paragraph.has_next());
    REQUIRE(footer_paragraph.set_text("Direct footer template column fixture"));
    auto footer_template = document.footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_table = footer_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(footer_table, {1600U, 2800U},
                                         {{"f00", "f01"},
                                          {"f10", "f11"}});

    REQUIRE_FALSE(document.save());
}

void create_cli_template_table_section_kind_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template section kind fixture"));

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    REQUIRE(default_header.set_text("Default section header kind fixture"));
    auto default_header_template = document.section_header_template(0U);
    REQUIRE(static_cast<bool>(default_header_template));
    auto default_header_table = default_header_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(default_header_table, {1800U, 3600U},
                                         {{"dh00", "dh01"},
                                          {"dh10", "dh11"}});

    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    REQUIRE(even_header.set_text("Even section header kind fixture"));
    auto even_header_template = document.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    auto even_header_table = even_header_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(even_header_table, {1800U, 3600U},
                                         {{"eh00", "eh01"},
                                          {"eh10", "eh11"}});

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    REQUIRE(default_footer.set_text("Default section footer kind fixture"));
    auto default_footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(default_footer_template));
    auto default_footer_table = default_footer_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(default_footer_table, {1600U, 2800U},
                                         {{"df00", "df01"},
                                          {"df10", "df11"}});

    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    REQUIRE(first_footer.set_text("First section footer kind fixture"));
    auto first_footer_template = document.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    auto first_footer_table = first_footer_template.append_table(2U, 2U);
    configure_cli_template_table_fixture(first_footer_table, {1600U, 2800U},
                                         {{"ff00", "ff01"},
                                          {"ff10", "ff11"}});

    REQUIRE_FALSE(document.save());
}

void create_cli_template_table_section_kind_merge_unmerge_fixture(
    const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Body template section kind merge fixture"));

    auto &default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.has_next());
    REQUIRE(default_header.set_text("Default section header merge fixture"));
    auto default_header_template = document.section_header_template(0U);
    REQUIRE(static_cast<bool>(default_header_template));
    auto default_header_table = default_header_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(default_header_table,
                                         {1600U, 1800U, 3000U},
                                         {{"dh00", "dh01", "dh02"},
                                          {"dh10", "dh11", "dh12"}});

    auto &even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    REQUIRE(even_header.set_text("Even section header merge fixture"));
    auto even_header_template = document.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    auto even_header_table = even_header_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(even_header_table,
                                         {1600U, 1800U, 3000U},
                                         {{"eh00", "eh01", "eh02"},
                                          {"eh10", "eh11", "eh12"}});

    auto &default_footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(default_footer.has_next());
    REQUIRE(default_footer.set_text("Default section footer merge fixture"));
    auto default_footer_template = document.section_footer_template(0U);
    REQUIRE(static_cast<bool>(default_footer_template));
    auto default_footer_table = default_footer_template.append_table(3U, 2U);
    configure_cli_template_table_fixture(default_footer_table, {1800U, 4200U},
                                         {{"df00", "df01"},
                                          {"df10", "df11"},
                                          {"df20", "df21"}});
    auto default_footer_cell = default_footer_table.rows().cells();
    REQUIRE(default_footer_cell.has_next());
    REQUIRE(default_footer_cell.merge_down(2U));

    auto &first_footer = document.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    REQUIRE(first_footer.set_text("First section footer merge fixture"));
    auto first_footer_template = document.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    auto first_footer_table = first_footer_template.append_table(3U, 2U);
    configure_cli_template_table_fixture(first_footer_table, {1800U, 4200U},
                                         {{"ff00", "ff01"},
                                          {"ff10", "ff11"},
                                          {"ff20", "ff21"}});
    auto first_footer_cell = first_footer_table.rows().cells();
    REQUIRE(first_footer_cell.has_next());
    REQUIRE(first_footer_cell.merge_down(2U));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_cell_style_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1, 1);
    REQUIRE(table.has_next());
    auto cell = table.rows().cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("styled-cell"));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_row_style_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(2, 1);
    REQUIRE(table.has_next());

    auto row = table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("header-row"));

    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("body-row"));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_merge_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(3, 3);
    REQUIRE(table.has_next());

    const std::vector<std::vector<std::string>> rows = {
        {"A", "B", "C"},
        {"D", "E", "F"},
        {"G", "H", "I"},
    };

    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        REQUIRE(row.has_next());
        auto cell = row.cells();
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            REQUIRE(cell.has_next());
            REQUIRE(cell.set_text(rows[row_index][cell_index]));
            if (cell_index + 1U < rows[row_index].size()) {
                cell.next();
            }
        }
        if (row_index + 1U < rows.size()) {
            row.next();
        }
    }

    REQUIRE_FALSE(document.save());
}

void create_cli_table_unmerge_right_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(1, 3);
    REQUIRE(table.has_next());

    auto row = table.rows();
    REQUIRE(row.has_next());

    auto cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("A"));
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("B"));
    cell.next();
    REQUIRE(cell.has_next());
    REQUIRE(cell.set_text("C"));

    cell = row.cells();
    REQUIRE(cell.has_next());
    REQUIRE(cell.merge_right(1U));

    REQUIRE_FALSE(document.save());
}

void create_cli_table_unmerge_down_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(3, 2);
    REQUIRE(table.has_next());

    const std::vector<std::vector<std::string>> rows = {
        {"A", "B"},
        {"C", "D"},
        {"E", "F"},
    };

    auto row = table.rows();
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        REQUIRE(row.has_next());
        auto cell = row.cells();
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            REQUIRE(cell.has_next());
            REQUIRE(cell.set_text(rows[row_index][cell_index]));
            if (cell_index + 1U < rows[row_index].size()) {
                cell.next();
            }
        }
        if (row_index + 1U < rows.size()) {
            row.next();
        }
    }

    auto anchor = table.rows().cells();
    REQUIRE(anchor.has_next());
    REQUIRE(anchor.merge_down(2U));

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
    body_options.z_order = 72U;
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
    header_options.z_order = 84U;
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
    std::wstring command_line;
    append_windows_command_line_argument(executable_path.wstring(), command_line);
    for (const auto &argument : arguments) {
        append_windows_command_line_argument(utf8_to_wide(argument), command_line);
    }

    SECURITY_ATTRIBUTES output_attributes{};
    output_attributes.nLength = sizeof(output_attributes);
    output_attributes.bInheritHandle = TRUE;

    HANDLE output_handle = INVALID_HANDLE_VALUE;
    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    if (captured_output.has_value()) {
        remove_if_exists(*captured_output);
        output_handle = CreateFileW(captured_output->c_str(),
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    &output_attributes,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr);
        REQUIRE(output_handle != INVALID_HANDLE_VALUE);

        startup_info.dwFlags |= STARTF_USESTDHANDLES;
        startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startup_info.hStdOutput = output_handle;
        startup_info.hStdError = output_handle;
    }

    std::vector<wchar_t> mutable_command_line(command_line.begin(), command_line.end());
    mutable_command_line.push_back(L'\0');

    PROCESS_INFORMATION process_info{};
    const BOOL created = CreateProcessW(executable_path.c_str(),
                                        mutable_command_line.data(),
                                        nullptr,
                                        nullptr,
                                        captured_output.has_value() ? TRUE : FALSE,
                                        0,
                                        nullptr,
                                        fs::current_path().c_str(),
                                        &startup_info,
                                        &process_info);
    if (output_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(output_handle);
    }
    REQUIRE(created != FALSE);

    REQUIRE_EQ(WaitForSingleObject(process_info.hProcess, INFINITE), WAIT_OBJECT_0);

    DWORD exit_code = static_cast<DWORD>(-1);
    REQUIRE(GetExitCodeProcess(process_info.hProcess, &exit_code) != FALSE);

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    return static_cast<int>(exit_code);
#else
    std::string command = shell_quote(executable_path.string());
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

    return normalize_system_status(std::system(command.c_str()));
#endif
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
    const fs::path footer_text_source =
        working_directory / "cli_section_footer_input.txt";
    const fs::path default_header_text_source =
        working_directory / "cli_section_default_header_input.txt";
    const fs::path header_updated = working_directory / "cli_section_header_updated.docx";
    const fs::path shown_even_header = working_directory / "cli_section_even_header.txt";
    const fs::path footer_updated = working_directory / "cli_section_footer_updated.docx";
    const fs::path shown_footer = working_directory / "cli_section_footer.txt";
    const fs::path default_header_updated =
        working_directory / "cli_section_default_header_updated.docx";
    const fs::path shown_default_header =
        working_directory / "cli_section_default_header.txt";

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(text_source);
    remove_if_exists(footer_text_source);
    remove_if_exists(default_header_text_source);
    remove_if_exists(header_updated);
    remove_if_exists(shown_even_header);
    remove_if_exists(footer_updated);
    remove_if_exists(shown_footer);
    remove_if_exists(default_header_updated);
    remove_if_exists(shown_default_header);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"show-section-header", source.string(), "1"}, header_output), 0);
    CHECK_EQ(read_text_file(header_output), std::string("section 1 header\n"));

    {
        std::ofstream stream(text_source);
        REQUIRE(stream.good());
        stream << "alpha\nbeta\n";
    }
    {
        std::ofstream stream(footer_text_source);
        REQUIRE(stream.good());
        stream << "front footer\nsecond footer\n";
    }
    {
        std::ofstream stream(default_header_text_source);
        REQUIRE(stream.good());
        stream << "section 1 default header\nsecond header line\n";
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

    CHECK_EQ(run_cli({"set-section-footer", header_updated.string(), "0", "--text-file",
                      footer_text_source.string(), "--output", footer_updated.string()}),
             0);
    CHECK_EQ(run_cli({"show-section-footer", footer_updated.string(), "0"},
                     shown_footer),
             0);
    CHECK_EQ(read_text_file(shown_footer),
             std::string("front footer\nsecond footer\n"));

    featherdoc::Document footer_doc(footer_updated);
    REQUIRE_FALSE(footer_doc.open());
    CHECK_EQ(collect_part_lines(footer_doc.section_footer_paragraphs(0)),
             std::vector<std::string>({"front footer", "second footer"}));
    CHECK_EQ(collect_part_lines(footer_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"alpha", "beta"}));

    CHECK_EQ(run_cli({"set-section-header", footer_updated.string(), "1", "--kind",
                      "default", "--text-file",
                      default_header_text_source.string(), "--output",
                      default_header_updated.string()}),
             0);
    CHECK_EQ(run_cli({"show-section-header", default_header_updated.string(), "1",
                      "--kind", "default"},
                     shown_default_header),
             0);
    CHECK_EQ(read_text_file(shown_default_header),
             std::string("section 1 default header\nsecond header line\n"));

    featherdoc::Document default_header_doc(default_header_updated);
    REQUIRE_FALSE(default_header_doc.open());
    CHECK_EQ(collect_part_lines(default_header_doc.section_header_paragraphs(1)),
             std::vector<std::string>(
                 {"section 1 default header", "second header line"}));
    CHECK_EQ(collect_part_lines(default_header_doc.section_footer_paragraphs(0)),
             std::vector<std::string>({"front footer", "second footer"}));
    CHECK_EQ(collect_part_lines(default_header_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"alpha", "beta"}));

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(text_source);
    remove_if_exists(footer_text_source);
    remove_if_exists(default_header_text_source);
    remove_if_exists(header_updated);
    remove_if_exists(shown_even_header);
    remove_if_exists(footer_updated);
    remove_if_exists(shown_footer);
    remove_if_exists(default_header_updated);
    remove_if_exists(shown_default_header);
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

TEST_CASE("cli rename-style rewrites paragraph style ids and references") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_rename_style_source.docx";
    const fs::path renamed = working_directory / "cli_rename_style_renamed.docx";
    const fs::path output = working_directory / "cli_rename_style_output.json";
    const fs::path usage_output = working_directory / "cli_rename_style_usage.json";
    const fs::path conflict_output = working_directory / "cli_rename_style_conflict.json";

    remove_if_exists(source);
    remove_if_exists(renamed);
    remove_if_exists(output);
    remove_if_exists(usage_output);
    remove_if_exists(conflict_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::paragraph_style_definition child_style;
        child_style.name = "Custom Child";
        child_style.based_on = "CustomBody";
        child_style.next_style = "CustomBody";
        REQUIRE(document.ensure_paragraph_style("CustomChild", child_style));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"rename-style",
                      source.string(),
                      "CustomBody",
                      "RenamedBody",
                      "--output",
                      renamed.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            R"({"command":"rename-style","ok":true,)"
            R"("in_place":false,"sections":1,"headers":1,"footers":1,)"
            R"("old_style_id":"CustomBody","new_style_id":"RenamedBody",)"
            R"("style":{"style_id":"RenamedBody","name":"Custom Body",)"
            R"("based_on":"Normal","kind":"paragraph","type":"paragraph",)"
            R"("numbering":null,"is_default":false,"is_custom":true,)"
            R"("is_semi_hidden":false,"is_unhide_when_used":false,)"
            R"("is_quick_format":true}})"
            "\n"});

    CHECK_EQ(run_cli({"inspect-styles", renamed.string(), "--style", "RenamedBody",
                      "--usage", "--json"},
                     usage_output),
             0);
    const auto usage_json = read_text_file(usage_output);
    CHECK_NE(usage_json.find(R"("style_id":"RenamedBody")"), std::string::npos);
    CHECK_NE(usage_json.find(R"("paragraph_count":3)"), std::string::npos);
    CHECK_NE(usage_json.find(R"("body":{"paragraph_count":2)"),
             std::string::npos);
    CHECK_NE(usage_json.find(R"("header":{"paragraph_count":1)"),
             std::string::npos);

    const auto styles_xml = read_docx_entry(renamed, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    CHECK(find_style_xml_node(styles_root, "CustomBody") == pugi::xml_node{});
    CHECK(find_style_xml_node(styles_root, "RenamedBody") != pugi::xml_node{});
    const auto child_style = find_style_xml_node(styles_root, "CustomChild");
    REQUIRE(child_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{child_style.child("w:basedOn").attribute("w:val").value()},
             "RenamedBody");
    CHECK_EQ(std::string_view{child_style.child("w:next").attribute("w:val").value()},
             "RenamedBody");

    const auto document_xml = read_docx_entry(renamed, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="RenamedBody")"),
             std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);
    const auto header_xml = read_docx_entry(renamed, "word/header1.xml");
    CHECK_NE(header_xml.find(R"(w:pStyle w:val="RenamedBody")"),
             std::string::npos);
    CHECK_EQ(header_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);

    CHECK_EQ(run_cli({"rename-style", renamed.string(), "RenamedBody", "Strong",
                      "--json"},
                     conflict_output),
             1);
    const auto conflict_json = read_text_file(conflict_output);
    CHECK_NE(conflict_json.find(R"("command":"rename-style")"),
             std::string::npos);
    CHECK_NE(conflict_json.find(R"("stage":"mutate")"), std::string::npos);
    CHECK_NE(conflict_json.find(
                 R"("detail":"target style id 'Strong' already exists in word/styles.xml")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(renamed);
    remove_if_exists(output);
    remove_if_exists(usage_output);
    remove_if_exists(conflict_output);
}

TEST_CASE("cli rename-style rewrites run and table style references") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_rename_style_refs_source.docx";
    const fs::path run_renamed = working_directory / "cli_rename_style_run.docx";
    const fs::path table_renamed = working_directory / "cli_rename_style_table.docx";
    const fs::path run_output = working_directory / "cli_rename_style_run.json";
    const fs::path table_output = working_directory / "cli_rename_style_table.json";
    const fs::path run_usage_output = working_directory / "cli_rename_style_run_usage.json";
    const fs::path table_usage_output = working_directory / "cli_rename_style_table_usage.json";

    remove_if_exists(source);
    remove_if_exists(run_renamed);
    remove_if_exists(table_renamed);
    remove_if_exists(run_output);
    remove_if_exists(table_output);
    remove_if_exists(run_usage_output);
    remove_if_exists(table_usage_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"rename-style",
                      source.string(),
                      "Strong",
                      "StrongAccent",
                      "--output",
                      run_renamed.string(),
                      "--json"},
                     run_output),
             0);
    const auto run_json = read_text_file(run_output);
    CHECK_NE(run_json.find(R"("old_style_id":"Strong")"), std::string::npos);
    CHECK_NE(run_json.find(R"("new_style_id":"StrongAccent")"),
             std::string::npos);

    const auto run_document_xml = read_docx_entry(run_renamed, "word/document.xml");
    CHECK_NE(run_document_xml.find(R"(w:rStyle w:val="StrongAccent")"),
             std::string::npos);
    CHECK_EQ(run_document_xml.find(R"(w:rStyle w:val="Strong")"),
             std::string::npos);
    const auto footer_xml = read_docx_entry(run_renamed, "word/footer1.xml");
    CHECK_NE(footer_xml.find(R"(w:rStyle w:val="StrongAccent")"),
             std::string::npos);
    CHECK_EQ(footer_xml.find(R"(w:rStyle w:val="Strong")"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", run_renamed.string(), "--style",
                      "StrongAccent", "--usage", "--json"},
                     run_usage_output),
             0);
    const auto run_usage_json = read_text_file(run_usage_output);
    CHECK_NE(run_usage_json.find(R"("style_id":"StrongAccent")"),
             std::string::npos);
    CHECK_NE(run_usage_json.find(R"("run_count":3)"), std::string::npos);

    CHECK_EQ(run_cli({"rename-style",
                      source.string(),
                      "ReportTable",
                      "ReportTableRenamed",
                      "--output",
                      table_renamed.string(),
                      "--json"},
                     table_output),
             0);
    const auto table_json = read_text_file(table_output);
    CHECK_NE(table_json.find(R"("old_style_id":"ReportTable")"),
             std::string::npos);
    CHECK_NE(table_json.find(R"("new_style_id":"ReportTableRenamed")"),
             std::string::npos);

    const auto table_document_xml = read_docx_entry(table_renamed, "word/document.xml");
    CHECK_NE(table_document_xml.find(R"(w:tblStyle w:val="ReportTableRenamed")"),
             std::string::npos);
    CHECK_EQ(table_document_xml.find(R"(w:tblStyle w:val="ReportTable")"),
             std::string::npos);
    const auto header_xml = read_docx_entry(table_renamed, "word/header1.xml");
    CHECK_NE(header_xml.find(R"(w:tblStyle w:val="ReportTableRenamed")"),
             std::string::npos);
    CHECK_EQ(header_xml.find(R"(w:tblStyle w:val="ReportTable")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", table_renamed.string(), "--style",
                      "ReportTableRenamed", "--usage", "--json"},
                     table_usage_output),
             0);
    const auto table_usage_json = read_text_file(table_usage_output);
    CHECK_NE(table_usage_json.find(R"("style_id":"ReportTableRenamed")"),
             std::string::npos);
    CHECK_NE(table_usage_json.find(R"("table_count":2)"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(run_renamed);
    remove_if_exists(table_renamed);
    remove_if_exists(run_output);
    remove_if_exists(table_output);
    remove_if_exists(run_usage_output);
    remove_if_exists(table_usage_output);
}

TEST_CASE("cli merge-style rewrites paragraph references and removes source style") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_merge_style_source.docx";
    const fs::path merged = working_directory / "cli_merge_style_merged.docx";
    const fs::path output = working_directory / "cli_merge_style_output.json";
    const fs::path usage_output = working_directory / "cli_merge_style_usage.json";
    const fs::path missing_output = working_directory / "cli_merge_style_missing.json";
    const fs::path mismatch_output = working_directory / "cli_merge_style_mismatch.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(output);
    remove_if_exists(usage_output);
    remove_if_exists(missing_output);
    remove_if_exists(mismatch_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::paragraph_style_definition child_style;
        child_style.name = "Custom Child";
        child_style.based_on = "CustomBody";
        child_style.next_style = "CustomBody";
        REQUIRE(document.ensure_paragraph_style("CustomChild", child_style));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"merge-style", source.string(), "Strong", "Normal", "--json"},
                     mismatch_output),
             1);
    const auto mismatch_json = read_text_file(mismatch_output);
    CHECK_NE(mismatch_json.find(R"("command":"merge-style")"), std::string::npos);
    CHECK_NE(mismatch_json.find(R"("stage":"mutate")"), std::string::npos);
    CHECK_NE(mismatch_json.find(
                 R"("detail":"source style id 'Strong' has type 'character' but target style id 'Normal' has type 'paragraph'")"),
             std::string::npos);

    CHECK_EQ(run_cli({"merge-style",
                      source.string(),
                      "CustomBody",
                      "Normal",
                      "--output",
                      merged.string(),
                      "--json"},
                     output),
             0);
    const auto merge_json = read_text_file(output);
    CHECK_NE(merge_json.find(R"("command":"merge-style")"), std::string::npos);
    CHECK_NE(merge_json.find(R"("source_style_id":"CustomBody")"),
             std::string::npos);
    CHECK_NE(merge_json.find(R"("target_style_id":"Normal")"), std::string::npos);
    CHECK_NE(merge_json.find(R"("style":{"style_id":"Normal")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", merged.string(), "--style", "CustomBody",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find(R"("command":"inspect-styles")"),
             std::string::npos);
    CHECK_NE(missing_json.find(
                 R"("detail":"style id 'CustomBody' was not found in word/styles.xml")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", merged.string(), "--style", "Normal",
                      "--usage", "--json"},
                     usage_output),
             0);
    const auto usage_json = read_text_file(usage_output);
    CHECK_NE(usage_json.find(R"("style_id":"Normal")"), std::string::npos);
    CHECK_NE(usage_json.find(R"("paragraph_count":3)"), std::string::npos);

    const auto styles_xml = read_docx_entry(merged, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    CHECK(find_style_xml_node(styles_root, "CustomBody") == pugi::xml_node{});
    CHECK(find_style_xml_node(styles_root, "Normal") != pugi::xml_node{});
    const auto child_style = find_style_xml_node(styles_root, "CustomChild");
    REQUIRE(child_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{child_style.child("w:basedOn").attribute("w:val").value()},
             "Normal");
    CHECK_EQ(std::string_view{child_style.child("w:next").attribute("w:val").value()},
             "Normal");

    const auto document_xml = read_docx_entry(merged, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="Normal")"), std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);
    const auto header_xml = read_docx_entry(merged, "word/header1.xml");
    CHECK_NE(header_xml.find(R"(w:pStyle w:val="Normal")"), std::string::npos);
    CHECK_EQ(header_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(output);
    remove_if_exists(usage_output);
    remove_if_exists(missing_output);
    remove_if_exists(mismatch_output);
}

TEST_CASE("cli merge-style rewrites run and table style references") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_merge_style_refs_source.docx";
    const fs::path run_merged = working_directory / "cli_merge_style_run.docx";
    const fs::path table_merged = working_directory / "cli_merge_style_table.docx";
    const fs::path run_output = working_directory / "cli_merge_style_run.json";
    const fs::path table_output = working_directory / "cli_merge_style_table.json";
    const fs::path run_usage_output = working_directory / "cli_merge_style_run_usage.json";
    const fs::path table_usage_output = working_directory / "cli_merge_style_table_usage.json";

    remove_if_exists(source);
    remove_if_exists(run_merged);
    remove_if_exists(table_merged);
    remove_if_exists(run_output);
    remove_if_exists(table_output);
    remove_if_exists(run_usage_output);
    remove_if_exists(table_usage_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::character_style_definition character_style;
        character_style.name = "Strong Target";
        REQUIRE(document.ensure_character_style("StrongTarget", character_style));
        featherdoc::table_style_definition table_style;
        table_style.name = "Report Table Target";
        REQUIRE(document.ensure_table_style("ReportTableTarget", table_style));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"merge-style",
                      source.string(),
                      "Strong",
                      "StrongTarget",
                      "--output",
                      run_merged.string(),
                      "--json"},
                     run_output),
             0);
    const auto run_json = read_text_file(run_output);
    CHECK_NE(run_json.find(R"("source_style_id":"Strong")"), std::string::npos);
    CHECK_NE(run_json.find(R"("target_style_id":"StrongTarget")"),
             std::string::npos);

    const auto run_document_xml = read_docx_entry(run_merged, "word/document.xml");
    CHECK_NE(run_document_xml.find(R"(w:rStyle w:val="StrongTarget")"),
             std::string::npos);
    CHECK_EQ(run_document_xml.find(R"(w:rStyle w:val="Strong")"),
             std::string::npos);
    const auto footer_xml = read_docx_entry(run_merged, "word/footer1.xml");
    CHECK_NE(footer_xml.find(R"(w:rStyle w:val="StrongTarget")"),
             std::string::npos);
    CHECK_EQ(footer_xml.find(R"(w:rStyle w:val="Strong")"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", run_merged.string(), "--style",
                      "StrongTarget", "--usage", "--json"},
                     run_usage_output),
             0);
    const auto run_usage_json = read_text_file(run_usage_output);
    CHECK_NE(run_usage_json.find(R"("style_id":"StrongTarget")"),
             std::string::npos);
    CHECK_NE(run_usage_json.find(R"("run_count":3)"), std::string::npos);

    CHECK_EQ(run_cli({"merge-style",
                      source.string(),
                      "ReportTable",
                      "ReportTableTarget",
                      "--output",
                      table_merged.string(),
                      "--json"},
                     table_output),
             0);
    const auto table_json = read_text_file(table_output);
    CHECK_NE(table_json.find(R"("source_style_id":"ReportTable")"),
             std::string::npos);
    CHECK_NE(table_json.find(R"("target_style_id":"ReportTableTarget")"),
             std::string::npos);

    const auto table_document_xml = read_docx_entry(table_merged, "word/document.xml");
    CHECK_NE(table_document_xml.find(R"(w:tblStyle w:val="ReportTableTarget")"),
             std::string::npos);
    CHECK_EQ(table_document_xml.find(R"(w:tblStyle w:val="ReportTable")"),
             std::string::npos);
    const auto header_xml = read_docx_entry(table_merged, "word/header1.xml");
    CHECK_NE(header_xml.find(R"(w:tblStyle w:val="ReportTableTarget")"),
             std::string::npos);
    CHECK_EQ(header_xml.find(R"(w:tblStyle w:val="ReportTable")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-styles", table_merged.string(), "--style",
                      "ReportTableTarget", "--usage", "--json"},
                     table_usage_output),
             0);
    const auto table_usage_json = read_text_file(table_usage_output);
    CHECK_NE(table_usage_json.find(R"("style_id":"ReportTableTarget")"),
             std::string::npos);
    CHECK_NE(table_usage_json.find(R"("table_count":2)"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(run_merged);
    remove_if_exists(table_merged);
    remove_if_exists(run_output);
    remove_if_exists(table_output);
    remove_if_exists(run_usage_output);
    remove_if_exists(table_usage_output);
}

TEST_CASE("cli plan-style-refactor validates batch rename and merge operations") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_style_refactor_plan_source.docx";
    const fs::path clean_output = working_directory / "cli_style_refactor_plan_clean.json";
    const fs::path dirty_output = working_directory / "cli_style_refactor_plan_dirty.json";
    const fs::path clean_plan_file =
        working_directory / "cli_style_refactor_plan_clean.plan.json";
    const fs::path parse_output = working_directory / "cli_style_refactor_plan_parse.json";

    remove_if_exists(source);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(clean_plan_file);
    remove_if_exists(parse_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "CustomBody:Normal",
                      "--output-plan",
                      clean_plan_file.string(),
                      "--json"},
                     clean_output),
             0);
    const auto clean_json = read_text_file(clean_output);
    CHECK_NE(clean_json.find(R"("command":"plan-style-refactor")"),
             std::string::npos);
    CHECK_NE(clean_json.find(R"("clean":true)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("operation_count":2)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("applyable_count":2)"), std::string::npos);
    CHECK_NE(clean_json.find(R"("source_reference_count":3)"), std::string::npos);
    CHECK_NE(clean_json.find(
                 R"("command_template":"featherdoc_cli rename-style <input.docx> CustomBody ReviewBody --output <output.docx> --json")"),
             std::string::npos);
    CHECK_NE(clean_json.find(
                 R"("command_template":"featherdoc_cli merge-style <input.docx> CustomBody Normal --output <output.docx> --json")"),
             std::string::npos);
    const auto clean_plan_json = read_text_file(clean_plan_file);
    CHECK_NE(clean_plan_json.find(R"("command":"plan-style-refactor")"),
             std::string::npos);
    CHECK_NE(clean_plan_json.find(R"("operations":[)"), std::string::npos);

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--merge",
                      "Strong:Normal",
                      "--rename",
                      "MissingBody:NewBody",
                      "--rename",
                      "CustomBody:Strong",
                      "--json"},
                     dirty_output),
             1);
    const auto dirty_json = read_text_file(dirty_output);
    CHECK_NE(dirty_json.find(R"("clean":false)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("operation_count":3)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("applyable_count":0)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("issue_count":3)"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"style_type_mismatch")"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"missing_source_style")"), std::string::npos);
    CHECK_NE(dirty_json.find(R"("code":"target_style_exists")"), std::string::npos);

    const auto source_styles_xml = read_docx_entry(source, "word/styles.xml");
    CHECK_NE(source_styles_xml.find(R"(w:styleId="CustomBody")"), std::string::npos);
    CHECK_EQ(source_styles_xml.find(R"(w:styleId="ReviewBody")"), std::string::npos);

    CHECK_EQ(run_cli({"plan-style-refactor", source.string(), "--json"}, parse_output), 2);
    CHECK_NE(read_text_file(parse_output)
                 .find("plan-style-refactor expects at least one --rename or --merge option"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(clean_plan_file);
    remove_if_exists(parse_output);
}

TEST_CASE("cli suggest-style-merges writes reviewable duplicate merge plan") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_suggest_style_merges_source.docx";
    const fs::path output = working_directory / "cli_suggest_style_merges_output.json";
    const fs::path filtered_output =
        working_directory / "cli_suggest_style_merges_filtered.json";
    const fs::path gate_output =
        working_directory / "cli_suggest_style_merges_gate.json";
    const fs::path filtered_gate_output =
        working_directory / "cli_suggest_style_merges_filtered_gate.json";
    const fs::path plan_file =
        working_directory / "cli_suggest_style_merges_plan.json";
    const fs::path filtered_plan_file =
        working_directory / "cli_suggest_style_merges_filtered.plan.json";
    const fs::path applied = working_directory / "cli_suggest_style_merges_applied.docx";
    const fs::path apply_output =
        working_directory / "cli_suggest_style_merges_apply.json";
    const fs::path rollback_file =
        working_directory / "cli_suggest_style_merges_rollback.json";
    const fs::path restored =
        working_directory / "cli_suggest_style_merges_restored.docx";
    const fs::path restore_output =
        working_directory / "cli_suggest_style_merges_restore.json";
    const fs::path dry_run_output =
        working_directory / "cli_suggest_style_merges_restore_dry_run.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(filtered_output);
    remove_if_exists(gate_output);
    remove_if_exists(filtered_gate_output);
    remove_if_exists(plan_file);
    remove_if_exists(filtered_plan_file);
    remove_if_exists(applied);
    remove_if_exists(apply_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restored);
    remove_if_exists(restore_output);
    remove_if_exists(dry_run_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--output-plan",
                      plan_file.string(),
                      "--json"},
                     output),
             0);

    const auto suggestion_json = read_text_file(output);
    CHECK_NE(suggestion_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("fail_on_suggestion":false)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("suggestion_gate_failed":false)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("action":"merge")"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(
                 R"("reason_code":"matching_style_signature_and_xml")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("confidence":95)"), std::string::npos);
    CHECK_NE(suggestion_json.find(R"("target_has_more_references")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("style_definition_xml_matches")"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("differences":[])"), std::string::npos);
    CHECK_NE(suggestion_json.find(
                 R"("suggestion_confidence_summary":{"suggestion_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("min_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("max_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("xml_difference_count":0)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(suggestion_json.find("automation gates"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--fail-on-suggestion",
                      "--json"},
                     gate_output),
             1);
    const auto gate_json = read_text_file(gate_output);
    CHECK_NE(gate_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("fail_on_suggestion":true)"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("suggestion_gate_failed":true)"),
             std::string::npos);
    CHECK_NE(gate_json.find(R"("clean":true)"), std::string::npos);
    CHECK_NE(gate_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(gate_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);

    const auto plan_json = read_text_file(plan_file);
    CHECK_NE(plan_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("operations":[)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("confidence":95)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_EQ(plan_json.find(R"("fail_on_suggestion")"), std::string::npos);
    CHECK_EQ(plan_json.find(R"("suggestion_gate_failed")"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "96",
                      "--output-plan",
                      filtered_plan_file.string(),
                      "--json"},
                     filtered_output),
             0);
    const auto filtered_json = read_text_file(filtered_output);
    CHECK_NE(filtered_json.find(R"("command":"suggest-style-merges")"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("operation_count":0)"), std::string::npos);
    CHECK_NE(filtered_json.find(R"("applyable_count":0)"), std::string::npos);
    CHECK_NE(filtered_json.find(R"("suggestion_confidence_summary":{"suggestion_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("max_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("recommended_min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_json.find(R"("operations":[])"), std::string::npos);
    CHECK_EQ(filtered_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "96",
                      "--fail-on-suggestion",
                      "--json"},
                     filtered_gate_output),
             0);
    const auto filtered_gate_json = read_text_file(filtered_gate_output);
    CHECK_NE(filtered_gate_json.find(R"("fail_on_suggestion":true)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("suggestion_gate_failed":false)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_gate_json.find(R"("operations":[])"), std::string::npos);

    const auto filtered_plan_json = read_text_file(filtered_plan_file);
    CHECK_NE(filtered_plan_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_NE(filtered_plan_json.find(R"("recommended_min_confidence":null)"),
             std::string::npos);
    CHECK_NE(filtered_plan_json.find(R"("operations":[])"), std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--plan-file",
                      plan_file.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_NE(read_text_file(apply_output).find(R"("command":"apply-style-refactor")"),
             std::string::npos);
    const auto suggestion_rollback_json = read_text_file(rollback_file);
    CHECK_NE(suggestion_rollback_json.find(R"("rollback_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("restorable":true)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("source_reference_count":1)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("source_style_xml":")"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"("node_ordinal":3)"),
             std::string::npos);
    CHECK_NE(suggestion_rollback_json.find(R"(w:styleId=\"DuplicateBodyB\")"),
             std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK_FALSE(document.find_style("DuplicateBodyB").has_value());
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--dry-run",
                      "--json"},
                     dry_run_output),
             0);
    const auto dry_run_json = read_text_file(dry_run_output);
    CHECK_NE(dry_run_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("changed":false)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("issue_count":0)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("issue_summary":[])"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_count":1)"), std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_style_count":1)"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("restored_reference_count":1)"),
             std::string::npos);
    CHECK_NE(dry_run_json.find(R"("entry_index":0)"), std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK_FALSE(document.find_style("DuplicateBodyB").has_value());
        const auto target_usage = document.find_style_usage("DuplicateBodyA");
        REQUIRE(target_usage.has_value());
        CHECK_EQ(target_usage->paragraph_count, 3U);
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--output",
                      restored.string(),
                      "--json"},
                     restore_output),
             0);
    const auto restore_json = read_text_file(restore_output);
    CHECK_NE(restore_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_count":1)"), std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_style_count":1)"),
             std::string::npos);
    CHECK_NE(restore_json.find(R"("restored_reference_count":1)"),
             std::string::npos);
    {
        featherdoc::Document document(restored);
        REQUIRE_FALSE(document.open());
        CHECK(document.find_style("DuplicateBodyA").has_value());
        CHECK(document.find_style("DuplicateBodyB").has_value());
        const auto target_usage = document.find_style_usage("DuplicateBodyA");
        REQUIRE(target_usage.has_value());
        CHECK_EQ(target_usage->paragraph_count, 2U);
        const auto source_usage = document.find_style_usage("DuplicateBodyB");
        REQUIRE(source_usage.has_value());
        CHECK_EQ(source_usage->paragraph_count, 1U);
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      restored.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "0",
                      "--dry-run",
                      "--json"},
                     parse_output),
             1);
    const auto restore_conflict_json = read_text_file(parse_output);
    CHECK_NE(restore_conflict_json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("issue_count":1)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(
                 R"("issue_summary":[{"code":"source_style_exists","count":1)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("code":"source_style_exists")"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find(R"("suggestion":"The source style already exists;)"),
             std::string::npos);
    CHECK_NE(restore_conflict_json.find("skip this rollback entry"),
             std::string::npos);

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--dry-run",
                      "--output",
                      restored.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("restore-style-merge --dry-run cannot be combined with --output"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--min-confidence",
                      "101",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--min-confidence expects an integer from 0 to 100"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges", source.string(), "--bad", "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find(R"("stage":"parse")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(filtered_output);
    remove_if_exists(gate_output);
    remove_if_exists(filtered_gate_output);
    remove_if_exists(plan_file);
    remove_if_exists(filtered_plan_file);
    remove_if_exists(applied);
    remove_if_exists(apply_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restored);
    remove_if_exists(restore_output);
    remove_if_exists(dry_run_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli suggest-style-merges applies confidence profiles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_suggest_style_merges_profiles_source.docx";
    const fs::path recommended_output =
        working_directory / "cli_suggest_style_merges_profiles_recommended.json";
    const fs::path strict_output =
        working_directory / "cli_suggest_style_merges_profiles_strict.json";
    const fs::path review_output =
        working_directory / "cli_suggest_style_merges_profiles_review.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_profiles_parse.json";

    remove_if_exists(source);
    remove_if_exists(recommended_output);
    remove_if_exists(strict_output);
    remove_if_exists(review_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_profile_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "recommended",
                      "--json"},
                     recommended_output),
             0);
    const auto recommended_json = read_text_file(recommended_output);
    CHECK_NE(recommended_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(recommended_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(recommended_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(recommended_json.find(R"("min_confidence":95)"),
             std::string::npos);
    CHECK_NE(recommended_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(recommended_json.find("automation gates"), std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "strict",
                      "--json"},
                     strict_output),
             0);
    const auto strict_json = read_text_file(strict_output);
    CHECK_NE(strict_json.find(R"("operation_count":1)"), std::string::npos);
    CHECK_NE(strict_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(strict_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("min_confidence":95)"), std::string::npos);
    CHECK_NE(strict_json.find(R"("recommended_min_confidence":95)"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(strict_json.find(R"("xml_difference_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "review",
                      "--json"},
                     review_output),
             0);
    const auto review_json = read_text_file(review_output);
    CHECK_NE(review_json.find(R"("operation_count":2)"), std::string::npos);
    CHECK_NE(review_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("confidence":80)"), std::string::npos);
    CHECK_NE(review_json.find(R"("min_confidence":80)"), std::string::npos);
    CHECK_NE(review_json.find(R"("exact_xml_match_count":1)"),
             std::string::npos);
    CHECK_NE(review_json.find(R"("xml_difference_count":1)"),
             std::string::npos);
    CHECK_NE(review_json.find("review lower-confidence XML differences manually"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "strict",
                      "--min-confidence",
                      "80",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--confidence-profile cannot be combined with --min-confidence"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--confidence-profile",
                      "unsafe",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("--confidence-profile expects recommended, strict, review, or exploratory"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(recommended_output);
    remove_if_exists(strict_output);
    remove_if_exists(review_output);
    remove_if_exists(parse_output);
}


TEST_CASE("cli suggest-style-merges filters by style ids") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_suggest_style_merges_filters_source.docx";
    const fs::path source_filter_output =
        working_directory / "cli_suggest_style_merges_filters_source.json";
    const fs::path target_filter_output =
        working_directory / "cli_suggest_style_merges_filters_target.json";
    const fs::path missing_filter_output =
        working_directory / "cli_suggest_style_merges_filters_missing.json";
    const fs::path parse_output =
        working_directory / "cli_suggest_style_merges_filters_parse.json";

    remove_if_exists(source);
    remove_if_exists(source_filter_output);
    remove_if_exists(target_filter_output);
    remove_if_exists(missing_filter_output);
    remove_if_exists(parse_output);

    create_cli_duplicate_style_profile_fixture(source);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "DuplicateBodyC",
                      "--json"},
                     source_filter_output),
             0);
    const auto source_filter_json = read_text_file(source_filter_output);
    CHECK_NE(source_filter_json.find(R"("operation_count":1)"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_EQ(source_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("confidence":80)"), std::string::npos);
    CHECK_NE(source_filter_json.find(R"("exact_xml_match_count":0)"),
             std::string::npos);
    CHECK_NE(source_filter_json.find(R"("xml_difference_count":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--target-style",
                      "DuplicateBodyA",
                      "--json"},
                     target_filter_output),
             0);
    const auto target_filter_json = read_text_file(target_filter_output);
    CHECK_NE(target_filter_json.find(R"("operation_count":2)"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);
    CHECK_NE(target_filter_json.find(R"("target_style_id":"DuplicateBodyA")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "MissingStyle",
                      "--json"},
                     missing_filter_output),
             0);
    const auto missing_filter_json = read_text_file(missing_filter_output);
    CHECK_NE(missing_filter_json.find(R"("operation_count":0)"),
             std::string::npos);
    CHECK_EQ(missing_filter_json.find(R"("source_style_id":"DuplicateBodyB")"),
             std::string::npos);
    CHECK_EQ(missing_filter_json.find(R"("source_style_id":"DuplicateBodyC")"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--source-style",
                      "DuplicateBodyB",
                      "--source-style",
                      "DuplicateBodyB",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find("duplicate --source-style id"),
             std::string::npos);

    CHECK_EQ(run_cli({"suggest-style-merges",
                      source.string(),
                      "--target-style",
                      "DuplicateBodyA",
                      "--target-style",
                      "DuplicateBodyA",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find("duplicate --target-style id"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(source_filter_output);
    remove_if_exists(target_filter_output);
    remove_if_exists(missing_filter_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli apply-style-refactor applies clean batch and blocks conflicts") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_style_refactor_apply_source.docx";
    const fs::path applied = working_directory / "cli_style_refactor_apply_output.docx";
    const fs::path dirty_output =
        working_directory / "cli_style_refactor_apply_dirty.docx";
    const fs::path output = working_directory / "cli_style_refactor_apply.json";
    const fs::path plan_file =
        working_directory / "cli_style_refactor_apply.plan.json";
    const fs::path plan_output =
        working_directory / "cli_style_refactor_apply_plan_output.json";
    const fs::path rollback_file =
        working_directory / "cli_style_refactor_apply.rollback.json";
    const fs::path restore_dry_run_output =
        working_directory / "cli_style_refactor_apply_restore_dry_run.json";
    const fs::path dirty_json =
        working_directory / "cli_style_refactor_apply_dirty.json";
    const fs::path parse_output =
        working_directory / "cli_style_refactor_apply_parse.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(dirty_output);
    remove_if_exists(output);
    remove_if_exists(plan_file);
    remove_if_exists(plan_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restore_dry_run_output);
    remove_if_exists(dirty_json);
    remove_if_exists(parse_output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        featherdoc::paragraph_style_definition archive_body;
        archive_body.name = "Archive Body";
        archive_body.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("ArchiveBody", archive_body));
        featherdoc::paragraph_style_definition archive_note;
        archive_note.name = "Archive Note";
        archive_note.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("ArchiveNote", archive_note));
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"plan-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "ArchiveBody:Normal",
                      "--merge",
                      "ArchiveNote:Normal",
                      "--output-plan",
                      plan_file.string(),
                      "--json"},
                     plan_output),
             0);
    const auto plan_json = read_text_file(plan_file);
    CHECK_NE(plan_json.find(R"("source_style_id":"ArchiveBody")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("source_style_id":"ArchiveNote")"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--plan-file",
                      plan_file.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    const auto apply_json = read_text_file(output);
    CHECK_NE(apply_json.find(R"("command":"apply-style-refactor")"),
             std::string::npos);
    CHECK_NE(apply_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("changed":true)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("requested_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("applied_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("skipped_count":0)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("rollback_count":3)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("plan_file":)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("rollback_plan_file":)"), std::string::npos);
    CHECK_NE(apply_json.find(R"("clean":true)"), std::string::npos);
    const auto rollback_json = read_text_file(rollback_file);
    CHECK_NE(rollback_json.find(R"("rollback_count":3)"), std::string::npos);
    CHECK_NE(rollback_json.find(
                 R"("command_template":"featherdoc_cli rename-style <input.docx> ReviewBody CustomBody --output <output.docx> --json")"),
             std::string::npos);
    CHECK_NE(rollback_json.find(R"("automatic":false)"), std::string::npos);
    CHECK_NE(rollback_json.find(R"("restorable":true)"), std::string::npos);
    CHECK_NE(rollback_json.find(R"("source_style_xml":")"), std::string::npos);
    CHECK_NE(rollback_json.find(R"(w:styleId=\"ArchiveBody\")"),
             std::string::npos);
    CHECK_NE(rollback_json.find(R"(w:styleId=\"ArchiveNote\")"),
             std::string::npos);

    const auto styles_xml = read_docx_entry(applied, "word/styles.xml");
    CHECK_EQ(styles_xml.find(R"(w:styleId="CustomBody")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="ReviewBody")"),
             std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="ArchiveBody")"),
             std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="ArchiveNote")"),
             std::string::npos);
    const auto document_xml = read_docx_entry(applied, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w:pStyle w:val="ReviewBody")"),
             std::string::npos);
    CHECK_EQ(document_xml.find(R"(w:pStyle w:val="CustomBody")"),
             std::string::npos);

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "1",
                      "--entry",
                      "2",
                      "--dry-run",
                      "--json"},
                     restore_dry_run_output),
             0);
    const auto restore_dry_run_json = read_text_file(restore_dry_run_output);
    CHECK_NE(restore_dry_run_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("requested_count":2)"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("restored_count":2)"),
             std::string::npos);
    CHECK_NE(restore_dry_run_json.find(R"("entry_indexes":[1,2])"),
             std::string::npos);
    {
        featherdoc::Document document(applied);
        REQUIRE_FALSE(document.open());
        CHECK_FALSE(document.find_style("ArchiveBody").has_value());
        CHECK_FALSE(document.find_style("ArchiveNote").has_value());
    }

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--source-style",
                      "ArchiveNote",
                      "--target-style",
                      "Normal",
                      "--plan-only",
                      "--json"},
                     restore_dry_run_output),
             0);
    const auto restore_filter_json = read_text_file(restore_dry_run_output);
    CHECK_NE(restore_filter_json.find(R"("command":"restore-style-merge")"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("dry_run":true)"), std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("requested_count":1)"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("restored_count":1)"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("source_style_ids":["ArchiveNote"])"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("target_style_ids":["Normal"])"),
             std::string::npos);
    CHECK_NE(restore_filter_json.find(R"("source_style_id":"ArchiveNote")"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor",
                      source.string(),
                      "--rename",
                      "CustomBody:ReviewBody",
                      "--merge",
                      "CustomBody:Normal",
                      "--output",
                      dirty_output.string(),
                      "--json"},
                     dirty_json),
             1);
    const auto dirty_text = read_text_file(dirty_json);
    CHECK_NE(dirty_text.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("changed":false)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("applied_count":0)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("skipped_count":2)"), std::string::npos);
    CHECK_NE(dirty_text.find(R"("code":"duplicate_source_operation")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(dirty_output));

    CHECK_EQ(run_cli({"restore-style-merge",
                      applied.string(),
                      "--rollback-plan",
                      rollback_file.string(),
                      "--entry",
                      "1",
                      "--source-style",
                      "ArchiveBody",
                      "--dry-run",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("restore-style-merge --entry cannot be combined with --source-style or --target-style"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-style-refactor", source.string(), "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output)
                 .find("apply-style-refactor expects --plan-file or at least one --rename or --merge option"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(dirty_output);
    remove_if_exists(output);
    remove_if_exists(plan_file);
    remove_if_exists(plan_output);
    remove_if_exists(rollback_file);
    remove_if_exists(restore_dry_run_output);
    remove_if_exists(dirty_json);
    remove_if_exists(parse_output);
}

TEST_CASE("cli prune-unused-styles removes unreachable custom styles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_prune_unused_styles_source.docx";
    const fs::path pruned = working_directory / "cli_prune_unused_styles_pruned.docx";
    const fs::path plan_output = working_directory / "cli_prune_unused_styles_plan.json";
    const fs::path output = working_directory / "cli_prune_unused_styles_output.json";

    remove_if_exists(source);
    remove_if_exists(pruned);
    remove_if_exists(plan_output);
    remove_if_exists(output);

    create_cli_style_usage_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());

        featherdoc::paragraph_style_definition dependency_style;
        dependency_style.name = "Dependency Style";
        dependency_style.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("DependencyStyle", dependency_style));

        featherdoc::paragraph_style_definition custom_body;
        custom_body.name = "Custom Body";
        custom_body.based_on = "DependencyStyle";
        custom_body.is_quick_format = true;
        REQUIRE(document.ensure_paragraph_style("CustomBody", custom_body));

        featherdoc::paragraph_style_definition unused_body;
        unused_body.name = "Unused Body";
        unused_body.based_on = "Normal";
        REQUIRE(document.ensure_paragraph_style("UnusedBody", unused_body));

        featherdoc::character_style_definition unused_character;
        unused_character.name = "Unused Character";
        REQUIRE(document.ensure_character_style("UnusedCharacter", unused_character));

        featherdoc::table_style_definition unused_table;
        unused_table.name = "Unused Table";
        REQUIRE(document.ensure_table_style("UnusedTable", unused_table));

        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"plan-prune-unused-styles", source.string(), "--json"},
                     plan_output),
             0);
    const auto plan_json = read_text_file(plan_output);
    CHECK_NE(plan_json.find(R"("command":"plan-prune-unused-styles")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("removable_style_count":3)"), std::string::npos);
    CHECK_NE(plan_json.find(R"("UnusedBody")"), std::string::npos);
    CHECK_NE(plan_json.find(R"("UnusedCharacter")"), std::string::npos);
    CHECK_NE(plan_json.find(R"("UnusedTable")"), std::string::npos);
    const auto source_styles_xml = read_docx_entry(source, "word/styles.xml");
    CHECK_NE(source_styles_xml.find(R"(w:styleId="UnusedBody")"), std::string::npos);

    CHECK_EQ(run_cli({"prune-unused-styles",
                      source.string(),
                      "--output",
                      pruned.string(),
                      "--json"},
                     output),
             0);
    const auto prune_json = read_text_file(output);
    CHECK_NE(prune_json.find(R"("command":"prune-unused-styles")"),
             std::string::npos);
    CHECK_NE(prune_json.find(R"("removed_style_count":3)"), std::string::npos);
    CHECK_NE(prune_json.find(R"("UnusedBody")"), std::string::npos);
    CHECK_NE(prune_json.find(R"("UnusedCharacter")"), std::string::npos);
    CHECK_NE(prune_json.find(R"("UnusedTable")"), std::string::npos);

    const auto styles_xml = read_docx_entry(pruned, "word/styles.xml");
    CHECK_NE(styles_xml.find(R"(w:styleId="CustomBody")"), std::string::npos);
    CHECK_NE(styles_xml.find(R"(w:styleId="DependencyStyle")"), std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="UnusedBody")"), std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="UnusedCharacter")"), std::string::npos);
    CHECK_EQ(styles_xml.find(R"(w:styleId="UnusedTable")"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(pruned);
    remove_if_exists(plan_output);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-styles reports full style usage catalog") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_styles_usage_report_source.docx";
    const fs::path output = working_directory / "cli_styles_usage_report.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"inspect-styles", source.string(), "--usage", "--json"}, output), 0);
    const auto report_json = read_text_file(output);
    CHECK_NE(report_json.find(R"("count":4)"), std::string::npos);
    CHECK_NE(report_json.find(R"("used_style_count":3)"), std::string::npos);
    CHECK_NE(report_json.find(R"("unused_style_count":1)"), std::string::npos);
    CHECK_NE(report_json.find(R"("total_reference_count":8)"), std::string::npos);
    CHECK_NE(report_json.find(R"("style":{"style_id":"CustomBody")"),
             std::string::npos);
    CHECK_NE(report_json.find(R"("usage":{"style_id":"CustomBody","paragraph_count":3)"),
             std::string::npos);
    CHECK_NE(report_json.find(R"("style":{"style_id":"Normal")"), std::string::npos);
    CHECK_NE(report_json.find(R"("usage":{"style_id":"Normal","paragraph_count":0)"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-style-inheritance resolves effective properties across basedOn chain") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_inheritance_source.docx";
    const fs::path rooted =
        working_directory / "cli_style_inheritance_rooted.docx";
    const fs::path based =
        working_directory / "cli_style_inheritance_based.docx";
    const fs::path updated =
        working_directory / "cli_style_inheritance_updated.docx";
    const fs::path root_output =
        working_directory / "cli_style_inheritance_root_output.json";
    const fs::path base_output =
        working_directory / "cli_style_inheritance_base_output.json";
    const fs::path child_output =
        working_directory / "cli_style_inheritance_child_output.json";
    const fs::path inspect_output =
        working_directory / "cli_style_inheritance_inspect_output.json";

    remove_if_exists(source);
    remove_if_exists(rooted);
    remove_if_exists(based);
    remove_if_exists(updated);
    remove_if_exists(root_output);
    remove_if_exists(base_output);
    remove_if_exists(child_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      source.string(),
                      "Normal",
                      "--bidi-language",
                      "ar-SA",
                      "--output",
                      rooted.string(),
                      "--json"},
                     root_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      rooted.string(),
                      "BaseStyle",
                      "--name",
                      "Base Style",
                      "--based-on",
                      "Normal",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-rtl",
                      "true",
                      "--output",
                      based.string(),
                      "--json"},
                     base_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      based.string(),
                      "ChildStyle",
                      "--name",
                      "Child Style",
                      "--based-on",
                      "BaseStyle",
                      "--run-language",
                      "en-US",
                      "--paragraph-bidi",
                      "true",
                      "--output",
                      updated.string(),
                      "--json"},
                     child_output),
             0);

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      updated.string(),
                      "ChildStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"type\":\"paragraph\","
            "\"kind\":\"paragraph\",\"based_on\":\"BaseStyle\","
            "\"inheritance_chain\":[\"ChildStyle\",\"BaseStyle\",\"Normal\"],"
            "\"resolved_properties\":{\"font_family\":{\"value\":\"Segoe UI\","
            "\"source_style_id\":\"BaseStyle\"},"
            "\"east_asia_font_family\":{\"value\":null,\"source_style_id\":null},"
            "\"language\":{\"value\":\"en-US\",\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_language\":{\"value\":\"zh-CN\","
            "\"source_style_id\":\"BaseStyle\"},"
            "\"bidi_language\":{\"value\":\"ar-SA\","
            "\"source_style_id\":\"Normal\"},"
            "\"rtl\":{\"value\":true,\"source_style_id\":\"BaseStyle\"},"
            "\"paragraph_bidi\":{\"value\":true,"
            "\"source_style_id\":\"ChildStyle\"}}}\n"});

    remove_if_exists(source);
    remove_if_exists(rooted);
    remove_if_exists(based);
    remove_if_exists(updated);
    remove_if_exists(root_output);
    remove_if_exists(base_output);
    remove_if_exists(child_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli inspect-style-inheritance reports parse and inspect errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_inheritance_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_style_inheritance_parse_output.json";
    const fs::path inspect_output =
        working_directory / "cli_style_inheritance_inspect_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      source.string(),
                      "Normal",
                      "--bogus",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-style-inheritance\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      source.string(),
                      "MissingStyle",
                      "--json"},
                     inspect_output),
             1);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"command\":\"inspect-style-inheritance\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(inspect_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli materialize-style-run-properties freezes inherited values on the child style") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_materialize_style_source.docx";
    const fs::path rooted =
        working_directory / "cli_materialize_style_rooted.docx";
    const fs::path based =
        working_directory / "cli_materialize_style_based.docx";
    const fs::path child =
        working_directory / "cli_materialize_style_child.docx";
    const fs::path materialized =
        working_directory / "cli_materialize_style_materialized.docx";
    const fs::path base_mutated =
        working_directory / "cli_materialize_style_base_mutated.docx";
    const fs::path updated =
        working_directory / "cli_materialize_style_updated.docx";
    const fs::path root_output =
        working_directory / "cli_materialize_style_root_output.json";
    const fs::path base_output =
        working_directory / "cli_materialize_style_base_output.json";
    const fs::path child_output =
        working_directory / "cli_materialize_style_child_output.json";
    const fs::path materialize_output =
        working_directory / "cli_materialize_style_materialize_output.json";
    const fs::path base_mutate_output =
        working_directory / "cli_materialize_style_base_mutate_output.json";
    const fs::path normal_mutate_output =
        working_directory / "cli_materialize_style_normal_mutate_output.json";
    const fs::path inspect_output =
        working_directory / "cli_materialize_style_inspect_output.json";

    remove_if_exists(source);
    remove_if_exists(rooted);
    remove_if_exists(based);
    remove_if_exists(child);
    remove_if_exists(materialized);
    remove_if_exists(base_mutated);
    remove_if_exists(updated);
    remove_if_exists(root_output);
    remove_if_exists(base_output);
    remove_if_exists(child_output);
    remove_if_exists(materialize_output);
    remove_if_exists(base_mutate_output);
    remove_if_exists(normal_mutate_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      source.string(),
                      "Normal",
                      "--bidi-language",
                      "ar-SA",
                      "--output",
                      rooted.string(),
                      "--json"},
                     root_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      rooted.string(),
                      "BaseStyle",
                      "--name",
                      "Base Style",
                      "--based-on",
                      "Normal",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-rtl",
                      "true",
                      "--paragraph-bidi",
                      "true",
                      "--output",
                      based.string(),
                      "--json"},
                     base_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      based.string(),
                      "ChildStyle",
                      "--name",
                      "Child Style",
                      "--based-on",
                      "BaseStyle",
                      "--output",
                      child.string(),
                      "--json"},
                     child_output),
             0);

    CHECK_EQ(run_cli({"materialize-style-run-properties",
                      child.string(),
                      "ChildStyle",
                      "--output",
                      materialized.string(),
                      "--json"},
                     materialize_output),
             0);
    CHECK_EQ(
        read_text_file(materialize_output),
        std::string{
            "{\"command\":\"materialize-style-run-properties\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"ChildStyle\",\"materialized\":["
            "{\"field\":\"font_family\",\"source_style_id\":\"BaseStyle\"},"
            "{\"field\":\"east_asia_language\",\"source_style_id\":\"BaseStyle\"},"
            "{\"field\":\"bidi_language\",\"source_style_id\":\"Normal\"},"
            "{\"field\":\"rtl\",\"source_style_id\":\"BaseStyle\"},"
            "{\"field\":\"paragraph_bidi\",\"source_style_id\":\"BaseStyle\"}]}\n"});

    CHECK_EQ(run_cli({"set-style-run-properties",
                      materialized.string(),
                      "BaseStyle",
                      "--font-family",
                      "Arial",
                      "--east-asia-language",
                      "ja-JP",
                      "--rtl",
                      "false",
                      "--paragraph-bidi",
                      "false",
                      "--output",
                      base_mutated.string(),
                      "--json"},
                     base_mutate_output),
             0);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      base_mutated.string(),
                      "Normal",
                      "--bidi-language",
                      "he-IL",
                      "--output",
                      updated.string(),
                      "--json"},
                     normal_mutate_output),
             0);

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      updated.string(),
                      "ChildStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"type\":\"paragraph\","
            "\"kind\":\"paragraph\",\"based_on\":\"BaseStyle\","
            "\"inheritance_chain\":[\"ChildStyle\",\"BaseStyle\",\"Normal\"],"
            "\"resolved_properties\":{\"font_family\":{\"value\":\"Segoe UI\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_font_family\":{\"value\":null,\"source_style_id\":null},"
            "\"language\":{\"value\":null,\"source_style_id\":null},"
            "\"east_asia_language\":{\"value\":\"zh-CN\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"bidi_language\":{\"value\":\"ar-SA\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"rtl\":{\"value\":true,\"source_style_id\":\"ChildStyle\"},"
            "\"paragraph_bidi\":{\"value\":true,"
            "\"source_style_id\":\"ChildStyle\"}}}\n"});

    remove_if_exists(source);
    remove_if_exists(rooted);
    remove_if_exists(based);
    remove_if_exists(child);
    remove_if_exists(materialized);
    remove_if_exists(base_mutated);
    remove_if_exists(updated);
    remove_if_exists(root_output);
    remove_if_exists(base_output);
    remove_if_exists(child_output);
    remove_if_exists(materialize_output);
    remove_if_exists(base_mutate_output);
    remove_if_exists(normal_mutate_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli materialize-style-run-properties reports parse and inspect errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_materialize_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_materialize_style_parse_output.json";
    const fs::path inspect_output =
        working_directory / "cli_materialize_style_inspect_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"materialize-style-run-properties",
                      source.string(),
                      "Normal",
                      "--bogus",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"materialize-style-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    CHECK_EQ(run_cli({"materialize-style-run-properties",
                      source.string(),
                      "MissingStyle",
                      "--json"},
                     inspect_output),
             1);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"command\":\"materialize-style-run-properties\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(inspect_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli rebase-paragraph-style-based-on preserves inherited values while switching parent") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_rebase_paragraph_style_source.docx";
    const fs::path base_a_doc =
        working_directory / "cli_rebase_paragraph_style_base_a.docx";
    const fs::path base_b_doc =
        working_directory / "cli_rebase_paragraph_style_base_b.docx";
    const fs::path child_doc =
        working_directory / "cli_rebase_paragraph_style_child.docx";
    const fs::path rebased_doc =
        working_directory / "cli_rebase_paragraph_style_rebased.docx";
    const fs::path base_a_output =
        working_directory / "cli_rebase_paragraph_style_base_a.json";
    const fs::path base_b_output =
        working_directory / "cli_rebase_paragraph_style_base_b.json";
    const fs::path child_output =
        working_directory / "cli_rebase_paragraph_style_child.json";
    const fs::path rebase_output =
        working_directory / "cli_rebase_paragraph_style_rebase.json";
    const fs::path inspect_output =
        working_directory / "cli_rebase_paragraph_style_inspect.json";
    const fs::path properties_output =
        working_directory / "cli_rebase_paragraph_style_properties.json";

    remove_if_exists(source);
    remove_if_exists(base_a_doc);
    remove_if_exists(base_b_doc);
    remove_if_exists(child_doc);
    remove_if_exists(rebased_doc);
    remove_if_exists(base_a_output);
    remove_if_exists(base_b_output);
    remove_if_exists(child_output);
    remove_if_exists(rebase_output);
    remove_if_exists(inspect_output);
    remove_if_exists(properties_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      source.string(),
                      "BaseA",
                      "--name",
                      "Base A",
                      "--based-on",
                      "Normal",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-rtl",
                      "true",
                      "--output",
                      base_a_doc.string(),
                      "--json"},
                     base_a_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      base_a_doc.string(),
                      "BaseB",
                      "--name",
                      "Base B",
                      "--based-on",
                      "Normal",
                      "--run-font-family",
                      "Arial",
                      "--run-east-asia-language",
                      "ja-JP",
                      "--run-rtl",
                      "false",
                      "--output",
                      base_b_doc.string(),
                      "--json"},
                     base_b_output),
             0);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      base_b_doc.string(),
                      "ChildStyle",
                      "--name",
                      "Child Style",
                      "--based-on",
                      "BaseA",
                      "--run-language",
                      "en-US",
                      "--output",
                      child_doc.string(),
                      "--json"},
                     child_output),
             0);

    CHECK_EQ(run_cli({"rebase-paragraph-style-based-on",
                      child_doc.string(),
                      "ChildStyle",
                      "BaseB",
                      "--output",
                      rebased_doc.string(),
                      "--json"},
                     rebase_output),
             0);
    CHECK_EQ(
        read_text_file(rebase_output),
        std::string{
            "{\"command\":\"rebase-paragraph-style-based-on\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"ChildStyle\",\"based_on\":\"BaseB\",\"preserved\":["
            "{\"field\":\"font_family\",\"source_style_id\":\"BaseA\"},"
            "{\"field\":\"east_asia_language\",\"source_style_id\":\"BaseA\"},"
            "{\"field\":\"rtl\",\"source_style_id\":\"BaseA\"}]}\n"});

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      rebased_doc.string(),
                      "ChildStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"type\":\"paragraph\","
            "\"kind\":\"paragraph\",\"based_on\":\"BaseB\","
            "\"inheritance_chain\":[\"ChildStyle\",\"BaseB\",\"Normal\"],"
            "\"resolved_properties\":{\"font_family\":{\"value\":\"Segoe UI\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_font_family\":{\"value\":null,\"source_style_id\":null},"
            "\"language\":{\"value\":\"en-US\",\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_language\":{\"value\":\"zh-CN\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"bidi_language\":{\"value\":null,\"source_style_id\":null},"
            "\"rtl\":{\"value\":true,\"source_style_id\":\"ChildStyle\"},"
            "\"paragraph_bidi\":{\"value\":null,"
            "\"source_style_id\":null}}}\n"});

    CHECK_EQ(run_cli({"inspect-paragraph-style-properties",
                      rebased_doc.string(),
                      "ChildStyle",
                      "--json"},
                     properties_output),
             0);
    CHECK_EQ(
        read_text_file(properties_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"paragraph_style_properties\":{"
            "\"based_on\":\"BaseB\",\"next_style\":null,"
            "\"outline_level\":null}}\n"});

    remove_if_exists(source);
    remove_if_exists(base_a_doc);
    remove_if_exists(base_b_doc);
    remove_if_exists(child_doc);
    remove_if_exists(rebased_doc);
    remove_if_exists(base_a_output);
    remove_if_exists(base_b_output);
    remove_if_exists(child_output);
    remove_if_exists(rebase_output);
    remove_if_exists(inspect_output);
    remove_if_exists(properties_output);
}

TEST_CASE("cli rebase-paragraph-style-based-on reports parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_rebase_paragraph_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_rebase_paragraph_style_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_rebase_paragraph_style_mutate.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"rebase-paragraph-style-based-on",
                      source.string(),
                      "Normal",
                      "Heading1",
                      "--bogus",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"rebase-paragraph-style-based-on\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    CHECK_EQ(run_cli({"rebase-paragraph-style-based-on",
                      source.string(),
                      "MissingStyle",
                      "Heading1",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"rebase-paragraph-style-based-on\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(mutate_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli rebase-character-style-based-on preserves inherited values while switching parent") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_rebase_character_style_source.docx";
    const fs::path base_a_doc =
        working_directory / "cli_rebase_character_style_base_a.docx";
    const fs::path base_b_doc =
        working_directory / "cli_rebase_character_style_base_b.docx";
    const fs::path child_doc =
        working_directory / "cli_rebase_character_style_child.docx";
    const fs::path rebased_doc =
        working_directory / "cli_rebase_character_style_rebased.docx";
    const fs::path base_a_output =
        working_directory / "cli_rebase_character_style_base_a.json";
    const fs::path base_b_output =
        working_directory / "cli_rebase_character_style_base_b.json";
    const fs::path child_output =
        working_directory / "cli_rebase_character_style_child.json";
    const fs::path rebase_output =
        working_directory / "cli_rebase_character_style_rebase.json";
    const fs::path inspect_output =
        working_directory / "cli_rebase_character_style_inspect.json";
    const fs::path style_output =
        working_directory / "cli_rebase_character_style_style.json";

    remove_if_exists(source);
    remove_if_exists(base_a_doc);
    remove_if_exists(base_b_doc);
    remove_if_exists(child_doc);
    remove_if_exists(rebased_doc);
    remove_if_exists(base_a_output);
    remove_if_exists(base_b_output);
    remove_if_exists(child_output);
    remove_if_exists(rebase_output);
    remove_if_exists(inspect_output);
    remove_if_exists(style_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-character-style",
                      source.string(),
                      "BaseA",
                      "--name",
                      "Base A",
                      "--based-on",
                      "DefaultParagraphFont",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-rtl",
                      "true",
                      "--output",
                      base_a_doc.string(),
                      "--json"},
                     base_a_output),
             0);

    CHECK_EQ(run_cli({"ensure-character-style",
                      base_a_doc.string(),
                      "BaseB",
                      "--name",
                      "Base B",
                      "--based-on",
                      "DefaultParagraphFont",
                      "--run-font-family",
                      "Arial",
                      "--run-east-asia-language",
                      "ja-JP",
                      "--run-rtl",
                      "false",
                      "--output",
                      base_b_doc.string(),
                      "--json"},
                     base_b_output),
             0);

    CHECK_EQ(run_cli({"ensure-character-style",
                      base_b_doc.string(),
                      "ChildStyle",
                      "--name",
                      "Child Style",
                      "--based-on",
                      "BaseA",
                      "--run-language",
                      "en-US",
                      "--output",
                      child_doc.string(),
                      "--json"},
                     child_output),
             0);

    CHECK_EQ(run_cli({"rebase-character-style-based-on",
                      child_doc.string(),
                      "ChildStyle",
                      "BaseB",
                      "--output",
                      rebased_doc.string(),
                      "--json"},
                     rebase_output),
             0);
    CHECK_EQ(
        read_text_file(rebase_output),
        std::string{
            "{\"command\":\"rebase-character-style-based-on\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"ChildStyle\",\"based_on\":\"BaseB\",\"preserved\":["
            "{\"field\":\"font_family\",\"source_style_id\":\"BaseA\"},"
            "{\"field\":\"east_asia_language\",\"source_style_id\":\"BaseA\"},"
            "{\"field\":\"rtl\",\"source_style_id\":\"BaseA\"}]}\n"});

    CHECK_EQ(run_cli({"inspect-style-inheritance",
                      rebased_doc.string(),
                      "ChildStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"ChildStyle\",\"type\":\"character\","
            "\"kind\":\"character\",\"based_on\":\"BaseB\","
            "\"inheritance_chain\":[\"ChildStyle\",\"BaseB\",\"DefaultParagraphFont\"],"
            "\"resolved_properties\":{\"font_family\":{\"value\":\"Segoe UI\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_font_family\":{\"value\":null,\"source_style_id\":null},"
            "\"language\":{\"value\":\"en-US\",\"source_style_id\":\"ChildStyle\"},"
            "\"east_asia_language\":{\"value\":\"zh-CN\","
            "\"source_style_id\":\"ChildStyle\"},"
            "\"bidi_language\":{\"value\":null,\"source_style_id\":null},"
            "\"rtl\":{\"value\":true,\"source_style_id\":\"ChildStyle\"},"
            "\"paragraph_bidi\":{\"value\":null,"
            "\"source_style_id\":null}}}\n"});

    CHECK_EQ(run_cli({"inspect-styles",
                      rebased_doc.string(),
                      "--style",
                      "ChildStyle",
                      "--json"},
                     style_output),
             0);
    const auto style_json = read_text_file(style_output);
    CHECK_NE(style_json.find("\"style_id\":\"ChildStyle\""), std::string::npos);
    CHECK_NE(style_json.find("\"type\":\"character\""), std::string::npos);
    CHECK_NE(style_json.find("\"based_on\":\"BaseB\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(base_a_doc);
    remove_if_exists(base_b_doc);
    remove_if_exists(child_doc);
    remove_if_exists(rebased_doc);
    remove_if_exists(base_a_output);
    remove_if_exists(base_b_output);
    remove_if_exists(child_output);
    remove_if_exists(rebase_output);
    remove_if_exists(inspect_output);
    remove_if_exists(style_output);
}

TEST_CASE("cli rebase-character-style-based-on reports parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_rebase_character_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_rebase_character_style_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_rebase_character_style_mutate.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"rebase-character-style-based-on",
                      source.string(),
                      "DefaultParagraphFont",
                      "Strong",
                      "--bogus",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"rebase-character-style-based-on\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    CHECK_EQ(run_cli({"rebase-character-style-based-on",
                      source.string(),
                      "MissingStyle",
                      "Strong",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"rebase-character-style-based-on\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(mutate_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli paragraph style property commands inspect set and clear metadata without disturbing run properties") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_style_properties_source.docx";
    const fs::path styled =
        working_directory / "cli_paragraph_style_properties_styled.docx";
    const fs::path updated =
        working_directory / "cli_paragraph_style_properties_updated.docx";
    const fs::path cleared =
        working_directory / "cli_paragraph_style_properties_cleared.docx";
    const fs::path ensure_output =
        working_directory / "cli_paragraph_style_properties_ensure.json";
    const fs::path set_output =
        working_directory / "cli_paragraph_style_properties_set.json";
    const fs::path inspect_output =
        working_directory / "cli_paragraph_style_properties_inspect.json";
    const fs::path run_output =
        working_directory / "cli_paragraph_style_properties_run.json";
    const fs::path clear_output =
        working_directory / "cli_paragraph_style_properties_clear.json";
    const fs::path cleared_inspect_output =
        working_directory / "cli_paragraph_style_properties_cleared_inspect.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(ensure_output);
    remove_if_exists(set_output);
    remove_if_exists(inspect_output);
    remove_if_exists(run_output);
    remove_if_exists(clear_output);
    remove_if_exists(cleared_inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      source.string(),
                      "WorkingStyle",
                      "--name",
                      "Working Style",
                      "--based-on",
                      "Normal",
                      "--next-style",
                      "WorkingStyle",
                      "--run-font-family",
                      "Consolas",
                      "--output",
                      styled.string(),
                      "--json"},
                     ensure_output),
             0);

    CHECK_EQ(run_cli({"set-paragraph-style-properties",
                      styled.string(),
                      "WorkingStyle",
                      "--based-on",
                      "Heading1",
                      "--next-style",
                      "BodyText",
                      "--outline-level",
                      "2",
                      "--output",
                      updated.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-paragraph-style-properties\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"WorkingStyle\",\"based_on\":\"Heading1\","
            "\"next_style\":\"BodyText\",\"outline_level\":2}\n"});

    CHECK_EQ(run_cli({"inspect-paragraph-style-properties",
                      updated.string(),
                      "WorkingStyle",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"style_id\":\"WorkingStyle\",\"paragraph_style_properties\":{"
            "\"based_on\":\"Heading1\",\"next_style\":\"BodyText\","
            "\"outline_level\":2}}\n"});

    CHECK_EQ(run_cli({"inspect-style-run-properties",
                      updated.string(),
                      "WorkingStyle",
                      "--json"},
                     run_output),
             0);
    CHECK_EQ(
        read_text_file(run_output),
        std::string{
            "{\"style_id\":\"WorkingStyle\",\"style_run_properties\":{"
            "\"font_family\":\"Consolas\",\"east_asia_font_family\":null,"
            "\"language\":null,\"east_asia_language\":null,"
            "\"bidi_language\":null,\"rtl\":null,"
            "\"paragraph_bidi\":null}}\n"});

    CHECK_EQ(run_cli({"clear-paragraph-style-properties",
                      updated.string(),
                      "WorkingStyle",
                      "--based-on",
                      "--next-style",
                      "--outline-level",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-paragraph-style-properties\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style_id\":\"WorkingStyle\",\"cleared\":[\"based_on\","
            "\"next_style\",\"outline_level\"]}\n"});

    CHECK_EQ(run_cli({"inspect-paragraph-style-properties",
                      cleared.string(),
                      "WorkingStyle",
                      "--json"},
                     cleared_inspect_output),
             0);
    CHECK_EQ(
        read_text_file(cleared_inspect_output),
        std::string{
            "{\"style_id\":\"WorkingStyle\",\"paragraph_style_properties\":{"
            "\"based_on\":null,\"next_style\":null,"
            "\"outline_level\":null}}\n"});

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(ensure_output);
    remove_if_exists(set_output);
    remove_if_exists(inspect_output);
    remove_if_exists(run_output);
    remove_if_exists(clear_output);
    remove_if_exists(cleared_inspect_output);
}

TEST_CASE("cli paragraph style property commands report parse and inspect errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_style_properties_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_paragraph_style_properties_parse.json";
    const fs::path inspect_output =
        working_directory / "cli_paragraph_style_properties_inspect.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-paragraph-style-properties",
                      source.string(),
                      "Normal",
                      "--outline-level",
                      "9",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-paragraph-style-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid --outline-level value: expected 0-8\"}\n"});

    CHECK_EQ(run_cli({"inspect-paragraph-style-properties",
                      source.string(),
                      "MissingStyle",
                      "--json"},
                     inspect_output),
             1);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"command\":\"inspect-paragraph-style-properties\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(inspect_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(inspect_output);
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

TEST_CASE("cli export and import numbering catalog preserves instances and overrides") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_catalog_source.docx";
    const fs::path target = working_directory / "cli_numbering_catalog_target.docx";
    const fs::path imported = working_directory / "cli_numbering_catalog_imported.docx";
    const fs::path catalog_json =
        working_directory / "cli_numbering_catalog_export.json";
    const fs::path export_output =
        working_directory / "cli_numbering_catalog_export_output.json";
    const fs::path import_output =
        working_directory / "cli_numbering_catalog_import_output.json";

    remove_if_exists(source);
    remove_if_exists(target);
    remove_if_exists(imported);
    remove_if_exists(catalog_json);
    remove_if_exists(export_output);
    remove_if_exists(import_output);

    auto source_document = featherdoc::Document(source);
    REQUIRE_FALSE(source_document.create_empty());

    auto catalog = featherdoc::numbering_catalog{};
    auto catalog_definition = featherdoc::numbering_catalog_definition{};
    catalog_definition.definition.name = "CliCatalogOutline";
    catalog_definition.definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 4U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{41U, {}});

    auto restarted_instance = featherdoc::numbering_instance_summary{};
    restarted_instance.instance_id = 42U;
    restarted_instance.level_overrides.push_back(
        featherdoc::numbering_level_override_summary{0U, 7U, std::nullopt});
    restarted_instance.level_overrides.push_back(
        featherdoc::numbering_level_override_summary{
            1U,
            std::nullopt,
            featherdoc::numbering_level_definition{
                featherdoc::list_kind::decimal, 9U, 1U, "(%2)"}});
    catalog_definition.instances.push_back(std::move(restarted_instance));
    catalog.definitions.push_back(std::move(catalog_definition));

    const auto import_summary = source_document.import_numbering_catalog(catalog);
    REQUIRE(static_cast<bool>(import_summary));
    REQUIRE_FALSE(source_document.save());

    auto target_document = featherdoc::Document(target);
    REQUIRE_FALSE(target_document.create_empty());
    REQUIRE_FALSE(target_document.save());

    CHECK_EQ(run_cli({"export-numbering-catalog",
                      source.string(),
                      "--output",
                      catalog_json.string(),
                      "--json"},
                     export_output),
             0);
    CHECK_EQ(read_text_file(export_output),
             std::string{"{\"command\":\"export-numbering-catalog\",\"ok\":true,"} +
                 "\"output_path\":" + json_quote(catalog_json.string()) +
                 ",\"definition_count\":1,\"instance_count\":2}\n");

    const auto exported_catalog = read_text_file(catalog_json);
    CHECK_NE(exported_catalog.find("\"name\":\"CliCatalogOutline\""),
             std::string::npos);
    CHECK_NE(exported_catalog.find("\"level_overrides\":[]"),
             std::string::npos);
    CHECK_NE(exported_catalog.find(
                 "{\"level\":0,\"start_override\":7,\"level_definition\":null}"),
             std::string::npos);
    CHECK_NE(exported_catalog.find(
                 "{\"level\":1,\"start_override\":null,\"level_definition\":"
                 "{\"level\":1,\"kind\":\"decimal\",\"start\":9,"
                 "\"text_pattern\":\"(%2)\"}}"),
             std::string::npos);

    CHECK_EQ(run_cli({"import-numbering-catalog",
                      target.string(),
                      "--catalog-file",
                      catalog_json.string(),
                      "--output",
                      imported.string(),
                      "--json"},
                     import_output),
             0);
    const auto import_json = read_text_file(import_output);
    CHECK_NE(import_json.find("\"command\":\"import-numbering-catalog\""),
             std::string::npos);
    CHECK_NE(import_json.find("\"in_place\":false"), std::string::npos);
    CHECK_NE(import_json.find("\"catalog_file\":" + json_quote(catalog_json.string())),
             std::string::npos);
    CHECK_NE(import_json.find("\"input_definition_count\":1"), std::string::npos);
    CHECK_NE(import_json.find("\"imported_definition_count\":1"),
             std::string::npos);
    CHECK_NE(import_json.find("\"imported_instance_count\":2"),
             std::string::npos);
    CHECK_NE(import_json.find("\"name\":\"CliCatalogOutline\""),
             std::string::npos);

    auto imported_document = featherdoc::Document(imported);
    REQUIRE_FALSE(imported_document.open());
    const auto imported_definitions = imported_document.list_numbering_definitions();
    REQUIRE_FALSE(imported_document.last_error());
    const auto imported_definition =
        std::find_if(imported_definitions.begin(), imported_definitions.end(),
                     [](const auto &definition) {
                         return definition.name == "CliCatalogOutline";
                     });
    REQUIRE(imported_definition != imported_definitions.end());
    CHECK_EQ(imported_definition->levels.size(), 2U);
    CHECK_EQ(imported_definition->instances.size(), 2U);

    const auto restarted_imported_instance = std::find_if(
        imported_definition->instances.begin(), imported_definition->instances.end(),
        [](const auto &instance) { return instance.level_overrides.size() == 2U; });
    REQUIRE(restarted_imported_instance != imported_definition->instances.end());
    CHECK_EQ(restarted_imported_instance->level_overrides[0].level, 0U);
    REQUIRE(restarted_imported_instance->level_overrides[0].start_override.has_value());
    CHECK_EQ(*restarted_imported_instance->level_overrides[0].start_override, 7U);
    CHECK_EQ(restarted_imported_instance->level_overrides[1].level, 1U);
    REQUIRE(restarted_imported_instance->level_overrides[1]
                .level_definition.has_value());
    CHECK_EQ(restarted_imported_instance->level_overrides[1]
                 .level_definition->text_pattern,
             "(%2)");

    remove_if_exists(source);
    remove_if_exists(target);
    remove_if_exists(imported);
    remove_if_exists(catalog_json);
    remove_if_exists(export_output);
    remove_if_exists(import_output);
}

TEST_CASE("cli patch-numbering-catalog upserts and removes overrides") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_numbering_catalog_patch_source.docx";
    const fs::path catalog_json =
        working_directory / "cli_numbering_catalog_patch_catalog.json";
    const fs::path patch_json =
        working_directory / "cli_numbering_catalog_patch_patch.json";
    const fs::path patched_json =
        working_directory / "cli_numbering_catalog_patch_patched.json";
    const fs::path export_output =
        working_directory / "cli_numbering_catalog_patch_export_output.json";
    const fs::path patch_output =
        working_directory / "cli_numbering_catalog_patch_output.json";
    const fs::path lint_output =
        working_directory / "cli_numbering_catalog_patch_lint_output.json";

    remove_if_exists(source);
    remove_if_exists(catalog_json);
    remove_if_exists(patch_json);
    remove_if_exists(patched_json);
    remove_if_exists(export_output);
    remove_if_exists(patch_output);
    remove_if_exists(lint_output);

    auto document = featherdoc::Document(source);
    REQUIRE_FALSE(document.create_empty());

    auto catalog = featherdoc::numbering_catalog{};
    auto catalog_definition = featherdoc::numbering_catalog_definition{};
    catalog_definition.definition.name = "PatchableOutline";
    catalog_definition.definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{3U, {}});
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{
            4U,
            {featherdoc::numbering_level_override_summary{0U, 2U,
                                                          std::nullopt}}});
    catalog.definitions.push_back(std::move(catalog_definition));

    const auto import_summary = document.import_numbering_catalog(catalog);
    REQUIRE(static_cast<bool>(import_summary));
    REQUIRE_FALSE(document.save());

    CHECK_EQ(run_cli({"export-numbering-catalog",
                      source.string(),
                      "--output",
                      catalog_json.string(),
                      "--json"},
                     export_output),
             0);

    write_binary_file(
        patch_json,
        "{\"upsert_levels\":["
        "{\"definition_name\":\"PatchableOutline\",\"level\":"
        "{\"level\":2,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.%2.%3.\"}}],"
        "\"upsert_overrides\":["
        "{\"definition_name\":\"PatchableOutline\",\"instance_index\":0,"
        "\"level\":1,\"start_override\":5},"
        "{\"definition_name\":\"PatchableOutline\",\"instance_index\":1,"
        "\"level\":2,\"level_definition\":{\"level\":2,\"kind\":\"bullet\","
        "\"start\":1,\"text_pattern\":\"•\"}}],"
        "\"remove_overrides\":["
        "{\"definition_name\":\"PatchableOutline\",\"instance_index\":1,"
        "\"level\":0},"
        "{\"definition_name\":\"PatchableOutline\",\"instance_index\":1,"
        "\"level\":7}]}\n");

    CHECK_EQ(run_cli({"patch-numbering-catalog",
                      catalog_json.string(),
                      "--patch-file",
                      patch_json.string(),
                      "--output",
                      patched_json.string(),
                      "--json"},
                     patch_output),
             0);

    CHECK_EQ(read_text_file(patch_output),
             std::string{"{\"command\":\"patch-numbering-catalog\",\"ok\":true,"} +
                 "\"output_path\":" + json_quote(patched_json.string()) +
                 ",\"definition_count\":1,\"instance_count\":2,"
                 "\"upserted_level_count\":1,\"upserted_override_count\":2,"
                 "\"removed_override_count\":1,\"missing_override_count\":1}\n");

    const auto patched_catalog = read_text_file(patched_json);
    CHECK_NE(patched_catalog.find(
                 "\"levels\":[{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
                 "\"text_pattern\":\"%1.\"},{\"level\":1,\"kind\":\"decimal\","
                 "\"start\":1,\"text_pattern\":\"%1.%2.\"},{\"level\":2,"
                 "\"kind\":\"decimal\",\"start\":1,"
                 "\"text_pattern\":\"%1.%2.%3.\"}]"),
             std::string::npos);
    CHECK_NE(patched_catalog.find(
                 "{\"instance_id\":1,\"level_overrides\":["
                 "{\"level\":1,\"start_override\":5,"
                 "\"level_definition\":null}]}"),
             std::string::npos);
    CHECK_NE(patched_catalog.find(
                 "{\"instance_id\":2,\"level_overrides\":["
                 "{\"level\":2,\"start_override\":null,\"level_definition\":"
                 "{\"level\":2,\"kind\":\"bullet\",\"start\":1,"
                 "\"text_pattern\":\"•\"}}]}"),
             std::string::npos);
    CHECK_EQ(patched_catalog.find("\"start_override\":2"), std::string::npos);

    CHECK_EQ(run_cli({"lint-numbering-catalog", patched_json.string(), "--json"},
                     lint_output),
             0);
    CHECK_EQ(read_text_file(lint_output),
             std::string{"{\"command\":\"lint-numbering-catalog\",\"ok\":true,"} +
                 "\"clean\":true,\"definition_count\":1,\"instance_count\":2,"
                 "\"level_count\":3,\"override_count\":2,\"issue_count\":0,"
                 "\"issues\":[]}\n");

    remove_if_exists(source);
    remove_if_exists(catalog_json);
    remove_if_exists(patch_json);
    remove_if_exists(patched_json);
    remove_if_exists(export_output);
    remove_if_exists(patch_output);
    remove_if_exists(lint_output);
}

TEST_CASE("cli lint-numbering-catalog reports clean and dirty catalogs") {
    const fs::path working_directory = fs::current_path();
    const fs::path clean_catalog =
        working_directory / "cli_lint_numbering_catalog_clean.json";
    const fs::path dirty_catalog =
        working_directory / "cli_lint_numbering_catalog_dirty.json";
    const fs::path clean_output =
        working_directory / "cli_lint_numbering_catalog_clean_output.json";
    const fs::path dirty_output =
        working_directory / "cli_lint_numbering_catalog_dirty_output.json";
    const fs::path parse_output =
        working_directory / "cli_lint_numbering_catalog_parse_output.json";

    remove_if_exists(clean_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);

    write_binary_file(
        clean_catalog,
        "{\"definition_count\":1,\"instance_count\":1,\"definitions\":["
        "{\"name\":\"CleanOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"}],\"instances\":["
        "{\"instance_id\":1,\"level_overrides\":["
        "{\"level\":0,\"start_override\":3,"
        "\"level_definition\":null}]}]}]}\n");

    CHECK_EQ(run_cli({"lint-numbering-catalog", clean_catalog.string(), "--json"},
                     clean_output),
             0);
    CHECK_EQ(read_text_file(clean_output),
             std::string{
                 "{\"command\":\"lint-numbering-catalog\",\"ok\":true,"
                 "\"clean\":true,\"definition_count\":1,\"instance_count\":1,"
                 "\"level_count\":1,\"override_count\":1,\"issue_count\":0,"
                 "\"issues\":[]}\n"});

    write_binary_file(
        dirty_catalog,
        "{\"definitions\":["
        "{\"name\":\"Broken\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":0,"
        "\"text_pattern\":\"\"},"
        "{\"level\":0,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"o\"}],\"instances\":["
        "{\"instance_id\":5,\"level_overrides\":["
        "{\"level\":9,\"start_override\":0,"
        "\"level_definition\":null},"
        "{\"level\":9,\"start_override\":1,"
        "\"level_definition\":{\"level\":9,\"kind\":\"decimal\","
        "\"start\":0,\"text_pattern\":\"\"}}]},"
        "{\"instance_id\":5,\"level_overrides\":[]}]},"
        "{\"name\":\"Broken\",\"levels\":[],\"instances\":[]}]}\n");

    CHECK_EQ(run_cli({"lint-numbering-catalog", dirty_catalog.string(), "--json"},
                     dirty_output),
             1);
    const auto dirty_json = read_text_file(dirty_output);
    CHECK_NE(dirty_json.find("\"command\":\"lint-numbering-catalog\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(dirty_json.find("\"definition_count\":2"), std::string::npos);
    CHECK_NE(dirty_json.find("\"instance_count\":2"), std::string::npos);
    CHECK_NE(dirty_json.find("\"level_count\":2"), std::string::npos);
    CHECK_NE(dirty_json.find("\"override_count\":2"), std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"duplicate_definition_name\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_levels\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"duplicate_level\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"invalid_start\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_text_pattern\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"duplicate_instance_id\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"invalid_override_level\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"duplicate_override_level\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"invalid_override_start\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"invalid_override_definition\""),
             std::string::npos);

    CHECK_EQ(run_cli({"lint-numbering-catalog", "--json"}, parse_output), 2);
    CHECK_EQ(read_text_file(parse_output),
             std::string{
                 "{\"command\":\"lint-numbering-catalog\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"lint-numbering-catalog "
                 "expects a catalog path\"}\n"});

    remove_if_exists(clean_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(clean_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli diff-numbering-catalog reports catalog changes as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_catalog =
        working_directory / "cli_diff_numbering_catalog_left.json";
    const fs::path right_catalog =
        working_directory / "cli_diff_numbering_catalog_right.json";
    const fs::path diff_output =
        working_directory / "cli_diff_numbering_catalog_output.json";
    const fs::path equal_output =
        working_directory / "cli_diff_numbering_catalog_equal_output.json";
    const fs::path parse_output =
        working_directory / "cli_diff_numbering_catalog_parse_output.json";

    remove_if_exists(left_catalog);
    remove_if_exists(right_catalog);
    remove_if_exists(diff_output);
    remove_if_exists(equal_output);
    remove_if_exists(parse_output);

    write_binary_file(
        left_catalog,
        "{\"definitions\":["
        "{\"name\":\"RemovedOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"}],\"instances\":[]},"
        "{\"name\":\"SharedOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.%2.\"}],\"instances\":["
        "{\"instance_id\":10,\"level_overrides\":["
        "{\"level\":0,\"start_override\":2,"
        "\"level_definition\":null}]},"
        "{\"instance_id\":11,\"level_overrides\":[]}]}]}\n");

    write_binary_file(
        right_catalog,
        "{\"definitions\":["
        "{\"name\":\"SharedOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":3,"
        "\"text_pattern\":\"%1)\"},"
        "{\"level\":2,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"•\"}],\"instances\":["
        "{\"instance_id\":20,\"level_overrides\":["
        "{\"level\":0,\"start_override\":4,"
        "\"level_definition\":null},"
        "{\"level\":2,\"start_override\":null,"
        "\"level_definition\":{\"level\":2,\"kind\":\"bullet\","
        "\"start\":1,\"text_pattern\":\"•\"}}]},"
        "{\"instance_id\":21,\"level_overrides\":[]},"
        "{\"instance_id\":22,\"level_overrides\":[]}]},"
        "{\"name\":\"AddedOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"•\"}],\"instances\":[]}]}\n");

    CHECK_EQ(run_cli({"diff-numbering-catalog",
                      left_catalog.string(),
                      right_catalog.string(),
                      "--json"},
                     diff_output),
             0);
    const auto diff_json = read_text_file(diff_output);
    CHECK_NE(diff_json.find("\"equal\":false"), std::string::npos);
    CHECK_NE(diff_json.find("\"added_definition_count\":1"),
             std::string::npos);
    CHECK_NE(diff_json.find("\"removed_definition_count\":1"),
             std::string::npos);
    CHECK_NE(diff_json.find("\"changed_definition_count\":1"),
             std::string::npos);
    CHECK_NE(diff_json.find("\"name\":\"AddedOutline\""),
             std::string::npos);
    CHECK_NE(diff_json.find("\"name\":\"RemovedOutline\""),
             std::string::npos);
    CHECK_NE(diff_json.find("\"name\":\"SharedOutline\""),
             std::string::npos);
    CHECK_NE(diff_json.find("\"added_level_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"removed_level_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"changed_level_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"added_instance_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"changed_instance_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"added_override_count\":1"), std::string::npos);
    CHECK_NE(diff_json.find("\"changed_override_count\":1"), std::string::npos);

    CHECK_EQ(run_cli({"diff-numbering-catalog",
                      left_catalog.string(),
                      left_catalog.string(),
                      "--fail-on-diff",
                      "--json"},
                     equal_output),
             0);
    CHECK_NE(read_text_file(equal_output).find("\"equal\":true"),
             std::string::npos);

    CHECK_EQ(run_cli({"diff-numbering-catalog",
                      left_catalog.string(),
                      right_catalog.string(),
                      "--fail-on-diff",
                      "--json"},
                     parse_output),
             1);

    remove_if_exists(left_catalog);
    remove_if_exists(right_catalog);
    remove_if_exists(diff_output);
    remove_if_exists(equal_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli check-numbering-catalog gates docx catalog against baseline") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_check_numbering_catalog_source.docx";
    const fs::path baseline_catalog =
        working_directory / "cli_check_numbering_catalog_baseline.json";
    const fs::path drift_catalog =
        working_directory / "cli_check_numbering_catalog_drift.json";
    const fs::path dirty_catalog =
        working_directory / "cli_check_numbering_catalog_dirty.json";
    const fs::path generated_catalog =
        working_directory / "cli_check_numbering_catalog_generated.json";
    const fs::path match_output =
        working_directory / "cli_check_numbering_catalog_match_output.json";
    const fs::path drift_output =
        working_directory / "cli_check_numbering_catalog_drift_output.json";
    const fs::path dirty_output =
        working_directory / "cli_check_numbering_catalog_dirty_output.json";
    const fs::path parse_output =
        working_directory / "cli_check_numbering_catalog_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(baseline_catalog);
    remove_if_exists(drift_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(generated_catalog);
    remove_if_exists(match_output);
    remove_if_exists(drift_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);

    auto document = featherdoc::Document(source);
    REQUIRE_FALSE(document.create_empty());

    auto catalog = featherdoc::numbering_catalog{};
    auto catalog_definition = featherdoc::numbering_catalog_definition{};
    catalog_definition.definition.name = "CheckOutline";
    catalog_definition.definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };
    catalog_definition.instances.push_back(
        featherdoc::numbering_instance_summary{
            99U,
            {featherdoc::numbering_level_override_summary{0U, 3U,
                                                          std::nullopt}}});
    catalog.definitions.push_back(std::move(catalog_definition));

    const auto import_summary = document.import_numbering_catalog(catalog);
    REQUIRE(static_cast<bool>(import_summary));
    REQUIRE_FALSE(document.save());

    write_binary_file(
        baseline_catalog,
        "{\"definitions\":[{\"name\":\"CheckOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,"
        "\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"o\"}],\"instances\":["
        "{\"instance_id\":99,\"level_overrides\":["
        "{\"level\":0,\"start_override\":3,"
        "\"level_definition\":null}]}]}]}\n");
    write_binary_file(
        drift_catalog,
        "{\"definitions\":[{\"name\":\"CheckOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":7,"
        "\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"bullet\",\"start\":1,"
        "\"text_pattern\":\"o\"}],\"instances\":["
        "{\"instance_id\":99,\"level_overrides\":["
        "{\"level\":0,\"start_override\":3,"
        "\"level_definition\":null}]}]}]}\n");
    write_binary_file(dirty_catalog,
                      "{\"definitions\":[{\"name\":\"\",\"levels\":[],"
                      "\"instances\":[]}]}\n");

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      baseline_catalog.string(),
                      "--output",
                      generated_catalog.string(),
                      "--json"},
                     match_output),
             0);
    const auto match_json = read_text_file(match_output);
    CHECK_NE(match_json.find("\"command\":\"check-numbering-catalog\""),
             std::string::npos);
    CHECK_NE(match_json.find("\"matches\":true"), std::string::npos);
    CHECK_NE(match_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(match_json.find("\"catalog_file\":" +
                             json_quote(baseline_catalog.string())),
             std::string::npos);
    CHECK_NE(match_json.find("\"generated_output_path\":" +
                             json_quote(generated_catalog.string())),
             std::string::npos);
    CHECK_NE(match_json.find("\"baseline_issue_count\":0"),
             std::string::npos);
    CHECK_NE(match_json.find("\"generated_issue_count\":0"),
             std::string::npos);
    CHECK_NE(match_json.find("\"changed_definition_count\":0"),
             std::string::npos);
    CHECK_NE(read_text_file(generated_catalog).find("\"name\":\"CheckOutline\""),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      drift_catalog.string(),
                      "--json"},
                     drift_output),
             1);
    const auto drift_json = read_text_file(drift_output);
    CHECK_NE(drift_json.find("\"matches\":false"), std::string::npos);
    CHECK_NE(drift_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(drift_json.find("\"changed_definition_count\":1"),
             std::string::npos);
    CHECK_NE(drift_json.find("\"changed_level_count\":1"),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog",
                      source.string(),
                      "--catalog-file",
                      dirty_catalog.string(),
                      "--json"},
                     dirty_output),
             1);
    const auto dirty_json = read_text_file(dirty_output);
    CHECK_NE(dirty_json.find("\"matches\":false"), std::string::npos);
    CHECK_NE(dirty_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(dirty_json.find("\"baseline_issue_count\":2"),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_definition_name\""),
             std::string::npos);
    CHECK_NE(dirty_json.find("\"issue\":\"empty_levels\""),
             std::string::npos);

    CHECK_EQ(run_cli({"check-numbering-catalog", source.string(), "--json"},
                     parse_output),
             2);
    CHECK_EQ(read_text_file(parse_output),
             std::string{
                 "{\"command\":\"check-numbering-catalog\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"missing "
                 "--catalog-file <catalog.json>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(baseline_catalog);
    remove_if_exists(drift_catalog);
    remove_if_exists(dirty_catalog);
    remove_if_exists(generated_catalog);
    remove_if_exists(match_output);
    remove_if_exists(drift_output);
    remove_if_exists(dirty_output);
    remove_if_exists(parse_output);
}

TEST_CASE(
    "cli ensure-numbering-definition and set-paragraph-numbering manage custom numbering") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_custom_numbering_source.docx";
    const fs::path defined =
        working_directory / "cli_custom_numbering_defined.docx";
    const fs::path numbered =
        working_directory / "cli_custom_numbering_numbered.docx";
    const fs::path nested =
        working_directory / "cli_custom_numbering_nested.docx";
    const fs::path ensure_output =
        working_directory / "cli_custom_numbering_ensure_output.json";
    const fs::path numbered_output =
        working_directory / "cli_custom_numbering_numbered_output.json";
    const fs::path nested_output =
        working_directory / "cli_custom_numbering_nested_output.json";

    remove_if_exists(source);
    remove_if_exists(defined);
    remove_if_exists(numbered);
    remove_if_exists(nested);
    remove_if_exists(ensure_output);
    remove_if_exists(numbered_output);
    remove_if_exists(nested_output);

    create_cli_paragraph_list_fixture(source);

    CHECK_EQ(run_cli({"ensure-numbering-definition",
                      source.string(),
                      "--definition-name",
                      "LegalOutline",
                      "--numbering-level",
                      "0:decimal:3:%1.",
                      "--numbering-level",
                      "1:decimal:1:%1.%2.",
                      "--output",
                      defined.string(),
                      "--json"},
                     ensure_output),
             0);

    featherdoc::Document defined_document(defined);
    REQUIRE_FALSE(defined_document.open());
    const auto defined_definitions = defined_document.list_numbering_definitions();
    REQUIRE_FALSE(defined_document.last_error());
    const auto defined_definition =
        std::find_if(defined_definitions.begin(), defined_definitions.end(),
                     [](const auto &summary) {
                         return summary.name == "LegalOutline";
                     });
    REQUIRE(defined_definition != defined_definitions.end());
    CHECK(defined_definition->instance_ids.empty());
    const auto definition_id = std::to_string(defined_definition->definition_id);

    CHECK_EQ(
        read_text_file(ensure_output),
        std::string{
            "{\"command\":\"ensure-numbering-definition\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"definition_id\":"} +
            definition_id +
            ",\"definition_name\":\"LegalOutline\",\"definition_levels\":["
            "{\"level\":0,\"kind\":\"decimal\",\"start\":3,\"text_pattern\":\"%1.\"},"
            "{\"level\":1,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"%1.%2.\"}]}\n");

    CHECK_EQ(run_cli({"set-paragraph-numbering",
                      defined.string(),
                      "0",
                      "--definition",
                      definition_id,
                      "--output",
                      numbered.string(),
                      "--json"},
                     numbered_output),
             0);
    CHECK_EQ(
        read_text_file(numbered_output),
        std::string{
            "{\"command\":\"set-paragraph-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":0,\"definition_id\":"} +
            definition_id + ",\"level\":0}\n");

    CHECK_EQ(run_cli({"set-paragraph-numbering",
                      numbered.string(),
                      "1",
                      "--definition",
                      definition_id,
                      "--level",
                      "1",
                      "--output",
                      nested.string(),
                      "--json"},
                     nested_output),
             0);
    CHECK_EQ(
        read_text_file(nested_output),
        std::string{
            "{\"command\":\"set-paragraph-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":1,\"definition_id\":"} +
            definition_id + ",\"level\":1}\n");

    featherdoc::Document nested_document(nested);
    REQUIRE_FALSE(nested_document.open());
    const auto nested_definitions = nested_document.list_numbering_definitions();
    REQUIRE_FALSE(nested_document.last_error());
    const auto nested_definition =
        std::find_if(nested_definitions.begin(), nested_definitions.end(),
                     [](const auto &summary) {
                         return summary.name == "LegalOutline";
                     });
    REQUIRE(nested_definition != nested_definitions.end());
    REQUIRE_EQ(nested_definition->instance_ids.size(), 1U);
    const auto num_id = std::to_string(nested_definition->instance_ids.front());

    const auto nested_document_xml = read_docx_entry(nested, "word/document.xml");
    pugi::xml_document nested_xml_document;
    REQUIRE(nested_xml_document.load_string(nested_document_xml.c_str()));
    const auto first_paragraph =
        find_body_paragraph_xml_node(nested_xml_document.child("w:document"), 0U);
    const auto second_paragraph =
        find_body_paragraph_xml_node(nested_xml_document.child("w:document"), 1U);
    const auto third_paragraph =
        find_body_paragraph_xml_node(nested_xml_document.child("w:document"), 2U);
    REQUIRE(first_paragraph != pugi::xml_node{});
    REQUIRE(second_paragraph != pugi::xml_node{});
    REQUIRE(third_paragraph != pugi::xml_node{});

    const auto first_num_pr = first_paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_num_pr.child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{second_num_pr.child("w:ilvl").attribute("w:val").value()},
             "1");
    CHECK_EQ(std::string{first_num_pr.child("w:numId").attribute("w:val").value()},
             num_id);
    CHECK_EQ(std::string{second_num_pr.child("w:numId").attribute("w:val").value()},
             num_id);
    CHECK(third_paragraph.child("w:pPr").child("w:numPr") == pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(defined);
    remove_if_exists(numbered);
    remove_if_exists(nested);
    remove_if_exists(ensure_output);
    remove_if_exists(numbered_output);
    remove_if_exists(nested_output);
}

TEST_CASE("cli custom numbering commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_custom_numbering_error_source.docx";
    const fs::path ensure_output =
        working_directory / "cli_custom_numbering_error_ensure.json";
    const fs::path parse_output =
        working_directory / "cli_custom_numbering_error_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_custom_numbering_error_mutate.json";

    remove_if_exists(source);
    remove_if_exists(ensure_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_paragraph_list_fixture(source);

    CHECK_EQ(run_cli({"ensure-numbering-definition",
                      source.string(),
                      "--definition-name",
                      "BrokenDefinition",
                      "--json"},
                     ensure_output),
             2);
    CHECK_EQ(
        read_text_file(ensure_output),
        std::string{
            "{\"command\":\"ensure-numbering-definition\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected at least one "
            "--numbering-level "
            "<level>:<kind>:<start>:<text-pattern>\"}\n"});

    CHECK_EQ(run_cli({"set-paragraph-numbering", source.string(), "0", "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-paragraph-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing --definition <id>\"}\n"});

    CHECK_EQ(run_cli({"set-paragraph-numbering",
                      source.string(),
                      "0",
                      "--definition",
                      "999",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-paragraph-numbering\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"numbering definition id does not exist\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/numbering.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(ensure_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli ensure-paragraph-style creates a paragraph style with optional metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_ensure_paragraph_style_source.docx";
    const fs::path updated = working_directory / "cli_ensure_paragraph_style_updated.docx";
    const fs::path output = working_directory / "cli_ensure_paragraph_style_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      source.string(),
                      "LegalBody",
                      "--name",
                      "Legal Body",
                      "--based-on",
                      "Normal",
                      "--next-style",
                      "LegalBody",
                      "--custom",
                      "true",
                      "--semi-hidden",
                      "false",
                      "--unhide-when-used",
                      "true",
                      "--quick-format",
                      "true",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-east-asia-font-family",
                      "Microsoft YaHei",
                      "--run-language",
                      "en-US",
                      "--run-east-asia-language",
                      "zh-CN",
                      "--run-bidi-language",
                      "ar-SA",
                      "--run-rtl",
                      "true",
                      "--paragraph-bidi",
                      "true",
                      "--outline-level",
                      "2",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-paragraph-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style\":{\"style_id\":\"LegalBody\",\"name\":\"Legal Body\","
            "\"based_on\":\"Normal\",\"kind\":\"paragraph\",\"type\":\"paragraph\","
            "\"numbering\":null,\"is_default\":false,\"is_custom\":true,"
            "\"is_semi_hidden\":false,\"is_unhide_when_used\":true,"
            "\"is_quick_format\":true}}\n"});

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto style =
        find_style_xml_node(styles_document.child("w:styles"), "LegalBody");
    REQUIRE(style != pugi::xml_node{});
    CHECK_EQ(std::string_view{style.attribute("w:type").value()}, "paragraph");
    CHECK_EQ(std::string_view{style.child("w:name").attribute("w:val").value()},
             "Legal Body");
    CHECK_EQ(std::string_view{style.child("w:basedOn").attribute("w:val").value()},
             "Normal");
    CHECK_EQ(std::string_view{style.child("w:next").attribute("w:val").value()},
             "LegalBody");
    CHECK(style.child("w:qFormat") != pugi::xml_node{});
    CHECK(style.child("w:semiHidden") == pugi::xml_node{});
    CHECK(style.child("w:unhideWhenUsed") != pugi::xml_node{});
    const auto paragraph_properties = style.child("w:pPr");
    REQUIRE(paragraph_properties != pugi::xml_node{});
    CHECK(paragraph_properties.child("w:bidi") != pugi::xml_node{});
    CHECK_EQ(std::string_view{paragraph_properties.child("w:outlineLvl")
                                  .attribute("w:val")
                                  .value()},
             "2");
    const auto run_properties = style.child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    const auto fonts = run_properties.child("w:rFonts");
    REQUIRE(fonts != pugi::xml_node{});
    CHECK_EQ(std::string_view{fonts.attribute("w:ascii").value()}, "Segoe UI");
    CHECK_EQ(std::string_view{fonts.attribute("w:eastAsia").value()},
             "Microsoft YaHei");
    const auto language = run_properties.child("w:lang");
    REQUIRE(language != pugi::xml_node{});
    CHECK_EQ(std::string_view{language.attribute("w:val").value()}, "en-US");
    CHECK_EQ(std::string_view{language.attribute("w:eastAsia").value()}, "zh-CN");
    CHECK_EQ(std::string_view{language.attribute("w:bidi").value()}, "ar-SA");
    CHECK(run_properties.child("w:rtl") != pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli ensure-character-style creates a character style with run metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_ensure_character_style_source.docx";
    const fs::path updated = working_directory / "cli_ensure_character_style_updated.docx";
    const fs::path output = working_directory / "cli_ensure_character_style_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-character-style",
                      source.string(),
                      "AccentStrong",
                      "--name",
                      "Accent Strong",
                      "--based-on",
                      "DefaultParagraphFont",
                      "--semi-hidden",
                      "true",
                      "--quick-format",
                      "true",
                      "--run-font-family",
                      "Segoe UI",
                      "--run-language",
                      "fr-FR",
                      "--run-rtl",
                      "false",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto style =
        find_style_xml_node(styles_document.child("w:styles"), "AccentStrong");
    REQUIRE(style != pugi::xml_node{});
    CHECK_EQ(std::string_view{style.attribute("w:type").value()}, "character");
    CHECK_EQ(std::string_view{style.child("w:name").attribute("w:val").value()},
             "Accent Strong");
    CHECK_EQ(std::string_view{style.child("w:basedOn").attribute("w:val").value()},
             "DefaultParagraphFont");
    CHECK(style.child("w:semiHidden") != pugi::xml_node{});
    CHECK(style.child("w:qFormat") != pugi::xml_node{});
    const auto run_properties = style.child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_properties.child("w:rFonts")
                                  .attribute("w:ascii")
                                  .value()},
             "Segoe UI");
    CHECK_EQ(std::string_view{run_properties.child("w:lang")
                                  .attribute("w:val")
                                  .value()},
             "fr-FR");
    CHECK_EQ(std::string_view{run_properties.child("w:rtl")
                                  .attribute("w:val")
                                  .value()},
             "0");

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli ensure-table-style creates a table style with catalog flags") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_ensure_table_style_source.docx";
    const fs::path updated = working_directory / "cli_ensure_table_style_updated.docx";
    const fs::path output = working_directory / "cli_ensure_table_style_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-table-style",
                      source.string(),
                      "ReportTable",
                      "--name",
                      "Report Table",
                      "--based-on",
                      "TableGrid",
                      "--unhide-when-used",
                      "true",
                      "--quick-format",
                      "true",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-table-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"style\":{\"style_id\":\"ReportTable\",\"name\":\"Report Table\","
            "\"based_on\":\"TableGrid\",\"kind\":\"table\",\"type\":\"table\","
            "\"numbering\":null,\"is_default\":false,\"is_custom\":true,"
            "\"is_semi_hidden\":false,\"is_unhide_when_used\":true,"
            "\"is_quick_format\":true}}\n"});

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto style =
        find_style_xml_node(styles_document.child("w:styles"), "ReportTable");
    REQUIRE(style != pugi::xml_node{});
    CHECK_EQ(std::string_view{style.attribute("w:type").value()}, "table");
    CHECK_EQ(std::string_view{style.child("w:name").attribute("w:val").value()},
             "Report Table");
    CHECK_EQ(std::string_view{style.child("w:basedOn").attribute("w:val").value()},
             "TableGrid");
    CHECK(style.child("w:qFormat") != pugi::xml_node{});
    CHECK(style.child("w:semiHidden") == pugi::xml_node{});
    CHECK(style.child("w:unhideWhenUsed") != pugi::xml_node{});
    CHECK(style.child("w:tblPr") == pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli ensure-paragraph-style reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_ensure_paragraph_style_parse_source.docx";
    const fs::path output =
        working_directory / "cli_ensure_paragraph_style_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"ensure-paragraph-style",
                      source.string(),
                      "BodyText",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-paragraph-style\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing required --name <name>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
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


TEST_CASE("cli inspect-style-numbering lists numbered paragraph styles only") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_style_numbering_source.docx";
    const fs::path updated =
        working_directory / "cli_inspect_style_numbering_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_inspect_style_numbering_mutate.json";
    const fs::path json_output =
        working_directory / "cli_inspect_style_numbering_output.json";
    const fs::path text_output =
        working_directory / "cli_inspect_style_numbering_output.txt";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(json_output);
    remove_if_exists(text_output);

    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.create_empty());

        auto legal_heading = featherdoc::paragraph_style_definition{};
        legal_heading.name = "Legal Heading";
        legal_heading.based_on = std::string{"Heading1"};
        legal_heading.is_quick_format = true;
        REQUIRE(document.ensure_paragraph_style("LegalHeading", legal_heading));

        auto body_numbered = featherdoc::paragraph_style_definition{};
        body_numbered.name = "Body Numbered";
        body_numbered.based_on = std::string{"Normal"};
        body_numbered.is_quick_format = true;
        REQUIRE(document.ensure_paragraph_style("BodyNumbered", body_numbered));

        auto plain_body = featherdoc::paragraph_style_definition{};
        plain_body.name = "Plain Body";
        plain_body.based_on = std::string{"Normal"};
        plain_body.is_quick_format = true;
        REQUIRE(document.ensure_paragraph_style("PlainBody", plain_body));

        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "SharedOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--numbering-level",
                      "1:decimal:1:%1.%2.",
                      "--style-link",
                      "LegalHeading:0",
                      "--style-link",
                      "BodyNumbered:1",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    CHECK_EQ(run_cli({"inspect-style-numbering", updated.string(), "--json"},
                     json_output),
             0);
    const auto inspect_json = read_text_file(json_output);
    CHECK_NE(inspect_json.find("\"count\":2"), std::string::npos);
    CHECK_NE(inspect_json.find("\"style_id\":\"LegalHeading\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"style_id\":\"BodyNumbered\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"definition_name\":\"SharedOutline\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"level\":0"), std::string::npos);
    CHECK_NE(inspect_json.find("\"level\":1"), std::string::npos);
    CHECK_NE(inspect_json.find("\"instance\":{\"instance_id\":"),
             std::string::npos);
    CHECK_EQ(inspect_json.find("\"style_id\":\"PlainBody\""),
             std::string::npos);
    CHECK_EQ(inspect_json.find("\"numbering\":null"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-style-numbering", updated.string()}, text_output), 0);
    const auto inspect_text = read_text_file(text_output);
    CHECK_NE(inspect_text.find("styles: 2"), std::string::npos);
    CHECK_NE(inspect_text.find("id=LegalHeading"), std::string::npos);
    CHECK_NE(inspect_text.find("id=BodyNumbered"), std::string::npos);
    CHECK_NE(inspect_text.find("definition_name=SharedOutline"),
             std::string::npos);
    CHECK_NE(inspect_text.find("numbering=(level=0,"), std::string::npos);
    CHECK_NE(inspect_text.find("numbering=(level=1,"), std::string::npos);
    CHECK_EQ(inspect_text.find("id=PlainBody"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
}

TEST_CASE("cli inspect-style-numbering reports missing input as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_inspect_style_numbering_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"inspect-style-numbering", "--json"}, output), 2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"inspect-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"inspect-style-numbering "
            "expects an input path\"}\n"});

    remove_if_exists(output);
}


TEST_CASE("cli audit-style-numbering reports clean numbered styles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_audit_style_numbering_clean_source.docx";
    const fs::path updated =
        working_directory / "cli_audit_style_numbering_clean_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_audit_style_numbering_clean_mutate.json";
    const fs::path audit_output =
        working_directory / "cli_audit_style_numbering_clean_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(audit_output);

    create_cli_paragraph_style_fixture(source, "LegalHeading", "Legal Heading",
                                       "Heading1", false);

    CHECK_EQ(run_cli({"set-paragraph-style-numbering",
                      source.string(),
                      "LegalHeading",
                      "--definition-name",
                      "LegalHeadingOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      updated.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             0);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"command\":\"audit-style-numbering\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(audit_json.find("\"numbered_style_count\":1"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":0"), std::string::npos);
    CHECK_NE(audit_json.find("\"suggestion_count\":0"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"style_id\":\"LegalHeading\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"definition_name\":\"LegalHeadingOutline\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(audit_output);
}

TEST_CASE("cli audit-style-numbering reports broken style bindings") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_audit_style_numbering_broken_source.docx";
    const fs::path json_output =
        working_directory / "cli_audit_style_numbering_broken_output.json";
    const fs::path text_output =
        working_directory / "cli_audit_style_numbering_broken_output.txt";
    const fs::path parse_output =
        working_directory / "cli_audit_style_numbering_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(parse_output);

    create_cli_style_numbering_audit_broken_fixture(source);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      source.string(),
                      "--json",
                      "--fail-on-issue"},
                     json_output),
             1);
    const auto audit_json = read_text_file(json_output);
    CHECK_NE(audit_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(audit_json.find("\"paragraph_style_count\":4"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"numbered_style_count\":2"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":2"), std::string::npos);
    CHECK_NE(audit_json.find("\"suggestion_count\":2"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"missing_num_id\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"style_id\":\"MissingNumId\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"orphan_instance\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"style_id\":\"OrphanNumId\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"num_id\":777"), std::string::npos);
    CHECK_NE(audit_json.find("\"action\":\"clear_style_numbering\""),
             std::string::npos);
    CHECK_NE(audit_json.find(
                 "\"command_template\":\"featherdoc_cli clear-paragraph-style-numbering <input.docx> MissingNumId --output <output.docx> --json\""),
             std::string::npos);
    CHECK_NE(audit_json.find(
                 "\"command_template\":\"featherdoc_cli clear-paragraph-style-numbering <input.docx> OrphanNumId --output <output.docx> --json\""),
             std::string::npos);
    CHECK_EQ(audit_json.find("\"style_id\":\"PlainBody\""),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering", source.string()}, text_output), 0);
    const auto audit_text = read_text_file(text_output);
    CHECK_NE(audit_text.find("clean: no"), std::string::npos);
    CHECK_NE(audit_text.find("numbered_styles: 2"), std::string::npos);
    CHECK_NE(audit_text.find("issues: 2"), std::string::npos);
    CHECK_NE(audit_text.find("suggestions: 2"), std::string::npos);
    CHECK_NE(audit_text.find("suggestion[0]: action=clear_style_numbering"),
             std::string::npos);
    CHECK_NE(audit_text.find(
                 "command=featherdoc_cli clear-paragraph-style-numbering <input.docx> MissingNumId --output <output.docx> --json"),
             std::string::npos);
    CHECK_NE(audit_text.find("kind=missing_num_id style_id=MissingNumId"),
             std::string::npos);
    CHECK_NE(audit_text.find("kind=orphan_instance style_id=OrphanNumId"),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering", "--json"}, parse_output), 2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"audit-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"audit-style-numbering "
            "expects an input path\"}\n"});

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
    remove_if_exists(parse_output);
}



TEST_CASE("cli repair-style-numbering plans and applies safe clear repairs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_source.docx";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_repaired.docx";
    const fs::path plan_output =
        working_directory / "cli_repair_style_numbering_plan.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_apply.json";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_audit.json";
    const fs::path output_without_apply =
        working_directory / "cli_repair_style_numbering_output_without_apply.json";
    const fs::path apply_without_output =
        working_directory / "cli_repair_style_numbering_apply_without_output.json";

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);
    remove_if_exists(output_without_apply);
    remove_if_exists(apply_without_output);

    create_cli_style_numbering_audit_broken_fixture(source);

    CHECK_EQ(run_cli({"repair-style-numbering", source.string(), "--json"},
                     plan_output),
             0);
    const auto plan_json = read_text_file(plan_output);
    CHECK_NE(plan_json.find("\"command\":\"repair-style-numbering\""),
             std::string::npos);
    CHECK_NE(plan_json.find("\"mode\":\"plan\""), std::string::npos);
    CHECK_NE(plan_json.find("\"before_clean\":false"), std::string::npos);
    CHECK_NE(plan_json.find("\"before_issue_count\":2"), std::string::npos);
    CHECK_NE(plan_json.find("\"after_issue_count\":null"), std::string::npos);
    CHECK_NE(plan_json.find("\"suggestion_count\":2"), std::string::npos);
    CHECK_NE(plan_json.find("\"applyable_count\":2"), std::string::npos);
    CHECK_NE(plan_json.find("\"applied_count\":0"), std::string::npos);
    CHECK_NE(plan_json.find("\"applyable\":true"), std::string::npos);
    CHECK_NE(plan_json.find("\"output_path\":null"), std::string::npos);
    CHECK_NE(plan_json.find(
                 "\"command_template\":\"featherdoc_cli clear-paragraph-style-numbering <input.docx> MissingNumId --output <output.docx> --json\""),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"mode\":\"apply\""), std::string::npos);
    CHECK_NE(apply_json.find("\"before_issue_count\":2"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":true"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":0"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":2"), std::string::npos);
    CHECK_NE(apply_json.find("\"output_path\":" + json_quote(repaired.string())),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             0);
    const auto repaired_audit_json = read_text_file(audit_output);
    CHECK_NE(repaired_audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(repaired_audit_json.find("\"numbered_style_count\":0"),
             std::string::npos);
    CHECK_NE(repaired_audit_json.find("\"issue_count\":0"), std::string::npos);

    const auto source_styles_xml = read_docx_entry(source, "word/styles.xml");
    CHECK_NE(source_styles_xml.find("w:styleId=\"OrphanNumId\""),
             std::string::npos);
    CHECK_NE(source_styles_xml.find("<w:numId w:val=\"777\"/>"),
             std::string::npos);
    const auto repaired_styles_xml = read_docx_entry(repaired, "word/styles.xml");
    CHECK_EQ(repaired_styles_xml.find("<w:numPr>"), std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--output",
                      repaired.string(),
                      "--json"},
                     output_without_apply),
             2);
    CHECK_EQ(
        read_text_file(output_without_apply),
        std::string{
            "{\"command\":\"repair-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--output requires --apply\"}\n"});

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--apply",
                      "--json"},
                     apply_without_output),
             2);
    CHECK_EQ(
        read_text_file(apply_without_output),
        std::string{
            "{\"command\":\"repair-style-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"repair-style-numbering "
            "--apply requires --output <path>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);
    remove_if_exists(output_without_apply);
    remove_if_exists(apply_without_output);
}


TEST_CASE("cli repair-style-numbering applies based-on alignment repairs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_based_on_source.docx";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_based_on_repaired.docx";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_based_on_audit.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_based_on_apply.json";
    const fs::path repaired_audit_output =
        working_directory / "cli_repair_style_numbering_based_on_repaired_audit.json";

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(audit_output);
    remove_if_exists(apply_output);
    remove_if_exists(repaired_audit_output);

    create_cli_style_numbering_based_on_mismatch_fixture(source);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      source.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             1);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":1"), std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"based_on_definition_mismatch\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"style_id\":\"ChildNumbered\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"action\":\"align_with_based_on_numbering\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_definition_name\":\"BaseOutline\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_level\":0"), std::string::npos);
    CHECK_NE(audit_json.find("\"applyable\":true"), std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"mode\":\"apply\""), std::string::npos);
    CHECK_NE(apply_json.find("\"before_issue_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"applyable_count\":1"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":1"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":true"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":0"), std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     repaired_audit_output),
             0);
    const auto repaired_audit_json = read_text_file(repaired_audit_output);
    CHECK_NE(repaired_audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(repaired_audit_json.find("\"issue_count\":0"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(audit_output);
    remove_if_exists(apply_output);
    remove_if_exists(repaired_audit_output);
}


TEST_CASE("cli repair-style-numbering relinks missing levels to unique same-name definitions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_relink_source.docx";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_relink_repaired.docx";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_relink_audit.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_relink_apply.json";
    const fs::path repaired_audit_output =
        working_directory / "cli_repair_style_numbering_relink_repaired_audit.json";

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(audit_output);
    remove_if_exists(apply_output);
    remove_if_exists(repaired_audit_output);

    create_cli_style_numbering_missing_level_relink_fixture(source);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      source.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             1);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"clean\":false"), std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":2"), std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"missing_level_definition\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"kind\":\"duplicate_definition_name\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"action\":\"relink_style_numbering\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_definition_id\":2"),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_definition_name\":\"SharedOutline\""),
             std::string::npos);
    CHECK_NE(audit_json.find("\"target_level\":1"), std::string::npos);
    CHECK_NE(audit_json.find("\"applyable\":true"), std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"before_issue_count\":2"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"applyable_count\":1"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":1"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":false"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":1"),
             std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     repaired_audit_output),
             1);
    const auto repaired_audit_json = read_text_file(repaired_audit_output);
    CHECK_EQ(repaired_audit_json.find("\"kind\":\"missing_level_definition\""),
             std::string::npos);
    CHECK_NE(repaired_audit_json.find("\"kind\":\"duplicate_definition_name\""),
             std::string::npos);

    const auto repaired_styles_xml = read_docx_entry(repaired, "word/styles.xml");
    pugi::xml_document repaired_styles_document;
    REQUIRE(repaired_styles_document.load_string(repaired_styles_xml.c_str()));
    const auto relinked_style = find_style_xml_node(
        repaired_styles_document.child("w:styles"), "NeedsRelink");
    REQUIRE(relinked_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{relinked_style.child("w:pPr")
                                  .child("w:numPr")
                                  .child("w:numId")
                                  .attribute("w:val")
                                  .value()},
             "8");

    remove_if_exists(source);
    remove_if_exists(repaired);
    remove_if_exists(audit_output);
    remove_if_exists(apply_output);
    remove_if_exists(repaired_audit_output);
}


TEST_CASE("cli repair-style-numbering imports catalog repairs before style fixes") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_repair_style_numbering_catalog_source.docx";
    const fs::path catalog =
        working_directory / "cli_repair_style_numbering_catalog.json";
    const fs::path repaired =
        working_directory / "cli_repair_style_numbering_catalog_repaired.docx";
    const fs::path plan_output =
        working_directory / "cli_repair_style_numbering_catalog_plan.json";
    const fs::path apply_output =
        working_directory / "cli_repair_style_numbering_catalog_apply.json";
    const fs::path audit_output =
        working_directory / "cli_repair_style_numbering_catalog_audit.json";

    remove_if_exists(source);
    remove_if_exists(catalog);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);

    create_cli_style_numbering_catalog_repair_fixture(source);
    write_binary_file(
        catalog,
        "{\"definition_count\":1,\"instance_count\":0,\"definitions\":["
        "{\"name\":\"CatalogOutline\",\"levels\":["
        "{\"level\":0,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"%1.\"},"
        "{\"level\":1,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"%1.%2.\"}"
        "],\"instances\":[]}]}");

    CHECK_EQ(run_cli({"repair-style-numbering", source.string(), "--json"},
                     plan_output),
             0);
    const auto plain_plan_json = read_text_file(plan_output);
    CHECK_NE(plain_plan_json.find("\"action\":\"add_numbering_level\""),
             std::string::npos);
    CHECK_NE(plain_plan_json.find(
                 "patch-numbering-catalog numbering-catalog.json --patch-file "
                 "<patch-with-upsert_levels.json>"),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--catalog-file",
                      catalog.string(),
                      "--json"},
                     plan_output),
             2);
    CHECK_NE(read_text_file(plan_output).find("--catalog-file requires --apply"),
             std::string::npos);

    CHECK_EQ(run_cli({"repair-style-numbering",
                      source.string(),
                      "--catalog-file",
                      catalog.string(),
                      "--apply",
                      "--output",
                      repaired.string(),
                      "--json"},
                     apply_output),
             0);
    const auto apply_json = read_text_file(apply_output);
    CHECK_NE(apply_json.find("\"before_issue_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"catalog_file\":" + json_quote(catalog.string())),
             std::string::npos);
    CHECK_NE(apply_json.find("\"catalog_import\":{"), std::string::npos);
    CHECK_NE(apply_json.find("\"input_definition_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"imported_definition_count\":1"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"imported_instance_count\":0"),
             std::string::npos);
    CHECK_NE(apply_json.find("\"applyable_count\":0"), std::string::npos);
    CHECK_NE(apply_json.find("\"applied_count\":0"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_clean\":true"), std::string::npos);
    CHECK_NE(apply_json.find("\"after_issue_count\":0"), std::string::npos);

    CHECK_EQ(run_cli({"audit-style-numbering",
                      repaired.string(),
                      "--json",
                      "--fail-on-issue"},
                     audit_output),
             0);
    const auto audit_json = read_text_file(audit_output);
    CHECK_NE(audit_json.find("\"clean\":true"), std::string::npos);
    CHECK_NE(audit_json.find("\"issue_count\":0"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(catalog);
    remove_if_exists(repaired);
    remove_if_exists(plan_output);
    remove_if_exists(apply_output);
    remove_if_exists(audit_output);
}


TEST_CASE("cli ensure-style-linked-numbering links multiple paragraph styles to one shared instance") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_linked_numbering_source.docx";
    const fs::path updated =
        working_directory / "cli_style_linked_numbering_updated.docx";
    const fs::path output =
        working_directory / "cli_style_linked_numbering_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.create_empty());

        auto heading_style = featherdoc::paragraph_style_definition{};
        heading_style.name = "Legal Heading";
        heading_style.based_on = std::string{"Heading1"};
        heading_style.paragraph_bidi = false;
        REQUIRE(document.ensure_paragraph_style("LegalHeading", heading_style));

        auto subheading_style = featherdoc::paragraph_style_definition{};
        subheading_style.name = "Legal Subheading";
        subheading_style.based_on = std::string{"Heading2"};
        subheading_style.paragraph_bidi = false;
        REQUIRE(document.ensure_paragraph_style("LegalSubheading", subheading_style));

        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "LegalOutlineShared",
                      "--numbering-level",
                      "0:decimal:7:(%1)",
                      "--numbering-level",
                      "1:decimal:1:(%1.%2)",
                      "--style-link",
                      "LegalHeading:0",
                      "--style-link",
                      "LegalSubheading:1",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    featherdoc::Document updated_document(updated);
    REQUIRE_FALSE(updated_document.open());
    const auto definitions = updated_document.list_numbering_definitions();
    REQUIRE_FALSE(updated_document.last_error());
    const auto definition = std::find_if(definitions.begin(), definitions.end(),
                                         [](const auto &summary) {
                                             return summary.name == "LegalOutlineShared";
                                         });
    REQUIRE(definition != definitions.end());
    REQUIRE_EQ(definition->instance_ids.size(), 1U);
    const auto definition_id = std::to_string(definition->definition_id);

    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"ensure-style-linked-numbering\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"definition_id\":"} +
            definition_id +
            ",\"definition_name\":\"LegalOutlineShared\",\"definition_levels\":["
            "{\"level\":0,\"kind\":\"decimal\",\"start\":7,\"text_pattern\":\"(%1)\"},"
            "{\"level\":1,\"kind\":\"decimal\",\"start\":1,\"text_pattern\":\"(%1.%2)\"}],"
            "\"style_links\":[{\"style_id\":\"LegalHeading\",\"level\":0},"
            "{\"style_id\":\"LegalSubheading\",\"level\":1}]}\n");

    const auto heading_style = updated_document.find_style("LegalHeading");
    const auto subheading_style = updated_document.find_style("LegalSubheading");
    REQUIRE(heading_style.has_value());
    REQUIRE(subheading_style.has_value());
    REQUIRE(heading_style->numbering.has_value());
    REQUIRE(subheading_style->numbering.has_value());
    REQUIRE(heading_style->numbering->num_id.has_value());
    REQUIRE(subheading_style->numbering->num_id.has_value());
    CHECK_EQ(*heading_style->numbering->num_id, *subheading_style->numbering->num_id);
    REQUIRE(heading_style->numbering->level.has_value());
    REQUIRE(subheading_style->numbering->level.has_value());
    CHECK_EQ(*heading_style->numbering->level, 0U);
    CHECK_EQ(*subheading_style->numbering->level, 1U);

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});
    const auto heading_style_xml = find_style_xml_node(styles_root, "LegalHeading");
    const auto subheading_style_xml = find_style_xml_node(styles_root, "LegalSubheading");
    REQUIRE(heading_style_xml != pugi::xml_node{});
    REQUIRE(subheading_style_xml != pugi::xml_node{});
    CHECK_EQ(std::string_view{
                 heading_style_xml.child("w:pPr").child("w:numPr").child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{
                 subheading_style_xml.child("w:pPr").child("w:numPr").child("w:ilvl").attribute("w:val").value()},
             "1");

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli ensure-style-linked-numbering reports parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_linked_numbering_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_style_linked_numbering_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_style_linked_numbering_mutate.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_paragraph_style_fixture(source, "LegalHeading", "Legal Heading",
                                       "Heading1", false);

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "BrokenOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"ensure-style-linked-numbering\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected at least one --style-link <style-id>:<level>\"}\n"});

    CHECK_EQ(run_cli({"ensure-style-linked-numbering",
                      source.string(),
                      "--definition-name",
                      "BrokenOutline",
                      "--numbering-level",
                      "0:decimal:1:%1.",
                      "--style-link",
                      "MissingStyle:0",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"ensure-style-linked-numbering\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
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

TEST_CASE("cli paragraph list commands manage body numbering") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_list_source.docx";
    const fs::path listed =
        working_directory / "cli_paragraph_list_listed.docx";
    const fs::path restarted =
        working_directory / "cli_paragraph_list_restarted.docx";
    const fs::path cleared =
        working_directory / "cli_paragraph_list_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_paragraph_list_set_output.json";
    const fs::path restart_output =
        working_directory / "cli_paragraph_list_restart_output.json";
    const fs::path clear_output =
        working_directory / "cli_paragraph_list_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(listed);
    remove_if_exists(restarted);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(restart_output);
    remove_if_exists(clear_output);

    create_cli_paragraph_list_fixture(source);

    CHECK_EQ(run_cli({"set-paragraph-list",
                      source.string(),
                      "0",
                      "--kind",
                      "decimal",
                      "--output",
                      listed.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-paragraph-list\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":0,\"kind\":\"decimal\",\"level\":0}\n"});

    featherdoc::Document listed_document(listed);
    REQUIRE_FALSE(listed_document.open());
    const auto listed_definitions = listed_document.list_numbering_definitions();
    REQUIRE_FALSE(listed_document.last_error());
    const auto listed_definition =
        std::find_if(listed_definitions.begin(), listed_definitions.end(),
                     [](const auto &summary) {
                         return summary.name == "FeatherDocDecimalList";
                     });
    REQUIRE(listed_definition != listed_definitions.end());
    REQUIRE_EQ(listed_definition->instance_ids.size(), 1U);
    const auto first_num_id = std::to_string(listed_definition->instance_ids.front());

    const auto listed_document_xml = read_docx_entry(listed, "word/document.xml");
    pugi::xml_document listed_xml_document;
    REQUIRE(listed_xml_document.load_string(listed_document_xml.c_str()));
    const auto listed_paragraph =
        find_body_paragraph_xml_node(listed_xml_document.child("w:document"), 0U);
    REQUIRE(listed_paragraph != pugi::xml_node{});
    const auto listed_num_pr = listed_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(listed_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{listed_num_pr.child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string{listed_num_pr.child("w:numId").attribute("w:val").value()},
             first_num_id);

    CHECK_EQ(run_cli({"restart-paragraph-list",
                      listed.string(),
                      "1",
                      "--kind",
                      "decimal",
                      "--level",
                      "1",
                      "--output",
                      restarted.string(),
                      "--json"},
                     restart_output),
             0);
    CHECK_EQ(
        read_text_file(restart_output),
        std::string{
            "{\"command\":\"restart-paragraph-list\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":1,\"kind\":\"decimal\",\"level\":1}\n"});

    featherdoc::Document restarted_document(restarted);
    REQUIRE_FALSE(restarted_document.open());
    const auto restarted_definitions = restarted_document.list_numbering_definitions();
    REQUIRE_FALSE(restarted_document.last_error());
    const auto restarted_definition =
        std::find_if(restarted_definitions.begin(), restarted_definitions.end(),
                     [](const auto &summary) {
                         return summary.name == "FeatherDocDecimalList";
                     });
    REQUIRE(restarted_definition != restarted_definitions.end());
    REQUIRE_EQ(restarted_definition->instance_ids.size(), 2U);
    CHECK_EQ(std::to_string(restarted_definition->instance_ids[0]), first_num_id);
    const auto restarted_num_id =
        std::to_string(restarted_definition->instance_ids[1]);

    const auto restarted_document_xml =
        read_docx_entry(restarted, "word/document.xml");
    pugi::xml_document restarted_xml_document;
    REQUIRE(restarted_xml_document.load_string(restarted_document_xml.c_str()));
    const auto restarted_first_paragraph =
        find_body_paragraph_xml_node(restarted_xml_document.child("w:document"), 0U);
    REQUIRE(restarted_first_paragraph != pugi::xml_node{});
    const auto restarted_first_num_pr =
        restarted_first_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(restarted_first_num_pr != pugi::xml_node{});
    CHECK_EQ(
        std::string{restarted_first_num_pr.child("w:numId").attribute("w:val").value()},
        first_num_id);

    const auto restarted_second_paragraph =
        find_body_paragraph_xml_node(restarted_xml_document.child("w:document"), 1U);
    REQUIRE(restarted_second_paragraph != pugi::xml_node{});
    const auto restarted_second_num_pr =
        restarted_second_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(restarted_second_num_pr != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{restarted_second_num_pr.child("w:ilvl").attribute("w:val").value()},
        "1");
    CHECK_EQ(
        std::string{restarted_second_num_pr.child("w:numId").attribute("w:val").value()},
        restarted_num_id);

    CHECK_EQ(run_cli({"clear-paragraph-list",
                      restarted.string(),
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-paragraph-list\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"paragraph_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_first_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 0U);
    REQUIRE(cleared_first_paragraph != pugi::xml_node{});
    CHECK(cleared_first_paragraph.child("w:pPr").child("w:numPr") == pugi::xml_node{});

    const auto cleared_second_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 1U);
    REQUIRE(cleared_second_paragraph != pugi::xml_node{});
    const auto cleared_second_num_pr =
        cleared_second_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(cleared_second_num_pr != pugi::xml_node{});
    CHECK_EQ(
        std::string{cleared_second_num_pr.child("w:numId").attribute("w:val").value()},
        restarted_num_id);

    remove_if_exists(source);
    remove_if_exists(listed);
    remove_if_exists(restarted);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(restart_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli set-paragraph-list reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_list_parse_source.docx";
    const fs::path output =
        working_directory / "cli_paragraph_list_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_paragraph_list_fixture(source);

    CHECK_EQ(run_cli({"set-paragraph-list", source.string(), "0", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-paragraph-list\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing --kind "
            "<bullet|decimal>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-paragraphs lists body paragraphs with section/style metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_paragraphs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_paragraphs_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_inspect_paragraphs_fixture(source);

    CHECK_EQ(run_cli({"inspect-paragraphs", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("paragraphs: 5"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[0]: section_index=0 style_id=CustomBody numbering=none "
            "section_break=no text=section 0 body"),
        std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[1]: section_index=0 style_id=none numbering=none "
            "section_break=yes text=<empty>"),
        std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[2]: section_index=1 style_id=none numbering=num_id="),
        std::string::npos);
    CHECK_NE(inspect_text.find("level=0"), std::string::npos);
    CHECK_NE(inspect_text.find("text=section 1 body"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[3]: section_index=1 style_id=none numbering=none "
            "section_break=yes text=<empty>"),
        std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[4]: section_index=2 style_id=none numbering=none "
            "section_break=no text=section 2 body"),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-paragraphs supports single-paragraph json output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_paragraphs_single_source.docx";
    const fs::path json_output =
        working_directory / "cli_inspect_paragraphs_single_output.json";
    const fs::path missing_output =
        working_directory / "cli_inspect_paragraphs_missing_output.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_paragraphs_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);

    create_cli_inspect_paragraphs_fixture(source);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    const auto paragraph_index =
        find_body_paragraph_index_by_text(document, "section 1 body");

    const auto document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));
    const auto paragraph =
        find_body_paragraph_xml_node(xml_document.child("w:document"), paragraph_index);
    REQUIRE(paragraph != pugi::xml_node{});
    const auto numbering = paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(numbering != pugi::xml_node{});
    const auto numbering_id =
        std::string{numbering.child("w:numId").attribute("w:val").value()};
    REQUIRE_FALSE(numbering_id.empty());
    const auto has_section_break =
        paragraph.child("w:pPr").child("w:sectPr") != pugi::xml_node{};

    CHECK_EQ(run_cli({"inspect-paragraphs",
                      source.string(),
                      "--paragraph",
                      std::to_string(paragraph_index),
                      "--json"},
                     json_output),
             0);
    CHECK_EQ(
        read_text_file(json_output),
        std::string{"{\"paragraph\":{\"index\":"} +
            std::to_string(paragraph_index) +
            ",\"section_index\":1,\"style_id\":null,\"numbering\":{\"num_id\":" +
            numbering_id +
            ",\"level\":0},\"section_break\":" +
            std::string{has_section_break ? "true" : "false"} +
            ",\"text\":\"section 1 body\"}}\n");

    CHECK_EQ(run_cli({"inspect-paragraphs",
                      source.string(),
                      "--paragraph",
                      "999",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-paragraphs\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find("\"detail\":\"paragraph index '999' is out of range\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-paragraphs", source.string(), "--json", "--paragraph"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-paragraphs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing value after "
            "--paragraph\"}\n"});

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli inspect-runs lists paragraph runs with style metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_runs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_runs_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"inspect-runs", source.string(), "1"}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("paragraph_index: 1"), std::string::npos);
    CHECK_NE(inspect_text.find("runs: 2"), std::string::npos);
    CHECK_NE(inspect_text.find("run[0]: style_id=none font_family=none "
                               "east_asia_font_family=none language=none text=beta"),
             std::string::npos);
    CHECK_NE(inspect_text.find("run[1]: style_id=Strong font_family=none "
                               "east_asia_font_family=none language=none text=gamma"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-runs supports single-run json output and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_runs_single_source.docx";
    const fs::path json_output =
        working_directory / "cli_inspect_runs_single_output.json";
    const fs::path missing_paragraph_output =
        working_directory / "cli_inspect_runs_missing_paragraph_output.json";
    const fs::path missing_run_output =
        working_directory / "cli_inspect_runs_missing_run_output.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_runs_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(missing_paragraph_output);
    remove_if_exists(missing_run_output);
    remove_if_exists(parse_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"inspect-runs",
                      source.string(),
                      "1",
                      "--run",
                      "1",
                      "--json"},
                     json_output),
             0);
    CHECK_EQ(
        read_text_file(json_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":1,\"style_id\":\"Strong\","
            "\"font_family\":null,\"east_asia_font_family\":null,"
            "\"language\":null,\"text\":\"gamma\"}}\n"});

    CHECK_EQ(run_cli({"inspect-runs", source.string(), "999", "--json"},
                     missing_paragraph_output),
             1);
    const auto missing_paragraph_json = read_text_file(missing_paragraph_output);
    CHECK_NE(missing_paragraph_json.find("\"command\":\"inspect-runs\""),
             std::string::npos);
    CHECK_NE(missing_paragraph_json.find("\"stage\":\"inspect\""),
             std::string::npos);
    CHECK_NE(
        missing_paragraph_json.find(
            "\"detail\":\"paragraph index '999' is out of range\""),
        std::string::npos);
    CHECK_NE(missing_paragraph_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-runs",
                      source.string(),
                      "1",
                      "--run",
                      "99",
                      "--json"},
                     missing_run_output),
             1);
    const auto missing_run_json = read_text_file(missing_run_output);
    CHECK_NE(missing_run_json.find("\"command\":\"inspect-runs\""),
             std::string::npos);
    CHECK_NE(missing_run_json.find("\"stage\":\"inspect\""),
             std::string::npos);
    CHECK_NE(
        missing_run_json.find(
            "\"detail\":\"run index '99' is out of range for paragraph index '1'\""),
        std::string::npos);
    CHECK_NE(missing_run_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-runs",
                      source.string(),
                      "1",
                      "--run",
                      "oops",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-runs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid run index: oops\"}\n"});

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(missing_paragraph_output);
    remove_if_exists(missing_run_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli inspect-tables lists body tables in text mode and supports single-table json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_inspect_tables_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_inspect_tables_output.txt";
    const fs::path single_output =
        working_directory / "cli_inspect_table_single.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-tables", source.string()}, inspect_output), 0);
    const auto inspect_text = read_text_file(inspect_output);
    CHECK_NE(inspect_text.find("tables: 2"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "table[0]: style_id=TableGrid width_twips=7200 rows=2 columns=2"),
        std::string::npos);
    CHECK_NE(inspect_text.find("column_widths=[1800,5400]"),
             std::string::npos);
    CHECK_NE(inspect_text.find("r0c0"), std::string::npos);
    CHECK_NE(inspect_text.find("merged-top"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-tables", source.string(), "--table", "1", "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"table\":{\"index\":1,\"style_id\":null,\"width_twips\":null,"
            "\"row_count\":2,\"column_count\":3,"
            "\"column_widths\":[1200,2200,3200],"
            "\"text\":\"merged-top\\ttop-right\\nbottom-left\\tbottom-middle\\t"
            "line1\\nline2\"}}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);
}

TEST_CASE("cli inspect-table-cells supports row filtering and single-cell json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_table_cells_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_inspect_table_cells_output.json";
    const fs::path single_output =
        working_directory / "cli_inspect_table_cell_single.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "1",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"table_index\":1,\"row_index\":1,\"count\":3,\"cells\":["
            "{\"row_index\":1,\"cell_index\":0,\"column_index\":0,"
            "\"column_span\":1,\"paragraph_count\":1,\"width_twips\":null,"
            "\"vertical_alignment\":null,\"text_direction\":null,"
            "\"text\":\"bottom-left\"},"
            "{\"row_index\":1,\"cell_index\":1,\"column_index\":1,"
            "\"column_span\":1,\"paragraph_count\":1,\"width_twips\":2222,"
            "\"vertical_alignment\":null,\"text_direction\":null,"
            "\"text\":\"bottom-middle\"},"
            "{\"row_index\":1,\"cell_index\":2,\"column_index\":2,"
            "\"column_span\":1,\"paragraph_count\":2,\"width_twips\":null,"
            "\"vertical_alignment\":null,\"text_direction\":null,"
            "\"text\":\"line1\\nline2\"}]}\n"});

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "0",
                      "--cell",
                      "0",
                      "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"table_index\":1,\"cell\":{\"row_index\":0,\"cell_index\":0,"
            "\"column_index\":0,\"column_span\":2,\"paragraph_count\":1,"
            "\"width_twips\":null,\"vertical_alignment\":\"center\","
            "\"text_direction\":\"top_to_bottom_right_to_left\","
            "\"text\":\"merged-top\"}}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);
}

TEST_CASE("cli inspect-table-rows supports list and single-row json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_table_rows_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_inspect_table_rows_output.json";
    const fs::path single_output =
        working_directory / "cli_inspect_table_row_single.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-table-rows", source.string(), "0", "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"table_index\":0,\"count\":2,\"rows\":["
            "{\"row_index\":0,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"r0c0\",\"r0c1\"]},"
            "{\"row_index\":1,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"r1c0\",\"r1c1\"]}"
            "]}\n"});

    CHECK_EQ(run_cli({"inspect-table-rows",
                      source.string(),
                      "0",
                      "--row",
                      "1",
                      "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"table_index\":0,\"row\":{\"row_index\":1,\"cell_count\":2,"
            "\"height_twips\":null,\"height_rule\":null,"
            "\"cant_split\":false,\"repeats_header\":false,"
            "\"cell_texts\":[\"r1c0\",\"r1c1\"]}}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(single_output);
}

TEST_CASE("cli inspect-table commands report parse and inspect errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_table_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_inspect_table_parse_output.json";
    const fs::path table_error_output =
        working_directory / "cli_inspect_table_error_output.json";
    const fs::path row_error_output =
        working_directory / "cli_inspect_table_row_error_output.json";
    const fs::path cell_error_output =
        working_directory / "cli_inspect_table_cell_error_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(table_error_output);
    remove_if_exists(row_error_output);
    remove_if_exists(cell_error_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--cell",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--cell requires --row\"}\n"});

    CHECK_EQ(run_cli({"inspect-tables", source.string(), "--table", "9", "--json"},
                     table_error_output),
             1);
    const auto table_error_json = read_text_file(table_error_output);
    CHECK_NE(table_error_json.find("\"command\":\"inspect-tables\""),
             std::string::npos);
    CHECK_NE(table_error_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(table_error_json.find("\"detail\":\"table index '9' is out of range\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "9",
                      "--json"},
                     row_error_output),
             1);
    const auto row_error_json = read_text_file(row_error_output);
    CHECK_NE(row_error_json.find("\"command\":\"inspect-table-cells\""),
             std::string::npos);
    CHECK_NE(row_error_json.find("\"detail\":\"row index '9' is out of range for "
                                 "table index '1'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-table-cells",
                      source.string(),
                      "1",
                      "--row",
                      "1",
                      "--cell",
                      "9",
                      "--json"},
                     cell_error_output),
             1);
    const auto cell_error_json = read_text_file(cell_error_output);
    CHECK_NE(cell_error_json.find("\"command\":\"inspect-table-cells\""),
             std::string::npos);
    CHECK_NE(
        cell_error_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '1' in "
            "table index '1'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(table_error_output);
    remove_if_exists(row_error_output);
    remove_if_exists(cell_error_output);
}

TEST_CASE("cli inspect-template-paragraphs inspects body template paragraphs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_paragraphs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_template_paragraphs_output.txt";
    const fs::path json_output =
        working_directory / "cli_inspect_template_paragraph_single.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(json_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-paragraphs", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("part: body\n"), std::string::npos);
    CHECK_NE(inspect_text.find("entry_name: word/document.xml\n"),
             std::string::npos);
    CHECK_NE(inspect_text.find("paragraphs: 2"), std::string::npos);
    CHECK_NE(
        inspect_text.find(
            "paragraph[0]: style_id=Heading1 bidi=yes numbering=none runs=1 "
            "text=Body heading"),
        std::string::npos);
    CHECK_NE(inspect_text.find("paragraph[1]: style_id=none bidi=none numbering=num_id="),
             std::string::npos);
    CHECK_NE(inspect_text.find("definition_name=CliTemplateInspectOutline"),
             std::string::npos);
    CHECK_NE(inspect_text.find("text=Body item"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-paragraphs",
                      source.string(),
                      "--paragraph",
                      "1",
                      "--json"},
                     json_output),
             0);
    const auto paragraph_json = read_text_file(json_output);
    CHECK_NE(paragraph_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(paragraph_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"paragraph\":{\"index\":1"),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"definition_name\":\"CliTemplateInspectOutline\""),
             std::string::npos);
    CHECK_NE(paragraph_json.find("\"run_count\":1"), std::string::npos);
    CHECK_NE(paragraph_json.find("\"text\":\"Body item\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-template-runs inspects section footer runs and errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_runs_source.docx";
    const fs::path output =
        working_directory / "cli_inspect_template_runs_output.txt";
    const fs::path missing_output =
        working_directory / "cli_inspect_template_runs_missing.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_template_runs_parse.json";

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--run",
                      "0"},
                     output),
             0);
    const auto run_text = read_text_file(output);
    CHECK_NE(run_text.find("part: section-footer\n"), std::string::npos);
    CHECK_NE(run_text.find("section: 0\n"), std::string::npos);
    CHECK_NE(run_text.find("kind: default\n"), std::string::npos);
    CHECK_NE(run_text.find("entry_name: word/footer1.xml\n"), std::string::npos);
    CHECK_NE(run_text.find("paragraph_index: 1\n"), std::string::npos);
    CHECK_NE(run_text.find("index: 0\n"), std::string::npos);
    CHECK_NE(run_text.find("style_id: Strong\n"), std::string::npos);
    CHECK_NE(run_text.find("font_family: Segoe UI\n"), std::string::npos);
    CHECK_NE(run_text.find("language: en-US\n"), std::string::npos);
    CHECK_NE(run_text.find("text: Footer styled\n"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--run",
                      "9",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find("\"command\":\"inspect-template-runs\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(missing_json.find(
                 "\"detail\":\"run index '9' is out of range for paragraph "
                 "index '1'\""),
             std::string::npos);
    CHECK_NE(missing_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-runs",
                      source.string(),
                      "1",
                      "--part",
                      "section-footer",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-runs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "inspection requires --section <index>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli inspect-template-table commands inspect header parts and report errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_tables_source.docx";
    const fs::path tables_output =
        working_directory / "cli_inspect_template_tables_output.json";
    const fs::path cell_output =
        working_directory / "cli_inspect_template_table_cell_output.json";
    const fs::path parse_output =
        working_directory / "cli_inspect_template_table_parse.json";
    const fs::path row_error_output =
        working_directory / "cli_inspect_template_table_row_error.json";

    remove_if_exists(source);
    remove_if_exists(tables_output);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-tables",
                      source.string(),
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     tables_output),
             0);
    const auto tables_json = read_text_file(tables_output);
    CHECK_NE(tables_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(tables_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(tables_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(tables_json.find("\"count\":1"), std::string::npos);
    CHECK_NE(tables_json.find("\"style_id\":\"TableGrid\""), std::string::npos);
    CHECK_NE(tables_json.find("\"column_widths\":[1800,3600]"),
             std::string::npos);
    CHECK_NE(tables_json.find("\"text\":\"h00\\th01\\nh10\\th11\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "1",
                      "--cell",
                      "1",
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(cell_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(cell_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(cell_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"width_twips\":2222"), std::string::npos);
    CHECK_NE(cell_json.find("\"text\":\"h11\""), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--cell",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--cell requires --row\"}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "9",
                      "--json"},
                     row_error_output),
             1);
    const auto row_error_json = read_text_file(row_error_output);
    CHECK_NE(row_error_json.find("\"command\":\"inspect-template-table-cells\""),
             std::string::npos);
    CHECK_NE(row_error_json.find("\"detail\":\"row index '9' is out of range "
                                 "for table index '0'\""),
             std::string::npos);
    CHECK_NE(row_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(tables_output);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_error_output);
}

TEST_CASE("cli inspect-template-table-rows inspects header rows and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_template_table_rows_source.docx";
    const fs::path list_output =
        working_directory / "cli_inspect_template_table_rows_output.json";
    const fs::path single_output =
        working_directory / "cli_inspect_template_table_row_single.json";
    const fs::path error_output =
        working_directory / "cli_inspect_template_table_row_error.json";

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(single_output);
    remove_if_exists(error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     list_output),
             0);
    CHECK_EQ(
        read_text_file(list_output),
        std::string{
            "{\"part\":\"header\",\"part_index\":0,"
            "\"entry_name\":\"word/header1.xml\",\"table_index\":0,"
            "\"count\":2,\"rows\":["
            "{\"row_index\":0,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"h00\",\"h01\"]},"
            "{\"row_index\":1,\"cell_count\":2,\"height_twips\":null,"
            "\"height_rule\":null,\"cant_split\":false,"
            "\"repeats_header\":false,\"cell_texts\":[\"h10\",\"h11\"]}"
            "]}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "1",
                      "--json"},
                     single_output),
             0);
    CHECK_EQ(
        read_text_file(single_output),
        std::string{
            "{\"part\":\"header\",\"part_index\":0,"
            "\"entry_name\":\"word/header1.xml\",\"table_index\":0,"
            "\"row\":{\"row_index\":1,\"cell_count\":2,"
            "\"height_twips\":null,\"height_rule\":null,"
            "\"cant_split\":false,\"repeats_header\":false,"
            "\"cell_texts\":[\"h10\",\"h11\"]}}\n"});

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--row",
                      "9",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"inspect-template-table-rows\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for "
                             "table index '0'\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(list_output);
    remove_if_exists(single_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli set-table-cell-text updates a cell and preserves inspection metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_table_cell_text_source.docx";
    const fs::path updated =
        working_directory / "cli_set_table_cell_text_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_set_table_cell_text_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-text",
                      source.string(),
                      "1",
                      "0",
                      "0",
                      "--text",
                      "updated merged",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);
    CHECK_EQ(
        read_text_file(mutate_output),
        std::string{
            "{\"command\":\"set-table-cell-text\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":1,\"row_index\":0,\"cell_index\":0}\n"});

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    const auto cell = reopened.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(cell.has_value());
    CHECK_EQ(cell->text, "updated merged");
    CHECK_EQ(cell->column_span, 2U);
    REQUIRE(cell->vertical_alignment.has_value());
    CHECK_EQ(*cell->vertical_alignment,
             featherdoc::cell_vertical_alignment::center);
    REQUIRE(cell->text_direction.has_value());
    CHECK_EQ(*cell->text_direction,
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli set-table-cell-text supports file input and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_table_cell_text_file_source.docx";
    const fs::path text_source =
        working_directory / "cli_set_table_cell_text_input.txt";
    const fs::path parse_output =
        working_directory / "cli_set_table_cell_text_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_set_table_cell_text_error_output.json";

    remove_if_exists(source);
    remove_if_exists(text_source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_inspection_fixture(source);

    {
        std::ofstream stream(text_source);
        REQUIRE(stream.good());
        stream << "from file";
    }

    CHECK_EQ(run_cli({"set-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--text-file",
                      text_source.string()}),
             0);

    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open());
    const auto updated_cell = reopened.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "from file");

    CHECK_EQ(run_cli({"set-table-cell-text",
                      source.string(),
                      "1",
                      "0",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-cell-text\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected --text <text> or "
            "--text-file <path>\"}\n"});

    CHECK_EQ(run_cli({"set-table-cell-text",
                      source.string(),
                      "1",
                      "1",
                      "9",
                      "--text",
                      "x",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"set-table-cell-text\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '1' in "
            "table index '1'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli set-template-table-cell-text updates header template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_template_table_cell_text_source.docx";
    const fs::path updated =
        working_directory / "cli_set_template_table_cell_text_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_set_template_table_cell_text_output.json";
    const fs::path parse_output =
        working_directory / "cli_set_template_table_cell_text_parse.json";
    const fs::path error_output =
        working_directory / "cli_set_template_table_cell_text_error.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(parse_output);
    remove_if_exists(error_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--text",
                      "updated header cell",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(mutate_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    auto header_template = reopened.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    const auto updated_cell = header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated header cell");
    REQUIRE(updated_cell->width_twips.has_value());
    CHECK_EQ(*updated_cell->width_twips, 2222U);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--text",
                      "x",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-cell-text\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "9",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--text",
                      "x",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for "
                             "table index '0'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(parse_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli set-template-table-cell-text updates direct footer template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_direct_footer_template_cell_text_source.docx";
    const fs::path updated =
        working_directory / "cli_set_direct_footer_template_cell_text_updated.docx";
    const fs::path mutate_output =
        working_directory / "cli_set_direct_footer_template_cell_text_output.json";
    const fs::path error_output =
        working_directory / "cli_set_direct_footer_template_cell_text_error.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(error_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--text",
                      "updated footer cell",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);

    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(mutate_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(mutate_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    auto footer_template = reopened.footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    const auto updated_cell = footer_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated footer cell");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "9",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--text",
                      "x",
                      "--json"},
                     error_output),
             1);
    const auto error_json = read_text_file(error_output);
    CHECK_NE(error_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(error_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(error_json.find("\"detail\":\"row index '9' is out of range for table index '0'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
    remove_if_exists(error_output);
}

TEST_CASE("cli set-template-table-cell-text updates section kind template cells") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_section_kind_template_cell_text_source.docx";
    const fs::path even_updated =
        working_directory / "cli_set_section_kind_even_header_updated.docx";
    const fs::path first_updated =
        working_directory / "cli_set_section_kind_first_footer_updated.docx";
    const fs::path even_output =
        working_directory / "cli_set_section_kind_even_header_output.json";
    const fs::path first_output =
        working_directory / "cli_set_section_kind_first_footer_output.json";

    remove_if_exists(source);
    remove_if_exists(even_updated);
    remove_if_exists(first_updated);
    remove_if_exists(even_output);
    remove_if_exists(first_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--text",
                      "updated even header cell",
                      "--output",
                      even_updated.string(),
                      "--json"},
                     even_output),
             0);
    const auto even_json = read_text_file(even_output);
    CHECK_NE(even_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(even_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(even_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(even_json.find("\"kind\":\"even\""), std::string::npos);

    featherdoc::Document reopened_even(even_updated);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto even_cell =
        even_header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(even_cell.has_value());
    CHECK_EQ(even_cell->text, "updated even header cell");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "0",
                      "1",
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--text",
                      "updated first footer cell",
                      "--output",
                      first_updated.string(),
                      "--json"},
                     first_output),
             0);
    const auto first_json = read_text_file(first_output);
    CHECK_NE(first_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(first_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(first_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(first_json.find("\"kind\":\"first\""), std::string::npos);

    featherdoc::Document reopened_first(first_updated);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "updated first footer cell");

    remove_if_exists(source);
    remove_if_exists(even_updated);
    remove_if_exists(first_updated);
    remove_if_exists(even_output);
    remove_if_exists(first_output);
}

TEST_CASE("cli template table row commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_row_source.docx";
    const fs::path even_appended =
        working_directory / "cli_section_kind_template_table_even_appended.docx";
    const fs::path first_removed =
        working_directory / "cli_section_kind_template_table_first_removed.docx";
    const fs::path append_output =
        working_directory / "cli_section_kind_template_table_even_append.json";
    const fs::path remove_output =
        working_directory / "cli_section_kind_template_table_first_remove.json";

    remove_if_exists(source);
    remove_if_exists(even_appended);
    remove_if_exists(first_removed);
    remove_if_exists(append_output);
    remove_if_exists(remove_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--output",
                      even_appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);

    featherdoc::Document reopened_even(even_appended);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto appended_table = even_header_template.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    const auto appended_cell =
        even_header_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_cell.has_value());
    CHECK_EQ(appended_cell->text, "");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--output",
                      first_removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_first(first_removed);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto removed_table = first_footer_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    const auto removed_first_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "ff10");

    remove_if_exists(source);
    remove_if_exists(even_appended);
    remove_if_exists(first_removed);
    remove_if_exists(append_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_column_source.docx";
    const fs::path even_inserted =
        working_directory / "cli_section_kind_template_table_even_column_before.docx";
    const fs::path first_removed =
        working_directory / "cli_section_kind_template_table_first_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_section_kind_template_table_even_column_before.json";
    const fs::path remove_output =
        working_directory / "cli_section_kind_template_table_first_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(even_inserted);
    remove_if_exists(first_removed);
    remove_if_exists(before_output);
    remove_if_exists(remove_output);

    create_cli_template_table_section_kind_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--output",
                      even_inserted.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_even(even_inserted);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto even_table = even_header_template.inspect_table(0U);
    REQUIRE(even_table.has_value());
    CHECK_EQ(even_table->column_count, 3U);
    REQUIRE(even_table->column_widths.size() >= 3U);
    CHECK(even_table->column_widths[0].has_value());
    CHECK(even_table->column_widths[1].has_value());
    CHECK(even_table->column_widths[2].has_value());
    CHECK_EQ(*even_table->column_widths[0], 1800U);
    CHECK_EQ(*even_table->column_widths[1], 3600U);
    CHECK_EQ(*even_table->column_widths[2], 3600U);
    const auto even_inserted_header =
        even_header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(even_inserted_header.has_value());
    CHECK_EQ(even_inserted_header->text, "");
    const auto even_shifted_header =
        even_header_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(even_shifted_header.has_value());
    CHECK_EQ(even_shifted_header->text, "eh01");
    const auto even_inserted_body =
        even_header_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(even_inserted_body.has_value());
    CHECK_EQ(even_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--output",
                      first_removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_first(first_removed);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_table = first_footer_template.inspect_table(0U);
    REQUIRE(first_table.has_value());
    CHECK_EQ(first_table->column_count, 1U);
    const auto first_header_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_header_cell.has_value());
    CHECK_EQ(first_header_cell->text, "ff00");
    const auto first_body_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(first_body_cell.has_value());
    CHECK_EQ(first_body_cell->text, "ff10");

    remove_if_exists(source);
    remove_if_exists(even_inserted);
    remove_if_exists(first_removed);
    remove_if_exists(before_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table merge and unmerge commands mutate section kind template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_section_kind_template_table_merge_source.docx";
    const fs::path even_merged =
        working_directory / "cli_section_kind_template_table_even_merged.docx";
    const fs::path first_unmerged =
        working_directory /
        "cli_section_kind_template_table_first_unmerged.docx";
    const fs::path merge_output =
        working_directory / "cli_section_kind_template_table_even_merge.json";
    const fs::path unmerge_output =
        working_directory /
        "cli_section_kind_template_table_first_unmerge.json";

    remove_if_exists(source);
    remove_if_exists(even_merged);
    remove_if_exists(first_unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);

    create_cli_template_table_section_kind_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "even",
                      "--direction",
                      "right",
                      "--count",
                      "1",
                      "--output",
                      even_merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"kind\":\"even\""), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_even(even_merged);
    REQUIRE_FALSE(reopened_even.open());
    auto even_header_template = reopened_even.section_header_template(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(static_cast<bool>(even_header_template));
    const auto merged_even_cell =
        even_header_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(merged_even_cell.has_value());
    CHECK_EQ(merged_even_cell->text, "eh00");
    CHECK_EQ(merged_even_cell->column_span, 2U);
    const auto merged_even_tail =
        even_header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(merged_even_tail.has_value());
    CHECK_EQ(merged_even_tail->text, "eh02");
    CHECK_EQ(merged_even_tail->column_index, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "first",
                      "--direction",
                      "down",
                      "--output",
                      first_unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"kind\":\"first\""), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"down\""), std::string::npos);

    featherdoc::Document reopened_first(first_unmerged);
    REQUIRE_FALSE(reopened_first.open());
    auto first_footer_template = reopened_first.section_footer_template(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(static_cast<bool>(first_footer_template));
    const auto first_top_cell =
        first_footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_top_cell.has_value());
    CHECK_EQ(first_top_cell->text, "ff00");
    const auto first_middle_cell =
        first_footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(first_middle_cell.has_value());
    CHECK_EQ(first_middle_cell->text, "");
    const auto first_bottom_cell =
        first_footer_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(first_bottom_cell.has_value());
    CHECK_EQ(first_bottom_cell->text, "");

    remove_if_exists(source);
    remove_if_exists(even_merged);
    remove_if_exists(first_unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);
}

TEST_CASE("cli template table row commands mutate header template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_row_source.docx";
    const fs::path appended =
        working_directory / "cli_template_table_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_row_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_row_append.json";
    const fs::path before_output =
        working_directory / "cli_template_table_row_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_row_remove.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(append_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_header = reopened_appended.header_template(0U);
    REQUIRE(static_cast<bool>(appended_header));
    const auto appended_table = appended_header.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    CHECK_EQ(appended_table->column_count, 2U);
    const auto appended_first_cell = appended_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_header.inspect_table_cell(0U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "0",
                      "1",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_header = reopened_before.header_template(0U);
    REQUIRE(static_cast<bool>(before_header));
    const auto before_table = before_header.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 3U);
    const auto before_inserted_first_cell =
        before_header.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        before_header.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    const auto before_shifted_first_cell =
        before_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(before_shifted_first_cell.has_value());
    CHECK_EQ(before_shifted_first_cell->text, "h10");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_header = reopened_after.header_template(0U);
    REQUIRE(static_cast<bool>(after_header));
    const auto after_table = after_header.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 3U);
    const auto after_inserted_first_cell =
        after_header.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        after_header.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_shifted_first_cell =
        after_header.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(after_shifted_first_cell.has_value());
    CHECK_EQ(after_shifted_first_cell->text, "h10");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_header = reopened_removed.header_template(0U);
    REQUIRE(static_cast<bool>(removed_header));
    const auto removed_table = removed_header.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_cell = removed_header.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "h10");
    const auto removed_second_cell = removed_header.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_cell.has_value());
    CHECK_EQ(removed_second_cell->text, "h11");

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table row commands mutate direct footer template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_direct_footer_template_table_row_source.docx";
    const fs::path appended =
        working_directory / "cli_direct_footer_template_table_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_direct_footer_template_table_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_direct_footer_template_table_row_after.docx";
    const fs::path removed =
        working_directory / "cli_direct_footer_template_table_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_direct_footer_template_table_row_append.json";
    const fs::path before_output =
        working_directory / "cli_direct_footer_template_table_row_before.json";
    const fs::path after_output =
        working_directory / "cli_direct_footer_template_table_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_direct_footer_template_table_row_remove.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(append_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_footer = reopened_appended.footer_template(0U);
    REQUIRE(static_cast<bool>(appended_footer));
    const auto appended_table = appended_footer.inspect_table(0U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    CHECK_EQ(appended_table->column_count, 2U);
    const auto appended_first_cell = appended_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_footer.inspect_table_cell(0U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "0",
                      "1",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_footer = reopened_before.footer_template(0U);
    REQUIRE(static_cast<bool>(before_footer));
    const auto before_table = before_footer.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 3U);
    const auto before_inserted_first_cell =
        before_footer.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        before_footer.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    const auto before_shifted_first_cell =
        before_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(before_shifted_first_cell.has_value());
    CHECK_EQ(before_shifted_first_cell->text, "f10");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_footer = reopened_after.footer_template(0U);
    REQUIRE(static_cast<bool>(after_footer));
    const auto after_table = after_footer.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 3U);
    const auto after_inserted_first_cell =
        after_footer.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        after_footer.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_shifted_first_cell =
        after_footer.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(after_shifted_first_cell.has_value());
    CHECK_EQ(after_shifted_first_cell->text, "f10");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(remove_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":0"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_footer = reopened_removed.footer_template(0U);
    REQUIRE(static_cast<bool>(removed_footer));
    const auto removed_table = removed_footer.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->row_count, 1U);
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_cell = removed_footer.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_cell.has_value());
    CHECK_EQ(removed_first_cell->text, "f10");
    const auto removed_second_cell = removed_footer.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_cell.has_value());
    CHECK_EQ(removed_second_cell->text, "f11");

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table row commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path parse_source =
        working_directory / "cli_template_table_row_parse_source.docx";
    const fs::path merged_source =
        working_directory / "cli_template_table_row_merge_source.docx";
    const fs::path single_row_source =
        working_directory / "cli_template_table_row_single_source.docx";
    const fs::path parse_output =
        working_directory / "cli_template_table_row_parse.json";
    const fs::path merge_error_output =
        working_directory / "cli_template_table_row_merge_error.json";
    const fs::path remove_error_output =
        working_directory / "cli_template_table_row_remove_error.json";

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_template_inspection_fixture(parse_source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      parse_source.string(),
                      "0",
                      "--part",
                      "section-header",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-template-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    featherdoc::Document merged_document(merged_source);
    REQUIRE_FALSE(merged_document.create_empty());
    auto &merged_header = merged_document.ensure_header_paragraphs();
    REQUIRE(merged_header.has_next());
    auto merged_template = merged_document.header_template(0U);
    REQUIRE(static_cast<bool>(merged_template));
    auto merged_table = merged_template.append_table(3U, 1U);
    REQUIRE(merged_table.has_next());
    auto merged_row = merged_table.rows();
    REQUIRE(merged_row.has_next());
    auto merged_cell = merged_row.cells();
    REQUIRE(merged_cell.has_next());
    REQUIRE(merged_cell.set_text("merged-root"));
    REQUIRE(merged_cell.merge_down(2U));
    REQUIRE_FALSE(merged_document.save());

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      merged_source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(merge_error_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(merge_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a row adjacent to row index '0' in "
            "table index '0' because the row participates in a vertical "
            "merge\""),
        std::string::npos);

    featherdoc::Document single_row_document(single_row_source);
    REQUIRE_FALSE(single_row_document.create_empty());
    auto &single_header = single_row_document.ensure_header_paragraphs();
    REQUIRE(single_header.has_next());
    auto single_template = single_row_document.header_template(0U);
    REQUIRE(static_cast<bool>(single_template));
    auto single_table = single_template.append_table(1U, 1U);
    REQUIRE(single_table.has_next());
    auto single_row = single_table.rows();
    REQUIRE(single_row.has_next());
    auto only_cell = single_row.cells();
    REQUIRE(only_cell.has_next());
    REQUIRE(only_cell.set_text("only-row"));
    REQUIRE_FALSE(single_row_document.save());

    CHECK_EQ(run_cli({"remove-template-table-row",
                      single_row_source.string(),
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(remove_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last row from table index '0'\""),
        std::string::npos);

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

TEST_CASE("cli template table commands support bookmark table selectors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_bookmark_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_template_table_bookmark_inspect.json";
    const fs::path updated =
        working_directory / "cli_template_table_bookmark_updated.docx";
    const fs::path updated_output =
        working_directory / "cli_template_table_bookmark_updated.json";
    const fs::path rows_updated =
        working_directory / "cli_template_table_bookmark_rows_updated.docx";
    const fs::path rows_output =
        working_directory / "cli_template_table_bookmark_rows_updated.json";
    const fs::path block_updated =
        working_directory / "cli_template_table_bookmark_block_updated.docx";
    const fs::path block_output =
        working_directory / "cli_template_table_bookmark_block_updated.json";
    const fs::path appended =
        working_directory / "cli_template_table_bookmark_appended.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_bookmark_appended.json";
    const fs::path inserted =
        working_directory / "cli_template_table_bookmark_inserted.docx";
    const fs::path insert_output =
        working_directory / "cli_template_table_bookmark_inserted.json";
    const fs::path range_error_output =
        working_directory / "cli_template_table_bookmark_row_range_error.json";
    const fs::path cell_error_output =
        working_directory / "cli_template_table_bookmark_row_cells_error.json";
    const fs::path block_error_output =
        working_directory / "cli_template_table_bookmark_block_error.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_bookmark_parse.json";
    const fs::path row_parse_output =
        working_directory / "cli_template_table_bookmark_row_parse.json";
    const fs::path block_parse_output =
        working_directory / "cli_template_table_bookmark_block_parse.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(updated);
    remove_if_exists(updated_output);
    remove_if_exists(rows_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_updated);
    remove_if_exists(block_output);
    remove_if_exists(appended);
    remove_if_exists(append_output);
    remove_if_exists(inserted);
    remove_if_exists(insert_output);
    remove_if_exists(range_error_output);
    remove_if_exists(cell_error_output);
    remove_if_exists(block_error_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_parse_output);
    remove_if_exists(block_parse_output);

    create_cli_template_table_bookmark_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(inspect_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(inspect_json.find("\"count\":2"), std::string::npos);
    CHECK_NE(inspect_json.find("\"cell_texts\":[\"target-00\",\"target-01\"]"),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"cell_texts\":[\"target-10\",\"target-11\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "1",
                      "1",
                      "--text",
                      "updated",
                      "--output",
                      updated.string(),
                      "--json"},
                     updated_output),
             0);
    const auto updated_json = read_text_file(updated_output);
    CHECK_NE(updated_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(updated_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(updated_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(updated_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(updated_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_updated(updated);
    REQUIRE_FALSE(reopened_updated.open());
    auto updated_body = reopened_updated.body_template();
    REQUIRE(static_cast<bool>(updated_body));
    const auto preserved_cell = updated_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_cell.has_value());
    CHECK_EQ(preserved_cell->text, "keep-00");
    const auto updated_cell = updated_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_cell.has_value());
    CHECK_EQ(updated_cell->text, "updated");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "--row",
                      "batch-00",
                      "--cell",
                      "batch-01",
                      "--row",
                      "batch-10",
                      "--cell",
                      "batch-11",
                      "--output",
                      rows_updated.string(),
                      "--json"},
                     rows_output),
             0);
    const auto rows_json = read_text_file(rows_output);
    CHECK_NE(rows_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(rows_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(rows_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(rows_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(rows_json.find("\"rows\":[[\"batch-00\",\"batch-01\"],[\"batch-10\",\"batch-11\"]]"),
             std::string::npos);

    featherdoc::Document reopened_rows(rows_updated);
    REQUIRE_FALSE(reopened_rows.open());
    auto rows_body = reopened_rows.body_template();
    REQUIRE(static_cast<bool>(rows_body));
    const auto preserved_batch_cell = rows_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_batch_cell.has_value());
    CHECK_EQ(preserved_batch_cell->text, "keep-00");
    const auto batch_first_cell = rows_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(batch_first_cell.has_value());
    CHECK_EQ(batch_first_cell->text, "batch-00");
    const auto batch_second_cell = rows_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(batch_second_cell.has_value());
    CHECK_EQ(batch_second_cell->text, "batch-01");
    const auto batch_third_cell = rows_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(batch_third_cell.has_value());
    CHECK_EQ(batch_third_cell->text, "batch-10");
    const auto batch_fourth_cell = rows_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(batch_fourth_cell.has_value());
    CHECK_EQ(batch_fourth_cell->text, "batch-11");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "1",
                      "--row",
                      "overflow-10",
                      "--cell",
                      "overflow-11",
                      "--row",
                      "overflow-20",
                      "--cell",
                      "overflow-21",
                      "--json"},
                     range_error_output),
             1);
    const auto range_error_json = read_text_file(range_error_output);
    CHECK_NE(range_error_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(range_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(range_error_json.find("\"detail\":\"row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "--row",
                      "only-one-cell",
                      "--json"},
                     cell_error_output),
             1);
    const auto cell_error_json = read_text_file(cell_error_output);
    CHECK_NE(cell_error_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(cell_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(cell_error_json.find("\"detail\":\"replacement row at offset '0' contains '1' cells but target row index '0' contains '2' cells\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "1",
                      "--row",
                      "block-01",
                      "--row",
                      "block-11",
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(block_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"rows\":[[\"block-01\"],[\"block-11\"]]"),
             std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_block_first = block_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(preserved_block_first.has_value());
    CHECK_EQ(preserved_block_first->text, "target-00");
    const auto updated_block_first = block_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(updated_block_first.has_value());
    CHECK_EQ(updated_block_first->text, "block-01");
    const auto preserved_block_second = block_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(preserved_block_second.has_value());
    CHECK_EQ(preserved_block_second->text, "target-10");
    const auto updated_block_second = block_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_block_second.has_value());
    CHECK_EQ(updated_block_second->text, "block-11");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "0",
                      "1",
                      "--row",
                      "too-wide-01",
                      "--cell",
                      "too-wide-02",
                      "--json"},
                     block_error_output),
             1);
    const auto block_error_json = read_text_file(block_error_output);
    CHECK_NE(block_error_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_error_json.find("\"stage\":\"mutate\""),
             std::string::npos);
    CHECK_NE(block_error_json.find("\"detail\":\"replacement row at offset '0' starting at cell index '1' with count '2' exceeds target row index '0' cell count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"cell_count\":2"), std::string::npos);

    featherdoc::Document reopened_appended(appended);
    REQUIRE_FALSE(reopened_appended.open());
    auto appended_body = reopened_appended.body_template();
    REQUIRE(static_cast<bool>(appended_body));
    const auto appended_table = appended_body.inspect_table(1U);
    REQUIRE(appended_table.has_value());
    CHECK_EQ(appended_table->row_count, 3U);
    const auto appended_first_cell = appended_body.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(appended_first_cell.has_value());
    CHECK_EQ(appended_first_cell->text, "");
    const auto appended_second_cell = appended_body.inspect_table_cell(1U, 2U, 1U);
    REQUIRE(appended_second_cell.has_value());
    CHECK_EQ(appended_second_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "1",
                      "--output",
                      inserted.string(),
                      "--json"},
                     insert_output),
             0);
    const auto insert_json = read_text_file(insert_output);
    CHECK_NE(insert_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(insert_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(insert_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(insert_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(insert_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_inserted(inserted);
    REQUIRE_FALSE(reopened_inserted.open());
    auto inserted_body = reopened_inserted.body_template();
    REQUIRE(static_cast<bool>(inserted_body));
    const auto inserted_table = inserted_body.inspect_table(1U);
    REQUIRE(inserted_table.has_value());
    CHECK_EQ(inserted_table->row_count, 3U);
    const auto inserted_first_cell = inserted_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(inserted_first_cell.has_value());
    CHECK_EQ(inserted_first_cell->text, "");
    const auto inserted_second_cell = inserted_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(inserted_second_cell.has_value());
    CHECK_EQ(inserted_second_cell->text, "");
    const auto shifted_row_cell = inserted_body.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(shifted_row_cell.has_value());
    CHECK_EQ(shifted_row_cell->text, "target-10");

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-template-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "1",
                      "0",
                      "--bookmark",
                      "target_inside_table",
                      "--row",
                      "dup-00",
                      "--cell",
                      "dup-01",
                      "--json"},
                     row_parse_output),
             2);
    CHECK_EQ(
        read_text_file(row_parse_output),
        std::string{
            "{\"command\":\"set-template-table-row-texts\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "1",
                      "0",
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--row",
                      "dup-block-01",
                      "--json"},
                     block_parse_output),
             2);
    CHECK_EQ(
        read_text_file(block_parse_output),
        std::string{
            "{\"command\":\"set-template-table-cell-block-texts\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(updated);
    remove_if_exists(updated_output);
    remove_if_exists(rows_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_updated);
    remove_if_exists(block_output);
    remove_if_exists(appended);
    remove_if_exists(append_output);
    remove_if_exists(inserted);
    remove_if_exists(insert_output);
    remove_if_exists(range_error_output);
    remove_if_exists(cell_error_output);
    remove_if_exists(block_error_output);
    remove_if_exists(parse_output);
    remove_if_exists(row_parse_output);
    remove_if_exists(block_parse_output);
}

TEST_CASE("cli set-template-table-from-json applies bookmark-targeted patches") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_json_patch_source.docx";
    const fs::path rows_patch =
        working_directory / "cli_template_table_rows_patch.json";
    const fs::path block_patch =
        working_directory / "cli_template_table_block_patch.json";
    const fs::path parse_patch =
        working_directory / "cli_template_table_parse_patch.json";
    const fs::path mutate_patch =
        working_directory / "cli_template_table_mutate_patch.json";
    const fs::path rows_updated =
        working_directory / "cli_template_table_rows_patch_updated.docx";
    const fs::path block_updated =
        working_directory / "cli_template_table_block_patch_updated.docx";
    const fs::path rows_output =
        working_directory / "cli_template_table_rows_patch_output.json";
    const fs::path block_output =
        working_directory / "cli_template_table_block_patch_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_parse_patch_output.json";
    const fs::path mutate_output =
        working_directory / "cli_template_table_mutate_patch_output.json";
    const fs::path selector_parse_output =
        working_directory / "cli_template_table_selector_patch_output.json";

    remove_if_exists(source);
    remove_if_exists(rows_patch);
    remove_if_exists(block_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(rows_updated);
    remove_if_exists(block_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(selector_parse_output);

    create_cli_template_table_bookmark_fixture(source);
    write_binary_file(
        rows_patch,
        R"({
  "mode": "rows",
  "start_row": 0,
  "rows": [
    ["json-00", 12],
    ["json-10", true]
  ]
})");
    write_binary_file(
        block_patch,
        R"({
  "command": "set-template-table-cell-block-texts",
  "start_row_index": 0,
  "start_cell_index": 1,
  "row_count": 2,
  "rows": [
    [12],
    [false]
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "mode": "rows",
  "rows": [
    ["missing-start-row", "value"]
  ]
})");
    write_binary_file(
        mutate_patch,
        R"({
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["overflow-10", "overflow-11"],
    ["overflow-20", "overflow-21"]
  ]
})");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--patch-file",
                      rows_patch.string(),
                      "--output",
                      rows_updated.string(),
                      "--json"},
                     rows_output),
             0);
    const auto rows_json = read_text_file(rows_output);
    CHECK_NE(rows_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(rows_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(rows_json.find("\"mode\":\"rows\""), std::string::npos);
    CHECK_NE(rows_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(rows_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(rows_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(rows_json.find("\"rows\":[[\"json-00\",\"12\"],[\"json-10\",\"true\"]]"),
             std::string::npos);

    featherdoc::Document reopened_rows(rows_updated);
    REQUIRE_FALSE(reopened_rows.open());
    auto rows_body = reopened_rows.body_template();
    REQUIRE(static_cast<bool>(rows_body));
    const auto preserved_rows_cell = rows_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(preserved_rows_cell.has_value());
    CHECK_EQ(preserved_rows_cell->text, "keep-00");
    const auto json_row_first = rows_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(json_row_first.has_value());
    CHECK_EQ(json_row_first->text, "json-00");
    const auto json_row_second = rows_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(json_row_second.has_value());
    CHECK_EQ(json_row_second->text, "12");
    const auto json_row_third = rows_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(json_row_third.has_value());
    CHECK_EQ(json_row_third->text, "json-10");
    const auto json_row_fourth = rows_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(json_row_fourth.has_value());
    CHECK_EQ(json_row_fourth->text, "true");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      block_patch.string(),
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"bookmark_name\":\"target_inside_table\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"mode\":\"block\""), std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":0"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"rows\":[[\"12\"],[\"false\"]]"),
             std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_block_first = block_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(preserved_block_first.has_value());
    CHECK_EQ(preserved_block_first->text, "target-00");
    const auto updated_block_first = block_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(updated_block_first.has_value());
    CHECK_EQ(updated_block_first->text, "12");
    const auto preserved_block_second = block_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(preserved_block_second.has_value());
    CHECK_EQ(preserved_block_second->text, "target-10");
    const auto updated_block_second = block_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_block_second.has_value());
    CHECK_EQ(updated_block_second->text, "false");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_before_table",
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"JSON patch must contain "
            "'start_row' or 'start_row_index'\"}\n"});

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      mutate_patch.string(),
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "1",
                      "--bookmark",
                      "target_inside_table",
                      "--patch-file",
                      rows_patch.string(),
                      "--json"},
                     selector_parse_output),
             2);
    CHECK_EQ(
        read_text_file(selector_parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index "
            "with --bookmark\"}\n"});

    remove_if_exists(source);
    remove_if_exists(rows_patch);
    remove_if_exists(block_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(rows_updated);
    remove_if_exists(block_updated);
    remove_if_exists(rows_output);
    remove_if_exists(block_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(selector_parse_output);
}

TEST_CASE("cli template table selectors support after-text, header cells, and occurrence") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_source.docx";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_after.json";
    const fs::path header_output =
        working_directory / "cli_template_table_selector_header.json";
    const fs::path combined_output =
        working_directory / "cli_template_table_selector_combined.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_parse.json";

    remove_if_exists(source);
    remove_if_exists(after_output);
    remove_if_exists(header_output);
    remove_if_exists(combined_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"cell_texts\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(after_json.find("\"cell_texts\":[\"North\",\"7\",\"12\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     header_output),
             0);
    const auto header_json = read_text_file(header_output);
    CHECK_NE(header_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(header_json.find("\"cell_texts\":[\"South\",\"9\",\"24\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     combined_output),
             0);
    const auto combined_json = read_text_file(combined_output);
    CHECK_NE(combined_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(combined_json.find("\"cell_texts\":[\"South\",\"9\",\"24\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-rows",
                      source.string(),
                      "0",
                      "--after-text",
                      "selector target table",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-rows\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cannot combine a table index or "
            "--bookmark with --after-text or --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(after_output);
    remove_if_exists(header_output);
    remove_if_exists(combined_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli set-template-table-from-json supports selector-based targeting") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_patch_source.docx";
    const fs::path patch_path =
        working_directory / "cli_template_table_selector_patch.json";
    const fs::path updated =
        working_directory / "cli_template_table_selector_patch_updated.docx";
    const fs::path output =
        working_directory / "cli_template_table_selector_patch_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_patch_parse.json";

    remove_if_exists(source);
    remove_if_exists(patch_path);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);
    write_binary_file(
        patch_path,
        R"({
  "mode": "rows",
  "start_row": 1,
  "rows": [
    ["South-updated", 18, 199]
  ]
})");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--patch-file",
                      patch_path.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    const auto output_json = read_text_file(output);
    CHECK_NE(output_json.find("\"command\":\"set-template-table-from-json\""),
             std::string::npos);
    CHECK_NE(output_json.find("\"after_text\":\"selector target table\""),
             std::string::npos);
    CHECK_NE(output_json.find("\"header_cells\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(output_json.find("\"header_row_index\":0"), std::string::npos);
    CHECK_NE(output_json.find("\"occurrence\":1"), std::string::npos);
    CHECK_NE(output_json.find("\"table_index\":2"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body = reopened.body_template();
    REQUIRE(static_cast<bool>(body));
    const auto preserved_first_target = body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_target.has_value());
    CHECK_EQ(preserved_first_target->text, "North");
    const auto updated_region = body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_region.has_value());
    CHECK_EQ(updated_region->text, "South-updated");
    const auto updated_qty = body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_qty.has_value());
    CHECK_EQ(updated_qty->text, "18");
    const auto updated_amount = body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_amount.has_value());
    CHECK_EQ(updated_amount->text, "199");

    CHECK_EQ(run_cli({"set-template-table-from-json",
                      source.string(),
                      "--header-row",
                      "1",
                      "--patch-file",
                      patch_path.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-table-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--header-row requires at least "
            "one --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(patch_path);
    remove_if_exists(updated);
    remove_if_exists(output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli set-template-tables-from-json applies multiple table patches") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_tables_batch_source.docx";
    const fs::path success_patch =
        working_directory / "cli_template_tables_batch_success.json";
    const fs::path parse_patch =
        working_directory / "cli_template_tables_batch_parse.json";
    const fs::path mutate_patch =
        working_directory / "cli_template_tables_batch_mutate.json";
    const fs::path updated =
        working_directory / "cli_template_tables_batch_updated.docx";
    const fs::path success_output =
        working_directory / "cli_template_tables_batch_success_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_tables_batch_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_template_tables_batch_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_bookmark_fixture(source);
    write_binary_file(
        success_patch,
        R"({
  "operations": [
    {
      "table_index": 0,
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["keep-json-00", "keep-json-01"]
      ]
    },
    {
      "bookmark": "target_before_table",
      "start_row_index": 0,
      "start_cell_index": 1,
      "rows": [
        [12],
        [false]
      ]
    }
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "operations": [
    {
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["missing-selector", "value"]
      ]
    }
  ]
})");
    write_binary_file(
        mutate_patch,
        R"({
  "operations": [
    {
      "table_index": 0,
      "mode": "rows",
      "start_row": 0,
      "rows": [
        ["keep-json-00", "keep-json-01"]
      ]
    },
    {
      "bookmark_name": "target_inside_table",
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["overflow-10", "overflow-11"],
        ["overflow-20", "overflow-21"]
      ]
    }
  ]
})");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      success_patch.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"operation_count\":2"), std::string::npos);
    CHECK_NE(success_json.find("\"operation_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"rows\":[[\"keep-json-00\",\"keep-json-01\"]]"),
             std::string::npos);
    CHECK_NE(success_json.find("\"operation_index\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"bookmark_name\":\"target_before_table\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"mode\":\"block\""), std::string::npos);
    CHECK_NE(success_json.find("\"start_cell_index\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"rows\":[[\"12\"],[\"false\"]]"),
             std::string::npos);

    featherdoc::Document reopened_updated(updated);
    REQUIRE_FALSE(reopened_updated.open());
    auto updated_body = reopened_updated.body_template();
    REQUIRE(static_cast<bool>(updated_body));
    const auto first_table_first_cell =
        updated_body.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_table_first_cell.has_value());
    CHECK_EQ(first_table_first_cell->text, "keep-json-00");
    const auto first_table_second_cell =
        updated_body.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(first_table_second_cell.has_value());
    CHECK_EQ(first_table_second_cell->text, "keep-json-01");
    const auto second_table_first_region =
        updated_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(second_table_first_region.has_value());
    CHECK_EQ(second_table_first_region->text, "target-00");
    const auto second_table_first_value =
        updated_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(second_table_first_value.has_value());
    CHECK_EQ(second_table_first_value->text, "12");
    const auto second_table_second_region =
        updated_body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(second_table_second_region.has_value());
    CHECK_EQ(second_table_second_region->text, "target-10");
    const auto second_table_second_value =
        updated_body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(second_table_second_value.has_value());
    CHECK_EQ(second_table_second_value->text, "false");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-tables-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"operation[0]: expected a table "
            "index, --bookmark <name>, --after-text <text>, or --header-cell "
            "<text>\"}\n"});

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      mutate_patch.string(),
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"operation[1]: row range starting at row index '1' with count '2' exceeds table index '1' row count '2'\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(mutate_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli set-template-tables-from-json supports selector-based operations") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_tables_selector_source.docx";
    const fs::path success_patch =
        working_directory / "cli_template_tables_selector_success.json";
    const fs::path parse_patch =
        working_directory / "cli_template_tables_selector_parse.json";
    const fs::path updated =
        working_directory / "cli_template_tables_selector_updated.docx";
    const fs::path success_output =
        working_directory / "cli_template_tables_selector_success_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_tables_selector_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);
    write_binary_file(
        success_patch,
        R"({
  "operations": [
    {
      "after_text": "selector target table",
      "header_cells": ["Region", "Qty", "Amount"],
      "occurrence": 1,
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["South-batch", 21, 240]
      ]
    },
    {
      "header_cell_texts": ["Label", "Value"],
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["Other-batch", 42]
      ]
    }
  ]
})");
    write_binary_file(
        parse_patch,
        R"({
  "operations": [
    {
      "table_index": 2,
      "after_text": "selector target table",
      "mode": "rows",
      "start_row": 1,
      "rows": [
        ["invalid-selector", 1, 2]
      ]
    }
  ]
})");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      success_patch.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"set-template-tables-from-json\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"after_text\":\"selector target table\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"header_cells\":[\"Region\",\"Qty\",\"Amount\"]"),
             std::string::npos);
    CHECK_NE(success_json.find("\"occurrence\":1"), std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(success_json.find("\"header_cells\":[\"Label\",\"Value\"]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body = reopened.body_template();
    REQUIRE(static_cast<bool>(body));
    const auto updated_other_left = body.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(updated_other_left.has_value());
    CHECK_EQ(updated_other_left->text, "Other-batch");
    const auto updated_other_right = body.inspect_table_cell(1U, 1U, 1U);
    REQUIRE(updated_other_right.has_value());
    CHECK_EQ(updated_other_right->text, "42");
    const auto updated_target_region = body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_target_region.has_value());
    CHECK_EQ(updated_target_region->text, "South-batch");
    const auto updated_target_qty = body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_target_qty.has_value());
    CHECK_EQ(updated_target_qty->text, "21");
    const auto updated_target_amount = body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_target_amount.has_value());
    CHECK_EQ(updated_target_amount->text, "240");

    CHECK_EQ(run_cli({"set-template-tables-from-json",
                      source.string(),
                      "--patch-file",
                      parse_patch.string(),
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-template-tables-from-json\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"operation[0]: cannot combine a "
            "table index or --bookmark with --after-text or --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(success_patch);
    remove_if_exists(parse_patch);
    remove_if_exists(updated);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli selector-based template table text mutations target the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_direct_text_source.docx";
    const fs::path cell_updated =
        working_directory / "cli_template_table_selector_direct_cell_updated.docx";
    const fs::path multiline_cell_updated =
        working_directory / "cli_template_table_selector_direct_multiline_cell_updated.docx";
    const fs::path row_updated =
        working_directory / "cli_template_table_selector_direct_row_updated.docx";
    const fs::path block_updated =
        working_directory / "cli_template_table_selector_direct_block_updated.docx";
    const fs::path multiline_block_updated =
        working_directory / "cli_template_table_selector_direct_multiline_block_updated.docx";
    const fs::path cell_output =
        working_directory / "cli_template_table_selector_direct_cell_output.json";
    const fs::path multiline_cell_output =
        working_directory / "cli_template_table_selector_direct_multiline_cell_output.json";
    const fs::path row_output =
        working_directory / "cli_template_table_selector_direct_row_output.json";
    const fs::path block_output =
        working_directory / "cli_template_table_selector_direct_block_output.json";
    const fs::path multiline_block_output =
        working_directory / "cli_template_table_selector_direct_multiline_block_output.json";

    remove_if_exists(source);
    remove_if_exists(cell_updated);
    remove_if_exists(multiline_cell_updated);
    remove_if_exists(row_updated);
    remove_if_exists(block_updated);
    remove_if_exists(multiline_block_updated);
    remove_if_exists(cell_output);
    remove_if_exists(multiline_cell_output);
    remove_if_exists(row_output);
    remove_if_exists(block_output);
    remove_if_exists(multiline_block_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--text",
                      "42",
                      "--output",
                      cell_updated.string(),
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_cell(cell_updated);
    REQUIRE_FALSE(reopened_cell.open());
    auto cell_body = reopened_cell.body_template();
    REQUIRE(static_cast<bool>(cell_body));
    const auto preserved_first_qty = cell_body.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(preserved_first_qty.has_value());
    CHECK_EQ(preserved_first_qty->text, "7");
    const auto updated_target_qty = cell_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_target_qty.has_value());
    CHECK_EQ(updated_target_qty->text, "42");

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "2",
                      "--text",
                      "240 total\nfinance review",
                      "--output",
                      multiline_cell_updated.string(),
                      "--json"},
                     multiline_cell_output),
             0);
    const auto multiline_cell_json = read_text_file(multiline_cell_output);
    CHECK_NE(multiline_cell_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(multiline_cell_json.find("\"cell_index\":2"), std::string::npos);

    featherdoc::Document reopened_multiline_cell(multiline_cell_updated);
    REQUIRE_FALSE(reopened_multiline_cell.open());
    auto multiline_cell_body = reopened_multiline_cell.body_template();
    REQUIRE(static_cast<bool>(multiline_cell_body));
    const auto preserved_first_amount =
        multiline_cell_body.inspect_table_cell(0U, 1U, 2U);
    REQUIRE(preserved_first_amount.has_value());
    CHECK_EQ(preserved_first_amount->text, "12");
    const auto updated_multiline_cell_amount =
        multiline_cell_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_multiline_cell_amount.has_value());
    CHECK_EQ(updated_multiline_cell_amount->text, "240 total\nfinance review");

    CHECK_EQ(run_cli({"set-template-table-row-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--row",
                      "South revised",
                      "--cell",
                      "18",
                      "--cell",
                      "199",
                      "--output",
                      row_updated.string(),
                      "--json"},
                     row_output),
             0);
    const auto row_json = read_text_file(row_output);
    CHECK_NE(row_json.find("\"command\":\"set-template-table-row-texts\""),
             std::string::npos);
    CHECK_NE(row_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(row_json.find("\"start_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_row(row_updated);
    REQUIRE_FALSE(reopened_row.open());
    auto row_body = reopened_row.body_template();
    REQUIRE(static_cast<bool>(row_body));
    const auto preserved_first_region = row_body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_region.has_value());
    CHECK_EQ(preserved_first_region->text, "North");
    const auto updated_target_region = row_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(updated_target_region.has_value());
    CHECK_EQ(updated_target_region->text, "South revised");
    const auto updated_target_amount = row_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_target_amount.has_value());
    CHECK_EQ(updated_target_amount->text, "199");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--row",
                      "18",
                      "--cell",
                      "240",
                      "--output",
                      block_updated.string(),
                      "--json"},
                     block_output),
             0);
    const auto block_json = read_text_file(block_output);
    CHECK_NE(block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(block_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(block_json.find("\"start_row_index\":1"), std::string::npos);
    CHECK_NE(block_json.find("\"start_cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_block(block_updated);
    REQUIRE_FALSE(reopened_block.open());
    auto block_body = reopened_block.body_template();
    REQUIRE(static_cast<bool>(block_body));
    const auto preserved_target_region = block_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(preserved_target_region.has_value());
    CHECK_EQ(preserved_target_region->text, "South");
    const auto updated_block_qty = block_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_block_qty.has_value());
    CHECK_EQ(updated_block_qty->text, "18");
    const auto updated_block_amount = block_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_block_amount.has_value());
    CHECK_EQ(updated_block_amount->text, "240");

    CHECK_EQ(run_cli({"set-template-table-cell-block-texts",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--row",
                      "18 units\npending approval",
                      "--cell",
                      "240 total\nfinance review",
                      "--output",
                      multiline_block_updated.string(),
                      "--json"},
                     multiline_block_output),
             0);
    const auto multiline_block_json = read_text_file(multiline_block_output);
    CHECK_NE(multiline_block_json.find("\"command\":\"set-template-table-cell-block-texts\""),
             std::string::npos);
    CHECK_NE(multiline_block_json.find("\"table_index\":2"), std::string::npos);

    featherdoc::Document reopened_multiline_block(multiline_block_updated);
    REQUIRE_FALSE(reopened_multiline_block.open());
    auto multiline_block_body = reopened_multiline_block.body_template();
    REQUIRE(static_cast<bool>(multiline_block_body));
    const auto updated_multiline_qty =
        multiline_block_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(updated_multiline_qty.has_value());
    CHECK_EQ(updated_multiline_qty->text, "18 units\npending approval");
    const auto updated_multiline_amount =
        multiline_block_body.inspect_table_cell(2U, 1U, 2U);
    REQUIRE(updated_multiline_amount.has_value());
    CHECK_EQ(updated_multiline_amount->text, "240 total\nfinance review");

    remove_if_exists(source);
    remove_if_exists(cell_updated);
    remove_if_exists(multiline_cell_updated);
    remove_if_exists(row_updated);
    remove_if_exists(block_updated);
    remove_if_exists(multiline_block_updated);
    remove_if_exists(cell_output);
    remove_if_exists(multiline_cell_output);
    remove_if_exists(row_output);
    remove_if_exists(block_output);
    remove_if_exists(multiline_block_output);
}

TEST_CASE("cli selector-based template table row commands mutate only the matched table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_row_commands_source.docx";
    const fs::path appended =
        working_directory / "cli_template_table_selector_row_appended.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_selector_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_selector_row_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_selector_row_removed.docx";
    const fs::path append_output =
        working_directory / "cli_template_table_selector_row_append.json";
    const fs::path before_output =
        working_directory / "cli_template_table_selector_row_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_row_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_selector_row_remove.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_row_parse.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"append-template-table-row",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--cell-count",
                      "3",
                      "--output",
                      appended.string(),
                      "--json"},
                     append_output),
             0);
    const auto append_json = read_text_file(append_output);
    CHECK_NE(append_json.find("\"command\":\"append-template-table-row\""),
             std::string::npos);
    CHECK_NE(append_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(append_json.find("\"row_index\":2"), std::string::npos);

    featherdoc::Document reopened_append(appended);
    REQUIRE_FALSE(reopened_append.open());
    auto append_body = reopened_append.body_template();
    REQUIRE(static_cast<bool>(append_body));
    const auto preserved_first_table = append_body.inspect_table(0U);
    REQUIRE(preserved_first_table.has_value());
    CHECK_EQ(preserved_first_table->row_count, 2U);
    const auto appended_target_table = append_body.inspect_table(2U);
    REQUIRE(appended_target_table.has_value());
    CHECK_EQ(appended_target_table->row_count, 3U);
    const auto appended_target_cell = append_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(appended_target_cell.has_value());
    CHECK_EQ(appended_target_cell->text, "");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-row-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_body = reopened_before.body_template();
    REQUIRE(static_cast<bool>(before_body));
    const auto before_target_table = before_body.inspect_table(2U);
    REQUIRE(before_target_table.has_value());
    CHECK_EQ(before_target_table->row_count, 3U);
    const auto before_inserted_cell = before_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(before_inserted_cell.has_value());
    CHECK_EQ(before_inserted_cell->text, "");
    const auto before_shifted_cell = before_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(before_shifted_cell.has_value());
    CHECK_EQ(before_shifted_cell->text, "South");

    CHECK_EQ(run_cli({"insert-template-table-row-after",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-row-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(after_json.find("\"inserted_row_index\":1"), std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_body = reopened_after.body_template();
    REQUIRE(static_cast<bool>(after_body));
    const auto after_target_table = after_body.inspect_table(2U);
    REQUIRE(after_target_table.has_value());
    CHECK_EQ(after_target_table->row_count, 3U);
    const auto after_inserted_cell = after_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(after_inserted_cell.has_value());
    CHECK_EQ(after_inserted_cell->text, "");
    const auto after_shifted_cell = after_body.inspect_table_cell(2U, 2U, 0U);
    REQUIRE(after_shifted_cell.has_value());
    CHECK_EQ(after_shifted_cell->text, "South");

    CHECK_EQ(run_cli({"remove-template-table-row",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-row\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(remove_json.find("\"row_index\":1"), std::string::npos);

    featherdoc::Document reopened_remove(removed);
    REQUIRE_FALSE(reopened_remove.open());
    auto remove_body = reopened_remove.body_template();
    REQUIRE(static_cast<bool>(remove_body));
    const auto removed_target_table = remove_body.inspect_table(2U);
    REQUIRE(removed_target_table.has_value());
    CHECK_EQ(removed_target_table->row_count, 1U);
    const auto removed_header_cell = remove_body.inspect_table_cell(2U, 0U, 0U);
    REQUIRE(removed_header_cell.has_value());
    CHECK_EQ(removed_header_cell->text, "Region");

    CHECK_EQ(run_cli({"insert-template-table-row-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"insert-template-table-row-before\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing row index after table "
            "selector\"}\n"});

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(append_output);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli selector-based template table column commands mutate only the matched table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_selector_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_selector_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_selector_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_selector_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_selector_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_selector_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_body = reopened_before.body_template();
    REQUIRE(static_cast<bool>(before_body));
    const auto before_first_table = before_body.inspect_table(0U);
    REQUIRE(before_first_table.has_value());
    CHECK_EQ(before_first_table->column_count, 3U);
    const auto before_target_table = before_body.inspect_table(2U);
    REQUIRE(before_target_table.has_value());
    CHECK_EQ(before_target_table->column_count, 4U);
    const auto before_inserted_header = before_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header = before_body.inspect_table_cell(2U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "Qty");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_body = reopened_after.body_template();
    REQUIRE(static_cast<bool>(after_body));
    const auto after_target_table = after_body.inspect_table(2U);
    REQUIRE(after_target_table.has_value());
    CHECK_EQ(after_target_table->column_count, 4U);
    const auto after_inserted_header = after_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header = after_body.inspect_table_cell(2U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "Qty");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "0",
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_remove(removed);
    REQUIRE_FALSE(reopened_remove.open());
    auto remove_body = reopened_remove.body_template();
    REQUIRE(static_cast<bool>(remove_body));
    const auto removed_target_table = remove_body.inspect_table(2U);
    REQUIRE(removed_target_table.has_value());
    CHECK_EQ(removed_target_table->column_count, 2U);
    const auto removed_header = remove_body.inspect_table_cell(2U, 0U, 1U);
    REQUIRE(removed_header.has_value());
    CHECK_EQ(removed_header->text, "Amount");
    const auto removed_value = remove_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(removed_value.has_value());
    CHECK_EQ(removed_value->text, "24");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands mutate section and body template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_column_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(before_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_template = reopened_before.section_header_template(0U);
    REQUIRE(static_cast<bool>(before_template));
    const auto before_table = before_template.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->column_count, 3U);
    REQUIRE(before_table->column_widths.size() >= 3U);
    CHECK(before_table->column_widths[0].has_value());
    CHECK(before_table->column_widths[1].has_value());
    CHECK(before_table->column_widths[2].has_value());
    CHECK_EQ(*before_table->column_widths[0], 1800U);
    CHECK_EQ(*before_table->column_widths[1], 3600U);
    CHECK_EQ(*before_table->column_widths[2], 3600U);
    const auto before_inserted_header =
        before_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header =
        before_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "h01");
    const auto before_inserted_body =
        before_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_body.has_value());
    CHECK_EQ(before_inserted_body->text, "");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"part\":\"section-footer\""), std::string::npos);
    CHECK_NE(after_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_template = reopened_after.section_footer_template(0U);
    REQUIRE(static_cast<bool>(after_template));
    const auto after_table = after_template.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->column_count, 3U);
    REQUIRE(after_table->column_widths.size() >= 3U);
    CHECK(after_table->column_widths[0].has_value());
    CHECK(after_table->column_widths[1].has_value());
    CHECK(after_table->column_widths[2].has_value());
    CHECK_EQ(*after_table->column_widths[0], 1600U);
    CHECK_EQ(*after_table->column_widths[1], 1600U);
    CHECK_EQ(*after_table->column_widths[2], 2800U);
    const auto after_inserted_header =
        after_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header =
        after_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "f01");
    const auto after_inserted_body =
        after_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_body.has_value());
    CHECK_EQ(after_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "body",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(remove_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_template = reopened_removed.body_template();
    REQUIRE(static_cast<bool>(removed_template));
    const auto removed_table = removed_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_header =
        removed_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_header.has_value());
    CHECK_EQ(removed_first_header->text, "b00");
    const auto removed_second_header =
        removed_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_header.has_value());
    CHECK_EQ(removed_second_header->text, "b02");
    const auto removed_second_body =
        removed_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(removed_second_body.has_value());
    CHECK_EQ(removed_second_body->text, "b12");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands mutate direct header and footer template tables") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_direct_column_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_template_table_direct_column_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_template_table_direct_column_after.docx";
    const fs::path removed =
        working_directory / "cli_template_table_direct_column_removed.docx";
    const fs::path before_output =
        working_directory / "cli_template_table_direct_column_before.json";
    const fs::path after_output =
        working_directory / "cli_template_table_direct_column_after.json";
    const fs::path remove_output =
        working_directory / "cli_template_table_direct_column_removed.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);

    create_cli_template_table_direct_column_fixture(source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    const auto before_json = read_text_file(before_output);
    CHECK_NE(before_json.find("\"command\":\"insert-template-table-column-before\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(before_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(before_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(before_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    auto before_template = reopened_before.header_template(0U);
    REQUIRE(static_cast<bool>(before_template));
    const auto before_table = before_template.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->column_count, 3U);
    REQUIRE(before_table->column_widths.size() >= 3U);
    CHECK(before_table->column_widths[0].has_value());
    CHECK(before_table->column_widths[1].has_value());
    CHECK(before_table->column_widths[2].has_value());
    CHECK_EQ(*before_table->column_widths[0], 1800U);
    CHECK_EQ(*before_table->column_widths[1], 3600U);
    CHECK_EQ(*before_table->column_widths[2], 3600U);
    const auto before_inserted_header =
        before_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(before_inserted_header.has_value());
    CHECK_EQ(before_inserted_header->text, "");
    const auto before_shifted_header =
        before_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(before_shifted_header.has_value());
    CHECK_EQ(before_shifted_header->text, "h01");
    const auto before_inserted_body =
        before_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_body.has_value());
    CHECK_EQ(before_inserted_body->text, "");

    CHECK_EQ(run_cli({"insert-template-table-column-after",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    const auto after_json = read_text_file(after_output);
    CHECK_NE(after_json.find("\"command\":\"insert-template-table-column-after\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"part\":\"footer\""), std::string::npos);
    CHECK_NE(after_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(after_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_cell_index\":1"),
             std::string::npos);
    CHECK_NE(after_json.find("\"inserted_column_index\":1"),
             std::string::npos);

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    auto after_template = reopened_after.footer_template(0U);
    REQUIRE(static_cast<bool>(after_template));
    const auto after_table = after_template.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->column_count, 3U);
    REQUIRE(after_table->column_widths.size() >= 3U);
    CHECK(after_table->column_widths[0].has_value());
    CHECK(after_table->column_widths[1].has_value());
    CHECK(after_table->column_widths[2].has_value());
    CHECK_EQ(*after_table->column_widths[0], 1600U);
    CHECK_EQ(*after_table->column_widths[1], 1600U);
    CHECK_EQ(*after_table->column_widths[2], 2800U);
    const auto after_inserted_header =
        after_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(after_inserted_header.has_value());
    CHECK_EQ(after_inserted_header->text, "");
    const auto after_shifted_header =
        after_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(after_shifted_header.has_value());
    CHECK_EQ(after_shifted_header->text, "f01");
    const auto after_inserted_body =
        after_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_body.has_value());
    CHECK_EQ(after_inserted_body->text, "");

    CHECK_EQ(run_cli({"remove-template-table-column",
                      source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "body",
                      "--output",
                      removed.string(),
                      "--json"},
                     remove_output),
             0);
    const auto remove_json = read_text_file(remove_output);
    CHECK_NE(remove_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(remove_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(remove_json.find("\"column_index\":1"), std::string::npos);

    featherdoc::Document reopened_removed(removed);
    REQUIRE_FALSE(reopened_removed.open());
    auto removed_template = reopened_removed.body_template();
    REQUIRE(static_cast<bool>(removed_template));
    const auto removed_table = removed_template.inspect_table(0U);
    REQUIRE(removed_table.has_value());
    CHECK_EQ(removed_table->column_count, 2U);
    const auto removed_first_header =
        removed_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(removed_first_header.has_value());
    CHECK_EQ(removed_first_header->text, "b00");
    const auto removed_second_header =
        removed_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(removed_second_header.has_value());
    CHECK_EQ(removed_second_header->text, "b02");
    const auto removed_second_body =
        removed_template.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(removed_second_body.has_value());
    CHECK_EQ(removed_second_body->text, "b12");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(removed);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
    remove_if_exists(remove_output);
}

TEST_CASE("cli template table column commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path parse_source =
        working_directory / "cli_template_table_column_parse_source.docx";
    const fs::path merged_source =
        working_directory / "cli_template_table_column_merge_source.docx";
    const fs::path single_column_source =
        working_directory / "cli_template_table_column_single_source.docx";
    const fs::path parse_output =
        working_directory / "cli_template_table_column_parse.json";
    const fs::path merge_error_output =
        working_directory / "cli_template_table_column_merge_error.json";
    const fs::path remove_error_output =
        working_directory / "cli_template_table_column_remove_error.json";

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_template_table_column_fixture(parse_source);

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      parse_source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"insert-template-table-column-before\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    featherdoc::Document merged_document(merged_source);
    REQUIRE_FALSE(merged_document.create_empty());
    auto &merged_header = merged_document.ensure_section_header_paragraphs(0U);
    REQUIRE(merged_header.has_next());
    REQUIRE(merged_header.set_text("Section header merged column fixture"));
    auto merged_template = merged_document.section_header_template(0U);
    REQUIRE(static_cast<bool>(merged_template));
    auto merged_table = merged_template.append_table(2U, 3U);
    configure_cli_template_table_fixture(merged_table, {1600U, 1800U, 2200U},
                                         {{"h00", "h01", "h02"},
                                          {"m00", "m01", "m02"}});
    auto merged_row = merged_table.rows();
    REQUIRE(merged_row.has_next());
    merged_row.next();
    REQUIRE(merged_row.has_next());
    auto merged_cell = merged_row.cells();
    REQUIRE(merged_cell.has_next());
    REQUIRE(merged_cell.merge_right(1U));
    REQUIRE_FALSE(merged_document.save());

    CHECK_EQ(run_cli({"insert-template-table-column-before",
                      merged_source.string(),
                      "0",
                      "0",
                      "1",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(
        merge_error_json.find("\"command\":\"insert-template-table-column-before\""),
        std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(merge_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a column before cell index '1' at row "
            "index '0' in table index '0' because the insertion boundary "
            "intersects a horizontal merge span\""),
        std::string::npos);

    featherdoc::Document single_column_document(single_column_source);
    REQUIRE_FALSE(single_column_document.create_empty());
    auto body_template = single_column_document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto body_paragraph = body_template.paragraphs();
    REQUIRE(body_paragraph.has_next());
    REQUIRE(body_paragraph.set_text("Single-column body fixture"));
    auto single_table = body_template.append_table(1U, 1U);
    configure_cli_template_table_fixture(single_table, {2400U}, {{"only"}});
    REQUIRE_FALSE(single_column_document.save());

    CHECK_EQ(run_cli({"remove-template-table-column",
                      single_column_source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "body",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-template-table-column\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(remove_error_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last column from table index '0'\""),
        std::string::npos);

    remove_if_exists(parse_source);
    remove_if_exists(merged_source);
    remove_if_exists(single_column_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

TEST_CASE("cli selector-based template table cell inspection resolves the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_cells_source.docx";
    const fs::path cell_output =
        working_directory / "cli_template_table_selector_cells_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_cells_parse.json";

    remove_if_exists(source);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "--row",
                      "1",
                      "--cell",
                      "2",
                      "--json"},
                     cell_output),
             0);
    const auto cell_json = read_text_file(cell_output);
    CHECK_NE(cell_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(cell_json.find("\"cell_index\":2"), std::string::npos);
    CHECK_NE(cell_json.find("\"text\":\"24\""), std::string::npos);

    CHECK_EQ(run_cli({"inspect-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-row",
                      "1",
                      "--row",
                      "1",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"inspect-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--header-row requires at least "
            "one --header-cell\"}\n"});

    remove_if_exists(source);
    remove_if_exists(cell_output);
    remove_if_exists(parse_output);
}

TEST_CASE(
    "cli selector-based template table merge and unmerge commands target the requested table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_selector_merge_source.docx";
    const fs::path merged =
        working_directory / "cli_template_table_selector_merged.docx";
    const fs::path merged_multiline =
        working_directory / "cli_template_table_selector_merged_multiline.docx";
    const fs::path unmerged =
        working_directory / "cli_template_table_selector_unmerged.docx";
    const fs::path unmerged_multiline =
        working_directory / "cli_template_table_selector_unmerged_multiline.docx";
    const fs::path merge_output =
        working_directory / "cli_template_table_selector_merge_output.json";
    const fs::path merge_multiline_output =
        working_directory / "cli_template_table_selector_merge_multiline_output.json";
    const fs::path unmerge_output =
        working_directory / "cli_template_table_selector_unmerge_output.json";
    const fs::path unmerge_multiline_output =
        working_directory / "cli_template_table_selector_unmerge_multiline_output.json";
    const fs::path parse_output =
        working_directory / "cli_template_table_selector_merge_parse.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(merged_multiline);
    remove_if_exists(unmerged);
    remove_if_exists(unmerged_multiline);
    remove_if_exists(merge_output);
    remove_if_exists(merge_multiline_output);
    remove_if_exists(unmerge_output);
    remove_if_exists(unmerge_multiline_output);
    remove_if_exists(parse_output);

    create_cli_template_table_selector_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(merge_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(merge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_merged(merged);
    REQUIRE_FALSE(reopened_merged.open());
    auto merged_body = reopened_merged.body_template();
    REQUIRE(static_cast<bool>(merged_body));
    const auto preserved_first_target = merged_body.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(preserved_first_target.has_value());
    CHECK_EQ(preserved_first_target->text, "North");
    CHECK_EQ(preserved_first_target->column_span, 1U);
    const auto merged_anchor = merged_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(merged_anchor.has_value());
    CHECK_EQ(merged_anchor->text, "South");
    CHECK_EQ(merged_anchor->column_span, 2U);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      merged.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "0",
                      "--text",
                      "South\npending approval",
                      "--output",
                      merged_multiline.string(),
                      "--json"},
                     merge_multiline_output),
             0);
    const auto merged_multiline_json = read_text_file(merge_multiline_output);
    CHECK_NE(merged_multiline_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(merged_multiline_json.find("\"cell_index\":0"), std::string::npos);

    featherdoc::Document reopened_merged_multiline(merged_multiline);
    REQUIRE_FALSE(reopened_merged_multiline.open());
    auto merged_multiline_body = reopened_merged_multiline.body_template();
    REQUIRE(static_cast<bool>(merged_multiline_body));
    const auto merged_multiline_anchor =
        merged_multiline_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(merged_multiline_anchor.has_value());
    CHECK_EQ(merged_multiline_anchor->text, "South\npending approval");
    CHECK_EQ(merged_multiline_anchor->column_span, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      merged.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened_unmerged(unmerged);
    REQUIRE_FALSE(reopened_unmerged.open());
    auto unmerged_body = reopened_unmerged.body_template();
    REQUIRE(static_cast<bool>(unmerged_body));
    const auto restored_anchor = unmerged_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(restored_anchor.has_value());
    CHECK_EQ(restored_anchor->text, "South");
    CHECK_EQ(restored_anchor->column_span, 1U);
    const auto restored_cell = unmerged_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");
    CHECK_EQ(restored_cell->column_span, 1U);

    CHECK_EQ(run_cli({"set-template-table-cell-text",
                      unmerged.string(),
                      "--after-text",
                      "selector target table",
                      "--header-cell",
                      "Region",
                      "--header-cell",
                      "Qty",
                      "--header-cell",
                      "Amount",
                      "--occurrence",
                      "1",
                      "1",
                      "1",
                      "--text",
                      "9 units\nhold",
                      "--output",
                      unmerged_multiline.string(),
                      "--json"},
                     unmerge_multiline_output),
             0);
    const auto unmerged_multiline_json = read_text_file(unmerge_multiline_output);
    CHECK_NE(unmerged_multiline_json.find("\"command\":\"set-template-table-cell-text\""),
             std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"table_index\":2"), std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"row_index\":1"), std::string::npos);
    CHECK_NE(unmerged_multiline_json.find("\"cell_index\":1"), std::string::npos);

    featherdoc::Document reopened_unmerged_multiline(unmerged_multiline);
    REQUIRE_FALSE(reopened_unmerged_multiline.open());
    auto unmerged_multiline_body = reopened_unmerged_multiline.body_template();
    REQUIRE(static_cast<bool>(unmerged_multiline_body));
    const auto restored_multiline_anchor =
        unmerged_multiline_body.inspect_table_cell(2U, 1U, 0U);
    REQUIRE(restored_multiline_anchor.has_value());
    CHECK_EQ(restored_multiline_anchor->text, "South");
    CHECK_EQ(restored_multiline_anchor->column_span, 1U);
    const auto restored_multiline_cell =
        unmerged_multiline_body.inspect_table_cell(2U, 1U, 1U);
    REQUIRE(restored_multiline_cell.has_value());
    CHECK_EQ(restored_multiline_cell->text, "9 units\nhold");
    CHECK_EQ(restored_multiline_cell->column_span, 1U);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "--after-text",
                      "selector target table",
                      "--direction",
                      "right",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"merge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing row index after table "
            "selector\"}\n"});

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(merged_multiline);
    remove_if_exists(unmerged);
    remove_if_exists(unmerged_multiline);
    remove_if_exists(merge_output);
    remove_if_exists(merge_multiline_output);
    remove_if_exists(unmerge_output);
    remove_if_exists(unmerge_multiline_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli template table merge and unmerge commands support bookmark selectors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_template_table_bookmark_merge_source.docx";
    const fs::path merged =
        working_directory / "cli_template_table_bookmark_merged.docx";
    const fs::path unmerged =
        working_directory / "cli_template_table_bookmark_unmerged.docx";
    const fs::path merge_output =
        working_directory / "cli_template_table_bookmark_merge.json";
    const fs::path unmerge_output =
        working_directory / "cli_template_table_bookmark_unmerge.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);

    create_cli_template_table_bookmark_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "--bookmark",
                      "target_inside_table",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     merge_output),
             0);
    const auto merge_json = read_text_file(merge_output);
    CHECK_NE(merge_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(merge_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(merge_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(merge_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(merge_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(merge_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened_merged(merged);
    REQUIRE_FALSE(reopened_merged.open());
    auto merged_body = reopened_merged.body_template();
    REQUIRE(static_cast<bool>(merged_body));
    const auto merged_cell = merged_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK_EQ(merged_cell->text, "target-00");
    CHECK_EQ(merged_cell->column_span, 2U);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      merged.string(),
                      "--bookmark",
                      "target_before_table",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     unmerge_output),
             0);
    const auto unmerge_json = read_text_file(unmerge_output);
    CHECK_NE(unmerge_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(unmerge_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(unmerge_json.find("\"table_index\":1"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(unmerge_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened_unmerged(unmerged);
    REQUIRE_FALSE(reopened_unmerged.open());
    auto unmerged_body = reopened_unmerged.body_template();
    REQUIRE(static_cast<bool>(unmerged_body));
    const auto first_cell = unmerged_body.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "target-00");
    CHECK_EQ(first_cell->column_span, 1U);
    const auto restored_cell = unmerged_body.inspect_table_cell(1U, 0U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(unmerged);
    remove_if_exists(merge_output);
    remove_if_exists(unmerge_output);
}

TEST_CASE("cli merge-template-table-cells merges template cells and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_merge_template_table_cells_source.docx";
    const fs::path merged =
        working_directory / "cli_merge_template_table_cells_output.docx";
    const fs::path success_output =
        working_directory / "cli_merge_template_table_cells_success.json";
    const fs::path parse_output =
        working_directory / "cli_merge_template_table_cells_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_merge_template_table_cells_error.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"part\":\"header\""), std::string::npos);
    CHECK_NE(success_json.find("\"part_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"entry_name\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"direction\":\"right\""), std::string::npos);
    CHECK_NE(success_json.find("\"count\":1"), std::string::npos);

    featherdoc::Document reopened(merged);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.header_template(0U);
    REQUIRE(static_cast<bool>(header_template));
    const auto merged_cell = header_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK_EQ(merged_cell->text, "hA");
    CHECK_EQ(merged_cell->column_span, 2U);

    const auto remaining_cell = header_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(remaining_cell.has_value());
    CHECK_EQ(remaining_cell->text, "hC");
    CHECK_EQ(remaining_cell->column_index, 2U);

    const auto header_xml = read_docx_entry(merged, "word/header1.xml");
    pugi::xml_document header_doc;
    REQUIRE(header_doc.load_string(header_xml.c_str()));

    const auto table_node = header_doc.child("w:hdr").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});

    std::size_t cell_count = 0U;
    for (auto cell = first_row.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++cell_count;
    }
    CHECK_EQ(cell_count, 2U);

    const auto grid_span = first_row.child("w:tc").child("w:tcPr").child("w:gridSpan");
    REQUIRE(grid_span != pugi::xml_node{});
    CHECK_EQ(std::string_view{grid_span.attribute("w:val").value()}, "2");

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "section-header",
                      "--direction",
                      "right",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"merge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"merge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--direction",
                      "right",
                      "--count",
                      "5",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"merge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_error_json.find("\"entry\":\"word/header1.xml\""),
             std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '0', and cell "
            "index '0' could not be merged towards 'right' with count '5'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli unmerge-template-table-cells splits template merges and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_unmerge_template_table_cells_source.docx";
    const fs::path unmerged =
        working_directory / "cli_unmerge_template_table_cells_output.docx";
    const fs::path direct_footer_unmerged =
        working_directory /
        "cli_unmerge_template_table_cells_direct_footer_output.docx";
    const fs::path success_output =
        working_directory / "cli_unmerge_template_table_cells_success.json";
    const fs::path direct_footer_success_output =
        working_directory /
        "cli_unmerge_template_table_cells_direct_footer_success.json";
    const fs::path footer_success_output =
        working_directory / "cli_unmerge_template_table_cells_footer_success.json";
    const fs::path parse_output =
        working_directory / "cli_unmerge_template_table_cells_parse.json";
    const fs::path mutate_output =
        working_directory / "cli_unmerge_template_table_cells_error.json";

    remove_if_exists(source);
    remove_if_exists(unmerged);
    remove_if_exists(direct_footer_unmerged);
    remove_if_exists(success_output);
    remove_if_exists(direct_footer_success_output);
    remove_if_exists(footer_success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_template_table_merge_unmerge_fixture(source);

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--part",
                      "body",
                      "--direction",
                      "right",
                      "--output",
                      unmerged.string(),
                      "--json"},
                     success_output),
             0);
    const auto success_json = read_text_file(success_output);
    CHECK_NE(success_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(success_json.find("\"entry_name\":\"word/document.xml\""),
             std::string::npos);
    CHECK_NE(success_json.find("\"table_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"row_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"cell_index\":0"), std::string::npos);
    CHECK_NE(success_json.find("\"direction\":\"right\""), std::string::npos);

    featherdoc::Document reopened(unmerged);
    REQUIRE_FALSE(reopened.open());
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto first_cell = body_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "A");
    CHECK_EQ(first_cell->column_span, 1U);
    const auto restored_cell = body_template.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");
    CHECK_EQ(restored_cell->column_span, 1U);
    const auto last_cell = body_template.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(last_cell.has_value());
    CHECK_EQ(last_cell->text, "C");

    const auto document_xml = read_docx_entry(unmerged, "word/document.xml");
    pugi::xml_document document_xml_doc;
    REQUIRE(document_xml_doc.load_string(document_xml.c_str()));

    const auto body_table_node = document_xml_doc.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(body_table_node != pugi::xml_node{});
    const auto body_row_node = body_table_node.child("w:tr");
    REQUIRE(body_row_node != pugi::xml_node{});

    std::size_t row_cell_count = 0U;
    for (auto cell = body_row_node.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++row_cell_count;
    }
    CHECK_EQ(row_cell_count, 3U);
    CHECK(body_row_node.child("w:tc").child("w:tcPr").child("w:gridSpan") ==
          pugi::xml_node{});

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "1",
                      "1",
                      "0",
                      "--part",
                      "footer",
                      "--index",
                      "0",
                      "--direction",
                      "down",
                      "--output",
                      direct_footer_unmerged.string(),
                      "--json"},
                     direct_footer_success_output),
             0);
    const auto direct_footer_success_json =
        read_text_file(direct_footer_success_output);
    CHECK_NE(direct_footer_success_json.find(
                 "\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"part\":\"footer\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"part_index\":0"),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"table_index\":1"),
             std::string::npos);
    CHECK_NE(direct_footer_success_json.find("\"direction\":\"down\""),
             std::string::npos);

    featherdoc::Document direct_footer_reopened(direct_footer_unmerged);
    REQUIRE_FALSE(direct_footer_reopened.open());
    auto direct_footer_template = direct_footer_reopened.footer_template(0U);
    REQUIRE(static_cast<bool>(direct_footer_template));
    const auto direct_footer_top =
        direct_footer_template.inspect_table_cell(1U, 0U, 0U);
    REQUIRE(direct_footer_top.has_value());
    CHECK_EQ(direct_footer_top->text, "dfA");
    const auto direct_footer_middle =
        direct_footer_template.inspect_table_cell(1U, 1U, 0U);
    REQUIRE(direct_footer_middle.has_value());
    CHECK_EQ(direct_footer_middle->text, "");
    const auto direct_footer_bottom =
        direct_footer_template.inspect_table_cell(1U, 2U, 0U);
    REQUIRE(direct_footer_bottom.has_value());
    CHECK_EQ(direct_footer_bottom->text, "");

    const auto direct_footer_xml =
        read_docx_entry(direct_footer_unmerged, "word/footer1.xml");
    pugi::xml_document direct_footer_doc;
    REQUIRE(direct_footer_doc.load_string(direct_footer_xml.c_str()));

    auto direct_footer_table_node =
        direct_footer_doc.child("w:ftr").child("w:tbl");
    REQUIRE(direct_footer_table_node != pugi::xml_node{});
    direct_footer_table_node = direct_footer_table_node.next_sibling("w:tbl");
    REQUIRE(direct_footer_table_node != pugi::xml_node{});
    for (auto direct_footer_row = direct_footer_table_node.child("w:tr");
         direct_footer_row != pugi::xml_node{};
         direct_footer_row = direct_footer_row.next_sibling("w:tr")) {
        const auto cell = direct_footer_row.child("w:tc");
        REQUIRE(cell != pugi::xml_node{});
        CHECK(cell.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    }

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--direction",
                      "down",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"unmerge-template-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     footer_success_output),
             0);
    const auto footer_success_json = read_text_file(footer_success_output);
    CHECK_NE(footer_success_json.find("\"part\":\"section-footer\""),
             std::string::npos);
    CHECK_NE(footer_success_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(footer_success_json.find("\"entry_name\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(footer_success_json.find("\"direction\":\"down\""),
             std::string::npos);

    featherdoc::Document footer_reopened(source);
    REQUIRE_FALSE(footer_reopened.open());
    auto footer_template = footer_reopened.section_footer_template(0U);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_top = footer_template.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(footer_top.has_value());
    CHECK_EQ(footer_top->text, "fA");
    const auto footer_middle = footer_template.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(footer_middle.has_value());
    CHECK_EQ(footer_middle->text, "");
    const auto footer_bottom = footer_template.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(footer_bottom.has_value());
    CHECK_EQ(footer_bottom->text, "");

    const auto footer_xml = read_docx_entry(source, "word/footer1.xml");
    pugi::xml_document footer_doc;
    REQUIRE(footer_doc.load_string(footer_xml.c_str()));

    const auto footer_table_node = footer_doc.child("w:ftr").child("w:tbl");
    REQUIRE(footer_table_node != pugi::xml_node{});
    auto footer_row = footer_table_node.child("w:tr");
    for (; footer_row != pugi::xml_node{}; footer_row = footer_row.next_sibling("w:tr")) {
        const auto cell = footer_row.child("w:tc");
        REQUIRE(cell != pugi::xml_node{});
        CHECK(cell.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    }

    CHECK_EQ(run_cli({"unmerge-template-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--part",
                      "section-footer",
                      "--section",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"unmerge-template-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_error_json.find("\"entry\":\"word/footer1.xml\""),
             std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '1', and cell "
            "index '0' could not be unmerged towards 'down'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(unmerged);
    remove_if_exists(direct_footer_unmerged);
    remove_if_exists(success_output);
    remove_if_exists(direct_footer_success_output);
    remove_if_exists(footer_success_output);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli merge-table-cells merges horizontally and reports json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_merge_table_cells_right_source.docx";
    const fs::path merged =
        working_directory / "cli_merge_table_cells_right_output.docx";
    const fs::path mutate_output =
        working_directory / "cli_merge_table_cells_right_output.json";

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(mutate_output);

    create_cli_table_merge_fixture(source);

    CHECK_EQ(run_cli({"merge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      merged.string(),
                      "--json"},
                     mutate_output),
             0);
    CHECK_EQ(
        read_text_file(mutate_output),
        std::string{
            "{\"command\":\"merge-table-cells\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"direction\":\"right\",\"count\":1}\n"});

    featherdoc::Document reopened(merged);
    REQUIRE_FALSE(reopened.open());

    const auto merged_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(merged_cell.has_value());
    CHECK_EQ(merged_cell->text, "A");
    CHECK_EQ(merged_cell->column_span, 2U);
    CHECK_EQ(merged_cell->column_index, 0U);

    const auto remaining_cell = reopened.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(remaining_cell.has_value());
    CHECK_EQ(remaining_cell->text, "C");
    CHECK_EQ(remaining_cell->column_index, 2U);

    const auto merged_table = reopened.inspect_table(0U);
    REQUIRE(merged_table.has_value());
    CHECK_EQ(merged_table->text, "A\tC\nD\tE\tF\nG\tH\tI");

    const auto document_xml = read_docx_entry(merged, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});

    std::size_t first_row_cell_count = 0U;
    for (auto cell = first_row.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++first_row_cell_count;
    }
    CHECK_EQ(first_row_cell_count, 2U);

    const auto first_cell = first_row.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    const auto grid_span = first_cell.child("w:tcPr").child("w:gridSpan");
    REQUIRE(grid_span != pugi::xml_node{});
    CHECK_EQ(std::string_view{grid_span.attribute("w:val").value()}, "2");

    remove_if_exists(source);
    remove_if_exists(merged);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli merge-table-cells merges down and reports parse/mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_merge_table_cells_down_source.docx";
    const fs::path parse_output =
        working_directory / "cli_merge_table_cells_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_merge_table_cells_error_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_merge_fixture(source);

    CHECK_EQ(run_cli({"merge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--direction",
                      "down",
                      "--count",
                      "2"}),
             0);

    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open());

    const auto anchor_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(anchor_cell.has_value());
    CHECK_EQ(anchor_cell->text, "A");

    const auto second_row_cell = reopened.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(second_row_cell.has_value());
    CHECK_EQ(second_row_cell->text, "");

    const auto third_row_cell = reopened.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(third_row_cell.has_value());
    CHECK_EQ(third_row_cell->text, "");

    const auto document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    const auto third_row = second_row.next_sibling("w:tr");
    REQUIRE(third_row != pugi::xml_node{});

    const auto first_cell = first_row.child("w:tc");
    const auto second_cell = second_row.child("w:tc");
    const auto third_cell = third_row.child("w:tc");
    REQUIRE(first_cell != pugi::xml_node{});
    REQUIRE(second_cell != pugi::xml_node{});
    REQUIRE(third_cell != pugi::xml_node{});

    CHECK_EQ(std::string_view{
                 first_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "restart");
    CHECK_EQ(std::string_view{
                 second_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");
    CHECK_EQ(std::string_view{
                 third_cell.child("w:tcPr").child("w:vMerge").attribute("w:val").value()},
             "continue");

    CHECK_EQ(run_cli({"merge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"merge-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing required --direction "
            "<right|down>\"}\n"});

    CHECK_EQ(run_cli({"merge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--count",
                      "5",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"merge-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '0', and cell "
            "index '0' could not be merged towards 'right' with count '5'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli unmerge-table-cells splits a horizontal merge and reports json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_unmerge_table_cells_right_source.docx";
    const fs::path updated =
        working_directory / "cli_unmerge_table_cells_right_output.docx";
    const fs::path mutate_output =
        working_directory / "cli_unmerge_table_cells_right_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);

    create_cli_table_unmerge_right_fixture(source);

    CHECK_EQ(run_cli({"unmerge-table-cells",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "--direction",
                      "right",
                      "--output",
                      updated.string(),
                      "--json"},
                     mutate_output),
             0);
    CHECK_EQ(
        read_text_file(mutate_output),
        std::string{
            "{\"command\":\"unmerge-table-cells\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"direction\":\"right\"}\n"});

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());

    const auto first_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "A");
    CHECK_EQ(first_cell->column_span, 1U);

    const auto restored_cell = reopened.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(restored_cell.has_value());
    CHECK_EQ(restored_cell->text, "");
    CHECK_EQ(restored_cell->column_span, 1U);

    const auto last_cell = reopened.inspect_table_cell(0U, 0U, 2U);
    REQUIRE(last_cell.has_value());
    CHECK_EQ(last_cell->text, "C");

    const auto document_xml = read_docx_entry(updated, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto row_node = table_node.child("w:tr");
    REQUIRE(row_node != pugi::xml_node{});

    std::size_t row_cell_count = 0U;
    for (auto cell = row_node.child("w:tc"); cell != pugi::xml_node{};
         cell = cell.next_sibling("w:tc")) {
        ++row_cell_count;
    }
    CHECK_EQ(row_cell_count, 3U);

    const auto first_cell_node = row_node.child("w:tc");
    REQUIRE(first_cell_node != pugi::xml_node{});
    CHECK(first_cell_node.child("w:tcPr").child("w:gridSpan") == pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli unmerge-table-cells splits a vertical merge and reports errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_unmerge_table_cells_down_source.docx";
    const fs::path parse_output =
        working_directory / "cli_unmerge_table_cells_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_unmerge_table_cells_error_output.json";
    const fs::path success_output =
        working_directory / "cli_unmerge_table_cells_success_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(success_output);

    create_cli_table_unmerge_down_fixture(source);

    CHECK_EQ(run_cli({"unmerge-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"unmerge-table-cells\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"missing required --direction "
            "<right|down>\"}\n"});

    CHECK_EQ(run_cli({"unmerge-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     success_output),
             0);
    CHECK_EQ(
        read_text_file(success_output),
        std::string{
            "{\"command\":\"unmerge-table-cells\",\"ok\":true,"
            "\"in_place\":true,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":1,\"cell_index\":0,"
            "\"direction\":\"down\"}\n"});

    featherdoc::Document reopened(source);
    REQUIRE_FALSE(reopened.open());

    const auto first_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "A");

    const auto second_cell = reopened.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(second_cell.has_value());
    CHECK_EQ(second_cell->text, "");

    const auto third_cell = reopened.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(third_cell.has_value());
    CHECK_EQ(third_cell->text, "");

    const auto document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));

    const auto table_node = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table_node != pugi::xml_node{});
    const auto first_row = table_node.child("w:tr");
    REQUIRE(first_row != pugi::xml_node{});
    const auto second_row = first_row.next_sibling("w:tr");
    REQUIRE(second_row != pugi::xml_node{});
    const auto third_row = second_row.next_sibling("w:tr");
    REQUIRE(third_row != pugi::xml_node{});

    const auto first_cell_node = first_row.child("w:tc");
    const auto second_cell_node = second_row.child("w:tc");
    const auto third_cell_node = third_row.child("w:tc");
    REQUIRE(first_cell_node != pugi::xml_node{});
    REQUIRE(second_cell_node != pugi::xml_node{});
    REQUIRE(third_cell_node != pugi::xml_node{});

    CHECK(first_cell_node.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    CHECK(second_cell_node.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});
    CHECK(third_cell_node.child("w:tcPr").child("w:vMerge") == pugi::xml_node{});

    CHECK_EQ(run_cli({"unmerge-table-cells",
                      source.string(),
                      "0",
                      "1",
                      "0",
                      "--direction",
                      "down",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_error_json = read_text_file(mutate_output);
    CHECK_NE(mutate_error_json.find("\"command\":\"unmerge-table-cells\""),
             std::string::npos);
    CHECK_NE(mutate_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_error_json.find(
            "\"detail\":\"cell at table index '0', row index '1', and cell "
            "index '0' could not be unmerged towards 'down'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
    remove_if_exists(success_output);
}

TEST_CASE("cli table cell fill commands set and clear body cell fill") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_table_cell_fill_source.docx";
    const fs::path styled = working_directory / "cli_table_cell_fill_styled.docx";
    const fs::path cleared = working_directory / "cli_table_cell_fill_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_fill_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_fill_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-fill",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "A1B2C3",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-fill\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"fill_color\":\"A1B2C3\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:shd") == pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_cell_node =
        styled_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(styled_cell_node != pugi::xml_node{});
    const auto shading = styled_cell_node.child("w:tcPr").child("w:shd");
    REQUIRE(shading != pugi::xml_node{});
    CHECK_EQ(std::string_view{shading.attribute("w:val").value()}, "clear");
    CHECK_EQ(std::string_view{shading.attribute("w:color").value()}, "auto");
    CHECK_EQ(std::string_view{shading.attribute("w:fill").value()}, "A1B2C3");

    featherdoc::Document reopened(styled);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.fill_color().has_value());
    CHECK_EQ(*reopened_cell.fill_color(), "A1B2C3");

    CHECK_EQ(run_cli({"clear-table-cell-fill",
                      styled.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-fill\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:shd") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.fill_color().has_value());

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell vertical alignment commands set and clear body cell alignment") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_vertical_alignment_source.docx";
    const fs::path aligned =
        working_directory / "cli_table_cell_vertical_alignment_aligned.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_vertical_alignment_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_vertical_alignment_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_vertical_alignment_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-vertical-alignment",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "center",
                      "--output",
                      aligned.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-vertical-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"vertical_alignment\":\"center\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:vAlign") == pugi::xml_node{});

    const auto aligned_document_xml = read_docx_entry(aligned, "word/document.xml");
    pugi::xml_document aligned_xml_document;
    REQUIRE(aligned_xml_document.load_string(aligned_document_xml.c_str()));
    const auto aligned_cell_node =
        aligned_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(aligned_cell_node != pugi::xml_node{});
    const auto vertical_alignment = aligned_cell_node.child("w:tcPr").child("w:vAlign");
    REQUIRE(vertical_alignment != pugi::xml_node{});
    CHECK_EQ(std::string_view{vertical_alignment.attribute("w:val").value()},
             "center");

    featherdoc::Document reopened(aligned);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.vertical_alignment().has_value());
    CHECK_EQ(*reopened_cell.vertical_alignment(),
             featherdoc::cell_vertical_alignment::center);

    CHECK_EQ(run_cli({"clear-table-cell-vertical-alignment",
                      aligned.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-vertical-alignment\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:vAlign") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.vertical_alignment().has_value());

    remove_if_exists(source);
    remove_if_exists(aligned);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell text direction commands set and clear body cell direction") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_text_direction_source.docx";
    const fs::path directed =
        working_directory / "cli_table_cell_text_direction_directed.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_text_direction_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_text_direction_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_text_direction_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(directed);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-text-direction",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "top_to_bottom_right_to_left",
                      "--output",
                      directed.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-text-direction\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"text_direction\":\"top_to_bottom_right_to_left\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_cell_node =
        source_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(source_cell_node != pugi::xml_node{});
    CHECK(source_cell_node.child("w:tcPr").child("w:textDirection") ==
          pugi::xml_node{});

    const auto directed_document_xml = read_docx_entry(directed, "word/document.xml");
    pugi::xml_document directed_xml_document;
    REQUIRE(directed_xml_document.load_string(directed_document_xml.c_str()));
    const auto directed_cell_node =
        directed_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(directed_cell_node != pugi::xml_node{});
    const auto text_direction =
        directed_cell_node.child("w:tcPr").child("w:textDirection");
    REQUIRE(text_direction != pugi::xml_node{});
    CHECK_EQ(std::string_view{text_direction.attribute("w:val").value()}, "tbRl");

    featherdoc::Document reopened(directed);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.text_direction().has_value());
    CHECK_EQ(*reopened_cell.text_direction(),
             featherdoc::cell_text_direction::top_to_bottom_right_to_left);

    CHECK_EQ(run_cli({"clear-table-cell-text-direction",
                      directed.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-text-direction\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:textDirection") ==
          pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.text_direction().has_value());

    remove_if_exists(source);
    remove_if_exists(directed);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell width commands set and clear body cell width") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_width_source.docx";
    const fs::path sized =
        working_directory / "cli_table_cell_width_sized.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_width_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_width_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_width_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-width",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "2222",
                      "--output",
                      sized.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"width_twips\":2222}\n"});

    const auto sized_document_xml = read_docx_entry(sized, "word/document.xml");
    pugi::xml_document sized_xml_document;
    REQUIRE(sized_xml_document.load_string(sized_document_xml.c_str()));
    const auto sized_cell_node =
        sized_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(sized_cell_node != pugi::xml_node{});
    const auto width_node = sized_cell_node.child("w:tcPr").child("w:tcW");
    REQUIRE(width_node != pugi::xml_node{});
    CHECK_EQ(std::string_view{width_node.attribute("w:type").value()}, "dxa");
    CHECK_EQ(std::string_view{width_node.attribute("w:w").value()}, "2222");

    featherdoc::Document reopened(sized);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.width_twips().has_value());
    CHECK_EQ(*reopened_cell.width_twips(), 2222U);

    CHECK_EQ(run_cli({"clear-table-cell-width",
                      sized.string(),
                      "0",
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-width\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcW") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.width_twips().has_value());

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell margin commands set and clear body cell margins") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_margin_source.docx";
    const fs::path margined =
        working_directory / "cli_table_cell_margin_margined.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_margin_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_margin_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_margin_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-margin",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "left",
                      "240",
                      "--output",
                      margined.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"left\",\"margin_twips\":240}\n"});

    const auto margined_document_xml =
        read_docx_entry(margined, "word/document.xml");
    pugi::xml_document margined_xml_document;
    REQUIRE(margined_xml_document.load_string(margined_document_xml.c_str()));
    const auto margined_cell_node =
        margined_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(margined_cell_node != pugi::xml_node{});
    const auto left_margin =
        margined_cell_node.child("w:tcPr").child("w:tcMar").child("w:left");
    REQUIRE(left_margin != pugi::xml_node{});
    CHECK_EQ(std::string_view{left_margin.attribute("w:w").value()}, "240");
    CHECK_EQ(std::string_view{left_margin.attribute("w:type").value()}, "dxa");

    featherdoc::Document reopened(margined);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());
    REQUIRE(reopened_cell.margin_twips(featherdoc::cell_margin_edge::left).has_value());
    CHECK_EQ(*reopened_cell.margin_twips(featherdoc::cell_margin_edge::left),
             240U);

    CHECK_EQ(run_cli({"clear-table-cell-margin",
                      margined.string(),
                      "0",
                      "0",
                      "0",
                      "left",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-margin\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"left\"}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcMar") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());
    CHECK_FALSE(reopened_cleared_cell.margin_twips(
        featherdoc::cell_margin_edge::left).has_value());

    remove_if_exists(source);
    remove_if_exists(margined);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell border commands set and clear body cell borders") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_border_source.docx";
    const fs::path bordered =
        working_directory / "cli_table_cell_border_bordered.docx";
    const fs::path cleared =
        working_directory / "cli_table_cell_border_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_cell_border_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_cell_border_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(bordered);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-border",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--style",
                      "dotted",
                      "--size",
                      "6",
                      "--color",
                      "0000FF",
                      "--space",
                      "1",
                      "--output",
                      bordered.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-cell-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"right\",\"style\":\"dotted\","
            "\"size_eighth_points\":6,\"color\":\"0000FF\","
            "\"space_points\":1}\n"});

    const auto bordered_document_xml =
        read_docx_entry(bordered, "word/document.xml");
    pugi::xml_document bordered_xml_document;
    REQUIRE(bordered_xml_document.load_string(bordered_document_xml.c_str()));
    const auto bordered_cell_node =
        bordered_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(bordered_cell_node != pugi::xml_node{});
    const auto right_border =
        bordered_cell_node.child("w:tcPr").child("w:tcBorders").child("w:right");
    REQUIRE(right_border != pugi::xml_node{});
    CHECK_EQ(std::string_view{right_border.attribute("w:val").value()}, "dotted");
    CHECK_EQ(std::string_view{right_border.attribute("w:sz").value()}, "6");
    CHECK_EQ(std::string_view{right_border.attribute("w:space").value()}, "1");
    CHECK_EQ(std::string_view{right_border.attribute("w:color").value()}, "0000FF");

    featherdoc::Document reopened(bordered);
    REQUIRE_FALSE(reopened.open());
    auto reopened_cell = reopened.tables().rows().cells();
    REQUIRE(reopened_cell.has_next());

    CHECK_EQ(run_cli({"clear-table-cell-border",
                      bordered.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-cell-border\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cell_index\":0,"
            "\"edge\":\"right\"}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_cell_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr").child("w:tc");
    REQUIRE(cleared_cell_node != pugi::xml_node{});
    CHECK(cleared_cell_node.child("w:tcPr").child("w:tcBorders").child("w:right") ==
          pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_cell = reopened_cleared.tables().rows().cells();
    REQUIRE(reopened_cleared_cell.has_next());

    remove_if_exists(source);
    remove_if_exists(bordered);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table cell style and geometry commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_cell_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_table_cell_style_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_table_cell_style_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_cell_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-cell-vertical-alignment",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "middle",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-cell-vertical-alignment\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid vertical alignment: "
            "middle\"}\n"});

    CHECK_EQ(run_cli({"set-table-cell-border",
                      source.string(),
                      "0",
                      "0",
                      "0",
                      "right",
                      "--style",
                      "groove",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-cell-border\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid border style: "
            "groove\"}\n"});

    CHECK_EQ(run_cli({"set-table-cell-text-direction",
                      source.string(),
                      "0",
                      "0",
                      "9",
                      "top_to_bottom_right_to_left",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"set-table-cell-text-direction\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '0' in "
            "table index '0'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-table-cell-width",
                      source.string(),
                      "0",
                      "0",
                      "9",
                      "2400",
                      "--json"},
                     mutate_output),
             1);
    const auto width_mutate_json = read_text_file(mutate_output);
    CHECK_NE(width_mutate_json.find("\"command\":\"set-table-cell-width\""),
             std::string::npos);
    CHECK_NE(width_mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        width_mutate_json.find(
            "\"detail\":\"cell index '9' is out of range for row index '0' in "
            "table index '0'\""),
        std::string::npos);
    CHECK_NE(width_mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli table row height commands set and clear body row height") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_height_source.docx";
    const fs::path sized =
        working_directory / "cli_table_row_height_sized.docx";
    const fs::path cleared =
        working_directory / "cli_table_row_height_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_row_height_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_row_height_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_row_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-row-height",
                      source.string(),
                      "0",
                      "0",
                      "360",
                      "exact",
                      "--output",
                      sized.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-row-height\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"height_twips\":360,"
            "\"height_rule\":\"exact\"}\n"});

    const auto sized_document_xml = read_docx_entry(sized, "word/document.xml");
    pugi::xml_document sized_xml_document;
    REQUIRE(sized_xml_document.load_string(sized_document_xml.c_str()));
    const auto sized_row_node =
        sized_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(sized_row_node != pugi::xml_node{});
    const auto row_height = sized_row_node.child("w:trPr").child("w:trHeight");
    REQUIRE(row_height != pugi::xml_node{});
    CHECK_EQ(std::string_view{row_height.attribute("w:val").value()}, "360");
    CHECK_EQ(std::string_view{row_height.attribute("w:hRule").value()}, "exact");

    featherdoc::Document reopened(sized);
    REQUIRE_FALSE(reopened.open());
    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    REQUIRE(reopened_row.height_twips().has_value());
    CHECK_EQ(*reopened_row.height_twips(), 360U);
    REQUIRE(reopened_row.height_rule().has_value());
    CHECK_EQ(*reopened_row.height_rule(), featherdoc::row_height_rule::exact);

    CHECK_EQ(run_cli({"clear-table-row-height",
                      sized.string(),
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-row-height\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_row_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(cleared_row_node != pugi::xml_node{});
    CHECK(cleared_row_node.child("w:trPr") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_row = reopened_cleared.tables().rows();
    REQUIRE(reopened_cleared_row.has_next());
    CHECK_FALSE(reopened_cleared_row.height_twips().has_value());
    CHECK_FALSE(reopened_cleared_row.height_rule().has_value());

    remove_if_exists(source);
    remove_if_exists(sized);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table row cant-split commands set and clear body row cant-split") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_cant_split_source.docx";
    const fs::path locked =
        working_directory / "cli_table_row_cant_split_locked.docx";
    const fs::path cleared =
        working_directory / "cli_table_row_cant_split_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_row_cant_split_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_row_cant_split_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(locked);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_row_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-row-cant-split",
                      source.string(),
                      "0",
                      "0",
                      "--output",
                      locked.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-row-cant-split\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"cant_split\":true}\n"});

    const auto locked_document_xml = read_docx_entry(locked, "word/document.xml");
    pugi::xml_document locked_xml_document;
    REQUIRE(locked_xml_document.load_string(locked_document_xml.c_str()));
    const auto locked_row_node =
        locked_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(locked_row_node != pugi::xml_node{});
    const auto cant_split = locked_row_node.child("w:trPr").child("w:cantSplit");
    REQUIRE(cant_split != pugi::xml_node{});
    CHECK_EQ(std::string_view{cant_split.attribute("w:val").value()}, "1");

    featherdoc::Document reopened(locked);
    REQUIRE_FALSE(reopened.open());
    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.cant_split());

    CHECK_EQ(run_cli({"clear-table-row-cant-split",
                      locked.string(),
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-row-cant-split\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_row_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(cleared_row_node != pugi::xml_node{});
    CHECK(cleared_row_node.child("w:trPr") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_row = reopened_cleared.tables().rows();
    REQUIRE(reopened_cleared_row.has_next());
    CHECK_FALSE(reopened_cleared_row.cant_split());

    remove_if_exists(source);
    remove_if_exists(locked);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table row repeat-header commands set and clear body row repeat-header") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_repeat_header_source.docx";
    const fs::path repeated =
        working_directory / "cli_table_row_repeat_header_repeated.docx";
    const fs::path cleared =
        working_directory / "cli_table_row_repeat_header_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_table_row_repeat_header_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_table_row_repeat_header_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(repeated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_table_row_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-row-repeat-header",
                      source.string(),
                      "0",
                      "0",
                      "--output",
                      repeated.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-table-row-repeat-header\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"repeats_header\":true}\n"});

    const auto repeated_document_xml =
        read_docx_entry(repeated, "word/document.xml");
    pugi::xml_document repeated_xml_document;
    REQUIRE(repeated_xml_document.load_string(repeated_document_xml.c_str()));
    const auto repeated_row_node =
        repeated_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(repeated_row_node != pugi::xml_node{});
    const auto table_header =
        repeated_row_node.child("w:trPr").child("w:tblHeader");
    REQUIRE(table_header != pugi::xml_node{});
    CHECK_EQ(std::string_view{table_header.attribute("w:val").value()}, "1");

    featherdoc::Document reopened(repeated);
    REQUIRE_FALSE(reopened.open());
    auto reopened_row = reopened.tables().rows();
    REQUIRE(reopened_row.has_next());
    CHECK(reopened_row.repeats_header());

    CHECK_EQ(run_cli({"clear-table-row-repeat-header",
                      repeated.string(),
                      "0",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-table-row-repeat-header\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_row_node =
        cleared_xml_document.child("w:document").child("w:body").child("w:tbl").child("w:tr");
    REQUIRE(cleared_row_node != pugi::xml_node{});
    CHECK(cleared_row_node.child("w:trPr") == pugi::xml_node{});

    featherdoc::Document reopened_cleared(cleared);
    REQUIRE_FALSE(reopened_cleared.open());
    auto reopened_cleared_row = reopened_cleared.tables().rows();
    REQUIRE(reopened_cleared_row.has_next());
    CHECK_FALSE(reopened_cleared_row.repeats_header());

    remove_if_exists(source);
    remove_if_exists(repeated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli table row commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_table_row_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_table_row_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_table_row_style_fixture(source);

    CHECK_EQ(run_cli({"set-table-row-height",
                      source.string(),
                      "0",
                      "0",
                      "360",
                      "minimum",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-table-row-height\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid row height rule: "
            "minimum\"}\n"});

    CHECK_EQ(run_cli({"clear-table-row-cant-split",
                      source.string(),
                      "0",
                      "9",
                      "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-table-row-cant-split\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"row index '9' is out of range for table index '0'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli append-table-row appends a body row") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_table_row_source.docx";
    const fs::path appended =
        working_directory / "cli_append_table_row_appended.docx";
    const fs::path output =
        working_directory / "cli_append_table_row_output.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-table-row",
                      source.string(),
                      "0",
                      "--output",
                      appended.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-table-row\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":2,\"cell_count\":2}\n"});

    featherdoc::Document reopened(appended);
    REQUIRE_FALSE(reopened.open());

    const auto table = reopened.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 3U);
    CHECK_EQ(table->column_count, 2U);

    const auto first_appended_cell = reopened.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(first_appended_cell.has_value());
    CHECK_EQ(first_appended_cell->text, "");

    const auto second_appended_cell = reopened.inspect_table_cell(0U, 2U, 1U);
    REQUIRE(second_appended_cell.has_value());
    CHECK_EQ(second_appended_cell->text, "");

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(output);
}

TEST_CASE("cli insert-table-row-before and insert-table-row-after commands insert body rows") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_insert_table_row_source.docx";
    const fs::path inserted_before =
        working_directory / "cli_insert_table_row_before.docx";
    const fs::path inserted_after =
        working_directory / "cli_insert_table_row_after.docx";
    const fs::path before_output =
        working_directory / "cli_insert_table_row_before_output.json";
    const fs::path after_output =
        working_directory / "cli_insert_table_row_after_output.json";

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(before_output);
    remove_if_exists(after_output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"insert-table-row-before",
                      source.string(),
                      "0",
                      "1",
                      "--output",
                      inserted_before.string(),
                      "--json"},
                     before_output),
             0);
    CHECK_EQ(
        read_text_file(before_output),
        std::string{
            "{\"command\":\"insert-table-row-before\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":1,\"inserted_row_index\":1}\n"});

    featherdoc::Document reopened_before(inserted_before);
    REQUIRE_FALSE(reopened_before.open());
    const auto before_table = reopened_before.inspect_table(0U);
    REQUIRE(before_table.has_value());
    CHECK_EQ(before_table->row_count, 3U);
    CHECK_EQ(before_table->column_count, 2U);

    const auto before_inserted_first_cell =
        reopened_before.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(before_inserted_first_cell.has_value());
    CHECK_EQ(before_inserted_first_cell->text, "");
    const auto before_inserted_second_cell =
        reopened_before.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(before_inserted_second_cell.has_value());
    CHECK_EQ(before_inserted_second_cell->text, "");
    const auto before_shifted_first_cell =
        reopened_before.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(before_shifted_first_cell.has_value());
    CHECK_EQ(before_shifted_first_cell->text, "r1c0");

    CHECK_EQ(run_cli({"insert-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--output",
                      inserted_after.string(),
                      "--json"},
                     after_output),
             0);
    CHECK_EQ(
        read_text_file(after_output),
        std::string{
            "{\"command\":\"insert-table-row-after\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0,\"inserted_row_index\":1}\n"});

    featherdoc::Document reopened_after(inserted_after);
    REQUIRE_FALSE(reopened_after.open());
    const auto after_table = reopened_after.inspect_table(0U);
    REQUIRE(after_table.has_value());
    CHECK_EQ(after_table->row_count, 3U);
    CHECK_EQ(after_table->column_count, 2U);

    const auto after_inserted_first_cell =
        reopened_after.inspect_table_cell(0U, 1U, 0U);
    REQUIRE(after_inserted_first_cell.has_value());
    CHECK_EQ(after_inserted_first_cell->text, "");
    const auto after_inserted_second_cell =
        reopened_after.inspect_table_cell(0U, 1U, 1U);
    REQUIRE(after_inserted_second_cell.has_value());
    CHECK_EQ(after_inserted_second_cell->text, "");
    const auto after_shifted_first_cell =
        reopened_after.inspect_table_cell(0U, 2U, 0U);
    REQUIRE(after_shifted_first_cell.has_value());
    CHECK_EQ(after_shifted_first_cell->text, "r1c0");

    remove_if_exists(source);
    remove_if_exists(inserted_before);
    remove_if_exists(inserted_after);
    remove_if_exists(before_output);
    remove_if_exists(after_output);
}

TEST_CASE("cli remove-table-row removes a body row") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_remove_table_row_source.docx";
    const fs::path removed =
        working_directory / "cli_remove_table_row_removed.docx";
    const fs::path output =
        working_directory / "cli_remove_table_row_output.json";

    remove_if_exists(source);
    remove_if_exists(removed);
    remove_if_exists(output);

    create_cli_table_inspection_fixture(source);

    CHECK_EQ(run_cli({"remove-table-row",
                      source.string(),
                      "0",
                      "0",
                      "--output",
                      removed.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"remove-table-row\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"table_index\":0,\"row_index\":0}\n"});

    featherdoc::Document reopened(removed);
    REQUIRE_FALSE(reopened.open());

    const auto table = reopened.inspect_table(0U);
    REQUIRE(table.has_value());
    CHECK_EQ(table->row_count, 1U);
    CHECK_EQ(table->column_count, 2U);

    const auto first_cell = reopened.inspect_table_cell(0U, 0U, 0U);
    REQUIRE(first_cell.has_value());
    CHECK_EQ(first_cell->text, "r1c0");

    const auto second_cell = reopened.inspect_table_cell(0U, 0U, 1U);
    REQUIRE(second_cell.has_value());
    CHECK_EQ(second_cell->text, "r1c1");

    remove_if_exists(source);
    remove_if_exists(removed);
    remove_if_exists(output);
}

TEST_CASE("cli table row structure commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_table_row_structure_error_source.docx";
    const fs::path single_row_source =
        working_directory / "cli_table_row_structure_single_row_source.docx";
    const fs::path parse_output =
        working_directory / "cli_table_row_structure_parse_output.json";
    const fs::path merge_error_output =
        working_directory / "cli_table_row_structure_merge_error_output.json";
    const fs::path remove_error_output =
        working_directory / "cli_table_row_structure_remove_error_output.json";

    remove_if_exists(source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);

    create_cli_table_unmerge_down_fixture(source);

    CHECK_EQ(run_cli({"append-table-row",
                      source.string(),
                      "0",
                      "--cell-count",
                      "0",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"append-table-row\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"cell count must be greater than "
            "0\"}\n"});

    CHECK_EQ(run_cli({"insert-table-row-after",
                      source.string(),
                      "0",
                      "0",
                      "--json"},
                     merge_error_output),
             1);
    const auto merge_error_json = read_text_file(merge_error_output);
    CHECK_NE(merge_error_json.find("\"command\":\"insert-table-row-after\""),
             std::string::npos);
    CHECK_NE(merge_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        merge_error_json.find(
            "\"detail\":\"cannot insert a row adjacent to row index '0' in "
            "table index '0' because the row participates in a vertical "
            "merge\""),
        std::string::npos);

    featherdoc::Document single_row_document(single_row_source);
    REQUIRE_FALSE(single_row_document.create_empty());
    auto single_row_table = single_row_document.append_table(1, 1);
    REQUIRE(single_row_table.has_next());
    auto single_row = single_row_table.rows();
    REQUIRE(single_row.has_next());
    auto only_cell = single_row.cells();
    REQUIRE(only_cell.has_next());
    REQUIRE(only_cell.set_text("only-row"));
    REQUIRE_FALSE(single_row_document.save());

    CHECK_EQ(run_cli({"remove-table-row",
                      single_row_source.string(),
                      "0",
                      "0",
                      "--json"},
                     remove_error_output),
             1);
    const auto remove_error_json = read_text_file(remove_error_output);
    CHECK_NE(remove_error_json.find("\"command\":\"remove-table-row\""),
             std::string::npos);
    CHECK_NE(remove_error_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        remove_error_json.find(
            "\"detail\":\"cannot remove the last row from table index '0'\""),
        std::string::npos);

    remove_if_exists(source);
    remove_if_exists(single_row_source);
    remove_if_exists(parse_output);
    remove_if_exists(merge_error_output);
    remove_if_exists(remove_error_output);
}

TEST_CASE("cli paragraph style commands set and clear body paragraph styles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_style_source.docx";
    const fs::path styled =
        working_directory / "cli_paragraph_style_styled.docx";
    const fs::path cleared =
        working_directory / "cli_paragraph_style_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_paragraph_style_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_paragraph_style_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_inspect_paragraphs_fixture(source);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.open());
    const auto section0_body_index =
        find_body_paragraph_index_by_text(source_document, "section 0 body");
    const auto section1_body_index =
        find_body_paragraph_index_by_text(source_document, "section 1 body");

    CHECK_EQ(run_cli({"set-paragraph-style",
                      source.string(),
                      std::to_string(section1_body_index),
                      "CustomBody",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-paragraph-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":3,\"headers\":2,\"footers\":1,"
            "\"paragraph_index\":"} +
            std::to_string(section1_body_index) +
            ",\"style_id\":\"CustomBody\"}\n");

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_section1_paragraph =
        find_body_paragraph_xml_node(source_xml_document.child("w:document"),
                                     section1_body_index);
    REQUIRE(source_section1_paragraph != pugi::xml_node{});
    CHECK(source_section1_paragraph.child("w:pPr").child("w:pStyle") ==
          pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_section1_paragraph =
        find_body_paragraph_xml_node(styled_xml_document.child("w:document"),
                                     section1_body_index);
    REQUIRE(styled_section1_paragraph != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{styled_section1_paragraph.child("w:pPr")
                             .child("w:pStyle")
                             .attribute("w:val")
                             .value()},
        "CustomBody");

    CHECK_EQ(run_cli({"clear-paragraph-style",
                      styled.string(),
                      std::to_string(section0_body_index),
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-paragraph-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":3,\"headers\":2,\"footers\":1,"
            "\"paragraph_index\":"} +
            std::to_string(section0_body_index) + "}\n");

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_section0_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"),
                                     section0_body_index);
    REQUIRE(cleared_section0_paragraph != pugi::xml_node{});
    CHECK(cleared_section0_paragraph.child("w:pPr").child("w:pStyle") ==
          pugi::xml_node{});
    const auto cleared_section1_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"),
                                     section1_body_index);
    REQUIRE(cleared_section1_paragraph != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{cleared_section1_paragraph.child("w:pPr")
                             .child("w:pStyle")
                             .attribute("w:val")
                             .value()},
        "CustomBody");

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli paragraph style commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_paragraph_style_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_paragraph_style_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_inspect_paragraphs_fixture(source);

    CHECK_EQ(run_cli({"set-paragraph-style",
                      source.string(),
                      "oops",
                      "CustomBody",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-paragraph-style\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid paragraph index: "
            "oops\"}\n"});

    CHECK_EQ(run_cli({"clear-paragraph-style", source.string(), "999", "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-paragraph-style\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(mutate_json.find("\"detail\":\"paragraph index '999' is out of range\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli run style commands set and clear body run styles") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_style_source.docx";
    const fs::path styled =
        working_directory / "cli_run_style_styled.docx";
    const fs::path cleared =
        working_directory / "cli_run_style_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_run_style_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_run_style_clear_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-style",
                      source.string(),
                      "1",
                      "0",
                      "Strong",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-run-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0,\"style_id\":\"Strong\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_paragraph =
        find_body_paragraph_xml_node(source_xml_document.child("w:document"), 1U);
    REQUIRE(source_paragraph != pugi::xml_node{});
    const auto source_first_run = find_run_xml_node(source_paragraph, 0U);
    REQUIRE(source_first_run != pugi::xml_node{});
    CHECK(source_first_run.child("w:rPr").child("w:rStyle") == pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_paragraph =
        find_body_paragraph_xml_node(styled_xml_document.child("w:document"), 1U);
    REQUIRE(styled_paragraph != pugi::xml_node{});
    const auto styled_first_run = find_run_xml_node(styled_paragraph, 0U);
    REQUIRE(styled_first_run != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{styled_first_run.child("w:rPr")
                             .child("w:rStyle")
                             .attribute("w:val")
                             .value()},
        "Strong");

    CHECK_EQ(run_cli({"clear-run-style",
                      styled.string(),
                      "1",
                      "1",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-run-style\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":1}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 1U);
    REQUIRE(cleared_paragraph != pugi::xml_node{});
    const auto cleared_first_run = find_run_xml_node(cleared_paragraph, 0U);
    REQUIRE(cleared_first_run != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{cleared_first_run.child("w:rPr")
                             .child("w:rStyle")
                             .attribute("w:val")
                             .value()},
        "Strong");
    const auto cleared_second_run = find_run_xml_node(cleared_paragraph, 1U);
    REQUIRE(cleared_second_run != pugi::xml_node{});
    CHECK(cleared_second_run.child("w:rPr").child("w:rStyle") == pugi::xml_node{});

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
}

TEST_CASE("cli run style commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_style_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_run_style_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_run_style_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-style",
                      source.string(),
                      "1",
                      "oops",
                      "Strong",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-run-style\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid run index: oops\"}\n"});

    CHECK_EQ(run_cli({"clear-run-style", source.string(), "1", "99", "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-run-style\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"run index '99' is out of range for paragraph index '1'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli run font family commands set and clear body run font families") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_font_family_source.docx";
    const fs::path styled =
        working_directory / "cli_run_font_family_styled.docx";
    const fs::path cleared =
        working_directory / "cli_run_font_family_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_run_font_family_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_run_font_family_clear_output.json";
    const fs::path inspect_set_output =
        working_directory / "cli_run_font_family_inspect_set_output.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_run_font_family_inspect_cleared_output.json";

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_set_output);
    remove_if_exists(inspect_cleared_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-font-family",
                      source.string(),
                      "1",
                      "0",
                      "Courier New",
                      "--output",
                      styled.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-run-font-family\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0,\"font_family\":\"Courier New\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_paragraph =
        find_body_paragraph_xml_node(source_xml_document.child("w:document"), 1U);
    REQUIRE(source_paragraph != pugi::xml_node{});
    const auto source_first_run = find_run_xml_node(source_paragraph, 0U);
    REQUIRE(source_first_run != pugi::xml_node{});
    CHECK(source_first_run.child("w:rPr").child("w:rFonts") == pugi::xml_node{});

    const auto styled_document_xml = read_docx_entry(styled, "word/document.xml");
    pugi::xml_document styled_xml_document;
    REQUIRE(styled_xml_document.load_string(styled_document_xml.c_str()));
    const auto styled_paragraph =
        find_body_paragraph_xml_node(styled_xml_document.child("w:document"), 1U);
    REQUIRE(styled_paragraph != pugi::xml_node{});
    const auto styled_first_run = find_run_xml_node(styled_paragraph, 0U);
    REQUIRE(styled_first_run != pugi::xml_node{});
    const auto styled_fonts = styled_first_run.child("w:rPr").child("w:rFonts");
    REQUIRE(styled_fonts != pugi::xml_node{});
    CHECK_EQ(std::string_view{styled_fonts.attribute("w:ascii").value()},
             "Courier New");
    CHECK_EQ(std::string_view{styled_fonts.attribute("w:hAnsi").value()},
             "Courier New");
    CHECK_EQ(std::string_view{styled_fonts.attribute("w:cs").value()},
             "Courier New");

    CHECK_EQ(run_cli({"inspect-runs",
                      styled.string(),
                      "1",
                      "--run",
                      "0",
                      "--json"},
                     inspect_set_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_set_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":0,\"style_id\":null,"
            "\"font_family\":\"Courier New\",\"east_asia_font_family\":null,"
            "\"language\":null,\"text\":\"beta\"}}\n"});

    CHECK_EQ(run_cli({"clear-run-font-family",
                      styled.string(),
                      "1",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-run-font-family\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 1U);
    REQUIRE(cleared_paragraph != pugi::xml_node{});
    const auto cleared_first_run = find_run_xml_node(cleared_paragraph, 0U);
    REQUIRE(cleared_first_run != pugi::xml_node{});
    CHECK(cleared_first_run.child("w:rPr") == pugi::xml_node{});
    const auto cleared_second_run = find_run_xml_node(cleared_paragraph, 1U);
    REQUIRE(cleared_second_run != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{cleared_second_run.child("w:rPr")
                             .child("w:rStyle")
                             .attribute("w:val")
                             .value()},
        "Strong");

    CHECK_EQ(run_cli({"inspect-runs",
                      cleared.string(),
                      "1",
                      "--run",
                      "0",
                      "--json"},
                     inspect_cleared_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_cleared_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":0,\"style_id\":null,"
            "\"font_family\":null,\"east_asia_font_family\":null,"
            "\"language\":null,\"text\":\"beta\"}}\n"});

    remove_if_exists(source);
    remove_if_exists(styled);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_set_output);
    remove_if_exists(inspect_cleared_output);
}

TEST_CASE("cli run font family commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_font_family_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_run_font_family_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_run_font_family_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-font-family",
                      source.string(),
                      "1",
                      "oops",
                      "Courier New",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-run-font-family\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid run index: oops\"}\n"});

    CHECK_EQ(run_cli({"clear-run-font-family", source.string(), "1", "99", "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-run-font-family\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"run index '99' is out of range for paragraph index '1'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli run language commands set and clear body run language") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_language_source.docx";
    const fs::path updated =
        working_directory / "cli_run_language_updated.docx";
    const fs::path cleared =
        working_directory / "cli_run_language_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_run_language_set_output.json";
    const fs::path clear_output =
        working_directory / "cli_run_language_clear_output.json";
    const fs::path inspect_set_output =
        working_directory / "cli_run_language_inspect_set_output.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_run_language_inspect_cleared_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_set_output);
    remove_if_exists(inspect_cleared_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-language",
                      source.string(),
                      "1",
                      "0",
                      "ja-JP",
                      "--output",
                      updated.string(),
                      "--json"},
                     set_output),
             0);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-run-language\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0,\"language\":\"ja-JP\"}\n"});

    const auto source_document_xml = read_docx_entry(source, "word/document.xml");
    pugi::xml_document source_xml_document;
    REQUIRE(source_xml_document.load_string(source_document_xml.c_str()));
    const auto source_paragraph =
        find_body_paragraph_xml_node(source_xml_document.child("w:document"), 1U);
    REQUIRE(source_paragraph != pugi::xml_node{});
    const auto source_first_run = find_run_xml_node(source_paragraph, 0U);
    REQUIRE(source_first_run != pugi::xml_node{});
    CHECK(source_first_run.child("w:rPr").child("w:lang") == pugi::xml_node{});

    const auto updated_document_xml = read_docx_entry(updated, "word/document.xml");
    pugi::xml_document updated_xml_document;
    REQUIRE(updated_xml_document.load_string(updated_document_xml.c_str()));
    const auto updated_paragraph =
        find_body_paragraph_xml_node(updated_xml_document.child("w:document"), 1U);
    REQUIRE(updated_paragraph != pugi::xml_node{});
    const auto updated_first_run = find_run_xml_node(updated_paragraph, 0U);
    REQUIRE(updated_first_run != pugi::xml_node{});
    const auto updated_language = updated_first_run.child("w:rPr").child("w:lang");
    REQUIRE(updated_language != pugi::xml_node{});
    CHECK_EQ(std::string_view{updated_language.attribute("w:val").value()},
             "ja-JP");

    CHECK_EQ(run_cli({"inspect-runs",
                      updated.string(),
                      "1",
                      "--run",
                      "0",
                      "--json"},
                     inspect_set_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_set_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":0,\"style_id\":null,"
            "\"font_family\":null,\"east_asia_font_family\":null,"
            "\"language\":\"ja-JP\",\"text\":\"beta\"}}\n"});

    CHECK_EQ(run_cli({"clear-run-language",
                      updated.string(),
                      "1",
                      "0",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-run-language\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":1,\"footers\":1,"
            "\"paragraph_index\":1,\"run_index\":0}\n"});

    const auto cleared_document_xml = read_docx_entry(cleared, "word/document.xml");
    pugi::xml_document cleared_xml_document;
    REQUIRE(cleared_xml_document.load_string(cleared_document_xml.c_str()));
    const auto cleared_paragraph =
        find_body_paragraph_xml_node(cleared_xml_document.child("w:document"), 1U);
    REQUIRE(cleared_paragraph != pugi::xml_node{});
    const auto cleared_first_run = find_run_xml_node(cleared_paragraph, 0U);
    REQUIRE(cleared_first_run != pugi::xml_node{});
    CHECK(cleared_first_run.child("w:rPr") == pugi::xml_node{});
    const auto cleared_second_run = find_run_xml_node(cleared_paragraph, 1U);
    REQUIRE(cleared_second_run != pugi::xml_node{});
    CHECK_EQ(
        std::string_view{cleared_second_run.child("w:rPr")
                             .child("w:rStyle")
                             .attribute("w:val")
                             .value()},
        "Strong");

    CHECK_EQ(run_cli({"inspect-runs",
                      cleared.string(),
                      "1",
                      "--run",
                      "0",
                      "--json"},
                     inspect_cleared_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_cleared_output),
        std::string{
            "{\"paragraph_index\":1,\"run\":{\"index\":0,\"style_id\":null,"
            "\"font_family\":null,\"east_asia_font_family\":null,"
            "\"language\":null,\"text\":\"beta\"}}\n"});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_set_output);
    remove_if_exists(inspect_cleared_output);
}

TEST_CASE("cli run language commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_run_language_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_run_language_parse_output.json";
    const fs::path mutate_output =
        working_directory / "cli_run_language_mutate_output.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);

    create_cli_style_usage_fixture(source);

    CHECK_EQ(run_cli({"set-run-language",
                      source.string(),
                      "1",
                      "oops",
                      "ja-JP",
                      "--json"},
                     parse_output),
             2);
    CHECK_EQ(
        read_text_file(parse_output),
        std::string{
            "{\"command\":\"set-run-language\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid run index: oops\"}\n"});

    CHECK_EQ(run_cli({"clear-run-language", source.string(), "1", "99", "--json"},
                     mutate_output),
             1);
    const auto mutate_json = read_text_file(mutate_output);
    CHECK_NE(mutate_json.find("\"command\":\"clear-run-language\""),
             std::string::npos);
    CHECK_NE(mutate_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(
        mutate_json.find(
            "\"detail\":\"run index '99' is out of range for paragraph index '1'\""),
        std::string::npos);
    CHECK_NE(mutate_json.find("\"entry\":\"word/document.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(mutate_output);
}

TEST_CASE("cli default run properties commands set inspect and clear docDefaults") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_default_run_properties_source.docx";
    const fs::path updated =
        working_directory / "cli_default_run_properties_updated.docx";
    const fs::path cleared =
        working_directory / "cli_default_run_properties_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_default_run_properties_set.json";
    const fs::path inspect_updated_output =
        working_directory / "cli_default_run_properties_inspect_updated.json";
    const fs::path clear_output =
        working_directory / "cli_default_run_properties_clear.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_default_run_properties_inspect_cleared.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_updated_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);

    create_cli_empty_document_fixture(source);

    CHECK_EQ(run_cli({"set-default-run-properties",
                      source.string(),
                      "--font-family",
                      "Segoe UI",
                      "--east-asia-font-family",
                      "Microsoft YaHei",
                      "--language",
                      "en-US",
                      "--east-asia-language",
                      "zh-CN",
                      "--bidi-language",
                      "ar-SA",
                      "--rtl",
                      "true",
                      "--paragraph-bidi",
                      "true",
                      "--output",
                      updated.string(),
                      "--json"},
                     set_output),
             0);
    const auto set_json = read_text_file(set_output);
    CHECK_NE(set_json.find("\"command\":\"set-default-run-properties\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"font_family\":\"Segoe UI\""), std::string::npos);
    CHECK_NE(set_json.find("\"east_asia_font_family\":\"Microsoft YaHei\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"language\":\"en-US\""), std::string::npos);
    CHECK_NE(set_json.find("\"east_asia_language\":\"zh-CN\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"bidi_language\":\"ar-SA\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"rtl\":true"), std::string::npos);
    CHECK_NE(set_json.find("\"paragraph_bidi\":true"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-default-run-properties", updated.string(), "--json"},
                     inspect_updated_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_updated_output),
        std::string{
            "{\"default_run_properties\":{\"font_family\":\"Segoe UI\","
            "\"east_asia_font_family\":\"Microsoft YaHei\","
            "\"language\":\"en-US\",\"east_asia_language\":\"zh-CN\","
            "\"bidi_language\":\"ar-SA\",\"rtl\":true,"
            "\"paragraph_bidi\":true}}\n"});

    CHECK_EQ(run_cli({"clear-default-run-properties",
                      updated.string(),
                      "--east-asia-font-family",
                      "--primary-language",
                      "--rtl",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    const auto clear_json = read_text_file(clear_output);
    CHECK_NE(clear_json.find("\"command\":\"clear-default-run-properties\""),
             std::string::npos);
    CHECK_NE(clear_json.find("\"cleared\":[\"east_asia_font_family\","
                             "\"primary_language\",\"rtl\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-default-run-properties", cleared.string(), "--json"},
                     inspect_cleared_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_cleared_output),
        std::string{
            "{\"default_run_properties\":{\"font_family\":\"Segoe UI\","
            "\"east_asia_font_family\":null,\"language\":null,"
            "\"east_asia_language\":\"zh-CN\",\"bidi_language\":\"ar-SA\","
            "\"rtl\":null,\"paragraph_bidi\":true}}\n"});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_updated_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);
}

TEST_CASE("cli default run properties commands report parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_default_run_properties_error_source.docx";
    const fs::path set_output =
        working_directory / "cli_default_run_properties_set_error.json";
    const fs::path clear_output =
        working_directory / "cli_default_run_properties_clear_error.json";
    const fs::path inspect_output =
        working_directory / "cli_default_run_properties_inspect_error.json";

    remove_if_exists(source);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);

    create_cli_empty_document_fixture(source);

    CHECK_EQ(run_cli({"set-default-run-properties",
                      source.string(),
                      "--rtl",
                      "maybe",
                      "--json"},
                     set_output),
             2);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-default-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid --rtl value: maybe\"}\n"});

    CHECK_EQ(run_cli({"clear-default-run-properties", source.string(), "--json"},
                     clear_output),
             2);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-default-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"clear-default-run-properties "
            "requires at least one clear option\"}\n"});

    CHECK_EQ(run_cli({"inspect-default-run-properties",
                      source.string(),
                      "--bogus",
                      "--json"},
                     inspect_output),
             2);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"command\":\"inspect-default-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"unknown option: --bogus\"}\n"});

    remove_if_exists(source);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli style run properties commands set inspect and clear style metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_run_properties_source.docx";
    const fs::path updated =
        working_directory / "cli_style_run_properties_updated.docx";
    const fs::path cleared =
        working_directory / "cli_style_run_properties_cleared.docx";
    const fs::path set_output =
        working_directory / "cli_style_run_properties_set.json";
    const fs::path inspect_updated_output =
        working_directory / "cli_style_run_properties_inspect_updated.json";
    const fs::path clear_output =
        working_directory / "cli_style_run_properties_clear.json";
    const fs::path inspect_cleared_output =
        working_directory / "cli_style_run_properties_inspect_cleared.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_updated_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      source.string(),
                      "Normal",
                      "--font-family",
                      "Segoe UI",
                      "--east-asia-font-family",
                      "Microsoft YaHei",
                      "--language",
                      "en-US",
                      "--east-asia-language",
                      "zh-CN",
                      "--bidi-language",
                      "ar-SA",
                      "--rtl",
                      "true",
                      "--paragraph-bidi",
                      "true",
                      "--output",
                      updated.string(),
                      "--json"},
                     set_output),
             0);
    const auto set_json = read_text_file(set_output);
    CHECK_NE(set_json.find("\"command\":\"set-style-run-properties\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"style_id\":\"Normal\""), std::string::npos);
    CHECK_NE(set_json.find("\"font_family\":\"Segoe UI\""), std::string::npos);
    CHECK_NE(set_json.find("\"east_asia_font_family\":\"Microsoft YaHei\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"language\":\"en-US\""), std::string::npos);
    CHECK_NE(set_json.find("\"east_asia_language\":\"zh-CN\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"bidi_language\":\"ar-SA\""),
             std::string::npos);
    CHECK_NE(set_json.find("\"rtl\":true"), std::string::npos);
    CHECK_NE(set_json.find("\"paragraph_bidi\":true"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-style-run-properties",
                      updated.string(),
                      "Normal",
                      "--json"},
                     inspect_updated_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_updated_output),
        std::string{
            "{\"style_id\":\"Normal\",\"style_run_properties\":{"
            "\"font_family\":\"Segoe UI\","
            "\"east_asia_font_family\":\"Microsoft YaHei\","
            "\"language\":\"en-US\",\"east_asia_language\":\"zh-CN\","
            "\"bidi_language\":\"ar-SA\",\"rtl\":true,"
            "\"paragraph_bidi\":true}}\n"});

    const auto styles_xml = read_docx_entry(updated, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(styles_xml.c_str()));
    const auto normal_style =
        find_style_xml_node(styles_document.child("w:styles"), "Normal");
    REQUIRE(normal_style != pugi::xml_node{});
    const auto run_properties = normal_style.child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    const auto fonts = run_properties.child("w:rFonts");
    REQUIRE(fonts != pugi::xml_node{});
    CHECK_EQ(std::string_view{fonts.attribute("w:ascii").value()}, "Segoe UI");
    CHECK_EQ(std::string_view{fonts.attribute("w:eastAsia").value()},
             "Microsoft YaHei");
    const auto language = run_properties.child("w:lang");
    REQUIRE(language != pugi::xml_node{});
    CHECK_EQ(std::string_view{language.attribute("w:val").value()}, "en-US");
    CHECK_EQ(std::string_view{language.attribute("w:eastAsia").value()}, "zh-CN");
    CHECK_EQ(std::string_view{language.attribute("w:bidi").value()}, "ar-SA");
    CHECK(run_properties.child("w:rtl") != pugi::xml_node{});
    CHECK(normal_style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    CHECK_EQ(run_cli({"clear-style-run-properties",
                      updated.string(),
                      "Normal",
                      "--east-asia-font-family",
                      "--primary-language",
                      "--rtl",
                      "--paragraph-bidi",
                      "--output",
                      cleared.string(),
                      "--json"},
                     clear_output),
             0);
    const auto clear_json = read_text_file(clear_output);
    CHECK_NE(clear_json.find("\"command\":\"clear-style-run-properties\""),
             std::string::npos);
    CHECK_NE(clear_json.find("\"style_id\":\"Normal\""), std::string::npos);
    CHECK_NE(clear_json.find("\"cleared\":[\"east_asia_font_family\","
                             "\"primary_language\",\"rtl\",\"paragraph_bidi\"]"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-style-run-properties",
                      cleared.string(),
                      "Normal",
                      "--json"},
                     inspect_cleared_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_cleared_output),
        std::string{
            "{\"style_id\":\"Normal\",\"style_run_properties\":{"
            "\"font_family\":\"Segoe UI\","
            "\"east_asia_font_family\":null,"
            "\"language\":null,\"east_asia_language\":\"zh-CN\","
            "\"bidi_language\":\"ar-SA\",\"rtl\":null,"
            "\"paragraph_bidi\":null}}\n"});

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(cleared);
    remove_if_exists(set_output);
    remove_if_exists(inspect_updated_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_cleared_output);
}

TEST_CASE("cli style run properties commands report parse and mutate errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_style_run_properties_error_source.docx";
    const fs::path set_output =
        working_directory / "cli_style_run_properties_set_error.json";
    const fs::path clear_output =
        working_directory / "cli_style_run_properties_clear_error.json";
    const fs::path inspect_output =
        working_directory / "cli_style_run_properties_inspect_error.json";

    remove_if_exists(source);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-style-run-properties",
                      source.string(),
                      "Normal",
                      "--rtl",
                      "maybe",
                      "--json"},
                     set_output),
             2);
    CHECK_EQ(
        read_text_file(set_output),
        std::string{
            "{\"command\":\"set-style-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"invalid --rtl value: maybe\"}\n"});

    CHECK_EQ(run_cli({"clear-style-run-properties",
                      source.string(),
                      "Normal",
                      "--json"},
                     clear_output),
             2);
    CHECK_EQ(
        read_text_file(clear_output),
        std::string{
            "{\"command\":\"clear-style-run-properties\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"clear-style-run-properties "
            "requires at least one clear option\"}\n"});

    CHECK_EQ(run_cli({"inspect-style-run-properties",
                      source.string(),
                      "MissingStyle",
                      "--json"},
                     inspect_output),
             1);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find("\"command\":\"inspect-style-run-properties\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"stage\":\"inspect\""), std::string::npos);
    CHECK_NE(inspect_json.find(
                 "\"detail\":\"style id 'MissingStyle' was not found in "
                 "word/styles.xml\""),
             std::string::npos);
    CHECK_NE(inspect_json.find("\"entry\":\"word/styles.xml\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(set_output);
    remove_if_exists(clear_output);
    remove_if_exists(inspect_output);
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
    CHECK_NE(json.find("\"z_order\":72"), std::string::npos);
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
    CHECK_NE(text.find("z_order: 84\n"), std::string::npos);
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
                 "--z-order",
                 "96",
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
    CHECK_NE(json.find("\"z_order\":96"), std::string::npos);
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
    CHECK_EQ(images[0].floating_options->z_order, 96U);
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

TEST_CASE("cli replace-bookmark-paragraphs replaces a body bookmark with paragraphs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_paragraphs_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_paragraphs_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_paragraphs.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      source.string(),
                      "body_logo",
                      "--paragraph",
                      "Alpha",
                      "--paragraph",
                      "Beta",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-paragraphs\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"paragraph_count\":2"), std::string::npos);
    CHECK_NE(json.find("\"paragraphs\":[\"Alpha\",\"Beta\"]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\nAlpha\nBeta\nafter\n"});
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("body_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-paragraphs requires at least one paragraph") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_paragraphs_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      "missing.docx",
                      "body_logo",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-bookmark-paragraphs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected at least one "
            "--paragraph <text> or --paragraph-file <path>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-paragraphs reads UTF-8 paragraphs from files") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_paragraphs_file_source.docx";
    const fs::path paragraph_one =
        working_directory / "cli_replace_bookmark_paragraphs_file_one.txt";
    const fs::path paragraph_two =
        working_directory / "cli_replace_bookmark_paragraphs_file_two.txt";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_paragraphs_file_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_paragraphs_file.json";

    remove_if_exists(source);
    remove_if_exists(paragraph_one);
    remove_if_exists(paragraph_two);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);
    write_binary_file(paragraph_one, std::string{"第一行：项目范围确认"});
    write_binary_file(paragraph_two, std::string{"第二行：交付物复核"});

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      source.string(),
                      "body_logo",
                      "--paragraph-file",
                      paragraph_one.string(),
                      "--paragraph-file",
                      paragraph_two.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-paragraphs\""),
             std::string::npos);
    CHECK_NE(json.find("\"paragraph_count\":2"), std::string::npos);
    CHECK_NE(
        json.find("\"paragraphs\":[\"第一行：项目范围确认\",\"第二行：交付物复核\"]"),
        std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\n第一行：项目范围确认\n第二行：交付物复核\nafter\n"});

    remove_if_exists(source);
    remove_if_exists(paragraph_one);
    remove_if_exists(paragraph_two);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table replaces a body bookmark with a generated table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_table_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_table_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_table.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-table",
                      source.string(),
                      "body_logo",
                      "--row",
                      "Name",
                      "--cell",
                      "Qty",
                      "--row",
                      "Apple",
                      "--cell",
                      "2",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-table\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(json.find("\"rows\":[[\"Name\",\"Qty\"],[\"Apple\",\"2\"]]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\nafter\n"});
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].text, "Name\tQty\nApple\t2");
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("body_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table rejects cell data before a row") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_table_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-table",
                      "missing.docx",
                      "body_logo",
                      "--cell",
                      "Qty",
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{"{\"command\":\"replace-bookmark-table\",\"ok\":false,"
                         "\"stage\":\"parse\",\"message\":\"--cell requires "
                         "--row\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table-rows expands a template row inside a body table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_table_rows_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_table_rows_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_table_rows.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_table_rows_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-table-rows",
                      source.string(),
                      "item_row",
                      "--row",
                      "Apple",
                      "--cell",
                      "2",
                      "--row",
                      "Pear",
                      "--cell",
                      "5",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-table-rows\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"item_row\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(json.find("\"rows\":[[\"Apple\",\"2\"],[\"Pear\",\"5\"]]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].text, "Name\tQty\nApple\t2\nPear\t5\nTotal\t7");
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("item_row").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table-rows reads UTF-8 rows from files") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_table_rows_file_source.docx";
    const fs::path row_one_name =
        working_directory / "cli_replace_bookmark_table_rows_file_one_name.txt";
    const fs::path row_one_qty =
        working_directory / "cli_replace_bookmark_table_rows_file_one_qty.txt";
    const fs::path row_two_name =
        working_directory / "cli_replace_bookmark_table_rows_file_two_name.txt";
    const fs::path row_two_qty =
        working_directory / "cli_replace_bookmark_table_rows_file_two_qty.txt";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_table_rows_file_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_table_rows_file.json";

    remove_if_exists(source);
    remove_if_exists(row_one_name);
    remove_if_exists(row_one_qty);
    remove_if_exists(row_two_name);
    remove_if_exists(row_two_qty);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_table_rows_fixture(source);
    write_binary_file(row_one_name, std::string{"苹果"});
    write_binary_file(row_one_qty, std::string{"2"});
    write_binary_file(row_two_name, std::string{"梨"});
    write_binary_file(row_two_qty, std::string{"5"});

    CHECK_EQ(run_cli({"replace-bookmark-table-rows",
                      source.string(),
                      "item_row",
                      "--row-file",
                      row_one_name.string(),
                      "--cell-file",
                      row_one_qty.string(),
                      "--row-file",
                      row_two_name.string(),
                      "--cell-file",
                      row_two_qty.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-table-rows\""),
             std::string::npos);
    CHECK_NE(json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(json.find("\"rows\":[[\"苹果\",\"2\"],[\"梨\",\"5\"]]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].text, "Name\tQty\n苹果\t2\n梨\t5\nTotal\t7");

    remove_if_exists(source);
    remove_if_exists(row_one_name);
    remove_if_exists(row_one_qty);
    remove_if_exists(row_two_name);
    remove_if_exists(row_two_qty);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table-rows supports empty replacement lists") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_table_rows_empty_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_table_rows_empty_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_table_rows_empty.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_table_rows_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-table-rows",
                      source.string(),
                      "item_row",
                      "--empty",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-table-rows\""),
             std::string::npos);
    CHECK_NE(json.find("\"row_count\":0"), std::string::npos);
    CHECK_NE(json.find("\"rows\":[]"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].text, "Name\tQty\nTotal\t7");
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("item_row").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli remove-bookmark-block removes a section header placeholder paragraph") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_remove_bookmark_block_source.docx";
    const fs::path updated =
        working_directory / "cli_remove_bookmark_block_updated.docx";
    const fs::path output =
        working_directory / "cli_remove_bookmark_block.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"remove-bookmark-block",
                      source.string(),
                      "header_logo",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"remove-bookmark-block\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"removed\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_part_lines(header_template.paragraphs()),
             std::vector<std::string>{"header before"});
    CHECK_FALSE(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE(
    "cli replace-bookmark-paragraphs expands standalone bookmarks in body and "
    "section header") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_paragraphs_source.docx";
    const fs::path body_updated =
        working_directory / "cli_replace_bookmark_paragraphs_body_updated.docx";
    const fs::path header_updated =
        working_directory / "cli_replace_bookmark_paragraphs_header_updated.docx";
    const fs::path body_output =
        working_directory / "cli_replace_bookmark_paragraphs_body.json";
    const fs::path header_output =
        working_directory / "cli_replace_bookmark_paragraphs_header.json";

    remove_if_exists(source);
    remove_if_exists(body_updated);
    remove_if_exists(header_updated);
    remove_if_exists(body_output);
    remove_if_exists(header_output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      source.string(),
                      "body_logo",
                      "--part",
                      "body",
                      "--paragraph",
                      "body line 1",
                      "--paragraph",
                      "body line 2",
                      "--output",
                      body_updated.string(),
                      "--json"},
                     body_output),
             0);

    const auto body_json = read_text_file(body_output);
    CHECK_NE(body_json.find("\"command\":\"replace-bookmark-paragraphs\""),
             std::string::npos);
    CHECK_NE(body_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(body_json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(body_json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(body_json.find("\"paragraph_count\":2"), std::string::npos);
    CHECK_NE(body_json.find("\"paragraphs\":[\"body line 1\",\"body line 2\"]"),
             std::string::npos);

    featherdoc::Document reopened_body(body_updated);
    REQUIRE_FALSE(reopened_body.open());
    auto updated_body_template = reopened_body.body_template();
    REQUIRE(static_cast<bool>(updated_body_template));
    CHECK_EQ(collect_part_lines(updated_body_template.paragraphs()),
             std::vector<std::string>{"before", "body line 1", "body line 2",
                                      "after"});
    CHECK_FALSE(updated_body_template.find_bookmark("body_logo").has_value());

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      source.string(),
                      "header_logo",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--paragraph",
                      "header line 1",
                      "--paragraph",
                      "header line 2",
                      "--output",
                      header_updated.string(),
                      "--json"},
                     header_output),
             0);

    const auto header_json = read_text_file(header_output);
    CHECK_NE(header_json.find("\"command\":\"replace-bookmark-paragraphs\""),
             std::string::npos);
    CHECK_NE(header_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(header_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(header_json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(header_json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(header_json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(header_json.find("\"paragraph_count\":2"), std::string::npos);
    CHECK_NE(
        header_json.find("\"paragraphs\":[\"header line 1\",\"header line 2\"]"),
        std::string::npos);

    featherdoc::Document reopened_header(header_updated);
    REQUIRE_FALSE(reopened_header.open());
    auto updated_header_template = reopened_header.section_header_template(0);
    REQUIRE(static_cast<bool>(updated_header_template));
    CHECK_EQ(collect_part_lines(updated_header_template.paragraphs()),
             std::vector<std::string>{"header before", "header line 1",
                                      "header line 2"});
    CHECK_FALSE(updated_header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(body_updated);
    remove_if_exists(header_updated);
    remove_if_exists(body_output);
    remove_if_exists(header_output);
}

TEST_CASE("cli set-bookmark-block-visibility removes a body block") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_bookmark_block_visibility_source.docx";
    const fs::path updated =
        working_directory / "cli_set_bookmark_block_visibility_updated.docx";
    const fs::path output =
        working_directory / "cli_set_bookmark_block_visibility.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_block_visibility_fixture(source);

    CHECK_EQ(run_cli({"set-bookmark-block-visibility",
                      source.string(),
                      "hide_block",
                      "--visible",
                      "false",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"set-bookmark-block-visibility\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"hide_block\""),
             std::string::npos);
    CHECK_NE(json.find("\"visible\":false"), std::string::npos);
    CHECK_NE(json.find("\"changed\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\nKeep me\nmiddle\nafter\n"});
    CHECK_EQ(reopened.inspect_tables().size(), 0U);
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.find_bookmark("keep_block").has_value());
    CHECK_FALSE(body_template.find_bookmark("hide_block").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli set-bookmark-block-visibility requires an explicit visibility value") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_set_bookmark_block_visibility_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"set-bookmark-block-visibility",
                      "missing.docx",
                      "hide_block",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-bookmark-block-visibility\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected --visible "
            "true|false\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli apply-bookmark-block-visibility keeps and removes body blocks") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_apply_bookmark_block_visibility_source.docx";
    const fs::path updated =
        working_directory / "cli_apply_bookmark_block_visibility_updated.docx";
    const fs::path output =
        working_directory / "cli_apply_bookmark_block_visibility.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_block_visibility_fixture(source);

    CHECK_EQ(run_cli({"apply-bookmark-block-visibility",
                      source.string(),
                      "--show",
                      "keep_block",
                      "--hide",
                      "hide_block",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"apply-bookmark-block-visibility\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"complete\":true"), std::string::npos);
    CHECK_NE(json.find("\"requested\":2"), std::string::npos);
    CHECK_NE(json.find("\"matched\":2"), std::string::npos);
    CHECK_NE(json.find("\"kept\":1"), std::string::npos);
    CHECK_NE(json.find("\"removed\":1"), std::string::npos);
    CHECK_NE(json.find("\"bindings\":[{\"bookmark_name\":\"keep_block\","
                       "\"visible\":true},{\"bookmark_name\":\"hide_block\","
                       "\"visible\":false}]"),
             std::string::npos);
    CHECK_NE(json.find("\"missing_bookmarks\":[]"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\nKeep me\nmiddle\nafter\n"});
    CHECK_EQ(reopened.inspect_tables().size(), 0U);
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("keep_block").has_value());
    CHECK_FALSE(body_template.find_bookmark("hide_block").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-text rewrites a section header bookmark range") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_text_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_text_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_text.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-text",
                      source.string(),
                      "header_logo",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--text",
                      "updated header",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-text\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"text\":\"updated header\""), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_part_lines(header_template.paragraphs()),
             std::vector<std::string>{"header before", "updated header"});
    CHECK(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE(
    "cli replace-bookmark-text preserves explicit line breaks in a section header "
    "bookmark range") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_text_multiline_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_text_multiline_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_text_multiline.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-text",
                      source.string(),
                      "header_logo",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--text",
                      "updated header\nsecond line",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-text\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"text\":\"updated header\\nsecond line\""),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_part_lines(header_template.paragraphs()),
             std::vector<std::string>{"header before",
                                      "updated header\nsecond line"});
    CHECK(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-text requires replacement text") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_text_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-text",
                      "missing.docx",
                      "header_logo",
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{"{\"command\":\"replace-bookmark-text\",\"ok\":false,"
                         "\"stage\":\"parse\",\"message\":\"expected --text "
                         "<text>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli fill-bookmarks rewrites multiple body bookmarks and reports misses") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_fill_bookmarks_source.docx";
    const fs::path updated =
        working_directory / "cli_fill_bookmarks_updated.docx";
    const fs::path output =
        working_directory / "cli_fill_bookmarks.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_fill_bookmarks_fixture(source);

    CHECK_EQ(run_cli({"fill-bookmarks",
                      source.string(),
                      "--set",
                      "customer",
                      "Acme Corp",
                      "--set",
                      "invoice",
                      "INV-2026-0001",
                      "--set",
                      "missing",
                      "ignored",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"fill-bookmarks\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"complete\":false"), std::string::npos);
    CHECK_NE(json.find("\"requested\":3"), std::string::npos);
    CHECK_NE(json.find("\"matched\":2"), std::string::npos);
    CHECK_NE(json.find("\"replaced\":2"), std::string::npos);
    CHECK_NE(json.find("\"bindings\":[{\"bookmark_name\":\"customer\","
                       "\"text\":\"Acme Corp\"},{\"bookmark_name\":\"invoice\","
                       "\"text\":\"INV-2026-0001\"},{\"bookmark_name\":"
                       "\"missing\",\"text\":\"ignored\"}]"),
             std::string::npos);
    CHECK_NE(json.find("\"missing_bookmarks\":[\"missing\"]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"Customer: Acme Corp\nInvoice: INV-2026-0001\n"});
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.find_bookmark("customer").has_value());
    CHECK(body_template.find_bookmark("invoice").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli fill-bookmarks reads UTF-8 values from files") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_fill_bookmarks_file_source.docx";
    const fs::path customer_text =
        working_directory / "cli_fill_bookmarks_customer.txt";
    const fs::path invoice_text =
        working_directory / "cli_fill_bookmarks_invoice.txt";
    const fs::path updated =
        working_directory / "cli_fill_bookmarks_file_updated.docx";
    const fs::path output =
        working_directory / "cli_fill_bookmarks_file.json";

    remove_if_exists(source);
    remove_if_exists(customer_text);
    remove_if_exists(invoice_text);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_fill_bookmarks_fixture(source);
    write_binary_file(customer_text, std::string{"上海羽文科技"});
    write_binary_file(invoice_text, std::string{"发票-2026-甲"});

    CHECK_EQ(run_cli({"fill-bookmarks",
                      source.string(),
                      "--set-file",
                      "customer",
                      customer_text.string(),
                      "--set-file",
                      "invoice",
                      invoice_text.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"fill-bookmarks\""), std::string::npos);
    CHECK_NE(json.find("\"complete\":true"), std::string::npos);
    CHECK_NE(json.find("\"matched\":2"), std::string::npos);
    CHECK_NE(json.find("\"replaced\":2"), std::string::npos);
    CHECK_NE(json.find("\"text\":\"上海羽文科技\""), std::string::npos);
    CHECK_NE(json.find("\"text\":\"发票-2026-甲\""), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"Customer: 上海羽文科技\nInvoice: 发票-2026-甲\n"});

    remove_if_exists(source);
    remove_if_exists(customer_text);
    remove_if_exists(invoice_text);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli fill-bookmarks rejects duplicate binding names") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_fill_bookmarks_duplicate_source.docx";
    const fs::path output =
        working_directory / "cli_fill_bookmarks_duplicate.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_fill_bookmarks_fixture(source);

    CHECK_EQ(run_cli({"fill-bookmarks",
                      source.string(),
                      "--set",
                      "customer",
                      "Acme Corp",
                      "--set",
                      "customer",
                      "Other",
                      "--json"},
                     output),
             1);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"fill-bookmarks\""), std::string::npos);
    CHECK_NE(json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(json.find("duplicate"), std::string::npos);

    remove_if_exists(source);
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
                 "--z-order",
                 "108",
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
    CHECK_NE(json.find("\"z_order\":108"), std::string::npos);
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
    CHECK_EQ(images[0].floating_options->z_order, 108U);
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

TEST_CASE("cli can reorder header and footer parts without rebinding section references") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_move_parts_source.docx";
    const fs::path header_moved =
        working_directory / "cli_move_parts_header_moved.docx";
    const fs::path footer_moved =
        working_directory / "cli_move_parts_footer_moved.docx";
    const fs::path move_header_output =
        working_directory / "cli_move_header_part.json";
    const fs::path move_footer_output =
        working_directory / "cli_move_footer_part.json";

    remove_if_exists(source);
    remove_if_exists(header_moved);
    remove_if_exists(footer_moved);
    remove_if_exists(move_header_output);
    remove_if_exists(move_footer_output);

    create_cli_reference_fixture(source);

    featherdoc::Document fixture_doc(source);
    REQUIRE_FALSE(fixture_doc.open());
    const auto source_header_index =
        find_header_index_by_text(fixture_doc, "section 1 header");
    const auto target_header_index =
        find_header_index_by_text(fixture_doc, "section 0 header");
    const auto source_footer_index =
        find_footer_index_by_text(fixture_doc, "section 1 first footer");
    const auto target_footer_index =
        find_footer_index_by_text(fixture_doc, "section 0 footer");

    CHECK_EQ(run_cli({"move-header-part", source.string(),
                      std::to_string(source_header_index),
                      std::to_string(target_header_index), "--output",
                      header_moved.string(), "--json"},
                     move_header_output),
             0);
    CHECK_EQ(
        read_text_file(move_header_output),
        std::string{"{\"command\":\"move-header-part\",\"ok\":true,"
                    "\"in_place\":false,\"sections\":3,\"headers\":2,"
                    "\"footers\":2,\"part\":\"header\",\"source\":"} +
            std::to_string(source_header_index) + ",\"target\":" +
            std::to_string(target_header_index) + "}\n");

    featherdoc::Document moved_header_doc(header_moved);
    REQUIRE_FALSE(moved_header_doc.open());
    CHECK_EQ(moved_header_doc.header_paragraphs(0).runs().get_text(),
             "section 1 header");
    CHECK_EQ(moved_header_doc.header_paragraphs(1).runs().get_text(),
             "section 0 header");
    CHECK_EQ(moved_header_doc.section_header_paragraphs(0).runs().get_text(),
             "section 0 header");
    CHECK_EQ(moved_header_doc.section_header_paragraphs(1).runs().get_text(),
             "section 1 header");

    CHECK_EQ(run_cli({"move-footer-part", header_moved.string(),
                      std::to_string(source_footer_index),
                      std::to_string(target_footer_index), "--output",
                      footer_moved.string(), "--json"},
                     move_footer_output),
             0);
    CHECK_EQ(
        read_text_file(move_footer_output),
        std::string{"{\"command\":\"move-footer-part\",\"ok\":true,"
                    "\"in_place\":false,\"sections\":3,\"headers\":2,"
                    "\"footers\":2,\"part\":\"footer\",\"source\":"} +
            std::to_string(source_footer_index) + ",\"target\":" +
            std::to_string(target_footer_index) + "}\n");

    featherdoc::Document moved_footer_doc(footer_moved);
    REQUIRE_FALSE(moved_footer_doc.open());
    const auto expected_footer_at_index_zero =
        target_footer_index == 0U ? "section 1 first footer" : "section 0 footer";
    const auto expected_footer_at_index_one =
        target_footer_index == 0U ? "section 0 footer" : "section 1 first footer";
    CHECK_EQ(moved_footer_doc.footer_paragraphs(0).runs().get_text(),
             expected_footer_at_index_zero);
    CHECK_EQ(moved_footer_doc.footer_paragraphs(1).runs().get_text(),
             expected_footer_at_index_one);
    CHECK_EQ(moved_footer_doc.section_footer_paragraphs(0).runs().get_text(),
             "section 0 footer");
    CHECK_EQ(moved_footer_doc.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    remove_if_exists(source);
    remove_if_exists(header_moved);
    remove_if_exists(footer_moved);
    remove_if_exists(move_header_output);
    remove_if_exists(move_footer_output);
}

TEST_CASE("cli move-header-part and move-footer-part report parse and runtime errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_move_parts_error_source.docx";
    const fs::path parse_output =
        working_directory / "cli_move_header_part_parse.json";
    const fs::path runtime_output =
        working_directory / "cli_move_footer_part_runtime.json";

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(runtime_output);

    create_cli_reference_fixture(source);

    CHECK_EQ(run_cli({"move-header-part", source.string(), "0", "--json"},
                     parse_output),
             2);
    CHECK_EQ(read_text_file(parse_output),
             std::string{
                 "{\"command\":\"move-header-part\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"invalid target index: --json\"}\n"});

    CHECK_EQ(run_cli({"move-footer-part", source.string(), "0", "3", "--json"},
                     runtime_output),
             1);
    const auto runtime_json = read_text_file(runtime_output);
    CHECK_NE(runtime_json.find("\"command\":\"move-footer-part\""), std::string::npos);
    CHECK_NE(runtime_json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(runtime_json.find("\"detail\":\"header/footer part index is out of range for reordering\""),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(parse_output);
    remove_if_exists(runtime_output);
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
                 "\"malformed_placeholders\":[\"summary_block\"],"
                 "\"unexpected_bookmarks\":[],\"kind_mismatches\":[],"
                 "\"occurrence_mismatches\":[]}\n"});

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
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]}\n"});

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
                 "malformed_placeholders: footer_summary\n"
                 "unexpected_bookmarks: none\n"
                 "kind_mismatches: none\n"
                 "occurrence_mismatches: none\n"});

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(footer_output);
}

TEST_CASE("cli validate-template reports unexpected bookmarks kind mismatches and occurrence mismatches") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_v2_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_schema_v2_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_schema_v2_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template",
                      source.string(),
                      "--part",
                      "body",
                      "--slot",
                      "summary_block:text",
                      "--slot",
                      "approver:text:count=2",
                      "--json"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"part\":\"body\",\"entry_name\":\"word/document.xml\","
                 "\"passed\":false,\"missing_required\":[],"
                 "\"duplicate_bookmarks\":[],\"malformed_placeholders\":[],"
                 "\"unexpected_bookmarks\":[{\"bookmark_name\":\"extra_slot\","
                 "\"occurrence_count\":1,\"kind\":\"text\","
                 "\"is_duplicate\":false}],"
                 "\"kind_mismatches\":[{\"bookmark_name\":\"summary_block\","
                 "\"expected_kind\":\"text\",\"actual_kind\":\"block\","
                 "\"occurrence_count\":1}],"
                 "\"occurrence_mismatches\":[{\"bookmark_name\":\"approver\","
                 "\"actual_occurrences\":1,\"min_occurrences\":2,"
                 "\"max_occurrences\":2}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
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

    CHECK_EQ(run_cli({"validate-template", source.string(), "--part", "body", "--slot",
                      "header_title:text:min=3:max=1", "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"validate-template\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"invalid --slot occurrence "
                 "range: max must be greater than or equal to min\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template-schema aggregates multiple targets as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_schema_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--target",
                      "section-header",
                      "--section",
                      "0",
                      "--kind",
                      "default",
                      "--slot",
                      "header_title:text",
                      "--slot",
                      "header_note:block",
                      "--slot",
                      "header_rows:table_rows",
                      "--target",
                      "section-footer",
                      "--section",
                      "0",
                      "--kind",
                      "default",
                      "--slot",
                      "footer_company:text",
                      "--slot",
                      "footer_summary:block",
                      "--slot",
                      "footer_signature:text",
                      "--json"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"passed\":false,\"part_results\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":false,"
                 "\"missing_required\":[\"footer_signature\"],"
                 "\"duplicate_bookmarks\":[\"footer_company\"],"
                 "\"malformed_placeholders\":[\"footer_summary\"],"
                 "\"unexpected_bookmarks\":[],\"kind_mismatches\":[],"
                 "\"occurrence_mismatches\":[]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template-schema reports unavailable targets in text output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_unavailable_source.docx";
    const fs::path output =
        working_directory / "cli_validate_template_schema_unavailable_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--target",
                      "section-header",
                      "--section",
                      "1",
                      "--slot",
                      "optional_header:text:optional",
                      "--target",
                      "section-footer",
                      "--section",
                      "1",
                      "--slot",
                      "required_footer:text"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "passed: no\n"
                 "part_result_count: 2\n"
                 "\n"
                 "part_result[0]\n"
                 "part: section-header\n"
                 "section: 1\n"
                 "kind: default\n"
                 "available: no\n"
                 "entry_name: \n"
                 "passed: yes\n"
                 "missing_required: none\n"
                 "duplicate_bookmarks: none\n"
                 "malformed_placeholders: none\n"
                 "unexpected_bookmarks: none\n"
                 "kind_mismatches: none\n"
                 "occurrence_mismatches: none\n"
                 "\n"
                 "part_result[1]\n"
                 "part: section-footer\n"
                 "section: 1\n"
                 "kind: default\n"
                 "available: no\n"
                 "entry_name: \n"
                 "passed: no\n"
                 "missing_required: required_footer\n"
                 "duplicate_bookmarks: none\n"
                 "malformed_placeholders: none\n"
                 "unexpected_bookmarks: none\n"
                 "kind_mismatches: none\n"
                 "occurrence_mismatches: none\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template-schema reads targets from a schema file") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_file_source.docx";
    const fs::path schema_file =
        working_directory / "cli_validate_template_schema_file.json";
    const fs::path output =
        working_directory / "cli_validate_template_schema_file_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);
    write_binary_file(
        schema_file,
        std::string{
            "{\n"
            "  \"targets\": [\n"
            "    {\n"
            "      \"part\": \"section-header\",\n"
            "      \"section\": 0,\n"
            "      \"kind\": \"default\",\n"
            "      \"slots\": [\"header_title:text\", \"header_note:block\", "
            "\"header_rows:table_rows\"]\n"
            "    },\n"
            "    {\n"
            "      \"part\": \"section-footer\",\n"
            "      \"section\": 0,\n"
            "      \"kind\": \"default\",\n"
            "      \"slots\": [\"footer_company:text\", "
            "\"footer_summary:block\", \"footer_signature:text\"]\n"
            "    }\n"
            "  ]\n"
            "}\n"});

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"passed\":false,\"part_results\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":false,"
                 "\"missing_required\":[\"footer_signature\"],"
                 "\"duplicate_bookmarks\":[\"footer_company\"],"
                 "\"malformed_placeholders\":[\"footer_summary\"],"
                 "\"unexpected_bookmarks\":[],\"kind_mismatches\":[],"
                 "\"occurrence_mismatches\":[]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template-schema reads structured slot objects from a schema file") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_object_file_source.docx";
    const fs::path schema_file =
        working_directory / "cli_validate_template_schema_object_file.json";
    const fs::path output =
        working_directory / "cli_validate_template_schema_object_file_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);
    write_binary_file(
        schema_file,
        std::string{
            "{\n"
            "  \"targets\": [\n"
            "    {\n"
            "      \"part\": \"section-header\",\n"
            "      \"section\": 0,\n"
            "      \"kind\": \"default\",\n"
            "      \"slots\": [\n"
            "        {\"bookmark\": \"header_title\", \"kind\": \"text\"},\n"
            "        {\"bookmark_name\": \"header_note\", \"kind\": \"block\", "
            "\"required\": true},\n"
            "        {\"bookmark\": \"header_rows\", \"kind\": \"table_rows\", "
            "\"count\": 1}\n"
            "      ]\n"
            "    },\n"
            "    {\n"
            "      \"part\": \"section-footer\",\n"
            "      \"section\": 0,\n"
            "      \"kind\": \"default\",\n"
            "      \"slots\": [\n"
            "        {\"bookmark\": \"footer_company\", \"kind\": \"text\"},\n"
            "        {\"bookmark\": \"footer_summary\", \"kind\": \"block\"},\n"
            "        {\"bookmark\": \"footer_signature\", \"kind\": \"text\"}\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}\n"});

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"passed\":false,\"part_results\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":false,"
                 "\"missing_required\":[\"footer_signature\"],"
                 "\"duplicate_bookmarks\":[\"footer_company\"],"
                 "\"malformed_placeholders\":[\"footer_summary\"],"
                 "\"unexpected_bookmarks\":[],\"kind_mismatches\":[],"
                 "\"occurrence_mismatches\":[]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);
}

TEST_CASE("cli validate-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_validate_template_schema_parse_source.docx";
    const fs::path schema_file =
        working_directory / "cli_validate_template_schema_parse_schema.json";
    const fs::path output =
        working_directory / "cli_validate_template_schema_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--slot",
                      "header_title:text",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template-schema\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"--slot requires a preceding "
            "--target <body|header|footer|section-header|section-footer>\"}\n"});

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--target",
                      "section-header",
                      "--slot",
                      "header_title:text",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template-schema\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "schema validation requires --section <index>\"}\n"});

    write_binary_file(schema_file,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"section-header\","
                          "\"slots\":[\"header_title:text\"]"
                          "}]"
                          "}"});
    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template-schema\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "schema validation requires --section <index>\"}\n"});

    write_binary_file(schema_file,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":[{"
                          "\"bookmark\":\"customer\","
                          "\"kind\":\"text\","
                          "\"count\":1,"
                          "\"min\":1"
                          "}]"
                          "}]"
                          "}"});
    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"validate-template-schema\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"JSON schema slot member "
            "'count' conflicts with 'min'/'max'\"}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);
}

TEST_CASE("cli export-template-schema prints reusable schema json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_source.docx";
    const fs::path output =
        working_directory / "cli_export_template_schema_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema", source.string()}, output), 0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"header\",\"index\":0,\"slots\":["
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"}]},"
                 "{\"part\":\"footer\",\"index\":0,\"slots\":["
                 "{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli export-template-schema writes schema file and summary json") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_summary_source.docx";
    const fs::path schema_output =
        working_directory / "cli_export_template_schema_summary.schema.json";
    const fs::path output =
        working_directory / "cli_export_template_schema_summary_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    create_cli_body_template_validation_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema",
                      source.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"export-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":1,\"slot_count\":2,"
                 "\"skipped_count\":0,\"skipped_bookmarks\":[]}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":[{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli export-template-schema emits section targets when requested") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_section_source.docx";
    const fs::path output =
        working_directory / "cli_export_template_schema_section_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema",
                      source.string(),
                      "--section-targets"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"slots\":["
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"}]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"slots\":["
                 "{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli export-template-schema emits resolved section targets and round-trips") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_export_template_schema_resolved_source.docx";
    const fs::path schema_output =
        working_directory / "cli_export_template_schema_resolved.schema.json";
    const fs::path export_output =
        working_directory / "cli_export_template_schema_resolved_output.json";
    const fs::path validate_output =
        working_directory / "cli_export_template_schema_resolved_validate.json";

    remove_if_exists(source);
    remove_if_exists(schema_output);
    remove_if_exists(export_output);
    remove_if_exists(validate_output);

    create_cli_resolved_part_template_validation_fixture(source);

    CHECK_EQ(run_cli({"export-template-schema",
                      source.string(),
                      "--resolved-section-targets",
                      "--output",
                      schema_output.string(),
                      "--json"},
                     export_output),
             0);
    CHECK_EQ(read_text_file(export_output),
             std::string{
                 "{\"command\":\"export-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":4,\"slot_count\":10,"
                 "\"skipped_count\":0,\"skipped_bookmarks\":[]}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"}]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"}]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}"
                 "]}\n"});

    CHECK_EQ(run_cli({"validate-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_output.string(),
                      "--json"},
                     validate_output),
             0);
    CHECK_EQ(read_text_file(validate_output),
             std::string{
                 "{\"passed\":true,\"part_results\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/header1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"available\":true,"
                 "\"entry_name\":\"word/footer1.xml\",\"passed\":true,"
                 "\"missing_required\":[],\"duplicate_bookmarks\":[],"
                 "\"malformed_placeholders\":[],\"unexpected_bookmarks\":[],"
                 "\"kind_mismatches\":[],\"occurrence_mismatches\":[]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_output);
    remove_if_exists(export_output);
    remove_if_exists(validate_output);
}

TEST_CASE("cli normalize-template-schema writes canonical schema file and summary json") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_normalize_template_schema_input.json";
    const fs::path schema_output =
        working_directory / "cli_normalize_template_schema_output.json";
    const fs::path output =
        working_directory / "cli_normalize_template_schema_summary.json";

    remove_if_exists(schema_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"linked_to_previous\":true,"
                          "\"resolved_from_section\":0,"
                          "\"slots\":["
                          "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"},"
                          "{\"bookmark_name\":\"footer_company\",\"kind\":\"text\","
                          "\"count\":2}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":["
                          "{\"bookmark\":\"header_title\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"header_note\",\"kind\":\"block\"}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"normalize-template-schema",
                      schema_input.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"normalize-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":3,\"slot_count\":6}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,\"slots\":["
                 "{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,\"slots\":["
                 "{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli lint-template-schema reports clean normalized schemas") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_lint_template_schema_clean_input.json";
    const fs::path output =
        working_directory / "cli_lint_template_schema_clean_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}"
                          "]}]}\n"});

    CHECK_EQ(run_cli({"lint-template-schema", schema_input.string(), "--json"}, output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"lint-template-schema\",\"ok\":true,"
                 "\"clean\":true,\"target_count\":1,\"slot_count\":2,"
                 "\"issue_count\":0,\"duplicate_target_count\":0,"
                 "\"duplicate_slot_count\":0,\"target_order_issue_count\":0,"
                 "\"slot_order_issue_count\":0,"
                 "\"entry_name_issue_count\":0,\"issues\":[]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(output);
}

TEST_CASE("cli lint-template-schema reports structural issues as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_lint_template_schema_dirty_input.json";
    const fs::path output =
        working_directory / "cli_lint_template_schema_dirty_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"entry_name\":\"word/header1.xml\","
                          "\"slots\":["
                          "{\"bookmark\":\"zeta\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"alpha\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"alpha\",\"kind\":\"text\"}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"account_id\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"customer\",\"kind\":\"text\"}]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"lint-template-schema", schema_input.string(), "--json"}, output),
             1);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"lint-template-schema\",\"ok\":true,"
            "\"clean\":false,\"target_count\":3,\"slot_count\":5,"
            "\"issue_count\":5,\"duplicate_target_count\":1,"
            "\"duplicate_slot_count\":1,\"target_order_issue_count\":1,"
            "\"slot_order_issue_count\":1,"
            "\"entry_name_issue_count\":1,\"issues\":["
            "{\"issue\":\"entry_name_present\",\"target_index\":0,"
            "\"target\":{\"part\":\"section-header\",\"section\":1,"
            "\"kind\":\"default\"},\"entry_name\":\"word/header1.xml\"},"
            "{\"issue\":\"slot_order\",\"target_index\":0,"
            "\"target\":{\"part\":\"section-header\",\"section\":1,"
            "\"kind\":\"default\"},\"slot_index\":1,\"previous_slot_index\":0,"
            "\"bookmark\":\"alpha\",\"previous_bookmark\":\"zeta\"},"
            "{\"issue\":\"duplicate_slot_name\",\"target_index\":0,"
            "\"target\":{\"part\":\"section-header\",\"section\":1,"
            "\"kind\":\"default\"},\"slot_index\":2,\"previous_slot_index\":1,"
            "\"bookmark\":\"alpha\"},"
            "{\"issue\":\"target_order\",\"target_index\":1,"
            "\"target\":{\"part\":\"body\"},\"previous_target_index\":0},"
            "{\"issue\":\"duplicate_target_identity\",\"target_index\":2,"
            "\"target\":{\"part\":\"body\"},\"previous_target_index\":1}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(output);
}

TEST_CASE("cli lint-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_lint_template_schema_parse_input.json";
    const fs::path output =
        working_directory / "cli_lint_template_schema_parse_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"lint-template-schema", "--json"}, output), 2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"lint-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"lint-template-schema "
                 "expects a schema path\"}\n"});

    CHECK_EQ(run_cli({"lint-template-schema",
                      schema_input.string(),
                      "--json",
                      "--bad-option"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"lint-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"unknown option: "
                 "--bad-option\"}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(output);
}

TEST_CASE("cli repair-template-schema canonicalizes and deduplicates schema") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_repair_template_schema_input.json";
    const fs::path schema_output =
        working_directory / "cli_repair_template_schema_output.json";
    const fs::path output =
        working_directory / "cli_repair_template_schema_summary.json";

    remove_if_exists(schema_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"entry_name\":\"word/header1.xml\","
                          "\"slots\":["
                          "{\"bookmark\":\"zeta\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"alpha\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"alpha\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"entry_name\":\"word/document.xml\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"repair-template-schema",
                      schema_input.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"repair-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"input_target_count\":3,\"input_slot_count\":6,"
                 "\"target_count\":2,\"slot_count\":4,"
                 "\"merged_duplicate_target_count\":1,"
                 "\"deduplicated_target_count\":1,"
                 "\"deduplicated_slot_count\":2,"
                 "\"stripped_entry_name_count\":2,"
                 "\"replaced_slot_count\":2,\"changed\":true}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"invoice_no\",\"kind\":\"text\",\"count\":2}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"slots\":["
                 "{\"bookmark\":\"alpha\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"zeta\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli repair-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_repair_template_schema_parse_input.json";
    const fs::path output =
        working_directory / "cli_repair_template_schema_parse_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"repair-template-schema", "--json"}, output), 2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"repair-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"repair-template-schema "
                 "expects a schema path\"}\n"});

    CHECK_EQ(run_cli({"repair-template-schema",
                      schema_input.string(),
                      "--json",
                      "--bad-option"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"repair-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"unknown option: "
                 "--bad-option\"}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(output);
}

TEST_CASE("cli merge-template-schema merges targets and replaces later slot definitions") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_merge_template_schema_left.json";
    const fs::path right_schema =
        working_directory / "cli_merge_template_schema_right.json";
    const fs::path merged_schema =
        working_directory / "cli_merge_template_schema_output.json";
    const fs::path output =
        working_directory / "cli_merge_template_schema_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(merged_schema);
    remove_if_exists(output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"slots\":[{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"merge-template-schema",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      merged_schema.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"merge-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(merged_schema.string()) +
                 "\",\"input_count\":2,\"target_count\":3,\"slot_count\":5,"
                 "\"updated_target_count\":1,\"replaced_slot_count\":1}\n"});
    CHECK_EQ(read_text_file(merged_schema),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"},"
                 "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,\"slots\":[{\"bookmark\":"
                 "\"header_title\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"slots\":[{\"bookmark\":"
                 "\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(merged_schema);
    remove_if_exists(output);
}

TEST_CASE("cli merge-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema =
        working_directory / "cli_merge_template_schema_parse_schema.json";
    const fs::path output =
        working_directory / "cli_merge_template_schema_parse_output.json";

    remove_if_exists(schema);
    remove_if_exists(output);

    write_binary_file(schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"merge-template-schema", schema.string(), "--json"}, output), 2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"merge-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"merge-template-schema "
                 "expects at least two schema paths\"}\n"});

    remove_if_exists(schema);
    remove_if_exists(output);
}

TEST_CASE("cli patch-template-schema applies upserts removals and slot pruning") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_patch_template_schema_input.json";
    const fs::path patch_input =
        working_directory / "cli_patch_template_schema_patch.json";
    const fs::path schema_output =
        working_directory / "cli_patch_template_schema_output.json";
    const fs::path output =
        working_directory / "cli_patch_template_schema_summary.json";

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"slots\":[{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(patch_input,
                      std::string{
                          "{"
                          "\"remove_targets\":[{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\""
                          "}],"
                          "\"remove_slots\":[{"
                          "\"part\":\"body\","
                          "\"bookmark\":\"summary_block\""
                          "}],"
                          "\"rename_slots\":[{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"bookmark\":\"header_title\","
                          "\"new_bookmark\":\"document_title\""
                          "}],"
                          "\"upsert_targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}"
                          "]"
                          "}]"
                          "}"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--output",
                      schema_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(schema_output.string()) +
                 "\",\"target_count\":2,\"slot_count\":3,"
                 "\"upsert_target_count\":1,\"remove_target_count\":1,"
                 "\"remove_slot_count\":1,\"rename_slot_count\":1,"
                 "\"updated_target_count\":1,\"replaced_slot_count\":1,"
                 "\"applied_remove_target_count\":1,"
                 "\"applied_remove_slot_count\":1,"
                 "\"applied_rename_slot_count\":1,"
                 "\"pruned_empty_target_count\":0}\n"});
    CHECK_EQ(read_text_file(schema_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                  "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                  "\"kind\":\"default\",\"resolved_from_section\":0,"
                  "\"linked_to_previous\":true,\"slots\":[{\"bookmark\":"
                 "\"document_title\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(schema_output);
    remove_if_exists(output);
}

TEST_CASE("cli patch-template-schema reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema_input =
        working_directory / "cli_patch_template_schema_parse_input.json";
    const fs::path patch_input =
        working_directory / "cli_patch_template_schema_parse_patch.json";
    const fs::path output =
        working_directory / "cli_patch_template_schema_parse_output.json";

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(output);

    write_binary_file(schema_input,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"missing required "
                 "--patch-file <path> option\"}\n"});

    write_binary_file(patch_input, std::string{"{}\n"});
    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"targets\":[{\"part\":\"body\",\"slots\":[{\"bookmark\":"
                 "\"customer\",\"kind\":\"text\"}]}]}\n"});

    write_binary_file(patch_input,
                      std::string{"{\"remove_targets\":[],\"remove_targets\":[]}\n"});
    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"JSON schema patch member "
                 "'remove_targets' must not be duplicated\"}\n"});

    write_binary_file(patch_input,
                      std::string{"{\"rename_slots\":[{\"part\":\"body\","
                                  "\"bookmark\":\"customer\"}]}\n"});
    CHECK_EQ(run_cli({"patch-template-schema",
                      schema_input.string(),
                      "--patch-file",
                      patch_input.string(),
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"patch-template-schema\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"JSON schema patch "
                 "rename-slot object must contain 'new_bookmark' or "
                 "'new_bookmark_name'\"}\n"});

    remove_if_exists(schema_input);
    remove_if_exists(patch_input);
    remove_if_exists(output);
}

TEST_CASE("cli build-template-schema-patch generates reusable patch output") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"slots\":[{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"}]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":1,\"removed_target_count\":1,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":1,"
                 "\"generated_remove_slot_count\":0,"
                 "\"generated_rename_slot_count\":1,"
                 "\"generated_upsert_target_count\":2,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"remove_targets\":["
                 "{\"part\":\"section-footer\",\"section\":1,\"kind\":\"default\"}],"
                 "\"rename_slots\":["
                 "{\"part\":\"body\",\"bookmark\":\"summary_block\","
                 "\"new_bookmark\":\"invoice_no\"}],"
                 "\"upsert_targets\":["
                 "{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2}]},"
                 "{\"part\":\"section-header\",\"section\":1,\"kind\":\"default\","
                 "\"resolved_from_section\":0,\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"header_title\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch emits slot-level rename and removal") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_slot_ops_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_slot_ops_right.json";
    const fs::path patch_output =
        working_directory / "cli_build_template_schema_patch_slot_ops_output.json";
    const fs::path build_output =
        working_directory / "cli_build_template_schema_patch_slot_ops_summary.json";
    const fs::path applied_schema =
        working_directory / "cli_build_template_schema_patch_slot_ops_applied.json";
    const fs::path apply_output =
        working_directory / "cli_build_template_schema_patch_slot_ops_apply_summary.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"obsolete_note\",\"kind\":\"block\","
                          "\"required\":false},"
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"invoice_no\",\"kind\":\"text\"}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--output",
                      patch_output.string(),
                      "--json"},
                     build_output),
             0);
    CHECK_EQ(read_text_file(build_output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":true,"
                 "\"output_path\":\"" +
                 json_escape_text(patch_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"generated_remove_target_count\":0,"
                 "\"generated_remove_slot_count\":1,"
                 "\"generated_rename_slot_count\":1,"
                 "\"generated_upsert_target_count\":0,"
                 "\"empty_patch\":false}\n"});
    CHECK_EQ(read_text_file(patch_output),
             std::string{
                 "{\"remove_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"obsolete_note\"}],"
                 "\"rename_slots\":[{\"part\":\"body\",\"bookmark\":"
                 "\"summary_block\",\"new_bookmark\":\"invoice_no\"}]}\n"});

    CHECK_EQ(run_cli({"patch-template-schema",
                      left_schema.string(),
                      "--patch-file",
                      patch_output.string(),
                      "--output",
                      applied_schema.string(),
                      "--json"},
                     apply_output),
             0);
    CHECK_EQ(read_text_file(applied_schema), read_text_file(right_schema) + "\n");

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(patch_output);
    remove_if_exists(build_output);
    remove_if_exists(applied_schema);
    remove_if_exists(apply_output);
}

TEST_CASE("cli build-template-schema-patch emits empty patch for equivalent schemas") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_build_template_schema_patch_equal_left.json";
    const fs::path right_schema =
        working_directory / "cli_build_template_schema_patch_equal_right.json";
    const fs::path output =
        working_directory / "cli_build_template_schema_patch_equal_output.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);

    write_binary_file(left_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});
    write_binary_file(right_schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"build-template-schema-patch",
                      left_schema.string(),
                      right_schema.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output), std::string{"{}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);
}

TEST_CASE("cli build-template-schema-patch reports parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path schema =
        working_directory / "cli_build_template_schema_patch_parse_schema.json";
    const fs::path output =
        working_directory / "cli_build_template_schema_patch_parse_output.json";

    remove_if_exists(schema);
    remove_if_exists(output);

    write_binary_file(schema,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(
        run_cli({"build-template-schema-patch", schema.string(), "--json"}, output), 2);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"build-template-schema-patch\",\"ok\":false,"
                 "\"stage\":\"parse\",\"message\":\"build-template-schema-patch "
                 "expects left and right schema paths\"}\n"});

    remove_if_exists(schema);
    remove_if_exists(output);
}

TEST_CASE("cli diff-template-schema reports added removed and changed targets as json") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_diff_template_schema_left.json";
    const fs::path right_schema =
        working_directory / "cli_diff_template_schema_right.json";
    const fs::path output =
        working_directory / "cli_diff_template_schema_output.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-header\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":["
                          "{\"bookmark\":\"header_title\",\"kind\":\"text\"}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2}"
                          "]"
                          "}"
                          "]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":["
                          "{"
                          "\"part\":\"section-footer\","
                          "\"section\":1,"
                          "\"kind\":\"default\","
                          "\"slots\":["
                          "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}"
                          "]"
                          "},"
                          "{"
                          "\"part\":\"body\","
                          "\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":3}"
                          "]"
                          "}"
                          "]"
                          "}"});

    CHECK_EQ(run_cli({"diff-template-schema",
                      left_schema.string(),
                      right_schema.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"equal\":false,\"added_target_count\":1,"
                 "\"removed_target_count\":1,\"changed_target_count\":1,"
                 "\"added_targets\":[{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"slots\":[{\"bookmark\":\"footer_summary\","
                 "\"kind\":\"text\"}]}],"
                 "\"removed_targets\":[{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,\"slots\":[{\"bookmark\":"
                 "\"header_title\",\"kind\":\"text\"}]}],"
                 "\"changed_targets\":[{\"left\":{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2}]},"
                 "\"right\":{\"part\":\"body\",\"slots\":[{\"bookmark\":"
                 "\"customer\",\"kind\":\"text\",\"count\":3}]}}]}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);
}

TEST_CASE("cli diff-template-schema supports fail-on-diff gate exit code") {
    const fs::path working_directory = fs::current_path();
    const fs::path left_schema =
        working_directory / "cli_diff_template_schema_fail_left.json";
    const fs::path right_schema =
        working_directory / "cli_diff_template_schema_fail_right.json";
    const fs::path output =
        working_directory / "cli_diff_template_schema_fail_output.json";

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);

    write_binary_file(left_schema,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"customer\",\"kind\":\"text\"}]"
                          "}]"
                          "}"});
    write_binary_file(right_schema,
                      std::string{
                          "{"
                          "\"targets\":[{"
                          "\"part\":\"body\","
                          "\"slots\":[{\"bookmark\":\"customer\",\"kind\":\"text\","
                          "\"count\":2}]"
                          "}]"
                          "}"});

    CHECK_EQ(run_cli({"diff-template-schema",
                      left_schema.string(),
                      right_schema.string(),
                      "--fail-on-diff",
                      "--json"},
                     output),
             1);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"equal\":false,\"added_target_count\":0,"
                 "\"removed_target_count\":0,\"changed_target_count\":1,"
                 "\"added_targets\":[],\"removed_targets\":[],"
                 "\"changed_targets\":[{\"left\":{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\"}]},"
                 "\"right\":{\"part\":\"body\",\"slots\":[{\"bookmark\":"
                 "\"customer\",\"kind\":\"text\",\"count\":2}]}}]}\n"});

    remove_if_exists(left_schema);
    remove_if_exists(right_schema);
    remove_if_exists(output);
}

TEST_CASE("cli check-template-schema matches resolved section baseline and writes generated schema") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_check_template_schema_match_source.docx";
    const fs::path schema_file =
        working_directory / "cli_check_template_schema_match_schema.json";
    const fs::path generated_output =
        working_directory / "cli_check_template_schema_match_generated.json";
    const fs::path output =
        working_directory / "cli_check_template_schema_match_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(generated_output);
    remove_if_exists(output);

    create_cli_resolved_part_template_validation_fixture(source);

    write_binary_file(schema_file,
                      std::string{
                          "{\"targets\":["
                          "{\"part\":\"section-header\",\"section\":0,"
                          "\"kind\":\"default\",\"resolved_from_section\":0,"
                          "\"slots\":[{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                          "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"},"
                          "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                          "{\"part\":\"section-footer\",\"section\":0,"
                          "\"kind\":\"default\",\"resolved_from_section\":0,"
                          "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]},"
                          "{\"part\":\"section-header\",\"section\":1,"
                          "\"kind\":\"default\",\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                          "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"},"
                          "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                          "{\"part\":\"section-footer\",\"section\":1,"
                          "\"kind\":\"default\",\"resolved_from_section\":0,"
                          "\"linked_to_previous\":true,"
                          "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                          "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"check-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--resolved-section-targets",
                      "--output",
                      generated_output.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"check-template-schema\",\"matches\":true,"
                 "\"schema_file\":\"" +
                 json_escape_text(schema_file.string()) +
                 "\",\"generated_output_path\":\"" +
                 json_escape_text(generated_output.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":0,\"added_targets\":[],"
                 "\"removed_targets\":[],\"changed_targets\":[]}\n"});
    CHECK_EQ(read_text_file(generated_output),
             std::string{
                 "{\"targets\":["
                 "{\"part\":\"section-header\",\"section\":0,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"slots\":[{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"},"
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-header\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"header_note\",\"kind\":\"block\"},"
                 "{\"bookmark\":\"header_rows\",\"kind\":\"table_rows\"},"
                 "{\"bookmark\":\"header_title\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-footer\",\"section\":0,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]},"
                 "{\"part\":\"section-footer\",\"section\":1,"
                 "\"kind\":\"default\",\"resolved_from_section\":0,"
                 "\"linked_to_previous\":true,"
                 "\"slots\":[{\"bookmark\":\"footer_company\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"footer_summary\",\"kind\":\"text\"}]}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(generated_output);
    remove_if_exists(output);
}

TEST_CASE("cli check-template-schema fails when generated schema drifts from baseline") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_check_template_schema_drift_source.docx";
    const fs::path schema_file =
        working_directory / "cli_check_template_schema_drift_schema.json";
    const fs::path output =
        working_directory / "cli_check_template_schema_drift_output.json";

    remove_if_exists(source);
    remove_if_exists(schema_file);
    remove_if_exists(output);

    create_cli_body_template_validation_fixture(source);

    write_binary_file(schema_file,
                      std::string{
                          "{\"targets\":[{\"part\":\"body\",\"slots\":["
                          "{\"bookmark\":\"customer\",\"kind\":\"text\"},"
                          "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]}]}\n"});

    CHECK_EQ(run_cli({"check-template-schema",
                      source.string(),
                      "--schema-file",
                      schema_file.string(),
                      "--json"},
                     output),
             1);
    CHECK_EQ(read_text_file(output),
             std::string{
                 "{\"command\":\"check-template-schema\",\"matches\":false,"
                 "\"schema_file\":\"" +
                 json_escape_text(schema_file.string()) +
                 "\",\"added_target_count\":0,\"removed_target_count\":0,"
                 "\"changed_target_count\":1,\"added_targets\":[],"
                 "\"removed_targets\":[],\"changed_targets\":[{\"left\":"
                 "{\"part\":\"body\",\"slots\":[{\"bookmark\":\"customer\","
                 "\"kind\":\"text\"},{\"bookmark\":\"summary_block\","
                 "\"kind\":\"text\"}]},\"right\":{\"part\":\"body\",\"slots\":["
                 "{\"bookmark\":\"customer\",\"kind\":\"text\",\"count\":2},"
                 "{\"bookmark\":\"summary_block\",\"kind\":\"text\"}]}}]}\n"});

    remove_if_exists(source);
    remove_if_exists(schema_file);
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
