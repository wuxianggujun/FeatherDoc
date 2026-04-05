#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unordered_set>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <featherdoc.hpp>

namespace {
constexpr auto test_document_xml_entry = "word/document.xml";
constexpr auto test_relationships_xml_entry = "_rels/.rels";
constexpr auto test_content_types_xml_entry = "[Content_Types].xml";
constexpr auto test_relationships_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
constexpr auto test_content_types_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";

std::unordered_set<void *> tracked_pugi_allocations;
bool saw_unexpected_pugi_deallocation = false;

auto tracked_pugi_allocate(std::size_t size) -> void * {
    void *ptr = std::malloc(size);
    if (ptr != nullptr) {
        tracked_pugi_allocations.insert(ptr);
    }
    return ptr;
}

auto tracked_pugi_deallocate(void *ptr) -> void {
    if (ptr == nullptr) {
        return;
    }

    if (tracked_pugi_allocations.erase(ptr) == 0U) {
        saw_unexpected_pugi_deallocation = true;
    }

    std::free(ptr);
}

struct pugi_memory_management_guard final {
    pugi::allocation_function allocation{};
    pugi::deallocation_function deallocation{};

    pugi_memory_management_guard()
        : allocation(pugi::get_memory_allocation_function()),
          deallocation(pugi::get_memory_deallocation_function()) {}

    ~pugi_memory_management_guard() {
        pugi::set_memory_management_functions(this->allocation, this->deallocation);
        tracked_pugi_allocations.clear();
        saw_unexpected_pugi_deallocation = false;
    }
};

auto collect_document_text(featherdoc::Document &doc) -> std::string {
    std::ostringstream stream;
    for (featherdoc::Paragraph paragraph = doc.paragraphs(); paragraph.has_next();
         paragraph.next()) {
        for (featherdoc::Run run = paragraph.runs(); run.has_next(); run.next()) {
            stream << run.get_text();
        }
        stream << '\n';
    }
    return stream.str();
}

auto write_test_docx(const std::filesystem::path &path, const std::string &document_xml)
    -> void {
    struct archive_entry {
        std::string name;
        std::string content;
    };

    const std::vector<archive_entry> entries{
        {test_content_types_xml_entry, test_content_types_xml},
        {test_relationships_xml_entry, test_relationships_xml},
        {test_document_xml_entry, document_xml},
    };

    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    REQUIRE(zip != nullptr);

    for (const auto &entry : entries) {
        REQUIRE_EQ(zip_entry_open(zip, entry.name.c_str()), 0);
        REQUIRE_GE(zip_entry_write(zip, entry.content.data(), entry.content.size()), 0);
        REQUIRE_EQ(zip_entry_close(zip), 0);
    }

    zip_close(zip);
}

auto write_test_archive_entries(
    const std::filesystem::path &path,
    const std::vector<std::pair<std::string, std::string>> &entries) -> void {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    REQUIRE(zip != nullptr);

    for (const auto &[name, content] : entries) {
        REQUIRE_EQ(zip_entry_open(zip, name.c_str()), 0);
        REQUIRE_GE(zip_entry_write(zip, content.data(), content.size()), 0);
        REQUIRE_EQ(zip_entry_close(zip), 0);
    }

    zip_close(zip);
}

auto read_test_docx_entry(const std::filesystem::path &path, const char *entry_name)
    -> std::string {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                   &zip_error);
    CHECK(zip != nullptr);
    if (zip == nullptr) {
        return {};
    }
    CHECK_EQ(zip_entry_open(zip, entry_name), 0);

    void *buffer = nullptr;
    size_t buffer_size = 0;
    CHECK_GE(zip_entry_read(zip, &buffer, &buffer_size), 0);
    if (buffer == nullptr) {
        zip_entry_close(zip);
        zip_close(zip);
        return {};
    }

    std::string content(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);

    CHECK_EQ(zip_entry_close(zip), 0);
    zip_close(zip);
    return content;
}

auto test_docx_entry_exists(const std::filesystem::path &path, const char *entry_name)
    -> bool {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                   &zip_error);
    CHECK(zip != nullptr);
    if (zip == nullptr) {
        return false;
    }

    const bool exists = zip_entry_open(zip, entry_name) == 0;
    if (exists) {
        CHECK_EQ(zip_entry_close(zip), 0);
    }
    zip_close(zip);
    return exists;
}
} // namespace

TEST_CASE("checks contents of my_test.docx") {
    featherdoc::Document doc("my_test.docx");
    const auto open_error = doc.open();
    CHECK_FALSE(open_error);
    CHECK(doc.is_open());

    std::ostringstream ss;

    for (featherdoc::Paragraph p = doc.paragraphs(); p.has_next(); p.next()) {
        for (featherdoc::Run r = p.runs(); r.has_next(); r.next()) {
            ss << r.get_text() << std::endl;
        }
    }

    CHECK_EQ("This is a test\nokay?\n", ss.str());
}

TEST_CASE("open fails for missing file") {
    featherdoc::Document doc;
    doc.set_path("missing.docx");

    CHECK_EQ(doc.path().filename().string(), "missing.docx");
    const auto error = doc.open();
    CHECK_EQ(error, std::make_error_code(std::errc::no_such_file_or_directory));
    CHECK_EQ(doc.last_error().code, error);
    CHECK_FALSE(doc.last_error().detail.empty());
    CHECK_FALSE(doc.is_open());
}

TEST_CASE("open reports explicit errors for empty path and invalid archive") {
    namespace fs = std::filesystem;

    featherdoc::Document empty_doc;
    CHECK_EQ(empty_doc.open(), featherdoc::document_errc::empty_path);
    CHECK_EQ(empty_doc.last_error().code, featherdoc::document_errc::empty_path);
    CHECK_FALSE(empty_doc.last_error().detail.empty());

    const fs::path invalid_path = fs::current_path() / "invalid.docx";
    {
        std::ofstream invalid_file(invalid_path, std::ios::binary);
        invalid_file << "not a docx archive";
    }

    featherdoc::Document invalid_doc(invalid_path);
    CHECK_EQ(invalid_doc.open(), featherdoc::document_errc::archive_open_failed);
    CHECK_EQ(invalid_doc.last_error().code,
             featherdoc::document_errc::archive_open_failed);
    CHECK_FALSE(invalid_doc.last_error().detail.empty());
    CHECK_FALSE(invalid_doc.is_open());

    fs::remove(invalid_path);
}

TEST_CASE("open keeps zip buffers out of pugixml-owned deallocation paths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "allocator_boundary_regression.docx";
    fs::remove(target);

    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>allocator safety</w:t></w:r></w:p>
  </w:body>
</w:document>
)");

    pugi_memory_management_guard guard;
    tracked_pugi_allocations.clear();
    saw_unexpected_pugi_deallocation = false;
    pugi::set_memory_management_functions(tracked_pugi_allocate, tracked_pugi_deallocate);

    {
        featherdoc::Document doc(target);
        CHECK_FALSE(doc.open());
        CHECK(doc.is_open());
        CHECK_EQ(collect_document_text(doc), "allocator safety\n");
    }

    CHECK_FALSE(saw_unexpected_pugi_deallocation);
    CHECK(tracked_pugi_allocations.empty());

    fs::remove(target);
}

TEST_CASE("open reports encrypted docx files as unsupported") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "encrypted.docx";
    fs::remove(target);

    zip_t *zip = zip_open_with_password(target.string().c_str(),
                                        ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                        "secret");
    REQUIRE(zip != nullptr);

    REQUIRE_EQ(zip_entry_open(zip, test_content_types_xml_entry), 0);
    REQUIRE_GE(zip_entry_write(zip, test_content_types_xml,
                               std::strlen(test_content_types_xml)),
               0);
    REQUIRE_EQ(zip_entry_close(zip), 0);

    REQUIRE_EQ(zip_entry_open(zip, test_relationships_xml_entry), 0);
    REQUIRE_GE(zip_entry_write(zip, test_relationships_xml,
                               std::strlen(test_relationships_xml)),
               0);
    REQUIRE_EQ(zip_entry_close(zip), 0);

    constexpr auto encrypted_document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>secret</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    REQUIRE_EQ(zip_entry_open(zip, test_document_xml_entry), 0);
    REQUIRE_GE(zip_entry_write(zip, encrypted_document_xml,
                               std::strlen(encrypted_document_xml)),
               0);
    REQUIRE_EQ(zip_entry_close(zip), 0);
    zip_close(zip);

    featherdoc::Document doc(target);
    CHECK_EQ(doc.open(), featherdoc::document_errc::encrypted_document_unsupported);
    CHECK_EQ(doc.last_error().code,
             featherdoc::document_errc::encrypted_document_unsupported);
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);
    CHECK(doc.last_error().detail.find("encrypted") != std::string::npos);
    CHECK_FALSE(doc.is_open());

    fs::remove(target);
}

TEST_CASE("open reports a missing document XML entry for non-docx zip archives") {
    namespace fs = std::filesystem;

    const fs::path archive_path = fs::current_path() / "not_a_word_document.docx";
    fs::remove(archive_path);

    int zip_error = 0;
    zip_t *zip = zip_openwitherror(archive_path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    REQUIRE(zip != nullptr);

    constexpr auto placeholder_entry = "payload.txt";
    constexpr auto placeholder_text = "plain zip content";
    REQUIRE_EQ(zip_entry_open(zip, placeholder_entry), 0);
    REQUIRE_GE(zip_entry_write(zip, placeholder_text,
                               std::strlen(placeholder_text)),
               0);
    REQUIRE_EQ(zip_entry_close(zip), 0);
    zip_close(zip);

    featherdoc::Document doc(archive_path);
    CHECK_EQ(doc.open(), featherdoc::document_errc::document_xml_open_failed);
    CHECK_EQ(doc.last_error().code,
             featherdoc::document_errc::document_xml_open_failed);
    CHECK_EQ(doc.last_error().entry_name, "word/document.xml");
    CHECK_FALSE(doc.last_error().detail.empty());
    CHECK_FALSE(doc.is_open());

    fs::remove(archive_path);
}

TEST_CASE("open exposes malformed XML context") {
    namespace fs = std::filesystem;

    const fs::path malformed_path = fs::current_path() / "malformed.docx";
    const std::string malformed_xml = "<w:document><w:body>";

    int zip_error = 0;
    zip_t *zip = zip_openwitherror(malformed_path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    REQUIRE(zip != nullptr);
    REQUIRE_EQ(zip_entry_open(zip, "word/document.xml"), 0);
    REQUIRE_GE(zip_entry_write(zip, malformed_xml.data(), malformed_xml.size()), 0);
    REQUIRE_EQ(zip_entry_close(zip), 0);
    zip_close(zip);

    featherdoc::Document doc(malformed_path);
    CHECK_EQ(doc.open(), featherdoc::document_errc::document_xml_parse_failed);
    CHECK_EQ(doc.last_error().code,
             featherdoc::document_errc::document_xml_parse_failed);
    CHECK_EQ(doc.last_error().entry_name, "word/document.xml");
    CHECK(doc.last_error().xml_offset.has_value());
    CHECK_FALSE(doc.last_error().detail.empty());

    fs::remove(malformed_path);
}

TEST_CASE("can save changes to a copied document") {
    namespace fs = std::filesystem;

    const fs::path source = "my_test.docx";
    const fs::path target = fs::current_path() / "my_test_copy.docx";

    fs::copy_file(source, target, fs::copy_options::overwrite_existing);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    paragraph.add_run("!", featherdoc::formatting_flag::bold);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    CHECK_NE(collect_document_text(reopened).find('!'), std::string::npos);

    fs::remove(target);
}

TEST_CASE("save reports unopened document") {
    featherdoc::Document doc("my_test.docx");
    CHECK_EQ(doc.save(), featherdoc::document_errc::document_not_open);
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::document_not_open);
    CHECK_FALSE(doc.last_error().detail.empty());
}

TEST_CASE("save_as writes to a new path without mutating the source path") {
    namespace fs = std::filesystem;

    const fs::path source_seed = "my_test.docx";
    const fs::path source = fs::current_path() / "my_test_save_as_source.docx";
    const fs::path target = fs::current_path() / "my_test_save_as_target.docx";

    fs::copy_file(source_seed, source, fs::copy_options::overwrite_existing);
    fs::remove(target);

    featherdoc::Document doc(source);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    paragraph.add_run("!", featherdoc::formatting_flag::bold);

    CHECK_FALSE(doc.save_as(target));
    CHECK_EQ(doc.path(), source);
    CHECK_FALSE(doc.last_error());

    featherdoc::Document reopened_source(source);
    CHECK_FALSE(reopened_source.open());

    featherdoc::Document reopened_target(target);
    CHECK_FALSE(reopened_target.open());

    const auto source_text = collect_document_text(reopened_source);
    const auto target_text = collect_document_text(reopened_target);

    CHECK_EQ(source_text.find('!'), std::string::npos);
    CHECK_NE(target_text.find('!'), std::string::npos);

    fs::remove(source);
    fs::remove(target);
}

TEST_CASE("create_empty builds an editable document without a source archive") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "generated_empty.docx";
    fs::remove(target);

    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());
    CHECK(doc.is_open());
    CHECK_FALSE(doc.last_error());

    auto paragraph = doc.paragraphs();
    CHECK(paragraph.has_next());
    paragraph.add_run("Generated from scratch",
                      featherdoc::formatting_flag::bold |
                          featherdoc::formatting_flag::italic);

    CHECK_FALSE(doc.save_as(target));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "Generated from scratch\n");

    fs::remove(target);
}

TEST_CASE("create_empty supports save through the configured path") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "generated_empty_save.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    paragraph.add_run("Saved with save()");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "Saved with save()\n");

    fs::remove(target);
}

TEST_CASE("save_as rejects an empty output path") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.save_as({}), featherdoc::document_errc::empty_path);
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::empty_path);
    CHECK_FALSE(doc.last_error().detail.empty());
}

TEST_CASE("open and save work with an absolute path outside the build directory") {
    namespace fs = std::filesystem;

    const fs::path temp_root =
        fs::temp_directory_path() / "FeatherDoc absolute path regression";
    const fs::path nested_dir = temp_root / "nested docs";
    const fs::path target = nested_dir / "absolute-open-test.docx";

    fs::create_directories(nested_dir);
    fs::copy_file("my_test.docx", target, fs::copy_options::overwrite_existing);

    featherdoc::Document doc(fs::absolute(target));
    CHECK_FALSE(doc.open());
    CHECK(doc.is_open());

    auto paragraph = doc.paragraphs();
    CHECK(paragraph.has_next());
    paragraph.add_run(" [absolute path edit]");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(fs::absolute(target));
    CHECK_FALSE(reopened.open());
    CHECK_NE(collect_document_text(reopened).find("[absolute path edit]"),
             std::string::npos);

    fs::remove_all(temp_root);
}

#if defined(_WIN32)
TEST_CASE("open succeeds while another process keeps the docx writable but shareable") {
    namespace fs = std::filesystem;

    const fs::path temp_root =
        fs::temp_directory_path() / "FeatherDoc share mode regression";
    const fs::path target = temp_root / "opened-by-word.docx";

    std::error_code filesystem_error;
    fs::remove_all(temp_root, filesystem_error);
    filesystem_error.clear();
    fs::create_directories(temp_root, filesystem_error);
    REQUIRE_FALSE(filesystem_error);
    fs::copy_file("my_test.docx", target, fs::copy_options::overwrite_existing,
                  filesystem_error);
    REQUIRE_FALSE(filesystem_error);

    const HANDLE writer_handle =
        CreateFileW(target.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(writer_handle != INVALID_HANDLE_VALUE);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.is_open());

    CHECK(CloseHandle(writer_handle) != 0);
    fs::remove_all(temp_root, filesystem_error);
}
#endif

TEST_CASE("set_text preserves xml:space when leading or trailing spaces exist") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "preserve_spaces.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    doc.paragraphs().add_run("marker");
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto paragraph = reopened.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.runs();
    REQUIRE(run.has_next());
    CHECK(run.set_text("  padded text  "));
    CHECK_FALSE(reopened.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto text_node =
        xml_document.child("w:document").child("w:body").child("w:p").child("w:r").child("w:t");
    REQUIRE(text_node != pugi::xml_node{});
    CHECK_EQ(std::string{text_node.attribute("xml:space").value()}, "preserve");
    CHECK_EQ(std::string{text_node.text().get()}, "  padded text  ");

    CHECK(run.set_text("plain text"));
    CHECK_FALSE(reopened.save());

    const auto plain_xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document plain_xml_document;
    REQUIRE(plain_xml_document.load_string(plain_xml_text.c_str()));

    const auto plain_text_node = plain_xml_document.child("w:document")
                                     .child("w:body")
                                     .child("w:p")
                                     .child("w:r")
                                     .child("w:t");
    REQUIRE(plain_text_node != pugi::xml_node{});
    CHECK_FALSE(plain_text_node.attribute("xml:space"));
    CHECK_EQ(std::string{plain_text_node.text().get()}, "plain text");

    fs::remove(target);
}

TEST_CASE("add_run creates a top-level paragraph when the document body has none") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_only_body.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p>
            <w:r><w:t>cell text</w:t></w:r>
          </w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    CHECK_FALSE(paragraph.has_next());
    auto run = paragraph.add_run("appended after table");
    CHECK(run.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "appended after table\n");

    fs::remove(target);
}

TEST_CASE("paragraph iteration skips non-paragraph siblings and appends before sectPr") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_iteration_regression.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>first</w:t></w:r></w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p><w:r><w:t>table cell</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>second</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    std::size_t paragraph_count = 0;
    for (auto paragraph = doc.paragraphs(); paragraph.has_next(); paragraph.next()) {
        ++paragraph_count;
    }
    CHECK_EQ(paragraph_count, 2);

    auto paragraph = doc.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }
    paragraph.insert_paragraph_after("third");
    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\nw:p\nw:p\nw:sectPr\n");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "first\nsecond\nthird\n");

    fs::remove(target);
}

TEST_CASE("insert_paragraph_after saves cleanly from the document paragraph cursor") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_after_regression.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &paragraphs = doc.paragraphs();
    CHECK(paragraphs.has_next());

    const auto inserted =
        paragraphs.insert_paragraph_after("inserted after initial paragraph");
    CHECK(inserted.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened),
             "\ninserted after initial paragraph\n");

    fs::remove(target);
}

TEST_CASE("run iteration skips non-run siblings inside a paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_iteration_regression.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>alpha</w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="bookmark"/>
      <w:r><w:t>beta</w:t></w:r>
      <w:proofErr w:type="spellStart"/>
      <w:r><w:t>gamma</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    std::size_t run_count = 0;
    std::ostringstream text;
    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
        ++run_count;
        text << run.get_text();
    }

    CHECK_EQ(run_count, 3);
    CHECK_EQ(text.str(), "alphabetagamma");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_text rewrites bookmarked content and preserves markers") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix</w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="bookmark"/>
      <w:r><w:t>old</w:t></w:r>
      <w:proofErr w:type="spellStart"/>
      <w:r><w:t>content</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t>suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_text("bookmark", " updated value "), 1);
    CHECK_EQ(collect_document_text(doc), "prefix updated value suffix\n");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(xml_text.find("w:bookmarkStart"), std::string::npos);
    CHECK_NE(xml_text.find("w:bookmarkEnd"), std::string::npos);
    CHECK_NE(xml_text.find("updated value"), std::string::npos);
    CHECK_NE(xml_text.find("xml:space=\"preserve\""), std::string::npos);
    CHECK_EQ(xml_text.find("old"), std::string::npos);
    CHECK_EQ(xml_text.find("content"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "prefix updated value suffix\n");

    fs::remove(target);
}

TEST_CASE("existing header and footer paragraphs can be edited and saved") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_footer_access.docx";
    fs::remove(target);

    const std::string content_types_xml =
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

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
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

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>old header</w:t></w:r></w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>old footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
            {"word/footer1.xml", footer_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);

    auto header = doc.header_paragraphs();
    REQUIRE(header.has_next());
    auto header_run = header.runs();
    REQUIRE(header_run.has_next());
    CHECK(header_run.set_text("updated header"));

    auto footer = doc.footer_paragraphs();
    REQUIRE(footer.has_next());
    auto footer_run = footer.runs();
    REQUIRE(footer_run.has_next());
    CHECK(footer_run.set_text(" updated footer "));

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header.find("updated header"), std::string::npos);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("updated footer"), std::string::npos);
    CHECK_NE(saved_footer.find("xml:space=\"preserve\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);

    auto reopened_header = reopened.header_paragraphs();
    REQUIRE(reopened_header.has_next());
    CHECK_EQ(reopened_header.runs().get_text(), "updated header");

    auto reopened_footer = reopened.footer_paragraphs();
    REQUIRE(reopened_footer.has_next());
    CHECK_EQ(reopened_footer.runs().get_text(), " updated footer ");

    CHECK_FALSE(reopened.header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.footer_paragraphs(1).has_next());

    fs::remove(target);
}

TEST_CASE("ensure_header_paragraphs creates a default header for existing documents") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_header_existing.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body text</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 1);
    CHECK_EQ(doc.header_count(), 0);

    auto header = doc.ensure_header_paragraphs();
    REQUIRE(header.has_next());
    CHECK(header.add_run("generated header").has_next());
    CHECK_EQ(doc.header_count(), 1);

    auto same_header = doc.ensure_header_paragraphs();
    REQUIRE(same_header.has_next());
    CHECK_EQ(doc.header_count(), 1);

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("xmlns:r="), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("relationships/header"), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "body text\n");
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "generated header");

    fs::remove(target);
}

TEST_CASE("ensure_header_paragraphs and ensure_footer_paragraphs work for create_empty") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_header_footer_empty.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    CHECK_EQ(doc.section_count(), 1);

    auto header = doc.ensure_header_paragraphs();
    REQUIRE(header.has_next());
    CHECK(header.add_run("generated header").has_next());

    auto footer = doc.ensure_footer_paragraphs();
    REQUIRE(footer.has_next());
    CHECK(footer.add_run("generated footer").has_next());

    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("xmlns:r="), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:footerReference"), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "generated header");
    CHECK_EQ(reopened.footer_paragraphs().runs().get_text(), "generated footer");

    fs::remove(target);
}

TEST_CASE("section header and footer access resolves references per section") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "section_header_footer_access.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header3.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:headerReference w:type="even" r:id="rId5"/>
      <w:footerReference w:type="first" r:id="rId6"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header3.xml"/>
  <Relationship Id="rId6"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 default header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header3_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 even header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 first footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/header3.xml", header3_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.header_count(), 3);
    CHECK_EQ(doc.footer_count(), 2);

    auto section0_header = doc.section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK_EQ(section0_header.runs().get_text(), "section 1 header");

    auto section0_footer = doc.section_footer_paragraphs(0);
    REQUIRE(section0_footer.has_next());
    CHECK_EQ(section0_footer.runs().get_text(), "section 1 footer");

    auto section1_default_header = doc.section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK_EQ(section1_default_header.runs().get_text(), "section 2 default header");

    auto section1_even_header = doc.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page);
    REQUIRE(section1_even_header.has_next());
    CHECK_EQ(section1_even_header.runs().get_text(), "section 2 even header");

    auto section1_first_footer = doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK_EQ(section1_first_footer.runs().get_text(), "section 2 first footer");

    CHECK_FALSE(doc.section_header_paragraphs(
                    0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());

    CHECK(section1_default_header.runs().set_text("updated section 2 header"));
    CHECK(section1_even_header.runs().set_text("updated section 2 even"));
    CHECK(section1_first_footer.runs().set_text("updated section 2 first footer"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 2);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(),
             "updated section 2 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "updated section 2 even");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "updated section 2 first footer");

    fs::remove(target);
}

TEST_CASE("ensure section header and footer paragraphs create references for a single section") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "ensure_section_header_footer_single.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body text</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 1);
    CHECK_EQ(doc.header_count(), 0);
    CHECK_EQ(doc.footer_count(), 0);

    auto default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(default_header.has_next());
    CHECK(default_header.add_run("default header").has_next());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    auto same_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(same_even_header.has_next());
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_EQ(same_even_header.runs().get_text(), "even header");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto saved_section_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(saved_section_properties != pugi::xml_node{});
    CHECK_NE(saved_document_xml.find("<w:sectPr"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:headerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"default\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"even\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:footerReference"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"first\""), std::string::npos);
    CHECK(saved_section_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("relationships/settings"), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "default header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 0, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");

    fs::remove(target);
}

TEST_CASE("ensure section header and footer paragraphs create references per section") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "ensure_section_header_footer_multi.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr/>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 2);

    auto section0_footer = doc.ensure_section_footer_paragraphs(0);
    REQUIRE(section0_footer.has_next());
    CHECK(section0_footer.add_run("section 1 footer").has_next());

    auto section0_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(section0_even_header.has_next());
    CHECK(section0_even_header.add_run("section 1 even header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 2 first footer").has_next());

    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 2 default header").has_next());

    auto same_section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(same_section1_default_header.has_next());
    CHECK_EQ(same_section1_default_header.runs().get_text(), "section 2 default header");
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section0_properties = body.child("w:p").child("w:pPr").child("w:sectPr");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section0_properties != pugi::xml_node{});
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_NE(saved_document_xml.find("w:type=\"even\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"first\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:type=\"default\""), std::string::npos);
    CHECK(section0_properties.child("w:titlePg") == pugi::xml_node{});
    CHECK(section1_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 2);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 2);
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 1 even header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(),
             "section 2 default header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 2 first footer");

    fs::remove(target);
}

TEST_CASE("ensure even-page section headers preserve existing settings.xml content") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_section_even_settings_reuse.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body text</w:t></w:r></w:p>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="settings.xml"/>
</Relationships>
)";

    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:zoom w:percent="125"/>
</w:settings>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/settings.xml", settings_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    CHECK_FALSE(doc.save());

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    const auto settings_root = saved_settings.child("w:settings");
    REQUIRE(settings_root != pugi::xml_node{});
    CHECK(settings_root.child("w:zoom") != pugi::xml_node{});
    CHECK(settings_root.child("w:evenAndOddHeaders") != pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");

    fs::remove(target);
}

TEST_CASE("assign section header and footer paragraphs reuse existing parts") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "assign_section_header_footer_existing_parts.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);

    auto shared_default_header = doc.assign_section_header_paragraphs(1, 0);
    REQUIRE(shared_default_header.has_next());
    CHECK(shared_default_header.runs().set_text("shared header"));

    auto shared_even_header = doc.assign_section_header_paragraphs(
        1, 0, featherdoc::section_reference_kind::even_page);
    REQUIRE(shared_even_header.has_next());
    CHECK_EQ(shared_even_header.runs().get_text(), "shared header");

    auto shared_default_footer = doc.assign_section_footer_paragraphs(1, 0);
    REQUIRE(shared_default_footer.has_next());
    CHECK(shared_default_footer.runs().set_text("shared footer"));

    auto shared_first_footer = doc.assign_section_footer_paragraphs(
        1, 0, featherdoc::section_reference_kind::first_page);
    REQUIRE(shared_first_footer.has_next());
    CHECK_EQ(shared_first_footer.runs().get_text(), "shared footer");

    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section0_properties = body.child("w:p").child("w:pPr").child("w:sectPr");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section0_properties != pugi::xml_node{});
    REQUIRE(section1_properties != pugi::xml_node{});

    const auto find_reference_id =
        [](pugi::xml_node section_properties, const char *reference_name,
           const char *reference_type) -> std::string {
        for (auto reference = section_properties.child(reference_name);
             reference != pugi::xml_node{};
             reference = reference.next_sibling(reference_name)) {
            if (std::string_view{reference.attribute("w:type").value()} == reference_type) {
                return reference.attribute("r:id").value();
            }
        }
        return {};
    };

    CHECK_EQ(find_reference_id(section0_properties, "w:headerReference", "default"), "rId2");
    CHECK_EQ(find_reference_id(section1_properties, "w:headerReference", "default"), "rId2");
    CHECK_EQ(find_reference_id(section1_properties, "w:headerReference", "even"), "rId2");
    CHECK_EQ(find_reference_id(section0_properties, "w:footerReference", "default"), "rId3");
    CHECK_EQ(find_reference_id(section1_properties, "w:footerReference", "default"), "rId3");
    CHECK_EQ(find_reference_id(section1_properties, "w:footerReference", "first"), "rId3");
    CHECK(section1_properties.child("w:titlePg") != pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"settings.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/settings.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "shared header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "shared header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "shared header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "shared footer");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "shared footer");

    fs::remove(target);
}

TEST_CASE("remove section header and footer references prunes orphaned parts on save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "remove_section_header_footer_references.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.remove_section_header_reference(1));
    CHECK(doc.remove_section_footer_reference(1));
    CHECK_FALSE(doc.remove_section_header_reference(
        1, featherdoc::section_reference_kind::even_page));
    CHECK_FALSE(doc.last_error());
    CHECK_FALSE(doc.section_header_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type",
                                                         "default"),
             pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "default"),
             pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(1).has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_section_header_reference(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("removing first and even references cleans title page and settings flags") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "remove_section_first_even_reference_cleanup.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    CHECK(doc.remove_section_header_reference(
        0, featherdoc::section_reference_kind::even_page));
    CHECK(doc.remove_section_footer_reference(
        0, featherdoc::section_reference_kind::first_page));
    CHECK_FALSE(doc.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto section_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(section_properties != pugi::xml_node{});
    CHECK(section_properties.child("w:titlePg") == pugi::xml_node{});
    CHECK(section_properties.find_child_by_attribute("w:headerReference", "w:type", "even") ==
          pugi::xml_node{});
    CHECK(section_properties.find_child_by_attribute("w:footerReference", "w:type", "first") ==
          pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
          pugi::xml_node{});

    CHECK_FALSE(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer1.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 0);
    CHECK_EQ(reopened.footer_count(), 0);
    CHECK_FALSE(reopened.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());

    fs::remove(target);
}

TEST_CASE("remove header and footer parts updates counts and prunes archive output") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "remove_header_footer_parts.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_count(), 2);
    CHECK_EQ(doc.footer_count(), 2);
    CHECK(doc.remove_header_part(1));
    CHECK(doc.remove_footer_part(1));
    CHECK_EQ(doc.header_count(), 1);
    CHECK_EQ(doc.footer_count(), 1);
    CHECK_FALSE(doc.section_header_paragraphs(1).has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(1).has_next());
    CHECK_FALSE(doc.save());

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    const auto saved_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("/word/header1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/header2.xml"), std::string::npos);
    CHECK_NE(saved_content_types.find("/word/footer1.xml"), std::string::npos);
    CHECK_EQ(saved_content_types.find("/word/footer2.xml"), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(1).has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(1).has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_header_part(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("copying section header and footer references replaces target layout") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "copy_section_header_footer_refs.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:headerReference w:type="even" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
      <w:footerReference w:type="first" r:id="rId5"/>
      <w:titlePg w:val="1"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
  <Relationship Id="rId6"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="settings.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:evenAndOddHeaders w:val="1"/>
</w:settings>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
            {"word/settings.xml", settings_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.copy_section_header_references(0, 1));
    CHECK(doc.copy_section_footer_references(0, 1));
    CHECK_FALSE(doc.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto section1_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type",
                                                         "default")
                 .attribute("r:id")
                 .value(),
             std::string{"rId2"});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "default")
                 .attribute("r:id")
                 .value(),
             std::string{"rId3"});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "first"),
             pugi::xml_node{});
    CHECK(section1_properties.child("w:titlePg") == pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
          pugi::xml_node{});

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.copy_section_header_references(0, 1));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("append section can inherit or reset header and footer references") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(default_header.has_next());
    CHECK(default_header.add_run("default header").has_next());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    CHECK(doc.append_section());
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "default header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");

    CHECK(doc.append_section(false));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 3);

    CHECK(section_nodes[1].child("w:titlePg") != pugi::xml_node{});
    CHECK(section_nodes[2].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type",
                                                      "default"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:footerReference", "w:type",
                                                      "first"),
             pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 3);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "default header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");
    CHECK_FALSE(reopened.section_header_paragraphs(2).has_next());
    CHECK_FALSE(reopened.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.append_section());
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("insert section can split layout inheritance or reset references") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto section0_default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.add_run("section 0 header").has_next());

    auto section0_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(section0_even_header.has_next());
    CHECK(section0_even_header.add_run("section 0 even header").has_next());

    CHECK(doc.append_section(false));
    CHECK_EQ(doc.section_count(), 2);

    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 1 header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.insert_section(0));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 0 even header");
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK(doc.insert_section(1, false));
    CHECK_EQ(doc.section_count(), 4);
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(3).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 3, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK(doc.insert_section(3));
    CHECK_EQ(doc.section_count(), 5);
    CHECK_EQ(doc.section_header_paragraphs(4).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 4, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 5);

    CHECK(section_nodes[2].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type",
                                                      "default"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:footerReference", "w:type",
                                                      "first"),
             pugi::xml_node{});
    CHECK(section_nodes[3].child("w:titlePg") != pugi::xml_node{});
    CHECK(section_nodes[4].child("w:titlePg") != pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 5);
    CHECK_EQ(reopened.header_count(), 3);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 0 even header");
    CHECK_FALSE(reopened.section_header_paragraphs(2).has_next());
    CHECK_FALSE(reopened.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(reopened.section_header_paragraphs(3).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 3, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_EQ(reopened.section_header_paragraphs(4).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 4, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.insert_section(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("remove section merges boundaries and prunes orphaned header footer parts on save") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "remove_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto section0_default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.add_run("section 0 header").has_next());

    CHECK(doc.append_section(false));
    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 1 header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.append_section(false));
    auto section2_default_header = doc.ensure_section_header_paragraphs(2);
    REQUIRE(section2_default_header.has_next());
    CHECK(section2_default_header.add_run("section 2 header").has_next());

    auto section2_even_header = doc.ensure_section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page);
    REQUIRE(section2_even_header.has_next());
    CHECK(section2_even_header.add_run("section 2 even header").has_next());

    CHECK(doc.remove_section(1));
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    CHECK(doc.remove_section(1));
    CHECK_EQ(doc.section_count(), 1);
    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 0 header");
    CHECK_FALSE(doc.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.remove_section(0));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 1);
    CHECK(section_nodes[0].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[0].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[0].find_child_by_attribute("w:footerReference", "w:type", "first"),
             pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") ==
          pugi::xml_node{});

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header3.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer1.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 0);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 0 header");
    CHECK_FALSE(reopened.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_section(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("move section reorders body content and keeps header footer layouts attached") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "move_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.paragraphs().add_run("section 0 body").has_next());
    auto section0_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK(section0_header.add_run("section 0 header").has_next());

    CHECK(doc.append_section(false));

    auto append_body_paragraph = [](featherdoc::Document &document, const char *text) {
        auto paragraph = document.paragraphs();
        while (paragraph.has_next()) {
            paragraph.next();
        }

        const auto inserted = paragraph.insert_paragraph_after(text);
        REQUIRE(inserted.has_next());
    };

    append_body_paragraph(doc, "section 1 body");
    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.append_section(false));
    append_body_paragraph(doc, "section 2 body");

    auto section2_header = doc.ensure_section_header_paragraphs(2);
    REQUIRE(section2_header.has_next());
    CHECK(section2_header.add_run("section 2 header").has_next());

    auto section2_even_header = doc.ensure_section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page);
    REQUIRE(section2_even_header.has_next());
    CHECK(section2_even_header.add_run("section 2 even header").has_next());

    CHECK(doc.move_section(2, 0));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_FALSE(doc.move_section(3, 0));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto collect_non_empty_document_text = [](featherdoc::Document &document) {
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
    };

    CHECK_EQ(collect_non_empty_document_text(reopened),
             "section 2 body\nsection 0 body\nsection 1 body\n");
    CHECK_EQ(reopened.section_count(), 3);
    CHECK_EQ(reopened.header_count(), 3);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.move_section(0, 0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("table traversal exposes text stored inside table cells") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "table_text_access.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>outside</w:t></w:r></w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p><w:r><w:t>cell one</w:t></w:r></w:p>
          <w:p><w:r><w:t>cell two</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:p><w:r><w:t>cell three</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    std::ostringstream text;
    for (auto table = doc.tables(); table.has_next(); table.next()) {
        for (auto row = table.rows(); row.has_next(); row.next()) {
            for (auto cell = row.cells(); cell.has_next(); cell.next()) {
                for (auto paragraph = cell.paragraphs(); paragraph.has_next();
                     paragraph.next()) {
                    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                        text << run.get_text();
                    }
                    text << '\n';
                }
            }
        }
    }

    CHECK_EQ(text.str(), "cell one\ncell two\ncell three\n");

    fs::remove(target);
}
