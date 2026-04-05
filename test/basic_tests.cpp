#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <featherdoc.hpp>

namespace {
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
