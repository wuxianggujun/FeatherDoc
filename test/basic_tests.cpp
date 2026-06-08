#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <unordered_set>
#include <utility>
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
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"
#include <featherdoc.hpp>
#include <zip.h>

namespace {
template <class T, class = void>
inline constexpr bool supports_featherdoc_adl_begin_end = false;

template <class T>
inline constexpr bool supports_featherdoc_adl_begin_end<
    T, std::void_t<decltype(begin(std::declval<const T &>())),
                   decltype(end(std::declval<const T &>()))>> = true;

static_assert(supports_featherdoc_adl_begin_end<featherdoc::Paragraph>);
static_assert(!supports_featherdoc_adl_begin_end<featherdoc::template_schema>);

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

TEST_CASE("paragraph set_text replaces all runs in place") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_set_text.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    auto first_run = paragraph.add_run("first");
    REQUIRE(first_run.has_next());
    REQUIRE(paragraph.add_run(" second").has_next());
    REQUIRE(paragraph.add_run(" third").has_next());

    CHECK(paragraph.set_text("replaced paragraph"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "replaced paragraph\n");

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());

    std::size_t run_count = 0U;
    for (auto run = reopened_paragraph.runs(); run.has_next(); run.next()) {
        ++run_count;
    }
    CHECK_EQ(run_count, 1U);

    fs::remove(target);
}

TEST_CASE("run remove deletes the targeted run from a paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.set_text("seed"));

    auto first_run = paragraph.runs();
    REQUIRE(first_run.has_next());
    REQUIRE(first_run.set_text("left"));
    auto removed_run = paragraph.add_run(" middle");
    REQUIRE(removed_run.has_next());
    REQUIRE(paragraph.add_run(" right").has_next());

    CHECK(removed_run.remove());
    CHECK(removed_run.has_next());
    CHECK_EQ(removed_run.get_text(), " right");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "left right\n");

    fs::remove(target);
}

TEST_CASE("run insertions keep the anchor usable and preserve run order") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_insert_around_anchor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("anchor"));

    auto anchor = paragraph.runs();
    REQUIRE(anchor.has_next());

    auto inserted_before =
        anchor.insert_run_before("left ", featherdoc::formatting_flag::bold);
    REQUIRE(inserted_before.has_next());
    auto inserted_after = anchor.insert_run_after(" right");
    REQUIRE(inserted_after.has_next());

    CHECK(anchor.has_next());
    CHECK_EQ(anchor.get_text(), "anchor");
    CHECK_EQ(inserted_before.get_text(), "left ");
    CHECK_EQ(inserted_after.get_text(), " right");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "left anchor right\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "left ");
    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), " right");

    fs::remove(target);
}

TEST_CASE("insert_run_before saves cleanly from an empty paragraph run cursor") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_before_empty_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto runs = doc.paragraphs().runs();
    CHECK_FALSE(runs.has_next());

    auto inserted = runs.insert_run_before("created from empty run cursor");
    REQUIRE(inserted.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "created from empty run cursor\n");

    fs::remove(target);
}

TEST_CASE("insert_run_after appends cleanly from an exhausted run cursor") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_after_exhausted_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("seed"));

    auto runs = paragraph.runs();
    while (runs.has_next()) {
        runs.next();
    }

    auto inserted = runs.insert_run_after(" tail");
    REQUIRE(inserted.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "seed tail\n");

    fs::remove(target);
}

TEST_CASE("insert_run_like_before clones run properties and keeps the anchor usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_before_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto anchor = paragraph.add_run("anchor");
    REQUIRE(anchor.has_next());
    CHECK(doc.set_run_style(anchor, "Strong"));
    CHECK(anchor.set_font_family("Segoe UI"));
    CHECK(anchor.set_east_asia_font_family("Microsoft YaHei"));
    CHECK(anchor.set_language("en-US"));
    CHECK(anchor.set_east_asia_language("zh-CN"));
    CHECK(anchor.set_bidi_language("ar-SA"));
    CHECK(anchor.set_rtl());

    auto inserted = anchor.insert_run_like_before();
    REQUIRE(inserted.has_next());
    CHECK_EQ(inserted.get_text(), "");
    CHECK(inserted.set_text("before "));

    const auto inserted_font_family = inserted.font_family();
    REQUIRE(inserted_font_family.has_value());
    CHECK_EQ(*inserted_font_family, "Segoe UI");

    const auto inserted_east_asia_font = inserted.east_asia_font_family();
    REQUIRE(inserted_east_asia_font.has_value());
    CHECK_EQ(*inserted_east_asia_font, "Microsoft YaHei");

    const auto inserted_language = inserted.language();
    REQUIRE(inserted_language.has_value());
    CHECK_EQ(*inserted_language, "en-US");

    const auto inserted_east_asia_language = inserted.east_asia_language();
    REQUIRE(inserted_east_asia_language.has_value());
    CHECK_EQ(*inserted_east_asia_language, "zh-CN");

    const auto inserted_bidi_language = inserted.bidi_language();
    REQUIRE(inserted_bidi_language.has_value());
    CHECK_EQ(*inserted_bidi_language, "ar-SA");

    const auto inserted_rtl = inserted.rtl();
    REQUIRE(inserted_rtl.has_value());
    CHECK(*inserted_rtl);

    CHECK(anchor.has_next());
    CHECK_EQ(anchor.get_text(), "anchor");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:rStyle w:val=\"Strong\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:ascii=\"Segoe UI\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "w:eastAsia=\"Microsoft YaHei\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:val=\"en-US\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:eastAsia=\"zh-CN\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:bidi=\"ar-SA\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rtl w:val=\"1\""), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before anchor\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "before ");
    const auto reopened_inserted_language = reopened_runs.language();
    REQUIRE(reopened_inserted_language.has_value());
    CHECK_EQ(*reopened_inserted_language, "en-US");
    const auto reopened_inserted_rtl = reopened_runs.rtl();
    REQUIRE(reopened_inserted_rtl.has_value());
    CHECK(*reopened_inserted_rtl);

    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    const auto reopened_anchor_language = reopened_runs.language();
    REQUIRE(reopened_anchor_language.has_value());
    CHECK_EQ(*reopened_anchor_language, "en-US");
    const auto reopened_anchor_rtl = reopened_runs.rtl();
    REQUIRE(reopened_anchor_rtl.has_value());
    CHECK(*reopened_anchor_rtl);

    fs::remove(target);
}

TEST_CASE("insert_run_like_after clones run properties without copying anchor text") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_after_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto anchor = paragraph.add_run("anchor");
    REQUIRE(anchor.has_next());
    CHECK(doc.set_run_style(anchor, "Emphasis"));
    CHECK(anchor.set_language("en-US"));
    CHECK(anchor.set_rtl());

    auto inserted = anchor.insert_run_like_after();
    REQUIRE(inserted.has_next());
    CHECK_EQ(inserted.get_text(), "");
    CHECK(inserted.set_text(" after"));
    CHECK_EQ(anchor.get_text(), "anchor");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:rStyle w:val=\"Emphasis\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:t>anchor</w:t>"), 1U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "anchor after\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    const auto reopened_anchor_language = reopened_runs.language();
    REQUIRE(reopened_anchor_language.has_value());
    CHECK_EQ(*reopened_anchor_language, "en-US");

    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), " after");
    const auto reopened_inserted_language = reopened_runs.language();
    REQUIRE(reopened_inserted_language.has_value());
    CHECK_EQ(*reopened_inserted_language, "en-US");
    const auto reopened_inserted_rtl = reopened_runs.rtl();
    REQUIRE(reopened_inserted_rtl.has_value());
    CHECK(*reopened_inserted_rtl);

    fs::remove(target);
}

TEST_CASE("insert_run_like requires a live anchor run") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_invalid_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto empty_runs = doc.paragraphs().runs();
    CHECK_FALSE(empty_runs.has_next());

    auto inserted_before = empty_runs.insert_run_like_before();
    CHECK_FALSE(inserted_before.has_next());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("seed"));

    auto exhausted_runs = paragraph.runs();
    while (exhausted_runs.has_next()) {
        exhausted_runs.next();
    }

    auto inserted_after = exhausted_runs.insert_run_like_after();
    CHECK_FALSE(inserted_after.has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "seed\n");

    fs::remove(target);
}

TEST_CASE("paragraph remove deletes a middle body paragraph and keeps the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first = doc.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.set_text("first"));

    auto removed = first.insert_paragraph_after("second");
    REQUIRE(removed.has_next());
    auto third = removed.insert_paragraph_after("third");
    REQUIRE(third.has_next());

    CHECK(removed.remove());
    CHECK(removed.has_next());
    CHECK_EQ(removed.runs().get_text(), "third");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "first\nthird\n");

    fs::remove(target);
}

TEST_CASE("paragraph remove rejects removing the last body paragraph") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK_FALSE(paragraph.remove());
    CHECK(paragraph.has_next());
}

TEST_CASE("paragraph remove rejects removing the last paragraph in a table cell") {
    featherdoc::Document doc;
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    auto cell_paragraph = table.rows().cells().paragraphs();
    REQUIRE(cell_paragraph.has_next());
    CHECK_FALSE(cell_paragraph.remove());
    CHECK(cell_paragraph.has_next());
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

    CHECK(run.set_text("first line\nsecond line"));
    CHECK_FALSE(reopened.save());

    const auto multiline_xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document multiline_xml_document;
    REQUIRE(multiline_xml_document.load_string(multiline_xml_text.c_str()));

    const auto multiline_run =
        multiline_xml_document.child("w:document").child("w:body").child("w:p").child("w:r");
    REQUIRE(multiline_run != pugi::xml_node{});
    CHECK_EQ(std::string{multiline_run.child("w:t").text().get()}, "first line");
    const auto line_break = multiline_run.child("w:br");
    REQUIRE(line_break != pugi::xml_node{});
    const auto trailing_text = line_break.next_sibling();
    REQUIRE(trailing_text != pugi::xml_node{});
    CHECK_EQ(std::string_view{trailing_text.name()}, "w:t");
    CHECK_EQ(std::string{trailing_text.text().get()}, "second line");

    featherdoc::Document multiline_reopened(target);
    CHECK_FALSE(multiline_reopened.open());
    auto multiline_run_handle = multiline_reopened.paragraphs().runs();
    REQUIRE(multiline_run_handle.has_next());
    CHECK_EQ(multiline_run_handle.get_text(), "first line\nsecond line");

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

TEST_CASE("insert_paragraph_before prepends a body paragraph and keeps the anchor usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_before_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto anchor = doc.paragraphs();
    REQUIRE(anchor.has_next());
    CHECK(anchor.set_text("anchor"));

    auto inserted = anchor.insert_paragraph_before("before");
    REQUIRE(inserted.has_next());
    CHECK(inserted.add_run(" intro").has_next());
    CHECK(anchor.has_next());
    CHECK_EQ(anchor.runs().get_text(), "anchor");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before intro\nanchor\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:p"), 2U);
    CHECK_EQ(std::string_view{body_node.first_child().name()}, "w:p");

    fs::remove(target);
}

TEST_CASE("insert_paragraph_before works inside a table cell") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_before_cell.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto table = doc.append_table(1, 1);
    REQUIRE(table.has_next());

    auto anchor = table.rows().cells().paragraphs();
    REQUIRE(anchor.has_next());
    CHECK(anchor.set_text("cell anchor"));

    auto inserted = anchor.insert_paragraph_before("cell before");
    REQUIRE(inserted.has_next());
    CHECK(inserted.add_run(" intro").has_next());
    CHECK(anchor.has_next());
    CHECK_EQ(anchor.runs().get_text(), "cell anchor");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "cell before intro\ncell anchor\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto cell_node = xml_document.child("w:document")
                               .child("w:body")
                               .child("w:tbl")
                               .child("w:tr")
                               .child("w:tc");
    REQUIRE(cell_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(cell_node, "w:p"), 2U);

    fs::remove(target);
}

TEST_CASE("insert_paragraph_like_before clones paragraph properties and keeps the anchor usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_like_before_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto anchor = doc.paragraphs();
    REQUIRE(anchor.has_next());
    CHECK(doc.set_paragraph_style(anchor, "Heading1"));
    CHECK(anchor.set_bidi());
    CHECK(doc.set_paragraph_list(anchor, featherdoc::list_kind::bullet));
    CHECK(anchor.set_text("anchor"));

    auto inserted = anchor.insert_paragraph_like_before();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("before"));
    REQUIRE(inserted.bidi().has_value());
    CHECK(*inserted.bidi());
    CHECK(anchor.has_next());
    CHECK_EQ(anchor.runs().get_text(), "anchor");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:pStyle w:val=\"Heading1\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:bidi"), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:numPr>"), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nanchor\n");

    auto reopened_inserted = reopened.paragraphs();
    REQUIRE(reopened_inserted.has_next());
    REQUIRE(reopened_inserted.bidi().has_value());
    CHECK(*reopened_inserted.bidi());
    reopened_inserted.next();
    REQUIRE(reopened_inserted.has_next());
    REQUIRE(reopened_inserted.bidi().has_value());
    CHECK(*reopened_inserted.bidi());

    fs::remove(target);
}

TEST_CASE("insert_paragraph_like_after clones cell paragraph properties without copying body text") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_like_after_cell.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto anchor = doc.append_table(1, 1).rows().cells().paragraphs();
    REQUIRE(anchor.has_next());
    CHECK(doc.set_paragraph_style(anchor, "Heading2"));
    CHECK(anchor.set_bidi());
    CHECK(anchor.set_text("cell anchor"));

    auto inserted = anchor.insert_paragraph_like_after();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("cell like"));
    REQUIRE(inserted.bidi().has_value());
    CHECK(*inserted.bidi());

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:pStyle w:val=\"Heading2\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:bidi"), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "cell anchor\ncell like\n");

    fs::remove(target);
}

TEST_CASE("insert_paragraph_like strips section breaks from cloned paragraph properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_paragraph_like_sectpr.docx";
    fs::remove(target);

    const auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:pgSz w:w="12240" w:h="15840"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>anchor</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>tail</w:t></w:r>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto anchor = doc.paragraphs();
    REQUIRE(anchor.has_next());
    CHECK_EQ(anchor.runs().get_text(), "anchor");

    auto inserted = anchor.insert_paragraph_like_before();
    REQUIRE(inserted.has_next());
    CHECK(inserted.set_text("before"));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:sectPr"), 2U);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto body = xml_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    const auto inserted_paragraph = body.child("w:p");
    REQUIRE(inserted_paragraph != pugi::xml_node{});
    CHECK_EQ(inserted_paragraph.child("w:pPr").child("w:sectPr"), pugi::xml_node{});

    const auto anchor_paragraph = inserted_paragraph.next_sibling("w:p");
    REQUIRE(anchor_paragraph != pugi::xml_node{});
    CHECK_NE(anchor_paragraph.child("w:pPr").child("w:sectPr"), pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nanchor\ntail\n");

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

TEST_CASE("body template part can append and edit tables") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "body_template_tables.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body intro"));

    auto first_table = body_template.append_table(1, 2);
    REQUIRE(first_table.has_next());
    auto first_cell = first_table.rows().cells();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_text("body-a"));
    first_cell.next();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_text("body-b"));

    auto removable_table = body_template.append_table(1, 1);
    REQUIRE(removable_table.has_next());
    CHECK(removable_table.rows().cells().set_text("temporary table"));

    CHECK(removable_table.remove());
    CHECK(removable_table.has_next());
    CHECK_EQ(removable_table.rows().cells().get_text(), "body-a");
    CHECK(removable_table.rows().cells().set_text("body-a-updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    CHECK_EQ(collect_template_part_text(reopened_body), "body intro\n");
    CHECK_EQ(collect_template_part_table_text(reopened_body), "body-a-updated\nbody-b\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 1U);

    fs::remove(target);
}

TEST_CASE("body template part can append paragraphs before section properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "body_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("header"));

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body intro"));

    auto appended = body_template.append_paragraph("body appended");
    REQUIRE(appended.has_next());
    CHECK(appended.add_run(" tail").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    CHECK_EQ(collect_template_part_text(reopened_body), "body intro\nbody appended tail\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:p"), 2U);
    CHECK_EQ(std::string_view{body_node.last_child().name()}, "w:sectPr");

    fs::remove(target);
}

TEST_CASE("header template part can append paragraphs and keep the returned paragraph editable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto appended = header_template.append_paragraph("Header appended");
    REQUIRE(appended.has_next());
    CHECK(appended.add_run(" tail").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\nHeader appended tail\n");

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(header_xml.c_str()));
    const auto header_node = xml_document.child("w:hdr");
    REQUIRE(header_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:p"), 2U);

    fs::remove(target);
}

TEST_CASE("footer template part can append an empty paragraph and populate it later") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "footer_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &footer = doc.ensure_section_footer_paragraphs(0);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Footer intro"));

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));

    auto appended = footer_template.append_paragraph();
    REQUIRE(appended.has_next());
    CHECK(appended.add_run("Footer appended").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(collect_template_part_text(footer_template), "Footer intro\nFooter appended\n");

    const auto footer_xml = read_test_docx_entry(target, "word/footer1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(footer_xml.c_str()));
    const auto footer_node = xml_document.child("w:ftr");
    REQUIRE(footer_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(footer_node, "w:p"), 2U);

    fs::remove(target);
}

TEST_CASE("header and footer template parts can append page number fields") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_page_number_fields.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &header = doc.ensure_section_header_paragraphs(0);
    auto &footer = doc.ensure_section_footer_paragraphs(0);
    REQUIRE(header.has_next());
    REQUIRE(footer.has_next());

    auto header_template = doc.section_header_template(0);
    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(header_template));
    REQUIRE(static_cast<bool>(footer_template));

    CHECK(header_template.append_page_number_field());
    CHECK(footer_template.append_total_pages_field());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    REQUIRE(static_cast<bool>(reopened.section_header_template(0)));
    REQUIRE(static_cast<bool>(reopened.section_footer_template(0)));

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    const auto footer_xml = read_test_docx_entry(target, "word/footer1.xml");

    pugi::xml_document header_document;
    pugi::xml_document footer_document;
    REQUIRE(header_document.load_string(header_xml.c_str()));
    REQUIRE(footer_document.load_string(footer_xml.c_str()));

    const auto header_node = header_document.child("w:hdr");
    const auto footer_node = footer_document.child("w:ftr");
    REQUIRE(header_node != pugi::xml_node{});
    REQUIRE(footer_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:p"), 1U);
    CHECK_EQ(count_named_children(footer_node, "w:p"), 1U);

    const auto header_field = header_node.child("w:p").child("w:fldSimple");
    const auto footer_field = footer_node.child("w:p").child("w:fldSimple");
    REQUIRE(header_field != pugi::xml_node{});
    REQUIRE(footer_field != pugi::xml_node{});
    CHECK_EQ(std::string_view{header_field.attribute("w:instr").value()}, " PAGE ");
    CHECK_EQ(std::string_view{footer_field.attribute("w:instr").value()},
             " NUMPAGES ");
    CHECK_EQ(std::string_view{header_field.child("w:r").child("w:t").text().get()},
             "1");
    CHECK_EQ(std::string_view{footer_field.child("w:r").child("w:t").text().get()},
             "1");

    fs::remove(target);
}

TEST_CASE("template parts can append inspect and replace TOC REF and SEQ fields") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_generic_fields.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    REQUIRE(body_template.paragraphs().has_next());
    CHECK(body_template.paragraphs().set_text("Field API fixture"));

    auto toc_options = featherdoc::table_of_contents_field_options{};
    toc_options.min_outline_level = 1U;
    toc_options.max_outline_level = 2U;
    CHECK(body_template.append_table_of_contents_field(toc_options, "TOC placeholder"));
    CHECK(body_template.append_reference_field("target_bookmark", {}, "Referenced heading"));

    auto page_reference_options = featherdoc::page_reference_field_options{};
    page_reference_options.relative_position = true;
    CHECK(body_template.append_page_reference_field("target_bookmark",
                                                   page_reference_options, "3"));

    auto style_reference_options = featherdoc::style_reference_field_options{};
    style_reference_options.paragraph_number = true;
    style_reference_options.relative_position = true;
    CHECK(body_template.append_style_reference_field(
        "Heading 1", style_reference_options, "Section heading"));

    CHECK(body_template.append_document_property_field("Title", {}, "FeatherDoc"));

    auto date_options = featherdoc::date_field_options{};
    date_options.format = "yyyy-MM-dd";
    date_options.state.dirty = true;
    CHECK(body_template.append_date_field(date_options, "2026-05-01"));

    auto hyperlink_options = featherdoc::hyperlink_field_options{};
    hyperlink_options.anchor = "target_heading";
    hyperlink_options.tooltip = "Open target heading";
    hyperlink_options.state.locked = true;
    CHECK(body_template.append_hyperlink_field(
        "https://example.com/report", hyperlink_options, "Open report"));

    auto sequence_options = featherdoc::sequence_field_options{};
    sequence_options.restart_value = 1U;
    CHECK(body_template.append_sequence_field("Figure", sequence_options, "1"));

    auto fields = body_template.list_fields();
    REQUIRE(fields.size() == 8U);
    CHECK_EQ(fields[0].kind, featherdoc::field_kind::table_of_contents);
    CHECK_EQ(fields[0].instruction, " TOC \\o \"1-2\" \\h \\z \\u ");
    CHECK_EQ(fields[0].result_text, "TOC placeholder");
    CHECK_EQ(fields[1].kind, featherdoc::field_kind::reference);
    CHECK_EQ(fields[1].instruction, " REF target_bookmark \\h \\* MERGEFORMAT ");
    CHECK_EQ(fields[1].result_text, "Referenced heading");
    CHECK_EQ(fields[2].kind, featherdoc::field_kind::page_reference);
    CHECK_EQ(fields[2].instruction,
             " PAGEREF target_bookmark \\h \\p \\* MERGEFORMAT ");
    CHECK_EQ(fields[2].result_text, "3");
    CHECK_EQ(fields[3].kind, featherdoc::field_kind::style_reference);
    CHECK_EQ(fields[3].instruction,
             " STYLEREF \"Heading 1\" \\n \\p \\* MERGEFORMAT ");
    CHECK_EQ(fields[3].result_text, "Section heading");
    CHECK_EQ(fields[4].kind, featherdoc::field_kind::document_property);
    CHECK_EQ(fields[4].instruction, " DOCPROPERTY Title \\* MERGEFORMAT ");
    CHECK_EQ(fields[4].result_text, "FeatherDoc");
    CHECK_EQ(fields[5].kind, featherdoc::field_kind::date);
    CHECK_EQ(fields[5].instruction, " DATE \\@ \"yyyy-MM-dd\" \\* MERGEFORMAT ");
    CHECK_EQ(fields[5].result_text, "2026-05-01");
    CHECK(fields[5].dirty);
    CHECK_FALSE(fields[5].locked);
    CHECK_EQ(fields[6].kind, featherdoc::field_kind::hyperlink);
    CHECK_EQ(fields[6].instruction,
             " HYPERLINK \"https://example.com/report\" \\l \"target_heading\" "
             "\\o \"Open target heading\" \\* MERGEFORMAT ");
    CHECK_EQ(fields[6].result_text, "Open report");
    CHECK_FALSE(fields[6].dirty);
    CHECK(fields[6].locked);
    CHECK_EQ(fields[7].kind, featherdoc::field_kind::sequence);
    CHECK_EQ(fields[7].instruction, " SEQ Figure \\* ARABIC \\r 1 \\* MERGEFORMAT ");

    CHECK(body_template.replace_field(7U, " SEQ Table \\* ARABIC \\r 1 ", "1"));
    fields = body_template.list_fields();
    REQUIRE(fields.size() == 8U);
    CHECK_EQ(fields[7].kind, featherdoc::field_kind::sequence);
    CHECK_EQ(fields[7].instruction, " SEQ Table \\* ARABIC \\r 1 ");

    CHECK_FALSE(doc.save());

    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));
    const auto body = xml_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});
    CHECK_EQ(count_named_descendants(body, "w:fldSimple"), 8U);
    CHECK_NE(document_xml.find("TOC \\o &quot;1-2&quot; \\h \\z \\u"),
             std::string::npos);
    CHECK_NE(document_xml.find("REF target_bookmark \\h \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("PAGEREF target_bookmark \\h \\p \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("STYLEREF &quot;Heading 1&quot; \\n \\p \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("DOCPROPERTY Title \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("DATE \\@ &quot;yyyy-MM-dd&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("HYPERLINK &quot;https://example.com/report&quot; "
                               "\\l &quot;target_heading&quot; "
                               "\\o &quot;Open target heading&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("SEQ Table \\* ARABIC \\r 1"),
             std::string::npos);
    CHECK_EQ(document_xml.find("SEQ Figure"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("template part generic fields validate options and indexes") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_generic_fields_errors.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    CHECK_FALSE(body_template.append_field(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "field instruction must not be empty");

    auto toc_options = featherdoc::table_of_contents_field_options{};
    toc_options.min_outline_level = 3U;
    toc_options.max_outline_level = 2U;
    CHECK_FALSE(body_template.append_table_of_contents_field(toc_options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("TOC outline levels"), std::string::npos);

    CHECK_FALSE(body_template.append_reference_field("bad name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("REF bookmark name"), std::string::npos);

    CHECK_FALSE(body_template.append_sequence_field("bad name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("SEQ identifier"), std::string::npos);

    CHECK_FALSE(body_template.append_page_reference_field("bad name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("PAGEREF bookmark name"),
             std::string::npos);

    CHECK_FALSE(body_template.append_style_reference_field("bad\"name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("STYLEREF style name"),
             std::string::npos);

    CHECK_FALSE(body_template.append_document_property_field("bad\\name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("DOCPROPERTY property name"),
             std::string::npos);

    auto bad_date_options = featherdoc::date_field_options{};
    bad_date_options.format = "bad\\format";
    CHECK_FALSE(body_template.append_date_field(bad_date_options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("DATE format"), std::string::npos);

    CHECK_FALSE(body_template.append_hyperlink_field("bad\\target"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("HYPERLINK target"),
             std::string::npos);

    auto bad_hyperlink_options = featherdoc::hyperlink_field_options{};
    bad_hyperlink_options.anchor = "bad\\anchor";
    CHECK_FALSE(body_template.append_hyperlink_field("https://example.com",
                                                     bad_hyperlink_options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("HYPERLINK anchor"),
             std::string::npos);

    auto author_state = featherdoc::field_state_options{};
    author_state.dirty = true;
    author_state.locked = true;
    CHECK(body_template.append_field(" AUTHOR ", "Author Name", author_state));
    auto fields = body_template.list_fields();
    REQUIRE(fields.size() == 1U);
    CHECK_EQ(fields.front().kind, featherdoc::field_kind::custom);
    CHECK(fields.front().dirty);
    CHECK(fields.front().locked);
    CHECK_FALSE(body_template.replace_field(9U, " PAGE ", "1"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "field index is out of range");

    fs::remove(target);
}


TEST_CASE("template parts can append and inspect complex nested fields") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "template_part_complex_nested_fields.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto outer_options = featherdoc::complex_field_options{};
    outer_options.state.dirty = true;
    outer_options.state.locked = true;
    CHECK(body_template.append_complex_field(
        " IF 1 = 1 \"Yes\" \"No\" ", "Yes", outer_options));

    CHECK(body_template.append_complex_field(
        {featherdoc::complex_field_text_fragment(" IF "),
         featherdoc::complex_field_nested_fragment(
             " MERGEFIELD CustomerName ", "Ada"),
         featherdoc::complex_field_text_fragment(
             " = \"Ada\" \"Matched\" \"Other\" ")},
        "Matched"));

    auto fields = body_template.list_fields();
    REQUIRE(fields.size() == 3U);
    CHECK(fields[0].complex);
    CHECK_EQ(fields[0].depth, 0U);
    CHECK_EQ(fields[0].instruction, " IF 1 = 1 \"Yes\" \"No\" ");
    CHECK_EQ(fields[0].result_text, "Yes");
    CHECK(fields[0].dirty);
    CHECK(fields[0].locked);

    CHECK(fields[1].complex);
    CHECK_EQ(fields[1].depth, 0U);
    CHECK_EQ(fields[1].instruction, " IF  = \"Ada\" \"Matched\" \"Other\" ");
    CHECK_EQ(fields[1].result_text, "Matched");

    CHECK(fields[2].complex);
    CHECK_EQ(fields[2].depth, 1U);
    CHECK_EQ(fields[2].instruction, " MERGEFIELD CustomerName ");
    CHECK_EQ(fields[2].result_text, "Ada");

    CHECK_FALSE(body_template.replace_field(0U, " AUTHOR ", "Ada"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "field index refers to a complex field");

    CHECK_FALSE(doc.save());

    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:fldCharType=\"begin\""), 3U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:fldCharType=\"separate\""), 3U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:fldCharType=\"end\""), 3U);
    CHECK_NE(document_xml.find("<w:instrText xml:space=\"preserve\"> IF "),
             std::string::npos);
    CHECK_NE(document_xml.find("MERGEFIELD CustomerName"), std::string::npos);
    CHECK_NE(document_xml.find("Matched"), std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_fields = reopened.body_template().list_fields();
    REQUIRE(reopened_fields.size() == 3U);
    CHECK(reopened_fields[1].complex);
    CHECK_EQ(reopened_fields[2].depth, 1U);

    fs::remove(target);
}

TEST_CASE("template parts can append captions and index fields") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_caption_index_fields.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto caption_options = featherdoc::caption_field_options{};
    caption_options.restart_value = 1U;
    caption_options.state.dirty = true;
    CHECK(body_template.append_caption("Figure", "Architecture overview",
                                       caption_options, "1"));

    auto index_entry_options = featherdoc::index_entry_field_options{};
    index_entry_options.subentry = "API";
    index_entry_options.bookmark_name = "target_bookmark";
    index_entry_options.cross_reference = "See API";
    index_entry_options.bold_page_number = true;
    index_entry_options.italic_page_number = true;
    index_entry_options.state.locked = true;
    CHECK(body_template.append_index_entry_field("FeatherDoc",
                                                index_entry_options));

    auto index_options = featherdoc::index_field_options{};
    index_options.columns = 2U;
    CHECK(body_template.append_index_field(index_options, "Index placeholder"));

    auto fields = body_template.list_fields();
    REQUIRE(fields.size() == 3U);
    CHECK_EQ(fields[0].kind, featherdoc::field_kind::sequence);
    CHECK_EQ(fields[0].instruction, " SEQ Figure \\* ARABIC \\r 1 \\* MERGEFORMAT ");
    CHECK_EQ(fields[0].result_text, "1");
    CHECK(fields[0].dirty);
    CHECK_FALSE(fields[0].locked);
    CHECK_EQ(fields[1].kind, featherdoc::field_kind::index_entry);
    CHECK_EQ(fields[1].instruction,
             " XE \"FeatherDoc:API\" \\r target_bookmark \\t \"See API\" \\b \\i ");
    CHECK(fields[1].result_text.empty());
    CHECK_FALSE(fields[1].dirty);
    CHECK(fields[1].locked);
    CHECK_EQ(fields[2].kind, featherdoc::field_kind::index);
    CHECK_EQ(fields[2].instruction, " INDEX \\c \"2\" \\* MERGEFORMAT ");
    CHECK_EQ(fields[2].result_text, "Index placeholder");

    CHECK_FALSE(body_template.append_caption("bad label", "Invalid"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("SEQ identifier"), std::string::npos);

    auto bad_index_options = featherdoc::index_field_options{};
    bad_index_options.columns = 0U;
    CHECK_FALSE(body_template.append_index_field(bad_index_options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("INDEX columns"), std::string::npos);

    CHECK_FALSE(body_template.append_index_entry_field("bad\\entry"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("XE entry text"), std::string::npos);

    CHECK_FALSE(doc.save());

    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(document_xml.find("Figure"), std::string::npos);
    CHECK_NE(document_xml.find("Architecture overview"), std::string::npos);
    CHECK_NE(document_xml.find("SEQ Figure \\* ARABIC \\r 1 \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("XE &quot;FeatherDoc:API&quot; \\r target_bookmark "
                               "\\t &quot;See API&quot; \\b \\i"),
             std::string::npos);
    CHECK_NE(document_xml.find("INDEX \\c &quot;2&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    fs::remove(target);
}

TEST_CASE("document and template part can inspect append replace and remove OMML") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "omml_inspect_append_replace_remove.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math">
  <w:body>
    <w:p><w:r><w:t>Before equation </w:t></w:r><m:oMath><m:r><m:t>x+1</m:t></m:r></m:oMath></w:p>
    <m:oMathPara><m:oMath><m:r><m:t>y=2</m:t></m:r></m:oMath></m:oMathPara>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto summaries = doc.list_omml();
    REQUIRE(summaries.size() == 2U);
    CHECK_FALSE(summaries[0].display);
    CHECK_EQ(summaries[0].text, "x+1");
    CHECK(summaries[0].xml.find("m:oMath") != std::string::npos);
    CHECK(summaries[1].display);
    CHECK_EQ(summaries[1].text, "y=2");

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_omml(
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>z=3</m:t></m:r></m:oMath>)"));
    CHECK(doc.replace_omml(
        1U,
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>y=4</m:t></m:r></m:oMath>)"));
    CHECK(doc.remove_omml(0U));

    summaries = body_template.list_omml();
    REQUIRE(summaries.size() == 2U);
    CHECK(summaries[0].display);
    CHECK_EQ(summaries[0].text, "y=4");
    CHECK_FALSE(summaries[1].display);
    CHECK_EQ(summaries[1].text, "z=3");

    CHECK_FALSE(doc.save());

    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\""),
             std::string::npos);
    CHECK_EQ(saved_xml.find("x+1"), std::string::npos);
    CHECK_NE(saved_xml.find("y=4"), std::string::npos);
    CHECK_NE(saved_xml.find("z=3"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_summaries = reopened.list_omml();
    REQUIRE(reopened_summaries.size() == 2U);
    CHECK_EQ(reopened_summaries[0].text, "y=4");
    CHECK_EQ(reopened_summaries[1].text, "z=3");

    fs::remove(target);
}

TEST_CASE("OMML APIs validate fragments indexes and replacement shape") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "omml_validation_errors.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    CHECK_FALSE(body_template.append_omml(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "OMML XML must not be empty");

    CHECK_FALSE(body_template.append_omml("<w:p/>"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "OMML XML root must be m:oMath or m:oMathPara");

    CHECK_FALSE(body_template.replace_omml(
        7U,
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>a</m:t></m:r></m:oMath>)"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "OMML index is out of range");

    CHECK(body_template.append_omml(
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>a</m:t></m:r></m:oMath>)"));
    CHECK_FALSE(body_template.replace_omml(
        0U,
        R"(<m:oMathPara xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:oMath><m:r><m:t>b</m:t></m:r></m:oMath></m:oMathPara>)"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "display OMML cannot replace an inline OMML target");

    CHECK_FALSE(doc.remove_omml(9U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "OMML index is out of range");

    fs::remove(target);
}

TEST_CASE("document can inspect footnotes endnotes comments and revisions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_notes_and_revisions.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/footnotes.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"/>
  <Override PartName="/word/endnotes.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"/>
  <Override PartName="/word/comments.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"/>
</Types>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rFootnotes" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes" Target="footnotes.xml"/>
  <Relationship Id="rEndnotes" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes" Target="endnotes.xml"/>
  <Relationship Id="rComments" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments" Target="comments.xml"/>
</Relationships>
)";
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Body with footnote</w:t></w:r><w:r><w:footnoteReference w:id="2"/></w:r></w:p>
    <w:p><w:r><w:t>Body with endnote</w:t></w:r><w:r><w:endnoteReference w:id="3"/></w:r></w:p>
    <w:p>
      <w:commentRangeStart w:id="4"/>
      <w:r><w:t>Commented text</w:t></w:r>
      <w:commentRangeEnd w:id="4"/>
      <w:r><w:commentReference w:id="4"/></w:r>
    </w:p>
    <w:p>
      <w:ins w:id="5" w:author="Ada" w:date="2026-05-01T10:00:00Z"><w:r><w:t>Inserted text</w:t></w:r></w:ins>
      <w:del w:id="6" w:author="Grace" w:date="2026-05-01T11:00:00Z"><w:r><w:delText>Deleted text</w:delText></w:r></w:del>
    </w:p>
  </w:body>
</w:document>
)";
    const std::string footnotes_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="2"><w:p><w:r><w:t>Footnote body text</w:t></w:r></w:p></w:footnote>
</w:footnotes>
)";
    const std::string endnotes_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:endnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:endnote w:id="3"><w:p><w:r><w:t>Endnote body text</w:t></w:r></w:p></w:endnote>
</w:endnotes>
)";
    const std::string comments_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-05-01T12:00:00Z"><w:p><w:r><w:t>Comment body text</w:t></w:r></w:p></w:comment>
</w:comments>
)";
    write_test_archive_entries(
        target,
        {{test_content_types_xml_entry, content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"word/_rels/document.xml.rels", document_relationships_xml},
         {"word/footnotes.xml", footnotes_xml},
         {"word/endnotes.xml", endnotes_xml},
         {"word/comments.xml", comments_xml}});

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto footnotes = doc.list_footnotes();
    REQUIRE(footnotes.size() == 1U);
    CHECK_EQ(footnotes[0].kind, featherdoc::review_note_kind::footnote);
    CHECK_EQ(footnotes[0].id, "2");
    CHECK_EQ(footnotes[0].text, "Footnote body text");

    const auto endnotes = doc.list_endnotes();
    REQUIRE(endnotes.size() == 1U);
    CHECK_EQ(endnotes[0].kind, featherdoc::review_note_kind::endnote);
    CHECK_EQ(endnotes[0].id, "3");
    CHECK_EQ(endnotes[0].text, "Endnote body text");

    const auto comments = doc.list_comments();
    REQUIRE(comments.size() == 1U);
    CHECK_EQ(comments[0].kind, featherdoc::review_note_kind::comment);
    CHECK_EQ(comments[0].id, "4");
    REQUIRE(comments[0].author.has_value());
    CHECK_EQ(*comments[0].author, "Reviewer");
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Commented text");
    CHECK_EQ(comments[0].text, "Comment body text");

    const auto revisions = doc.list_revisions();
    REQUIRE(revisions.size() == 2U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].id, "5");
    CHECK_EQ(revisions[0].text, "Inserted text");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[1].id, "6");
    CHECK_EQ(revisions[1].text, "Deleted text");

    fs::remove(target);
}

TEST_CASE("document review inspection returns empty lists when optional parts are absent") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_notes_absent.docx";
    fs::remove(target);

    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>No review metadata</w:t></w:r></w:p></w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.list_footnotes().empty());
    CHECK_FALSE(doc.last_error());
    CHECK(doc.list_endnotes().empty());
    CHECK_FALSE(doc.last_error());
    CHECK(doc.list_comments().empty());
    CHECK_FALSE(doc.last_error());
    CHECK(doc.list_revisions().empty());
    CHECK_FALSE(doc.last_error());

    fs::remove(target);
}

TEST_CASE("template part page number fields report unavailable parts") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "template_part_page_number_fields_error.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto missing_header_template = doc.section_header_template(1);
    CHECK_FALSE(static_cast<bool>(missing_header_template));
    CHECK_FALSE(missing_header_template.append_page_number_field());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "template part is not available");

    auto missing_footer_template = doc.section_footer_template(1);
    CHECK_FALSE(static_cast<bool>(missing_footer_template));
    CHECK_FALSE(missing_footer_template.append_total_pages_field());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "template part is not available");

    fs::remove(target);
}

TEST_CASE("header template part tables can remove a middle table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_table_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto header_paragraph = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto first_table = header_template.append_table(2, 2);
    REQUIRE(first_table.has_next());
    auto row = first_table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Section"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Status"));
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Retained"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Header table"));

    auto removable_table = header_template.append_table(1, 1);
    REQUIRE(removable_table.has_next());
    CHECK(removable_table.rows().cells().set_text("temporary middle table"));

    auto final_table = header_template.append_table(2, 2);
    REQUIRE(final_table.has_next());
    row = final_table.rows();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Final"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("State"));
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Pending"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Will be updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    auto selected_table = header_template.tables();
    REQUIRE(selected_table.has_next());
    selected_table.next();
    REQUIRE(selected_table.has_next());
    CHECK(selected_table.remove());
    CHECK(selected_table.has_next());

    row = selected_table.rows();
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Final"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Reached after middle-table removal"));

    CHECK_FALSE(reopened.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document header_document;
    REQUIRE(header_document.load_string(saved_header.c_str()));
    const auto header_root = header_document.child("w:hdr");
    REQUIRE(header_root != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_root, "w:tbl"), 2U);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\n");
    CHECK_EQ(collect_template_part_table_text(header_template),
             "Section\nStatus\nRetained\nHeader table\nFinal\nState\nFinal\nReached after middle-table removal\n");

    fs::remove(target);
}

TEST_CASE("header and footer template parts reuse bookmark template APIs") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_footer_template_parts.docx";
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
          <w:r><w:t>template item</w:t></w:r>
          <w:bookmarkEnd w:id="0"/>
        </w:p>
      </w:tc>
      <w:tc><w:p><w:r><w:t>0</w:t></w:r></w:p></w:tc>
    </w:tr>
  </w:tbl>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Company: </w:t></w:r>
    <w:bookmarkStart w:id="1" w:name="company_name"/>
    <w:r><w:t>old company</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_lines"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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
    CHECK_EQ(doc.body_template().entry_name(), test_document_xml_entry);
    CHECK_EQ(doc.header_template().entry_name(), "word/header1.xml");
    CHECK_EQ(doc.footer_template().entry_name(), "word/footer1.xml");
    CHECK_FALSE(static_cast<bool>(doc.section_header_template(1)));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_table_rows(
                 "item_row", {{"Apple", "2"}, {"Pear", "5"}}),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_fill_result =
        footer_template.fill_bookmarks({{"company_name", "Acme Corp"}});
    CHECK_EQ(footer_fill_result.requested, 1);
    CHECK_EQ(footer_fill_result.matched, 1);
    CHECK_EQ(footer_fill_result.replaced, 1);
    CHECK(footer_fill_result);
    CHECK_EQ(footer_template.replace_bookmark_with_paragraphs(
                 "footer_lines", {"First line", "Second line"}),
             1);

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(saved_header.find("template item"), std::string::npos);
    CHECK_EQ(saved_header.find("w:name=\"item_row\""), std::string::npos);
    CHECK_NE(saved_header.find("Apple"), std::string::npos);
    CHECK_NE(saved_header.find("Pear"), std::string::npos);

    pugi::xml_document header_document;
    REQUIRE(header_document.load_string(saved_header.c_str()));
    const auto header_table = header_document.child("w:hdr").child("w:tbl");
    REQUIRE(header_table != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_table, "w:tr"), 3);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("Company: "), std::string::npos);
    CHECK_NE(saved_footer.find("Acme Corp"), std::string::npos);
    CHECK_NE(saved_footer.find("First line"), std::string::npos);
    CHECK_NE(saved_footer.find("Second line"), std::string::npos);
    CHECK_EQ(saved_footer.find("w:name=\"footer_lines\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    std::ostringstream footer_lines;
    for (auto paragraph = reopened.section_footer_paragraphs(0); paragraph.has_next();
         paragraph.next()) {
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            footer_lines << run.get_text();
        }
        footer_lines << '\n';
    }
    CHECK_EQ(footer_lines.str(), "Company: Acme Corp\nFirst line\nSecond line\n");

    fs::remove(target);
}

TEST_CASE("template parts can validate header and footer bookmark schemas") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_validate.docx";
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
  <w:p>
    <w:r><w:t>Header title: </w:t></w:r>
    <w:bookmarkStart w:id="0" w:name="header_title"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_note"/>
    <w:r><w:t>standalone note</w:t></w:r>
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

    const std::string footer_xml =
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

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_result = header_template.validate_template(
        {
            {"header_title", featherdoc::template_slot_kind::text, true},
            {"header_note", featherdoc::template_slot_kind::block, true},
            {"header_rows", featherdoc::template_slot_kind::table_rows, true},
        });
    CHECK_FALSE(doc.last_error());
    CHECK(header_result.missing_required.empty());
    CHECK(header_result.duplicate_bookmarks.empty());
    CHECK(header_result.malformed_placeholders.empty());
    CHECK(header_result.unexpected_bookmarks.empty());
    CHECK(header_result.kind_mismatches.empty());
    CHECK(header_result.occurrence_mismatches.empty());
    CHECK(static_cast<bool>(header_result));

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_result = footer_template.validate_template(
        {
            {"footer_company", featherdoc::template_slot_kind::text, true},
            {"footer_summary", featherdoc::template_slot_kind::block, true},
            {"footer_signature", featherdoc::template_slot_kind::text, true},
        });
    CHECK_FALSE(doc.last_error());
    REQUIRE(footer_result.missing_required.size() == 1);
    CHECK_EQ(footer_result.missing_required.front(), "footer_signature");
    REQUIRE(footer_result.duplicate_bookmarks.size() == 1);
    CHECK_EQ(footer_result.duplicate_bookmarks.front(), "footer_company");
    REQUIRE(footer_result.malformed_placeholders.size() == 1);
    CHECK_EQ(footer_result.malformed_placeholders.front(), "footer_summary");
    CHECK(footer_result.unexpected_bookmarks.empty());
    CHECK(footer_result.kind_mismatches.empty());
    CHECK(footer_result.occurrence_mismatches.empty());
    CHECK_FALSE(static_cast<bool>(footer_result));

    auto missing_header_template = doc.section_header_template(1);
    CHECK_FALSE(static_cast<bool>(missing_header_template));
    const auto unavailable_result = missing_header_template.validate_template(
        {{"unused", featherdoc::template_slot_kind::text, true}});
    CHECK(unavailable_result.missing_required.empty());
    CHECK(unavailable_result.duplicate_bookmarks.empty());
    CHECK(unavailable_result.malformed_placeholders.empty());
    CHECK(unavailable_result.unexpected_bookmarks.empty());
    CHECK(unavailable_result.kind_mismatches.empty());
    CHECK(unavailable_result.occurrence_mismatches.empty());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("template part is not available"),
             std::string::npos);

    fs::remove(target);
}

TEST_CASE("header and footer template parts can remove standalone bookmark blocks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_remove_bookmark_block.docx";
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
  <w:p><w:r><w:t>Header keep</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_block"/>
    <w:r><w:t>Header delete</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>Footer keep</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="footer_block"/>
    <w:r><w:t>Footer delete</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
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

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.remove_bookmark_block("header_block"), 1);
    CHECK_FALSE(doc.last_error());

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.remove_bookmark_block("footer_block"), 1);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header.find("Header keep"), std::string::npos);
    CHECK_EQ(saved_header.find("Header delete"), std::string::npos);
    CHECK_EQ(saved_header.find("w:name=\"header_block\""), std::string::npos);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("Footer keep"), std::string::npos);
    CHECK_EQ(saved_footer.find("Footer delete"), std::string::npos);
    CHECK_EQ(saved_footer.find("w:name=\"footer_block\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    fs::remove(target);
}

TEST_CASE("header and footer template parts can replace bookmark placeholders with images") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "header_footer_template_images.docx";
    const fs::path image_path = fs::current_path() / "header_footer_template_images.png";
    fs::remove(target);
    fs::remove(image_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="1" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo", image_path, 20U, 10U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 30U, 15U),
             1);

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header.find("<w:drawing"), std::string::npos);
    CHECK_NE(saved_header.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_header.find("cy=\"95250\""), std::string::npos);
    CHECK_EQ(saved_header.find("w:name=\"header_logo\""), std::string::npos);

    const auto saved_footer = read_test_docx_entry(target, "word/footer1.xml");
    CHECK_NE(saved_footer.find("<w:drawing"), std::string::npos);
    CHECK_NE(saved_footer.find("cx=\"285750\""), std::string::npos);
    CHECK_NE(saved_footer.find("cy=\"142875\""), std::string::npos);
    CHECK_EQ(saved_footer.find("w:name=\"footer_logo\""), std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);

    const auto saved_footer_relationships =
        read_test_docx_entry(target, "word/_rels/footer1.xml.rels");
    CHECK_NE(saved_footer_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/_rels/header1.xml.rels"));
    CHECK(test_docx_entry_exists(target, "word/_rels/footer1.xml.rels"));
    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("template parts list existing inline images and can extract them") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "template_part_inline_images.docx";
    const fs::path image_path = fs::current_path() / "template_part_inline_images.png";
    const fs::path extracted_path =
        fs::current_path() / "template_part_inline_images_extracted.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(), image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one", image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two", image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 40U, 20U),
             1);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[0].width_px, 20U);
    CHECK_EQ(header_images[0].height_px, 10U);
    CHECK_EQ(header_images[1].entry_name, "word/media/image2.png");
    CHECK_EQ(header_images[1].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK(header_template.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);
    CHECK_FALSE(header_template.extract_inline_image(2U, extracted_path));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");
    CHECK_EQ(footer_images[0].width_px, 40U);
    CHECK_EQ(footer_images[0].height_px, 20U);

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("template part replace_inline_image updates only the selected header image") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "template_part_replace_inline_image.docx";
    const fs::path source_image_path =
        fs::current_path() / "template_part_replace_inline_image_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "template_part_replace_inline_image_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "template_part_replace_inline_image_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one",
                                                         source_image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two",
                                                         source_image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", source_image_path,
                                                         40U, 20U),
             1);
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.replace_inline_image(1U, replacement_image_path));

    auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].entry_name, "word/media/image1.png");
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK_EQ(header_images[1].content_type, "image/gif");
    CHECK(header_images[1].entry_name.ends_with(".gif"));
    CHECK_NE(header_images[1].entry_name, "word/media/image2.png");

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    const auto replacement_entry_name = header_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image3.png"));
    CHECK(test_docx_entry_exists(target, replacement_entry_name.c_str()));
    CHECK_EQ(read_test_docx_entry(target, replacement_entry_name.c_str()),
             replacement_image_data);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_EQ(saved_header_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    const auto saved_footer_relationships =
        read_test_docx_entry(target, "word/_rels/footer1.xml.rels");
    CHECK_NE(saved_footer_relationships.find("Target=\"media/image3.png\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"gif\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/gif\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[1].content_type, "image/gif");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);
    CHECK_EQ(header_images[1].entry_name, replacement_entry_name);
    CHECK(header_template.extract_inline_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("template part drawing_images includes anchored header images") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    const fs::path target = fs::current_path() / "template_part_drawing_images_anchor.docx";
    const fs::path source_image_path =
        fs::current_path() / "template_part_drawing_images_anchor_source.png";
    const fs::path replacement_image_path =
        fs::current_path() / "template_part_drawing_images_anchor_replacement.gif";
    const fs::path extracted_path =
        fs::current_path() / "template_part_drawing_images_anchor_extracted.gif";
    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string source_image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                        sizeof(tiny_png_bytes));
    const std::string replacement_image_data(reinterpret_cast<const char *>(tiny_gif_bytes),
                                             sizeof(tiny_gif_bytes));
    const std::vector<unsigned char> expected_replacement_data(
        replacement_image_data.begin(), replacement_image_data.end());
    write_binary_file(source_image_path, source_image_data);
    write_binary_file(replacement_image_path, replacement_image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one",
                                                         source_image_path, 20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two",
                                                         source_image_path, 30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", source_image_path,
                                                         40U, 20U),
             1);
    CHECK_FALSE(doc.save());

    auto anchored_header_xml = read_test_docx_entry(target, "word/header1.xml");
    anchored_header_xml = convert_nth_inline_drawing_to_anchor(anchored_header_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_header_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, "word/header1.xml", std::move(anchored_header_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    const auto header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 2U);
    CHECK_EQ(header_drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(header_drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(header_drawing_images[1].width_px, 30U);
    CHECK_EQ(header_drawing_images[1].height_px, 15U);

    const auto header_inline_images = header_template.inline_images();
    REQUIRE_EQ(header_inline_images.size(), 1U);
    CHECK_EQ(header_inline_images[0].entry_name, "word/media/image1.png");

    CHECK(header_template.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path),
             std::vector<unsigned char>(source_image_data.begin(), source_image_data.end()));

    CHECK(header_template.replace_drawing_image(1U, replacement_image_path));

    auto updated_header_images = header_template.drawing_images();
    REQUIRE_EQ(updated_header_images.size(), 2U);
    CHECK_EQ(updated_header_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_header_images[1].content_type, "image/gif");
    CHECK(updated_header_images[1].entry_name.ends_with(".gif"));
    const auto replacement_entry_name = updated_header_images[1].entry_name;
    const auto replacement_target =
        (fs::path{"media"} / fs::path{replacement_entry_name}.filename()).generic_string();

    CHECK_FALSE(reopened.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 1U);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_EQ(saved_header_relationships.find("Target=\"media/image2.png\""), std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"" + replacement_target + "\""),
             std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    updated_header_images = header_template.drawing_images();
    REQUIRE_EQ(updated_header_images.size(), 2U);
    CHECK_EQ(updated_header_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(updated_header_images[1].content_type, "image/gif");
    CHECK_EQ(updated_header_images[1].entry_name, replacement_entry_name);
    CHECK(header_template.extract_drawing_image(1U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_replacement_data);

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");
    CHECK_EQ(footer_images[0].content_type, "image/png");

    fs::remove(target);
    fs::remove(source_image_path);
    fs::remove(replacement_image_path);
    fs::remove(extracted_path);
}

TEST_CASE("template part remove_drawing_image and remove_inline_image prune header media") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "template_part_remove_images.docx";
    const fs::path image_path = fs::current_path() / "template_part_remove_images_source.png";
    fs::remove(target);
    fs::remove(image_path);

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
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
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

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_one", image_path,
                                                         20U, 10U),
             1);
    CHECK_EQ(header_template.replace_bookmark_with_image("header_logo_two", image_path,
                                                         30U, 15U),
             1);

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.replace_bookmark_with_image("footer_logo", image_path, 40U, 20U),
             1);
    CHECK_FALSE(doc.save());

    auto anchored_header_xml = read_test_docx_entry(target, "word/header1.xml");
    anchored_header_xml = convert_nth_inline_drawing_to_anchor(anchored_header_xml, 1U);
    CHECK_EQ(count_substring_occurrences(anchored_header_xml, "<wp:anchor"), 1U);
    rewrite_test_docx_entry(target, "word/header1.xml", std::move(anchored_header_xml));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 2U);
    CHECK_EQ(header_drawing_images[1].placement,
             featherdoc::drawing_image_placement::anchored_object);

    CHECK(header_template.remove_drawing_image(1U));
    header_drawing_images = header_template.drawing_images();
    REQUIRE_EQ(header_drawing_images.size(), 1U);
    CHECK_EQ(header_drawing_images[0].placement,
             featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(header_template.inline_images().size(), 1U);

    CHECK(header_template.remove_inline_image(0U));
    CHECK(header_template.drawing_images().empty());
    CHECK(header_template.inline_images().empty());
    CHECK_FALSE(header_template.remove_drawing_image(0U));
    CHECK_EQ(reopened.last_error().code,
             std::make_error_code(std::errc::result_out_of_range));

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    const auto footer_images = footer_template.inline_images();
    REQUIRE_EQ(footer_images.size(), 1U);
    CHECK_EQ(footer_images[0].entry_name, "word/media/image3.png");

    CHECK_FALSE(reopened.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 0U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 0U);
    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(saved_header_relationships.find("relationships/image"), std::string::npos);

    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image3.png"));

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());
    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.drawing_images().empty());
    CHECK(header_template.inline_images().empty());

    footer_template = reopened_again.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(footer_template.inline_images().size(), 1U);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("template part replace_bookmark_with_floating_image writes anchored header drawings") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target =
        fs::current_path() / "template_part_replace_bookmark_floating_image.docx";
    const fs::path image_path =
        fs::current_path() / "template_part_replace_bookmark_floating_image.png";
    const fs::path extracted_path =
        fs::current_path() /
        "template_part_replace_bookmark_floating_image_extracted.png";
    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);

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
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>before</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p><w:r><w:t>after</w:t></w:r></w:p>
</w:hdr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
        });

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    const std::vector<unsigned char> expected_image_data(image_data.begin(), image_data.end());
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 40;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = 12;

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.replace_bookmark_with_floating_image("header_logo",
                                                                  image_path, 30U, 15U,
                                                                  options),
             1);
    CHECK_FALSE(doc.save());

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_NE(saved_header_xml.find("<wp:anchor"), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>381000</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>114300</wp:posOffset>"),
             std::string::npos);
    CHECK_EQ(saved_header_xml.find("w:name=\"header_logo\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto drawing_images = header_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].width_px, 30U);
    CHECK_EQ(drawing_images[0].height_px, 15U);
    CHECK(header_template.extract_drawing_image(0U, extracted_path));
    CHECK_EQ(read_binary_file(extracted_path), expected_image_data);

    fs::remove(target);
    fs::remove(image_path);
    fs::remove(extracted_path);
}

TEST_CASE("body template part can append inline images and preserve them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "body_template_append_image_roundtrip.docx";
    const fs::path image_path =
        fs::current_path() / "body_template_append_image_roundtrip.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_image(image_path));
    CHECK(body_template.append_image(image_path, 20U, 10U));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);
    CHECK_EQ(read_test_docx_entry(target, "word/media/image2.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             2U);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"media/image2.png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 2U);
    CHECK_NE(saved_document_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto body_images = body_template.inline_images();
    REQUIRE_EQ(body_images.size(), 2U);
    CHECK_EQ(body_images[0].content_type, "image/png");
    CHECK_EQ(body_images[0].width_px, 1U);
    CHECK_EQ(body_images[0].height_px, 1U);
    CHECK_EQ(body_images[1].content_type, "image/png");
    CHECK_EQ(body_images[1].width_px, 20U);
    CHECK_EQ(body_images[1].height_px, 10U);

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("body template part can append floating images and preserve them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "body_template_append_floating_image_roundtrip.docx";
    const fs::path image_path =
        fs::current_path() / "body_template_append_floating_image_roundtrip.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 24;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = -8;
    options.behind_text = true;
    options.allow_overlap = false;
    options.z_order = 32U;
    options.wrap_mode = featherdoc::floating_image_wrap_mode::top_bottom;
    options.wrap_distance_top_px = 4U;
    options.wrap_distance_bottom_px = 9U;
    options.crop = featherdoc::floating_image_crop{25U, 50U, 75U, 100U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_floating_image(image_path, 20U, 10U, options));
    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             1U);
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:anchor"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<wp:inline"), 0U);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>228600</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>-76200</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("behindDoc=\"1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("allowOverlap=\"0\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeHeight=\"32\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distT=\"38100\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distB=\"85725\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:wrapTopAndBottom"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<a:srcRect l=\"2500\" t=\"5000\" r=\"7500\" b=\"10000\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto drawing_images = body_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 20U);
    CHECK_EQ(drawing_images[0].height_px, 10U);
    REQUIRE(drawing_images[0].floating_options.has_value());
    CHECK_EQ(drawing_images[0].floating_options->horizontal_reference,
             featherdoc::floating_image_horizontal_reference::page);
    CHECK_EQ(drawing_images[0].floating_options->horizontal_offset_px, 24);
    CHECK_EQ(drawing_images[0].floating_options->vertical_reference,
             featherdoc::floating_image_vertical_reference::margin);
    CHECK_EQ(drawing_images[0].floating_options->vertical_offset_px, -8);
    CHECK(drawing_images[0].floating_options->behind_text);
    CHECK_FALSE(drawing_images[0].floating_options->allow_overlap);
    CHECK_EQ(drawing_images[0].floating_options->z_order, 32U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::top_bottom);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_top_px, 4U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_bottom_px, 9U);
    REQUIRE(drawing_images[0].floating_options->crop.has_value());
    CHECK_EQ(drawing_images[0].floating_options->crop->left_per_mille, 25U);
    CHECK_EQ(drawing_images[0].floating_options->crop->top_per_mille, 50U);
    CHECK_EQ(drawing_images[0].floating_options->crop->right_per_mille, 75U);
    CHECK_EQ(drawing_images[0].floating_options->crop->bottom_per_mille, 100U);
    CHECK_EQ(body_template.inline_images().size(), 0U);

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run("reopened edit").has_next());
    CHECK_FALSE(reopened.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("header template part can append inline images") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_append_images.docx";
    const fs::path image_path =
        fs::current_path() / "header_template_append_images.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.add_run("header intro").has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.append_image(image_path));
    CHECK(header_template.append_image(image_path, 30U, 15U));
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK(test_docx_entry_exists(target, "word/media/image2.png"));

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 2U);
    CHECK_NE(saved_header_xml.find("cx=\"9525\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cy=\"9525\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cx=\"285750\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("cy=\"142875\""), std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_header_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             2U);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image2.png\""),
             std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("Extension=\"png\""), std::string::npos);
    CHECK_NE(saved_content_types.find("ContentType=\"image/png\""), std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto header_images = header_template.inline_images();
    REQUIRE_EQ(header_images.size(), 2U);
    CHECK_EQ(header_images[0].content_type, "image/png");
    CHECK_EQ(header_images[0].width_px, 1U);
    CHECK_EQ(header_images[0].height_px, 1U);
    CHECK_EQ(header_images[1].content_type, "image/png");
    CHECK_EQ(header_images[1].width_px, 30U);
    CHECK_EQ(header_images[1].height_px, 15U);

    CHECK(reopened_again.header_paragraphs().add_run(" reopened edit").has_next());
    CHECK_FALSE(reopened_again.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);
    CHECK_NE(relationships_after_resave.find("Target=\"media/image2.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("header template part can append floating images") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "header_template_append_floating_images.docx";
    const fs::path image_path =
        fs::current_path() / "header_template_append_floating_images.png";
    fs::remove(target);
    fs::remove(image_path);

    const auto image_data = tiny_png_data();
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 40;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = 12;
    options.z_order = 48U;
    options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    options.wrap_distance_left_px = 5U;
    options.wrap_distance_right_px = 7U;
    options.crop = featherdoc::floating_image_crop{10U, 20U, 30U, 40U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.add_run("header intro").has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK(header_template.append_floating_image(image_path, 30U, 15U, options));
    CHECK_FALSE(reopened.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_header_xml = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:anchor"), 1U);
    CHECK_EQ(count_substring_occurrences(saved_header_xml, "<wp:inline"), 0U);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>381000</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:posOffset>114300</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("relativeHeight=\"48\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("distL=\"47625\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("distR=\"66675\""), std::string::npos);
    CHECK_NE(saved_header_xml.find("<wp:wrapSquare wrapText=\"bothSides\""),
             std::string::npos);
    CHECK_NE(saved_header_xml.find("<a:srcRect l=\"1000\" t=\"2000\" r=\"3000\" b=\"4000\""),
             std::string::npos);

    const auto saved_header_relationships =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_EQ(count_substring_occurrences(
                 saved_header_relationships,
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\""),
             1U);
    CHECK_NE(saved_header_relationships.find("Target=\"media/image1.png\""),
             std::string::npos);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto drawing_images = header_template.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].content_type, "image/png");
    CHECK_EQ(drawing_images[0].width_px, 30U);
    CHECK_EQ(drawing_images[0].height_px, 15U);
    REQUIRE(drawing_images[0].floating_options.has_value());
    CHECK_EQ(drawing_images[0].floating_options->z_order, 48U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_left_px, 5U);
    CHECK_EQ(drawing_images[0].floating_options->wrap_distance_right_px, 7U);
    REQUIRE(drawing_images[0].floating_options->crop.has_value());
    CHECK_EQ(drawing_images[0].floating_options->crop->left_per_mille, 10U);
    CHECK_EQ(drawing_images[0].floating_options->crop->top_per_mille, 20U);
    CHECK_EQ(drawing_images[0].floating_options->crop->right_per_mille, 30U);
    CHECK_EQ(drawing_images[0].floating_options->crop->bottom_per_mille, 40U);
    CHECK_EQ(header_template.inline_images().size(), 0U);

    CHECK(reopened_again.header_paragraphs().add_run(" reopened edit").has_next());
    CHECK_FALSE(reopened_again.save());

    const auto relationships_after_resave =
        read_test_docx_entry(target, "word/_rels/header1.xml.rels");
    CHECK_NE(relationships_after_resave.find("Target=\"media/image1.png\""),
             std::string::npos);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("apply_bookmark_block_visibility keeps and removes body template blocks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_block_visibility.docx";
    fs::remove(target);

    const std::string document_xml =
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
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.apply_bookmark_block_visibility({
        {"keep_block", true},
        {"hide_block", false},
    });
    CHECK_EQ(result.requested, 2);
    CHECK_EQ(result.matched, 2);
    CHECK_EQ(result.kept, 1);
    CHECK_EQ(result.removed, 1);
    CHECK(result);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("keep_block"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("hide_block"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Keep me"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("Hide me"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("Secret Cell"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nKeep me\nmiddle\nafter\n");

    fs::remove(target);
}

TEST_CASE("header template parts can change bookmark block visibility") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_block_visibility.docx";
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
</Relationships>
)";

    const std::string header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:bookmarkStart w:id="0" w:name="offer_block"/></w:p>
  <w:p><w:r><w:t>Offer line 1</w:t></w:r></w:p>
  <w:p><w:r><w:t>Offer line 2</w:t></w:r></w:p>
  <w:p><w:bookmarkEnd w:id="0"/></w:p>
  <w:p><w:r><w:t>Always here</w:t></w:r></w:p>
</w:hdr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(header_template.set_bookmark_block_visibility("offer_block", false), 1);

    CHECK_FALSE(doc.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    CHECK_EQ(saved_header.find("offer_block"), std::string::npos);
    CHECK_EQ(saved_header.find("Offer line 1"), std::string::npos);
    CHECK_EQ(saved_header.find("Offer line 2"), std::string::npos);
    CHECK_NE(saved_header.find("Always here"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_paragraphs().runs().get_text(), "Always here");

    fs::remove(target);
}

TEST_CASE("apply_bookmark_block_visibility rejects duplicate or empty binding names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_block_visibility_validation.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>body</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document unopened(target);
    const auto unopened_result =
        unopened.apply_bookmark_block_visibility({{"promo_block", true}});
    CHECK_EQ(unopened_result.requested, 1);
    CHECK_EQ(unopened.last_error().code, featherdoc::document_errc::document_not_open);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto duplicate_result = doc.apply_bookmark_block_visibility(
        {{"promo_block", true}, {"promo_block", false}});
    CHECK_EQ(duplicate_result.requested, 2);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    const auto empty_result = doc.apply_bookmark_block_visibility({{"", true}});
    CHECK_EQ(empty_result.requested, 1);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

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

TEST_CASE("fill_bookmarks replaces multiple bookmark bindings and reports misses") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_fill_batch.docx";
    fs::remove(target);

    const std::string document_xml =
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
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.fill_bookmarks(
        {
            {"customer", "Acme Corp"},
            {"invoice", "INV-2026-0001"},
            {"missing", "ignored"},
        });

    CHECK_FALSE(doc.last_error());
    CHECK_EQ(result.requested, 3);
    CHECK_EQ(result.matched, 2);
    CHECK_EQ(result.replaced, 2);
    REQUIRE(result.missing_bookmarks.size() == 1);
    CHECK_EQ(result.missing_bookmarks.front(), "missing");
    CHECK_FALSE(static_cast<bool>(result));
    CHECK_EQ(collect_document_text(doc), "Customer: Acme Corp\nInvoice: INV-2026-0001\n");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(xml_text.find("Acme Corp"), std::string::npos);
    CHECK_NE(xml_text.find("INV-2026-0001"), std::string::npos);
    CHECK_EQ(xml_text.find("old customer"), std::string::npos);
    CHECK_EQ(xml_text.find("old invoice"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened),
             "Customer: Acme Corp\nInvoice: INV-2026-0001\n");

    fs::remove(target);
}

TEST_CASE("fill_bookmarks rejects duplicate or empty binding names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_fill_validation.docx";
    fs::remove(target);

    featherdoc::Document unopened(target);
    const auto unopened_result = unopened.fill_bookmarks({{"customer", "Acme Corp"}});
    CHECK_EQ(unopened_result.requested, 1);
    CHECK_EQ(unopened.last_error().code, featherdoc::document_errc::document_not_open);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto duplicate_result = doc.fill_bookmarks(
        {
            {"customer", "Acme Corp"},
            {"customer", "Other"},
        });
    CHECK_EQ(duplicate_result.requested, 2);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("duplicate"), std::string::npos);

    const auto empty_result = doc.fill_bookmarks({{"", "blank"}});
    CHECK_EQ(empty_result.requested, 1);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("must not be empty"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("external hyperlinks can be appended and inspected") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "external_hyperlinks_append.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.append_hyperlink("OpenAI", "https://openai.com/?a=1&b=2"), 1U);
    CHECK_FALSE(doc.last_error());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_EQ(body_template.append_hyperlink("FeatherDoc docs", "https://example.com/docs"),
             1U);
    CHECK_FALSE(doc.last_error());

    const auto links = doc.list_hyperlinks();
    REQUIRE_EQ(links.size(), 2U);
    CHECK_EQ(links[0].text, "OpenAI");
    REQUIRE(links[0].target.has_value());
    CHECK_EQ(*links[0].target, "https://openai.com/?a=1&b=2");
    CHECK(links[0].external);
    REQUIRE(links[0].relationship_id.has_value());
    CHECK_EQ(links[1].text, "FeatherDoc docs");
    REQUIRE(links[1].target.has_value());
    CHECK_EQ(*links[1].target, "https://example.com/docs");
    CHECK(links[1].external);

    const auto part_links = body_template.list_hyperlinks();
    REQUIRE_EQ(part_links.size(), 2U);
    CHECK_EQ(part_links[0].text, "OpenAI");
    CHECK_EQ(part_links[1].text, "FeatherDoc docs");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:hyperlink"), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:rStyle w:val=\"Hyperlink\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:color w:val=\"0563C1\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:u w:val=\"single\""),
             2U);
    CHECK_NE(saved_document_xml.find("OpenAI"), std::string::npos);
    CHECK_NE(saved_document_xml.find("FeatherDoc docs"), std::string::npos);

    const auto relationships_xml =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(relationships_xml, "/relationships/hyperlink"),
             2U);
    CHECK_NE(relationships_xml.find("TargetMode=\"External\""), std::string::npos);
    CHECK_NE(relationships_xml.find("Target=\"https://openai.com/?a=1&amp;b=2\""),
             std::string::npos);
    CHECK_NE(relationships_xml.find("Target=\"https://example.com/docs\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_links = reopened.list_hyperlinks();
    REQUIRE_EQ(reopened_links.size(), 2U);
    CHECK_EQ(reopened_links[0].text, "OpenAI");
    REQUIRE(reopened_links[0].target.has_value());
    CHECK_EQ(*reopened_links[0].target, "https://openai.com/?a=1&b=2");
    CHECK(reopened_links[0].external);
    CHECK_EQ(reopened_links[1].text, "FeatherDoc docs");
    REQUIRE(reopened_links[1].target.has_value());
    CHECK_EQ(*reopened_links[1].target, "https://example.com/docs");
    CHECK(reopened_links[1].external);

    fs::remove(target);
}

TEST_CASE("external hyperlink API validates text and target") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "external_hyperlinks_invalid.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.append_hyperlink({}, "https://example.com"), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("text"), std::string::npos);

    CHECK_EQ(doc.append_hyperlink("Example", {}), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("target"), std::string::npos);

    CHECK(doc.list_hyperlinks().empty());
    CHECK_FALSE(doc.last_error());

    fs::remove(target);
}

TEST_CASE("external hyperlinks can be replaced and removed") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "external_hyperlinks_replace_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.append_hyperlink("Old docs", "https://old.example/docs"), 1U);
    CHECK_EQ(doc.append_hyperlink("Temporary docs", "https://temp.example/docs"), 1U);
    CHECK(doc.replace_hyperlink(0U, "New docs", "https://new.example/docs?a=1&b=2"));
    CHECK(doc.remove_hyperlink(1U));
    CHECK_FALSE(doc.last_error());

    const auto links = doc.list_hyperlinks();
    REQUIRE_EQ(links.size(), 1U);
    CHECK_EQ(links.front().text, "New docs");
    REQUIRE(links.front().target.has_value());
    CHECK_EQ(*links.front().target, "https://new.example/docs?a=1&b=2");

    CHECK_FALSE(doc.replace_hyperlink(9U, "Missing", "https://missing.example"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "hyperlink index is out of range");

    CHECK_FALSE(doc.save());
    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:hyperlink"), 1U);
    CHECK_NE(saved_document_xml.find("New docs"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Temporary docs"), std::string::npos);

    const auto relationships_xml =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_EQ(count_substring_occurrences(relationships_xml, "/relationships/hyperlink"),
             1U);
    CHECK_NE(relationships_xml.find("Target=\"https://new.example/docs?a=1&amp;b=2\""),
             std::string::npos);
    CHECK_EQ(relationships_xml.find("temp.example"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_links = reopened.list_hyperlinks();
    REQUIRE_EQ(reopened_links.size(), 1U);
    CHECK_EQ(reopened_links.front().text, "New docs");

    fs::remove(target);
}

TEST_CASE("OMML builder helpers create appendable formulas") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "omml_builder_helpers.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto fraction = featherdoc::make_omml_fraction("a+b", "c", true);
    const auto superscript = featherdoc::make_omml_superscript("x", "2");
    const auto subscript = featherdoc::make_omml_subscript("a", "i");
    const auto radical = featherdoc::make_omml_radical("x+1");
    const auto delimited = featherdoc::make_omml_delimiter("x+1", "[", "]");
    const auto nary = featherdoc::make_omml_nary("∑", "i", "i=1", "n");
    const auto inline_text = featherdoc::make_omml_text("k=7");

    CHECK(doc.append_omml(fraction));
    CHECK(doc.append_omml(superscript));
    CHECK(doc.append_omml(subscript));
    CHECK(doc.replace_omml(0U, radical));
    CHECK(doc.append_omml(delimited));
    CHECK(doc.append_omml(nary));
    CHECK(doc.append_omml(inline_text));

    const auto summaries = doc.list_omml();
    REQUIRE_EQ(summaries.size(), 6U);
    CHECK(summaries[0].display);
    CHECK_NE(summaries[0].xml.find("m:rad"), std::string::npos);
    CHECK_FALSE(summaries[1].display);
    CHECK_NE(summaries[1].xml.find("m:sSup"), std::string::npos);
    CHECK_FALSE(summaries[2].display);
    CHECK_NE(summaries[2].xml.find("m:sSub"), std::string::npos);
    CHECK_FALSE(summaries[3].display);
    CHECK_NE(summaries[3].xml.find("m:d"), std::string::npos);
    CHECK(summaries[4].display);
    CHECK_NE(summaries[4].xml.find("m:nary"), std::string::npos);
    CHECK_EQ(summaries[5].text, "k=7");

    CHECK_FALSE(doc.save());
    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("m:rad"), std::string::npos);
    CHECK_NE(saved_xml.find("m:sSup"), std::string::npos);
    CHECK_NE(saved_xml.find("m:sSub"), std::string::npos);
    CHECK_NE(saved_xml.find("m:d"), std::string::npos);
    CHECK_NE(saved_xml.find("m:nary"), std::string::npos);
    CHECK_NE(saved_xml.find("m:chr m:val=\"∑\""), std::string::npos);
    CHECK_NE(saved_xml.find("k=7"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("review notes comments can be appended replaced removed and saved") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_notes_comments_mutation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.append_footnote("Footnote anchor ", "Original footnote"), 1U);
    CHECK(doc.replace_footnote(0U, "Replaced footnote"));
    CHECK_EQ(doc.append_footnote("Removed footnote anchor ", "Removed footnote"), 1U);
    CHECK(doc.remove_footnote(1U));

    CHECK_EQ(doc.append_endnote("Endnote anchor ", "Original endnote"), 1U);
    CHECK(doc.replace_endnote(0U, "Replaced endnote"));
    CHECK_EQ(doc.append_endnote("Removed endnote anchor ", "Removed endnote"), 1U);
    CHECK(doc.remove_endnote(1U));

    CHECK_EQ(doc.append_comment("Commented text", "Original comment", "Reviewer", "RV",
                                "2026-05-02T08:00:00Z"),
             1U);
    CHECK(doc.replace_comment(0U, "Replaced comment"));
    CHECK(doc.set_comment_resolved(0U, true));
    CHECK_EQ(doc.append_comment_reply(0U, "Reply comment", "Responder", "RS",
                                      "2026-05-02T08:05:00Z"),
             1U);
    CHECK_EQ(doc.append_comment("Removed comment text", "Removed comment"), 1U);
    CHECK(doc.set_comment_resolved(2U, true));
    CHECK(doc.remove_comment(2U));

    auto footnotes = doc.list_footnotes();
    REQUIRE_EQ(footnotes.size(), 1U);
    CHECK_EQ(footnotes.front().text, "Replaced footnote");
    auto endnotes = doc.list_endnotes();
    REQUIRE_EQ(endnotes.size(), 1U);
    CHECK_EQ(endnotes.front().text, "Replaced endnote");
    auto comments = doc.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments.front().anchor_text.has_value());
    CHECK_EQ(*comments.front().anchor_text, "Commented text");
    CHECK_EQ(comments.front().text, "Replaced comment");
    CHECK(comments.front().resolved);
    REQUIRE(comments.front().author.has_value());
    CHECK_EQ(*comments.front().author, "Reviewer");
    CHECK_EQ(comments.front().date,
             std::optional<std::string>{"2026-05-02T08:00:00Z"});
    CHECK_EQ(comments[1].text, "Reply comment");
    CHECK_EQ(comments[1].parent_index, std::optional<std::size_t>{0U});
    CHECK_EQ(comments[1].parent_id, std::optional<std::string>{comments[0].id});
    CHECK_EQ(comments[1].date,
             std::optional<std::string>{"2026-05-02T08:05:00Z"});
    CHECK_FALSE(comments[1].anchor_text.has_value());

    featherdoc::comment_metadata_update metadata_update;
    metadata_update.author = "Updated Reviewer";
    metadata_update.clear_initials = true;
    metadata_update.date = "2026-05-02T08:30:00Z";
    CHECK(doc.set_comment_metadata(0U, metadata_update));
    comments = doc.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    CHECK_EQ(comments.front().author,
             std::optional<std::string>{"Updated Reviewer"});
    CHECK_FALSE(comments.front().initials.has_value());
    CHECK_EQ(comments.front().date,
             std::optional<std::string>{"2026-05-02T08:30:00Z"});

    CHECK_FALSE(doc.replace_footnote(4U, "Missing"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "footnote index is out of range");

    CHECK_FALSE(doc.save());
    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:footnoteReference"), 1U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:endnoteReference"), 1U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:commentReference"), 1U);

    const auto footnotes_xml = read_test_docx_entry(target, "word/footnotes.xml");
    CHECK_NE(footnotes_xml.find("Replaced footnote"), std::string::npos);
    CHECK_EQ(footnotes_xml.find("Removed footnote"), std::string::npos);
    const auto endnotes_xml = read_test_docx_entry(target, "word/endnotes.xml");
    CHECK_NE(endnotes_xml.find("Replaced endnote"), std::string::npos);
    CHECK_EQ(endnotes_xml.find("Removed endnote"), std::string::npos);
    const auto comments_xml = read_test_docx_entry(target, "word/comments.xml");
    CHECK_NE(comments_xml.find("Replaced comment"), std::string::npos);
    CHECK_NE(comments_xml.find("Reply comment"), std::string::npos);
    CHECK_NE(comments_xml.find("w:author=\"Updated Reviewer\""),
             std::string::npos);
    CHECK_NE(comments_xml.find("w:date=\"2026-05-02T08:30:00Z\""),
             std::string::npos);
    CHECK_NE(comments_xml.find("w:date=\"2026-05-02T08:05:00Z\""),
             std::string::npos);
    CHECK_EQ(comments_xml.find("w:date=\"2026-05-02T08:00:00Z\""),
             std::string::npos);
    CHECK_EQ(comments_xml.find("Removed comment"), std::string::npos);
    CHECK_NE(comments_xml.find("w14:paraId"), std::string::npos);
    const auto comments_extended_xml =
        read_test_docx_entry(target, "word/commentsExtended.xml");
    CHECK_EQ(count_substring_occurrences(comments_extended_xml, "<w15:commentEx"), 2U);
    CHECK_NE(comments_extended_xml.find("w15:done=\"1\""), std::string::npos);
    CHECK_NE(comments_extended_xml.find("w15:paraIdParent=\""), std::string::npos);

    const auto relationships_xml =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_xml.find("/relationships/footnotes"), std::string::npos);
    CHECK_NE(relationships_xml.find("/relationships/endnotes"), std::string::npos);
    CHECK_NE(relationships_xml.find("/relationships/comments"), std::string::npos);
    CHECK_NE(relationships_xml.find("/relationships/commentsExtended"), std::string::npos);
    const auto content_types_xml = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(content_types_xml.find("/word/footnotes.xml"), std::string::npos);
    CHECK_NE(content_types_xml.find("/word/endnotes.xml"), std::string::npos);
    CHECK_NE(content_types_xml.find("/word/comments.xml"), std::string::npos);
    CHECK_NE(content_types_xml.find("/word/commentsExtended.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    REQUIRE_EQ(reopened.list_footnotes().size(), 1U);
    REQUIRE_EQ(reopened.list_endnotes().size(), 1U);
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    CHECK(comments.front().resolved);
    CHECK_EQ(comments.front().author,
             std::optional<std::string>{"Updated Reviewer"});
    CHECK_FALSE(comments.front().initials.has_value());
    CHECK_EQ(comments.front().date,
             std::optional<std::string>{"2026-05-02T08:30:00Z"});
    CHECK_EQ(comments[1].parent_index, std::optional<std::size_t>{0U});
    CHECK_EQ(comments[1].parent_id, std::optional<std::string>{comments[0].id});
    CHECK_EQ(comments[1].date,
             std::optional<std::string>{"2026-05-02T08:05:00Z"});
    CHECK(reopened.set_comment_resolved(0U, false));
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    CHECK_FALSE(comments.front().resolved);
    CHECK_FALSE(reopened.save());
    const auto unresolved_comments_extended_xml =
        read_test_docx_entry(target, "word/commentsExtended.xml");
    CHECK_NE(unresolved_comments_extended_xml.find("w15:done=\"0\""),
             std::string::npos);
    CHECK_NE(unresolved_comments_extended_xml.find("w15:paraIdParent=\""),
             std::string::npos);

    CHECK(reopened.remove_comment(0U));
    comments = reopened.list_comments();
    CHECK(comments.empty());
    CHECK_FALSE(reopened.save());
    const auto removed_thread_comments_xml =
        read_test_docx_entry(target, "word/comments.xml");
    CHECK_EQ(removed_thread_comments_xml.find("Replaced comment"), std::string::npos);
    CHECK_EQ(removed_thread_comments_xml.find("Reply comment"), std::string::npos);
    const auto removed_thread_comments_extended_xml =
        read_test_docx_entry(target, "word/commentsExtended.xml");
    CHECK_EQ(count_substring_occurrences(removed_thread_comments_extended_xml,
                                         "<w15:commentEx"),
             0U);

    fs::remove(target);
}

TEST_CASE("review comments can target paragraph text ranges") {
    namespace fs = std::filesystem;

    const auto write_comment_range_source = [](const fs::path &path) {
        write_test_docx(path,
                        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t xml:space="preserve">Alpha </w:t></w:r>
      <w:r><w:rPr><w:b/></w:rPr><w:t>Beta</w:t></w:r>
      <w:r><w:t xml:space="preserve"> Gamma</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Middle </w:t></w:r>
      <w:r><w:rPr><w:i/></w:rPr><w:t>Text</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Gamma</w:t></w:r>
      <w:r><w:t>Delta</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");
    };

    const fs::path target =
        fs::current_path() / "review_comments_text_range.docx";
    const fs::path invalid_target =
        fs::current_path() / "review_comments_text_range_invalid.docx";
    fs::remove(target);
    fs::remove(invalid_target);

    write_comment_range_source(target);
    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.append_paragraph_text_comment(
                 0U, 0U, 3U, "Paragraph range comment", "Reviewer", "RV",
                 "2026-05-02T09:00:00Z"),
             1U);
    CHECK_EQ(doc.append_text_range_comment(
                 0U, 6U, 2U, 5U, "Cross paragraph comment", "Cross", "CP",
                 "2026-05-02T09:10:00Z"),
             1U);

    auto comments = doc.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Alp");
    CHECK_EQ(comments[0].text, "Paragraph range comment");
    REQUIRE(comments[0].author.has_value());
    CHECK_EQ(*comments[0].author, "Reviewer");
    CHECK_EQ(comments[0].date,
             std::optional<std::string>{"2026-05-02T09:00:00Z"});
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Beta GammaMiddle TextGamma");
    CHECK_EQ(comments[1].text, "Cross paragraph comment");
    CHECK_EQ(comments[1].date,
             std::optional<std::string>{"2026-05-02T09:10:00Z"});

    CHECK_FALSE(doc.save());
    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:commentRangeStart"), 2U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:commentRangeEnd"), 2U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:commentReference"), 2U);
    const auto comments_xml = read_test_docx_entry(target, "word/comments.xml");
    CHECK_NE(comments_xml.find("Paragraph range comment"), std::string::npos);
    CHECK_NE(comments_xml.find("Cross paragraph comment"), std::string::npos);
    CHECK_NE(comments_xml.find("w:date=\"2026-05-02T09:00:00Z\""),
             std::string::npos);
    CHECK_NE(comments_xml.find("w:date=\"2026-05-02T09:10:00Z\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Beta GammaMiddle TextGamma");
    CHECK_EQ(comments[0].date,
             std::optional<std::string>{"2026-05-02T09:00:00Z"});
    CHECK_EQ(comments[1].date,
             std::optional<std::string>{"2026-05-02T09:10:00Z"});
    CHECK(reopened.set_paragraph_text_comment_range(0U, 0U, 6U, 4U));
    CHECK(reopened.set_text_range_comment_range(1U, 1U, 0U, 2U, 5U));
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Beta");
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Middle TextGamma");
    CHECK_EQ(comments[0].text, "Paragraph range comment");
    CHECK_EQ(comments[1].text, "Cross paragraph comment");
    CHECK(reopened.remove_comment(1U));
    CHECK_FALSE(reopened.save());
    const auto removed_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(removed_xml, "w:commentReference"), 1U);

    write_comment_range_source(invalid_target);
    featherdoc::Document invalid(invalid_target);
    CHECK_FALSE(invalid.open());
    CHECK_EQ(invalid.append_paragraph_text_comment(0U, 6U, 0U, "bad"), 0U);
    CHECK_EQ(invalid.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(invalid.append_text_range_comment(2U, 0U, 1U, 1U, "bad"), 0U);
    CHECK_EQ(invalid.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(invalid.append_text_range_comment(0U, 6U, 2U, 5U, ""), 0U);
    CHECK_EQ(invalid.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid.set_paragraph_text_comment_range(0U, 0U, 0U, 1U));
    CHECK_EQ(invalid.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
    fs::remove(invalid_target);
}

TEST_CASE("review comment inspection preserves nested anchor text") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "review_comments_nested_anchor_text.docx";
    fs::remove(target);

    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t xml:space="preserve">Alpha </w:t></w:r>
      <w:r><w:t>Beta</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.append_paragraph_text_comment(
                 0U, 0U, 10U, "Outer comment", "Outer", "OC",
                 "2026-05-03T14:00:00Z"),
             1U);
    CHECK_EQ(doc.append_paragraph_text_comment(
                 0U, 6U, 4U, "Inner comment", "Inner", "IC",
                 "2026-05-03T14:01:00Z"),
             1U);

    auto comments = doc.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Alpha Beta");
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Beta");
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Alpha Beta");
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Beta");

    fs::remove(target);
}

TEST_CASE("revisions can be accepted and rejected") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_revisions_accept_reject.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:ins w:id="1" w:author="Ada"><w:r><w:t>Accepted insertion</w:t></w:r></w:ins>
      <w:del w:id="2" w:author="Grace"><w:r><w:delText>Rejected deletion</w:delText></w:r></w:del>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    REQUIRE_EQ(doc.list_revisions().size(), 2U);
    CHECK(doc.accept_revision(0U));
    REQUIRE_EQ(doc.list_revisions().size(), 1U);
    CHECK(doc.reject_revision(0U));
    CHECK(doc.list_revisions().empty());

    CHECK_FALSE(doc.save());
    auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_xml.find("<w:ins"), std::string::npos);
    CHECK_EQ(saved_xml.find("<w:del"), std::string::npos);
    CHECK_EQ(saved_xml.find("w:delText"), std::string::npos);
    CHECK_NE(saved_xml.find("Accepted insertion"), std::string::npos);
    CHECK_NE(saved_xml.find("Rejected deletion"), std::string::npos);

    const fs::path accept_all_target = fs::current_path() / "review_revisions_accept_all.docx";
    fs::remove(accept_all_target);
    write_test_docx(accept_all_target, document_xml);
    featherdoc::Document accept_all_doc(accept_all_target);
    CHECK_FALSE(accept_all_doc.open());
    CHECK_EQ(accept_all_doc.accept_all_revisions(), 2U);
    CHECK(accept_all_doc.list_revisions().empty());
    CHECK_FALSE(accept_all_doc.save());
    saved_xml = read_test_docx_entry(accept_all_target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("Accepted insertion"), std::string::npos);
    CHECK_EQ(saved_xml.find("Rejected deletion"), std::string::npos);

    const fs::path reject_all_target = fs::current_path() / "review_revisions_reject_all.docx";
    fs::remove(reject_all_target);
    write_test_docx(reject_all_target, document_xml);
    featherdoc::Document reject_all_doc(reject_all_target);
    CHECK_FALSE(reject_all_doc.open());
    CHECK_EQ(reject_all_doc.reject_all_revisions(), 2U);
    CHECK(reject_all_doc.list_revisions().empty());
    CHECK_FALSE(reject_all_doc.save());
    saved_xml = read_test_docx_entry(reject_all_target, test_document_xml_entry);
    CHECK_EQ(saved_xml.find("Accepted insertion"), std::string::npos);
    CHECK_NE(saved_xml.find("Rejected deletion"), std::string::npos);

    fs::remove(target);
    fs::remove(accept_all_target);
    fs::remove(reject_all_target);
}

TEST_CASE("revision authoring APIs append insertion and deletion markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_revisions_authoring.docx";
    fs::remove(target);
    write_test_docx(target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>Revision authoring fixture</w:t></w:r></w:p></w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.append_insertion_revision("Inserted authored revision", "Ada",
                                           "2026-05-02T10:00:00Z"),
             1U);
    CHECK_EQ(doc.append_deletion_revision("Deleted authored revision", "Grace",
                                          "2026-05-02T11:00:00Z"),
             1U);

    auto revisions = doc.list_revisions();
    REQUIRE_EQ(revisions.size(), 2U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].id, "1");
    REQUIRE(revisions[0].author.has_value());
    CHECK_EQ(*revisions[0].author, "Ada");
    REQUIRE(revisions[0].date.has_value());
    CHECK_EQ(*revisions[0].date, "2026-05-02T10:00:00Z");
    CHECK_EQ(revisions[0].text, "Inserted authored revision");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[1].id, "2");
    REQUIRE(revisions[1].author.has_value());
    CHECK_EQ(*revisions[1].author, "Grace");
    CHECK_EQ(revisions[1].text, "Deleted authored revision");

    featherdoc::revision_metadata_update insertion_metadata;
    insertion_metadata.author = "Ada Updated";
    insertion_metadata.date = "2026-05-02T10:30:00Z";
    CHECK(doc.set_revision_metadata(0U, insertion_metadata));
    featherdoc::revision_metadata_update deletion_metadata;
    deletion_metadata.clear_author = true;
    deletion_metadata.date = "2026-05-02T11:30:00Z";
    CHECK(doc.set_revision_metadata(1U, deletion_metadata));
    revisions = doc.list_revisions();
    REQUIRE_EQ(revisions.size(), 2U);
    CHECK_EQ(revisions[0].author,
             std::optional<std::string>{"Ada Updated"});
    CHECK_EQ(revisions[0].date,
             std::optional<std::string>{"2026-05-02T10:30:00Z"});
    CHECK_FALSE(revisions[1].author.has_value());
    CHECK_EQ(revisions[1].date,
             std::optional<std::string>{"2026-05-02T11:30:00Z"});

    CHECK_FALSE(doc.save());
    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(R"(<w:ins w:id="1" w:author="Ada Updated" w:date="2026-05-02T10:30:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="2" w:date="2026-05-02T11:30:00Z">)"),
             std::string::npos);
    CHECK_EQ(saved_xml.find("Grace"), std::string::npos);
    CHECK_NE(saved_xml.find("<w:t>Inserted authored revision</w:t>"),
             std::string::npos);
    CHECK_NE(saved_xml.find("<w:delText>Deleted authored revision</w:delText>"),
             std::string::npos);

    featherdoc::Document invalid_doc(target);
    CHECK_FALSE(invalid_doc.open());
    CHECK_EQ(invalid_doc.append_insertion_revision(""), 0U);
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    featherdoc::revision_metadata_update empty_metadata;
    CHECK_FALSE(invalid_doc.set_revision_metadata(0U, empty_metadata));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("revision authoring APIs create in-place run revisions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_revisions_run_authoring.docx";
    fs::remove(target);
    write_test_docx(target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Alpha </w:t></w:r>
      <w:r><w:rPr><w:b/></w:rPr><w:t>Beta</w:t></w:r>
      <w:r><w:t> Gamma</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.insert_run_revision_after(0U, 0U, "Inserted run", "Ada",
                                        "2026-05-02T14:00:00Z"));
    CHECK(doc.delete_run_revision(0U, 1U, "Grace",
                                  "2026-05-02T15:00:00Z"));
    CHECK(doc.replace_run_revision(0U, 1U, "Replacement run", "Linus",
                                   "2026-05-02T16:00:00Z"));

    auto revisions = doc.list_revisions();
    REQUIRE_EQ(revisions.size(), 4U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].text, "Inserted run");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[1].text, "Beta");
    CHECK_EQ(revisions[2].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[2].text, " Gamma");
    CHECK_EQ(revisions[3].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[3].text, "Replacement run");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_EQ(revisions[3].id, "4");

    CHECK_FALSE(doc.save());
    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(R"(<w:ins w:id="1" w:author="Ada" w:date="2026-05-02T14:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="2" w:author="Grace" w:date="2026-05-02T15:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="3" w:author="Linus" w:date="2026-05-02T16:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:ins w:id="4" w:author="Linus" w:date="2026-05-02T16:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find("<w:delText>Beta</w:delText>"), std::string::npos);
    CHECK_NE(saved_xml.find("<w:t>Replacement run</w:t>"), std::string::npos);

    featherdoc::Document accepted(target);
    CHECK_FALSE(accepted.open());
    CHECK_EQ(accepted.accept_all_revisions(), 4U);
    CHECK_FALSE(accepted.save());
    featherdoc::Document accepted_reopened(target);
    CHECK_FALSE(accepted_reopened.open());
    CHECK_EQ(collect_document_text(accepted_reopened),
             "Alpha Inserted runReplacement run\n");

    const fs::path reject_target = fs::current_path() / "review_revisions_run_authoring_reject.docx";
    fs::remove(reject_target);
    write_test_docx(reject_target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>One</w:t></w:r><w:r><w:t>Two</w:t></w:r></w:p></w:body>
</w:document>
)");
    featherdoc::Document rejected(reject_target);
    CHECK_FALSE(rejected.open());
    CHECK(rejected.insert_run_revision_after(0U, 0U, "Add"));
    CHECK(rejected.delete_run_revision(0U, 1U));
    CHECK_EQ(rejected.reject_all_revisions(), 2U);
    CHECK_FALSE(rejected.save());
    featherdoc::Document rejected_reopened(reject_target);
    CHECK_FALSE(rejected_reopened.open());
    CHECK_EQ(collect_document_text(rejected_reopened), "OneTwo\n");

    featherdoc::Document invalid_doc(target);
    CHECK_FALSE(invalid_doc.open());
    CHECK_FALSE(invalid_doc.insert_run_revision_after(9U, 0U, "bad"));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.delete_run_revision(0U, 9U));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.replace_run_revision(0U, 0U, ""));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
    fs::remove(reject_target);
}

TEST_CASE("revision authoring APIs create paragraph text range revisions") {
    namespace fs = std::filesystem;

    const auto write_range_source = [](const fs::path &path) {
        write_test_docx(path,
                        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Alpha </w:t></w:r>
      <w:r><w:rPr><w:b/></w:rPr><w:t>Beta</w:t></w:r>
      <w:r><w:t> Gamma</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");
    };

    const fs::path insert_target =
        fs::current_path() / "review_revisions_paragraph_text_insert.docx";
    const fs::path delete_target =
        fs::current_path() / "review_revisions_paragraph_text_delete.docx";
    const fs::path replace_target =
        fs::current_path() / "review_revisions_paragraph_text_replace.docx";
    const fs::path reject_target =
        fs::current_path() / "review_revisions_paragraph_text_reject.docx";
    const fs::path guard_target =
        fs::current_path() / "review_revisions_paragraph_text_guard.docx";
    const fs::path invalid_target =
        fs::current_path() / "review_revisions_paragraph_text_invalid.docx";

    fs::remove(insert_target);
    fs::remove(delete_target);
    fs::remove(replace_target);
    fs::remove(reject_target);
    fs::remove(guard_target);
    fs::remove(invalid_target);

    write_range_source(insert_target);
    featherdoc::Document inserted(insert_target);
    CHECK_FALSE(inserted.open());
    CHECK(inserted.insert_paragraph_text_revision(
        0U, 6U, "Inserted ", "Ada", "2026-05-02T20:00:00Z"));
    auto revisions = inserted.list_revisions();
    REQUIRE_EQ(revisions.size(), 1U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].text, "Inserted ");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_FALSE(inserted.save());
    featherdoc::Document accepted_insert(insert_target);
    CHECK_FALSE(accepted_insert.open());
    CHECK_EQ(accepted_insert.accept_all_revisions(), 1U);
    CHECK_FALSE(accepted_insert.save());
    featherdoc::Document accepted_insert_reopened(insert_target);
    CHECK_FALSE(accepted_insert_reopened.open());
    CHECK_EQ(collect_document_text(accepted_insert_reopened),
             "Alpha Inserted Beta Gamma\n");

    write_range_source(delete_target);
    featherdoc::Document deleted(delete_target);
    CHECK_FALSE(deleted.open());
    featherdoc::revision_text_range_options delete_options;
    delete_options.author = "Grace";
    delete_options.date = "2026-05-02T21:00:00Z";
    delete_options.expected_text = "ha Beta";
    CHECK(deleted.delete_paragraph_text_revision(0U, 3U, 7U,
                                                 delete_options));
    revisions = deleted.list_revisions();
    REQUIRE_EQ(revisions.size(), 1U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[0].text, "ha Beta");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_FALSE(deleted.save());
    auto saved_xml = read_test_docx_entry(delete_target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="1" w:author="Grace" w:date="2026-05-02T21:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:delText xml:space="preserve">ha </w:delText>)"),
             std::string::npos);
    CHECK_NE(saved_xml.find("<w:delText>Beta</w:delText>"), std::string::npos);
    featherdoc::Document accepted_delete(delete_target);
    CHECK_FALSE(accepted_delete.open());
    CHECK_EQ(accepted_delete.accept_all_revisions(), 1U);
    CHECK_FALSE(accepted_delete.save());
    featherdoc::Document accepted_delete_reopened(delete_target);
    CHECK_FALSE(accepted_delete_reopened.open());
    CHECK_EQ(collect_document_text(accepted_delete_reopened), "Alp Gamma\n");

    write_range_source(replace_target);
    featherdoc::Document replaced(replace_target);
    CHECK_FALSE(replaced.open());
    featherdoc::revision_text_range_options replace_options;
    replace_options.author = "Linus";
    replace_options.date = "2026-05-02T22:00:00Z";
    replace_options.expected_text = "ha Beta";
    CHECK(replaced.replace_paragraph_text_revision(0U, 3U, 7U, "Range",
                                                   replace_options));
    revisions = replaced.list_revisions();
    REQUIRE_EQ(revisions.size(), 2U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[0].text, "ha Beta");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[1].text, "Range");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_EQ(revisions[1].id, "2");
    CHECK_FALSE(replaced.save());
    featherdoc::Document accepted_replace(replace_target);
    CHECK_FALSE(accepted_replace.open());
    CHECK_EQ(accepted_replace.accept_all_revisions(), 2U);
    CHECK_FALSE(accepted_replace.save());
    featherdoc::Document accepted_replace_reopened(replace_target);
    CHECK_FALSE(accepted_replace_reopened.open());
    CHECK_EQ(collect_document_text(accepted_replace_reopened),
             "AlpRange Gamma\n");

    write_range_source(reject_target);
    featherdoc::Document rejected(reject_target);
    CHECK_FALSE(rejected.open());
    CHECK(rejected.replace_paragraph_text_revision(0U, 3U, 7U, "Range"));
    CHECK_EQ(rejected.reject_all_revisions(), 2U);
    CHECK_FALSE(rejected.save());
    featherdoc::Document rejected_reopened(reject_target);
    CHECK_FALSE(rejected_reopened.open());
    CHECK_EQ(collect_document_text(rejected_reopened), "Alpha Beta Gamma\n");

    write_range_source(guard_target);
    featherdoc::Document guarded(guard_target);
    CHECK_FALSE(guarded.open());
    featherdoc::revision_text_range_options guard_options;
    guard_options.author = "Noop";
    guard_options.expected_text = "Wrong";
    CHECK_FALSE(guarded.delete_paragraph_text_revision(0U, 3U, 7U,
                                                       guard_options));
    CHECK_EQ(guarded.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(guarded.last_error().detail.find(
                 "expected text did not match selected text"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("expected: Wrong"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("actual: ha Beta"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("start_paragraph_index: 0"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("text_offset=3"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("text_length=7"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("text=ha Beta"),
             std::string::npos);
    CHECK_EQ(guarded.list_revisions().size(), 0U);
    CHECK_EQ(collect_document_text(guarded), "Alpha Beta Gamma\n");

    write_range_source(invalid_target);
    featherdoc::Document invalid_doc(invalid_target);
    CHECK_FALSE(invalid_doc.open());
    CHECK_FALSE(invalid_doc.insert_paragraph_text_revision(0U, 99U, "bad"));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.delete_paragraph_text_revision(0U, 0U, 0U));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.replace_paragraph_text_revision(0U, 0U, 99U, "bad"));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(insert_target);
    fs::remove(delete_target);
    fs::remove(replace_target);
    fs::remove(reject_target);
    fs::remove(guard_target);
    fs::remove(invalid_target);
}

TEST_CASE("revision authoring APIs create cross-paragraph text range revisions") {
    namespace fs = std::filesystem;

    const auto write_cross_paragraph_source = [](const fs::path &path) {
        write_test_docx(path,
                        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t xml:space="preserve">Alpha </w:t></w:r>
      <w:r><w:rPr><w:b/></w:rPr><w:t>Beta</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Middle </w:t></w:r>
      <w:r><w:rPr><w:i/></w:rPr><w:t>Text</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Gamma</w:t></w:r>
      <w:r><w:t>Delta</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");
    };

    const fs::path insert_target =
        fs::current_path() / "review_revisions_text_range_insert.docx";
    const fs::path delete_target =
        fs::current_path() / "review_revisions_text_range_delete.docx";
    const fs::path replace_target =
        fs::current_path() / "review_revisions_text_range_replace.docx";
    const fs::path reject_target =
        fs::current_path() / "review_revisions_text_range_reject.docx";
    const fs::path guard_target =
        fs::current_path() / "review_revisions_text_range_guard.docx";
    const fs::path invalid_target =
        fs::current_path() / "review_revisions_text_range_invalid.docx";

    fs::remove(insert_target);
    fs::remove(delete_target);
    fs::remove(replace_target);
    fs::remove(reject_target);
    fs::remove(guard_target);
    fs::remove(invalid_target);

    write_cross_paragraph_source(insert_target);
    featherdoc::Document inserted(insert_target);
    CHECK_FALSE(inserted.open());
    CHECK(inserted.insert_text_range_revision(1U, 7U, "Inserted ", "Ada",
                                              "2026-05-02T23:00:00Z"));
    auto revisions = inserted.list_revisions();
    REQUIRE_EQ(revisions.size(), 1U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].text, "Inserted ");
    CHECK_FALSE(inserted.save());
    featherdoc::Document accepted_insert(insert_target);
    CHECK_FALSE(accepted_insert.open());
    CHECK_EQ(accepted_insert.accept_all_revisions(), 1U);
    CHECK_FALSE(accepted_insert.save());
    featherdoc::Document accepted_insert_reopened(insert_target);
    CHECK_FALSE(accepted_insert_reopened.open());
    CHECK_EQ(collect_document_text(accepted_insert_reopened),
             "Alpha Beta\nMiddle Inserted Text\nGammaDelta\n");

    write_cross_paragraph_source(delete_target);
    featherdoc::Document deleted(delete_target);
    CHECK_FALSE(deleted.open());
    const auto preview =
        deleted.preview_text_range(0U, 6U, 2U, 5U);
    REQUIRE(preview.has_value());
    CHECK_EQ(preview->start_paragraph_index, 0U);
    CHECK_EQ(preview->start_text_offset, 6U);
    CHECK_EQ(preview->end_paragraph_index, 2U);
    CHECK_EQ(preview->end_text_offset, 5U);
    CHECK_EQ(preview->text_length, 20U);
    CHECK(preview->plain_text_runs_supported);
    CHECK_EQ(preview->text, "BetaMiddle TextGamma");
    REQUIRE_EQ(preview->segments.size(), 3U);
    CHECK_EQ(preview->segments[0].paragraph_index, 0U);
    CHECK_EQ(preview->segments[0].text, "Beta");
    CHECK_EQ(preview->segments[1].paragraph_index, 1U);
    CHECK_EQ(preview->segments[1].text, "Middle Text");
    CHECK_EQ(preview->segments[2].paragraph_index, 2U);
    CHECK_EQ(preview->segments[2].text, "Gamma");
    featherdoc::revision_text_range_options delete_options;
    delete_options.author = "Grace";
    delete_options.date = "2026-05-03T00:00:00Z";
    delete_options.expected_text = "BetaMiddle TextGamma";
    CHECK(deleted.delete_text_range_revision(0U, 6U, 2U, 5U,
                                             delete_options));
    revisions = deleted.list_revisions();
    REQUIRE_EQ(revisions.size(), 3U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[0].text, "Beta");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[1].text, "Middle Text");
    CHECK_EQ(revisions[2].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[2].text, "Gamma");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_EQ(revisions[2].id, "3");
    CHECK_FALSE(deleted.save());
    auto saved_xml = read_test_docx_entry(delete_target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="1" w:author="Grace" w:date="2026-05-03T00:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:delText>Beta</w:delText>)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:delText xml:space="preserve">Middle </w:delText>)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:delText>Gamma</w:delText>)"),
             std::string::npos);
    featherdoc::Document accepted_delete(delete_target);
    CHECK_FALSE(accepted_delete.open());
    CHECK_EQ(accepted_delete.accept_all_revisions(), 3U);
    CHECK_FALSE(accepted_delete.save());
    featherdoc::Document accepted_delete_reopened(delete_target);
    CHECK_FALSE(accepted_delete_reopened.open());
    CHECK_EQ(collect_document_text(accepted_delete_reopened),
             "Alpha \n\nDelta\n");

    write_cross_paragraph_source(replace_target);
    featherdoc::Document replaced(replace_target);
    CHECK_FALSE(replaced.open());
    featherdoc::revision_text_range_options replace_options;
    replace_options.author = "Linus";
    replace_options.date = "2026-05-03T01:00:00Z";
    replace_options.expected_text = "BetaMiddle TextGamma";
    CHECK(replaced.replace_text_range_revision(0U, 6U, 2U, 5U, "Range ",
                                               replace_options));
    revisions = replaced.list_revisions();
    REQUIRE_EQ(revisions.size(), 4U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[0].text, "Beta");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[1].text, "Range ");
    CHECK_EQ(revisions[2].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[2].text, "Middle Text");
    CHECK_EQ(revisions[3].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[3].text, "Gamma");
    CHECK_FALSE(replaced.save());
    featherdoc::Document accepted_replace(replace_target);
    CHECK_FALSE(accepted_replace.open());
    CHECK_EQ(accepted_replace.accept_all_revisions(), 4U);
    CHECK_FALSE(accepted_replace.save());
    featherdoc::Document accepted_replace_reopened(replace_target);
    CHECK_FALSE(accepted_replace_reopened.open());
    CHECK_EQ(collect_document_text(accepted_replace_reopened),
             "Alpha Range \n\nDelta\n");

    write_cross_paragraph_source(reject_target);
    featherdoc::Document rejected(reject_target);
    CHECK_FALSE(rejected.open());
    CHECK(rejected.replace_text_range_revision(0U, 6U, 2U, 5U, "Range "));
    CHECK_EQ(rejected.reject_all_revisions(), 4U);
    CHECK_FALSE(rejected.save());
    featherdoc::Document rejected_reopened(reject_target);
    CHECK_FALSE(rejected_reopened.open());
    CHECK_EQ(collect_document_text(rejected_reopened),
             "Alpha Beta\nMiddle Text\nGammaDelta\n");

    write_cross_paragraph_source(guard_target);
    featherdoc::Document guarded(guard_target);
    CHECK_FALSE(guarded.open());
    featherdoc::revision_text_range_options guard_options;
    guard_options.author = "Noop";
    guard_options.expected_text = "Wrong";
    CHECK_FALSE(guarded.replace_text_range_revision(0U, 6U, 2U, 5U,
                                                    "Range ", guard_options));
    CHECK_EQ(guarded.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(guarded.last_error().detail.find(
                 "expected text did not match selected text"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("expected: Wrong"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find(
                 "actual: BetaMiddle TextGamma"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("start_paragraph_index: 0"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("end_paragraph_index: 2"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("paragraph_index=1"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("text=Middle Text"),
             std::string::npos);
    CHECK_EQ(guarded.list_revisions().size(), 0U);
    CHECK_EQ(collect_document_text(guarded),
             "Alpha Beta\nMiddle Text\nGammaDelta\n");

    write_cross_paragraph_source(invalid_target);
    featherdoc::Document invalid_doc(invalid_target);
    CHECK_FALSE(invalid_doc.open());
    CHECK_FALSE(invalid_doc.preview_text_range(2U, 0U, 1U, 1U).has_value());
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.delete_text_range_revision(2U, 0U, 1U, 1U));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.delete_text_range_revision(0U, 11U, 1U, 1U));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.replace_text_range_revision(0U, 10U, 1U, 0U,
                                                        "bad"));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.replace_text_range_revision(0U, 6U, 2U, 5U, ""));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(insert_target);
    fs::remove(delete_target);
    fs::remove(replace_target);
    fs::remove(reject_target);
    fs::remove(guard_target);
    fs::remove(invalid_target);
}

TEST_CASE("find text ranges locates body text across paragraphs") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "find_text_ranges_body.docx";
    fs::remove(target);
    write_test_docx(target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t xml:space="preserve">Alpha </w:t></w:r>
      <w:r><w:t>Beta</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Middle </w:t></w:r>
      <w:r><w:t>Text</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Beta</w:t></w:r>
      <w:r><w:t xml:space="preserve"> Tail</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");

    featherdoc::Document document(target);
    CHECK_FALSE(document.open());

    const auto beta_matches = document.find_text_ranges("Beta");
    REQUIRE_EQ(beta_matches.size(), 2U);
    CHECK_EQ(beta_matches[0].start_paragraph_index, 0U);
    CHECK_EQ(beta_matches[0].start_text_offset, 6U);
    CHECK_EQ(beta_matches[0].end_paragraph_index, 0U);
    CHECK_EQ(beta_matches[0].end_text_offset, 10U);
    CHECK_EQ(beta_matches[0].text, "Beta");
    CHECK_EQ(beta_matches[1].start_paragraph_index, 2U);
    CHECK_EQ(beta_matches[1].start_text_offset, 0U);
    CHECK_EQ(beta_matches[1].end_paragraph_index, 2U);
    CHECK_EQ(beta_matches[1].end_text_offset, 4U);
    CHECK_EQ(beta_matches[1].text, "Beta");

    const auto cross_match = document.find_text_ranges("BetaMiddle TextBeta");
    REQUIRE_EQ(cross_match.size(), 1U);
    CHECK_EQ(cross_match[0].start_paragraph_index, 0U);
    CHECK_EQ(cross_match[0].start_text_offset, 6U);
    CHECK_EQ(cross_match[0].end_paragraph_index, 2U);
    CHECK_EQ(cross_match[0].end_text_offset, 4U);
    CHECK_EQ(cross_match[0].text, "BetaMiddle TextBeta");
    REQUIRE_EQ(cross_match[0].segments.size(), 3U);
    CHECK_EQ(cross_match[0].segments[0].text, "Beta");
    CHECK_EQ(cross_match[0].segments[1].text, "Middle Text");
    CHECK_EQ(cross_match[0].segments[2].text, "Beta");

    CHECK(document.find_text_ranges("Missing").empty());
    CHECK_FALSE(document.last_error().code);
    CHECK(document.find_text_ranges("").empty());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("validate_template reports missing required bookmarks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_missing.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {
            {"customer", featherdoc::template_slot_kind::text, true},
            {"invoice", featherdoc::template_slot_kind::text, true},
            {"notes", featherdoc::template_slot_kind::block, false},
        });

    CHECK_FALSE(doc.last_error());
    REQUIRE(result.missing_required.size() == 1);
    CHECK_EQ(result.missing_required.front(), "invoice");
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template reports duplicate bookmark names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_duplicates.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>customer A: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>first</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>customer B: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="customer"/>
      <w:r><w:t>second</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {{"customer", featherdoc::template_slot_kind::text, true}});

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    REQUIRE(result.duplicate_bookmarks.size() == 1);
    CHECK_EQ(result.duplicate_bookmarks.front(), "customer");
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template reports malformed block placeholders") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_malformed.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t> suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {{"items", featherdoc::template_slot_kind::block, true}});

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    REQUIRE(result.malformed_placeholders.size() == 1);
    CHECK_EQ(result.malformed_placeholders.front(), "items");
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template passes a valid mixed bookmark template") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_valid.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="intro_block"/>
      <w:r><w:t>intro placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="2" w:name="line_items"/>
            <w:r><w:t>row placeholder</w:t></w:r>
            <w:bookmarkEnd w:id="2"/>
          </w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:bookmarkStart w:id="3" w:name="summary_table"/>
      <w:r><w:t>table placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="3"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="4" w:name="signature_image"/>
      <w:r><w:t>image placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="4"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="5" w:name="seal_image"/>
      <w:r><w:t>floating image placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="5"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {
            {"customer", featherdoc::template_slot_kind::text, true},
            {"intro_block", featherdoc::template_slot_kind::block, true},
            {"line_items", featherdoc::template_slot_kind::table_rows, true},
            {"summary_table", featherdoc::template_slot_kind::table, true},
            {"signature_image", featherdoc::template_slot_kind::image, true},
            {"seal_image", featherdoc::template_slot_kind::floating_image, true},
            {"notes", featherdoc::template_slot_kind::text, false},
        });

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template reports unexpected bookmarks kind mismatches and occurrence mismatches") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_schema_v2.docx";
    fs::remove(target);

    const std::string document_xml =
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
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {
            {"summary_block", featherdoc::template_slot_kind::text, true},
            {"approver", featherdoc::template_slot_kind::text, true, 2U, 2U},
        });

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    REQUIRE(result.unexpected_bookmarks.size() == 1U);
    CHECK_EQ(result.unexpected_bookmarks.front().bookmark_name, "extra_slot");
    CHECK_EQ(result.unexpected_bookmarks.front().occurrence_count, 1U);
    CHECK_EQ(result.unexpected_bookmarks.front().kind, featherdoc::bookmark_kind::text);
    REQUIRE(result.kind_mismatches.size() == 1U);
    CHECK_EQ(result.kind_mismatches.front().bookmark_name, "summary_block");
    CHECK_EQ(result.kind_mismatches.front().expected_kind,
             featherdoc::template_slot_kind::text);
    CHECK_EQ(result.kind_mismatches.front().actual_kind,
             featherdoc::bookmark_kind::block);
    CHECK_EQ(result.kind_mismatches.front().occurrence_count, 1U);
    REQUIRE(result.occurrence_mismatches.size() == 1U);
    CHECK_EQ(result.occurrence_mismatches.front().bookmark_name, "approver");
    CHECK_EQ(result.occurrence_mismatches.front().actual_occurrences, 1U);
    CHECK_EQ(result.occurrence_mismatches.front().min_occurrences, 2U);
    REQUIRE(result.occurrence_mismatches.front().max_occurrences.has_value());
    CHECK_EQ(*result.occurrence_mismatches.front().max_occurrences, 2U);
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template allows declared multi occurrence bookmarks") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_multi_occurrence.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>first: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>first</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>second: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="customer"/>
      <w:r><w:t>second</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template(
        {{"customer", featherdoc::template_slot_kind::text, true, 2U, 2U}});

    CHECK_FALSE(doc.last_error());
    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table swaps a standalone bookmark paragraph for a table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const std::vector<std::vector<std::string>> rows{
        {"Name", "Qty"},
        {"Apple", "2"},
        {"Pear", "5"},
    };
    CHECK_EQ(doc.replace_bookmark_with_table("items", rows), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nApple\n2\nPear\n5\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:tbl>"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:tbl\nw:p\n");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nApple\n2\nPear\n5\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_paragraphs expands a standalone bookmark paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_paragraphs.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_paragraphs("items", {"Apple", "Pear"}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nApple\nPear\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:t>Apple</w:t>"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:t>Pear</w:t>"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    std::ostringstream child_order;
    const auto body = xml_document.child("w:document").child("w:body");
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        child_order << child.name() << '\n';
    }
    CHECK_EQ(child_order.str(), "w:p\nw:p\nw:p\nw:p\n");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nApple\nPear\nafter\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_paragraphs accepts an empty replacement list") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_paragraphs_empty.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_paragraphs("items", std::vector<std::string>{}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");

    fs::remove(target);
}

TEST_CASE("remove_bookmark_block deletes a standalone bookmark paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_remove_block.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="items"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.remove_bookmark_block("missing"), 0);
    CHECK_FALSE(doc.last_error());

    CHECK_EQ(doc.remove_bookmark_block("items"), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_document_text(doc), "before\nafter\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("placeholder"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"items\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\nafter\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows expands a template row inside an existing table") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows.docx";
    fs::remove(target);

    const std::string document_xml =
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
        <w:trPr><w:cantSplit/></w:trPr>
        <w:tc>
          <w:tcPr><w:shd w:fill="AAAAAA"/></w:tcPr>
          <w:p>
            <w:pPr><w:jc w:val="center"/></w:pPr>
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:rPr><w:b/></w:rPr><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc>
          <w:tcPr><w:shd w:fill="BBBBBB"/></w:tcPr>
          <w:p>
            <w:pPr><w:jc w:val="right"/></w:pPr>
            <w:r><w:rPr><w:i/></w:rPr><w:t>template qty</w:t></w:r>
          </w:p>
        </w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows(
                 "item_row", {{"Apple", "2"}, {"Pear", "5"}}),
             1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nApple\n2\nPear\n5\nTotal\n7\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("template name"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template qty"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"item_row\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:fill=\"AAAAAA\""), 2);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:fill=\"BBBBBB\""), 2);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto table = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table != pugi::xml_node{});
    CHECK_EQ(count_named_children(table, "w:tr"), 4);
    CHECK_EQ(count_named_descendants(table, "w:cantSplit"), 2);
    CHECK_EQ(count_named_descendants(table, "w:b"), 2);
    CHECK_EQ(count_named_descendants(table, "w:i"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nApple\n2\nPear\n5\nTotal\n7\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows accepts an empty replacement list") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows_empty.docx";
    fs::remove(target);

    const std::string document_xml =
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
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows("item_row", {}), 1);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(collect_table_text(doc), "Name\nQty\nTotal\n7\n");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("template name"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template qty"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"item_row\""), std::string::npos);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto table = xml_document.child("w:document").child("w:body").child("w:tbl");
    REQUIRE(table != pugi::xml_node{});
    CHECK_EQ(count_named_children(table, "w:tr"), 2);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_table_text(reopened), "Name\nQty\nTotal\n7\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_table_rows rejects mismatched row widths") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "bookmark_replace_table_rows_validation.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
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
    </w:tbl>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_with_table_rows("item_row", {{"Apple"}}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("cell count"), std::string::npos);
    CHECK_EQ(collect_table_text(doc), "template name\ntemplate qty\n");

    fs::remove(target);
}

TEST_CASE("replace_bookmark_with_image swaps a standalone bookmark paragraph for an inline image") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "bookmark_replace_image.docx";
    const fs::path image_path = fs::current_path() / "bookmark_replace_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="logo"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.replace_bookmark_with_image("logo", image_path, 20U, 10U), 1);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:drawing"), std::string::npos);
    CHECK_NE(saved_document_xml.find("cx=\"190500\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("cy=\"95250\""), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"logo\""), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"media/image1.png\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\n\nafter\n");

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("replace_bookmark_with_floating_image swaps a standalone bookmark paragraph for an anchored image") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "bookmark_replace_floating_image.docx";
    const fs::path image_path = fs::current_path() / "bookmark_replace_floating_image.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="logo"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::floating_image_options options;
    options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    options.horizontal_offset_px = 24;
    options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    options.vertical_offset_px = -8;
    options.behind_text = true;
    options.allow_overlap = false;
    options.wrap_mode = featherdoc::floating_image_wrap_mode::top_bottom;
    options.wrap_distance_top_px = 6U;
    options.wrap_distance_bottom_px = 10U;
    options.crop = featherdoc::floating_image_crop{40U, 0U, 20U, 60U};

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.replace_bookmark_with_floating_image("logo", image_path, 20U, 10U, options),
             1);
    CHECK_FALSE(doc.last_error());

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/media/image1.png"));
    CHECK_EQ(read_test_docx_entry(target, "word/media/image1.png"), image_data);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<wp:anchor"), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"page\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("relativeFrom=\"margin\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>228600</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:posOffset>-76200</wp:posOffset>"),
             std::string::npos);
    CHECK_NE(saved_document_xml.find("behindDoc=\"1\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("allowOverlap=\"0\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distT=\"57150\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("distB=\"95250\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("<wp:wrapTopAndBottom"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<a:srcRect l=\"4000\" t=\"0\" r=\"2000\" b=\"6000\""),
             std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:name=\"logo\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before\n\nafter\n");
    const auto drawing_images = reopened.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(drawing_images[0].width_px, 20U);
    CHECK_EQ(drawing_images[0].height_px, 10U);
    REQUIRE(drawing_images[0].floating_options.has_value());
    CHECK_EQ(drawing_images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::top_bottom);
    REQUIRE(drawing_images[0].floating_options->crop.has_value());
    CHECK_EQ(drawing_images[0].floating_options->crop->left_per_mille, 40U);
    CHECK_EQ(drawing_images[0].floating_options->crop->top_per_mille, 0U);
    CHECK_EQ(drawing_images[0].floating_options->crop->right_per_mille, 20U);
    CHECK_EQ(drawing_images[0].floating_options->crop->bottom_per_mille, 60U);

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("block bookmark replacements reject bookmarks that do not occupy their own paragraph") {
    namespace fs = std::filesystem;

    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    const fs::path target = fs::current_path() / "bookmark_replace_block_validation.docx";
    const fs::path image_path = fs::current_path() / "bookmark_replace_block_validation.png";
    fs::remove(target);
    fs::remove(image_path);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="block"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t> suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    const std::string image_data(reinterpret_cast<const char *>(tiny_png_bytes),
                                 sizeof(tiny_png_bytes));
    write_binary_file(image_path, image_data);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.remove_bookmark_block("block"), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("occupy its own paragraph"), std::string::npos);

    CHECK_EQ(doc.replace_bookmark_with_paragraphs("block", {"A", "B"}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_EQ(doc.replace_bookmark_with_table_rows("block", {{"A"}}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_EQ(doc.replace_bookmark_with_table("block", {{"A"}}), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_EQ(doc.replace_bookmark_with_image("block", image_path), 0);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
    fs::remove(image_path);
}

TEST_CASE("replace_bookmark_text preserves explicit line breaks inside bookmark ranges") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "bookmark_replace_multiline.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>prefix</w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="bookmark"/>
      <w:r><w:t>old value</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
      <w:r><w:t>suffix</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_bookmark_text("bookmark", "first line\nsecond line"), 1);
    CHECK_EQ(collect_document_text(doc), "prefixfirst line\nsecond linesuffix\n");

    CHECK_FALSE(doc.save());

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));

    const auto paragraph =
        xml_document.child("w:document").child("w:body").child("w:p");
    const auto bookmark_start = paragraph.child("w:bookmarkStart");
    REQUIRE(bookmark_start != pugi::xml_node{});
    const auto replacement_run = bookmark_start.next_sibling("w:r");
    REQUIRE(replacement_run != pugi::xml_node{});
    CHECK_EQ(std::string{replacement_run.child("w:t").text().get()}, "first line");
    REQUIRE(replacement_run.child("w:br") != pugi::xml_node{});
    CHECK_EQ(std::string{replacement_run.last_child().text().get()}, "second line");
    CHECK_NE(xml_text.find("w:bookmarkStart"), std::string::npos);
    CHECK_NE(xml_text.find("w:bookmarkEnd"), std::string::npos);
    CHECK_EQ(xml_text.find("old value"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened),
             "prefixfirst line\nsecond linesuffix\n");

    fs::remove(target);
}

TEST_CASE("validate_template rejects empty slot bookmark names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_validate_empty_slot_name.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto result = doc.validate_template(
        {{"", featherdoc::template_slot_kind::text, true}});

    CHECK(result.missing_required.empty());
    CHECK(result.duplicate_bookmarks.empty());
    CHECK(result.malformed_placeholders.empty());
    CHECK(result.unexpected_bookmarks.empty());
    CHECK(result.kind_mismatches.empty());
    CHECK(result.occurrence_mismatches.empty());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "template slot bookmark name must not be empty");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}
