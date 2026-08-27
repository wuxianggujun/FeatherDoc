#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "allocation_failure_test_case.hpp"
#include "basic_docx_archive_test_support.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <featherdoc.hpp>
#include <pugixml.hpp>

namespace {

[[nodiscard]] auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

enum class revision_mutation_kind : std::uint8_t {
    set_header_metadata,
    accept_footer_revision,
    reject_body_revision,
    accept_all,
    reject_all,
    insert_paragraph_revision,
    append_insertion_revision,
    append_deletion_revision,
    insert_body_run_revision,
    delete_body_run_revision,
    replace_body_run_revision,
};

struct revision_mutation_scenario final {
    revision_mutation_kind kind{};
    std::string_view name{};
    std::string_view allocation_entry{};
};

constexpr auto revision_scenarios = std::array{
    revision_mutation_scenario{revision_mutation_kind::set_header_metadata,
                               "set-header-metadata", "word/header1.xml"},
    revision_mutation_scenario{revision_mutation_kind::accept_footer_revision,
                               "accept-footer-revision", "word/footer1.xml"},
    revision_mutation_scenario{revision_mutation_kind::reject_body_revision,
                               "reject-body-revision", test_document_xml_entry},
    revision_mutation_scenario{
        revision_mutation_kind::accept_all, "accept-all-stories", {}},
    revision_mutation_scenario{
        revision_mutation_kind::reject_all, "reject-all-stories", {}},
    revision_mutation_scenario{
        revision_mutation_kind::insert_paragraph_revision,
        "insert-paragraph-revision", test_document_xml_entry},
    revision_mutation_scenario{
        revision_mutation_kind::append_insertion_revision,
        "append-insertion-revision", test_document_xml_entry},
    revision_mutation_scenario{
        revision_mutation_kind::append_deletion_revision,
        "append-deletion-revision", test_document_xml_entry},
    revision_mutation_scenario{revision_mutation_kind::insert_body_run_revision,
                               "insert-body-run-revision",
                               test_document_xml_entry},
    revision_mutation_scenario{revision_mutation_kind::delete_body_run_revision,
                               "delete-body-run-revision",
                               test_document_xml_entry},
    revision_mutation_scenario{
        revision_mutation_kind::replace_body_run_revision,
        "replace-body-run-revision", test_document_xml_entry},
};

const auto updated_revision_author = utf8_from_u8(u8"更新作者-中文-😀");
const auto inserted_revision_text = utf8_from_u8(u8"新增修订-中文-😀🪶");
const auto inserted_revision_author = utf8_from_u8(u8"审阅者😀");
const auto updated_revision_metadata = [] {
    auto metadata = featherdoc::revision_metadata_update{};
    metadata.author = updated_revision_author;
    metadata.date = "2026-07-17T12:34:56Z";
    return metadata;
}();

struct revision_story_handles final {
    featherdoc::Paragraph body;
    featherdoc::Paragraph header;
    featherdoc::Paragraph footer;
    featherdoc::Run body_run;
    featherdoc::Run header_run;
    featherdoc::Run footer_run;
};

struct revision_story_snapshot final {
    std::string body;
    std::string header;
    std::string footer;

    friend auto operator==(const revision_story_snapshot &left,
                           const revision_story_snapshot &right) -> bool {
        return left.body == right.body && left.header == right.header &&
               left.footer == right.footer;
    }
};

std::atomic_bool global_allocation_tracking_enabled{false};
std::atomic_size_t observed_global_allocation_calls{0U};
std::atomic_size_t global_allocation_failure_call{0U};

pugi::allocation_function delegated_pugi_allocate = nullptr;
std::size_t observed_pugi_allocation_calls = 0U;
std::size_t pugi_allocation_failure_call = 0U;

void record_global_allocation() {
    if (!global_allocation_tracking_enabled.load(std::memory_order_relaxed)) {
        return;
    }
    const auto call = observed_global_allocation_calls.fetch_add(
                          1U, std::memory_order_relaxed) +
                      1U;
    if (call ==
        global_allocation_failure_call.load(std::memory_order_relaxed)) {
        throw std::bad_alloc{};
    }
}

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                      \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
[[nodiscard]] auto allocate_controlled(std::size_t size) -> void * {
    record_global_allocation();
    if (void *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

[[nodiscard]] auto allocate_controlled_aligned(std::size_t size,
                                               std::size_t alignment)
    -> void * {
    record_global_allocation();
    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0U ? 1U : size) == 0) {
        return memory;
    }
    throw std::bad_alloc{};
}
#endif

auto controlled_pugi_allocate(std::size_t size) -> void * {
    ++observed_pugi_allocation_calls;
    if (pugi_allocation_failure_call != 0U &&
        observed_pugi_allocation_calls == pugi_allocation_failure_call) {
        return nullptr;
    }
    return delegated_pugi_allocate(size);
}

auto global_new_pugi_allocate(std::size_t size) noexcept -> void * {
    try {
        return ::operator new(size);
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}

void global_new_pugi_deallocate(void *memory) noexcept {
    ::operator delete(memory);
}

class pugi_failure_guard final {
  public:
    pugi_failure_guard()
        : previous_allocate_(pugi::get_memory_allocation_function()),
          previous_deallocate_(pugi::get_memory_deallocation_function()) {
        delegated_pugi_allocate = this->previous_allocate_;
        observed_pugi_allocation_calls = 0U;
        pugi_allocation_failure_call = 0U;
        pugi::set_memory_management_functions(controlled_pugi_allocate,
                                              this->previous_deallocate_);
    }

    pugi_failure_guard(const pugi_failure_guard &) = delete;
    auto operator=(const pugi_failure_guard &) -> pugi_failure_guard & = delete;

    ~pugi_failure_guard() {
        pugi::set_memory_management_functions(this->previous_allocate_,
                                              this->previous_deallocate_);
        delegated_pugi_allocate = nullptr;
        observed_pugi_allocation_calls = 0U;
        pugi_allocation_failure_call = 0U;
    }

  private:
    pugi::allocation_function previous_allocate_;
    pugi::deallocation_function previous_deallocate_;
};

class pugi_global_new_guard final {
  public:
    pugi_global_new_guard()
        : previous_allocate_(pugi::get_memory_allocation_function()),
          previous_deallocate_(pugi::get_memory_deallocation_function()) {
        pugi::set_memory_management_functions(global_new_pugi_allocate,
                                              global_new_pugi_deallocate);
    }

    pugi_global_new_guard(const pugi_global_new_guard &) = delete;
    auto operator=(const pugi_global_new_guard &)
        -> pugi_global_new_guard & = delete;

    ~pugi_global_new_guard() {
        pugi::set_memory_management_functions(this->previous_allocate_,
                                              this->previous_deallocate_);
    }

  private:
    pugi::allocation_function previous_allocate_;
    pugi::deallocation_function previous_deallocate_;
};

class global_allocation_window final {
  public:
    explicit global_allocation_window(std::size_t failure_call) noexcept {
        observed_global_allocation_calls.store(0U, std::memory_order_relaxed);
        global_allocation_failure_call.store(failure_call,
                                             std::memory_order_relaxed);
        global_allocation_tracking_enabled.store(true,
                                                 std::memory_order_relaxed);
    }

    global_allocation_window(const global_allocation_window &) = delete;
    auto operator=(const global_allocation_window &)
        -> global_allocation_window & = delete;

    ~global_allocation_window() {
        global_allocation_tracking_enabled.store(false,
                                                 std::memory_order_relaxed);
        global_allocation_failure_call.store(0U, std::memory_order_relaxed);
    }
};

class revision_test_paths final {
  public:
    explicit revision_test_paths(std::string_view scenario) {
        auto directory_name = std::u8string{u8"FeatherDoc-修订事务-中文-😀-"};
        for (const auto byte : scenario) {
            directory_name.push_back(
                static_cast<char8_t>(static_cast<unsigned char>(byte)));
        }
        this->directory_ = std::filesystem::temp_directory_path() /
                           std::filesystem::path{directory_name};
        this->raw_ =
            this->directory_ / std::filesystem::path{u8"原始-修订-😀.docx"};
        this->source_ =
            this->directory_ / std::filesystem::path{u8"基线-修订-😀.docx"};
        this->output_ =
            this->directory_ / std::filesystem::path{u8"输出-修订-😀.docx"};

        std::error_code ignored;
        std::filesystem::remove_all(this->directory_, ignored);
        ignored.clear();
        std::filesystem::create_directories(this->directory_, ignored);
        if (ignored) {
            throw std::filesystem::filesystem_error{
                "failed to create revision transaction test directory",
                this->directory_, ignored};
        }
    }

    revision_test_paths(const revision_test_paths &) = delete;
    auto operator=(const revision_test_paths &)
        -> revision_test_paths & = delete;

    ~revision_test_paths() {
        std::error_code ignored;
        std::filesystem::remove_all(this->directory_, ignored);
    }

    [[nodiscard]] auto raw() const -> const std::filesystem::path & {
        return this->raw_;
    }
    [[nodiscard]] auto source() const -> const std::filesystem::path & {
        return this->source_;
    }
    [[nodiscard]] auto output() const -> const std::filesystem::path & {
        return this->output_;
    }

  private:
    std::filesystem::path directory_;
    std::filesystem::path raw_;
    std::filesystem::path source_;
    std::filesystem::path output_;
};

[[nodiscard]] auto large_story_payload() -> std::string {
    constexpr auto alphabet = std::string_view{
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
    auto value = std::string(68U * 1024U, 'a');
    auto state = std::uint32_t{0x63F1A9D5U};
    for (auto &character : value) {
        state = state * 1664525U + 1013904223U;
        character = alphabet[(state >> 24U) % alphabet.size()];
    }
    return value;
}

[[nodiscard]] auto revision_story_paragraph(std::string_view label,
                                            std::string_view payload,
                                            int first_id) -> std::string {
    auto xml = std::string{"<w:p><w:r><w:t>"};
    xml += label;
    xml += "-稳定前缀😀-";
    xml += payload;
    xml += "</w:t></w:r><w:ins w:id=\"";
    xml += std::to_string(first_id);
    xml += "\" w:author=\"原作者😀\"><w:r><w:t>";
    xml += label;
    xml += "-插入中文😀</w:t></w:r></w:ins><w:del w:id=\"";
    xml += std::to_string(first_id + 1);
    xml += "\" w:author=\"删除作者🪶\"><w:r><w:delText>";
    xml += label;
    xml += "-删除中文🪶</w:delText></w:r></w:del>"
           "<w:r><w:t>稳定尾部</w:t></w:r></w:p>";
    return xml;
}

void write_revision_fixture(const std::filesystem::path &path) {
    const auto payload = large_story_payload();
    const auto body_story =
        revision_story_paragraph(utf8_from_u8(u8"正文"), payload, 1);
    const auto header_story =
        revision_story_paragraph(utf8_from_u8(u8"页眉"), payload, 3);
    const auto footer_story =
        revision_story_paragraph(utf8_from_u8(u8"页脚"), payload, 5);

    auto document_xml = utf8_from_u8(
        u8R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>)");
    document_xml += body_story;
    document_xml += R"(<w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr></w:body></w:document>)";

    auto header_xml = utf8_from_u8(
        u8R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">)");
    header_xml += header_story;
    header_xml += "</w:hdr>";

    auto footer_xml = utf8_from_u8(
        u8R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">)");
    footer_xml += footer_story;
    footer_xml += "</w:ftr>";

    constexpr auto content_types =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/comments.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"/>
</Types>)";
    constexpr auto document_relationships =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer" Target="footer1.xml"/>
  <Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments" Target="comments.xml"/>
</Relationships>)";
    constexpr auto comments_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="0" w:author="Review fixture"><w:p><w:r><w:t>Stable comment</w:t></w:r></w:p></w:comment>
</w:comments>)";

    write_test_archive_entries(
        path, {{test_content_types_xml_entry, content_types},
               {test_relationships_xml_entry, test_relationships_xml},
               {test_document_xml_entry, std::move(document_xml)},
               {"word/_rels/document.xml.rels", document_relationships},
               {"word/header1.xml", std::move(header_xml)},
               {"word/footer1.xml", std::move(footer_xml)},
               {"word/comments.xml", comments_xml}});
}

void create_canonical_fixture(revision_test_paths &paths) {
    write_revision_fixture(paths.raw());
    featherdoc::Document document(paths.raw());
    REQUIRE_FALSE(document.open());
    REQUIRE_FALSE(document.save_as(paths.source()));
}

[[nodiscard]] auto capture_handles(featherdoc::Document &document)
    -> revision_story_handles {
    revision_story_handles handles;
    handles.body = document.paragraphs();
    handles.header = document.header_paragraphs();
    handles.footer = document.footer_paragraphs();
    handles.body_run = handles.body.runs();
    handles.header_run = handles.header.runs();
    handles.footer_run = handles.footer.runs();
    return handles;
}

void check_all_handles_valid(const revision_story_handles &handles) {
    CHECK(handles.body.valid());
    CHECK(handles.header.valid());
    CHECK(handles.footer.valid());
    CHECK(handles.body_run.valid());
    CHECK(handles.header_run.valid());
    CHECK(handles.footer_run.valid());
}

void check_success_handle_retirement(const revision_mutation_scenario &scenario,
                                     const revision_story_handles &handles) {
    const auto body_retired =
        scenario.kind == revision_mutation_kind::reject_body_revision ||
        scenario.kind == revision_mutation_kind::accept_all ||
        scenario.kind == revision_mutation_kind::reject_all ||
        scenario.kind == revision_mutation_kind::insert_paragraph_revision ||
        scenario.kind == revision_mutation_kind::append_insertion_revision ||
        scenario.kind == revision_mutation_kind::append_deletion_revision ||
        scenario.kind == revision_mutation_kind::insert_body_run_revision ||
        scenario.kind == revision_mutation_kind::delete_body_run_revision ||
        scenario.kind == revision_mutation_kind::replace_body_run_revision;
    const auto header_retired =
        scenario.kind == revision_mutation_kind::set_header_metadata ||
        scenario.kind == revision_mutation_kind::accept_all ||
        scenario.kind == revision_mutation_kind::reject_all;
    const auto footer_retired =
        scenario.kind == revision_mutation_kind::accept_footer_revision ||
        scenario.kind == revision_mutation_kind::accept_all ||
        scenario.kind == revision_mutation_kind::reject_all;

    CHECK_EQ(handles.body.valid(), !body_retired);
    CHECK_EQ(handles.body_run.valid(), !body_retired);
    CHECK_EQ(handles.header.valid(), !header_retired);
    CHECK_EQ(handles.header_run.valid(), !header_retired);
    CHECK_EQ(handles.footer.valid(), !footer_retired);
    CHECK_EQ(handles.footer_run.valid(), !footer_retired);
}

[[nodiscard]] auto capture_snapshot(const std::filesystem::path &path)
    -> revision_story_snapshot {
    return {read_test_docx_entry(path, test_document_xml_entry),
            read_test_docx_entry(path, "word/header1.xml"),
            read_test_docx_entry(path, "word/footer1.xml")};
}

[[nodiscard]] auto
execute_revision_mutation(const revision_mutation_scenario &scenario,
                          featherdoc::Document &document) -> bool {
    switch (scenario.kind) {
    case revision_mutation_kind::set_header_metadata:
        return document.set_revision_metadata(2U, updated_revision_metadata);
    case revision_mutation_kind::accept_footer_revision:
        return document.accept_revision(4U);
    case revision_mutation_kind::reject_body_revision:
        return document.reject_revision(1U);
    case revision_mutation_kind::accept_all:
        return document.accept_all_revisions() == 6U;
    case revision_mutation_kind::reject_all:
        return document.reject_all_revisions() == 6U;
    case revision_mutation_kind::insert_paragraph_revision:
        return document.insert_paragraph_text_revision(
            0U, 0U, inserted_revision_text, inserted_revision_author,
            "2026-07-17T12:34:56Z");
    case revision_mutation_kind::append_insertion_revision:
        return document.append_insertion_revision(
                   inserted_revision_text, inserted_revision_author,
                   "2026-07-17T12:34:56Z") == 1U;
    case revision_mutation_kind::append_deletion_revision:
        return document.append_deletion_revision(
                   inserted_revision_text, inserted_revision_author,
                   "2026-07-17T12:34:56Z") == 1U;
    case revision_mutation_kind::insert_body_run_revision:
        return document.insert_run_revision_after(0U, 0U, "inserted body run",
                                                  inserted_revision_author,
                                                  "2026-07-17T12:34:56Z");
    case revision_mutation_kind::delete_body_run_revision:
        return document.delete_run_revision(0U, 0U, inserted_revision_author,
                                            "2026-07-17T12:34:56Z");
    case revision_mutation_kind::replace_body_run_revision:
        return document.replace_run_revision(0U, 0U, "replacement body run",
                                             inserted_revision_author,
                                             "2026-07-17T12:34:56Z");
    }
    return false;
}

void check_failure_state(const revision_mutation_scenario &scenario,
                         featherdoc::Document &document,
                         const revision_story_handles &handles,
                         const revision_story_snapshot &before,
                         const std::filesystem::path &output) {
    check_all_handles_valid(handles);
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::not_enough_memory));
    if (!scenario.allocation_entry.empty()) {
        CHECK_EQ(document.last_error().entry_name, scenario.allocation_entry);
    } else {
        const auto entry = std::string_view{document.last_error().entry_name};
        const auto is_story_entry = entry == test_document_xml_entry ||
                                    entry == "word/header1.xml" ||
                                    entry == "word/footer1.xml";
        CHECK(is_story_entry);
    }

    REQUIRE_FALSE(document.save_as(output));
    CHECK(capture_snapshot(output) == before);
}

void check_retry(const revision_mutation_scenario &scenario,
                 featherdoc::Document &document,
                 const revision_story_handles &handles) {
    REQUIRE(execute_revision_mutation(scenario, document));
    CHECK_FALSE(document.last_error());
    check_success_handle_retirement(scenario, handles);
}

} // namespace

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                      \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
void *operator new(std::size_t size) { return allocate_controlled(size); }
void *operator new[](std::size_t size) { return allocate_controlled(size); }
void *operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_controlled_aligned(size,
                                       static_cast<std::size_t>(alignment));
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_controlled_aligned(size,
                                       static_cast<std::size_t>(alignment));
}
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::size_t, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::size_t, std::align_val_t) noexcept {
    std::free(memory);
}
#endif

TEST_CASE("run revision insertion remains inside bookmark boundaries") {
    revision_test_paths paths{"run-revision-direct-sibling"};
    const auto document_xml = utf8_from_u8(
        u8R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p>
    <w:bookmarkStart w:id="7" w:name="修订范围_中文_😀"/>
    <w:r><w:t>锚点中文😀</w:t></w:r>
    <w:bookmarkEnd w:id="7"/>
    <w:r><w:t>范围外尾部</w:t></w:r>
  </w:p></w:body>
</w:document>)");
    write_test_docx(paths.source(), document_xml);

    featherdoc::Document document(paths.source());
    REQUIRE_FALSE(document.open());
    REQUIRE(document.insert_run_revision_after(
        0U, 0U, utf8_from_u8(u8"书签内修订😀"), utf8_from_u8(u8"作者中文"),
        "2026-07-17T12:34:56Z"));
    REQUIRE_FALSE(document.save_as(paths.output()));

    const auto saved_xml =
        read_test_docx_entry(paths.output(), test_document_xml_entry);
    pugi::xml_document parsed;
    REQUIRE(parsed.load_buffer(saved_xml.data(), saved_xml.size()));
    const auto paragraph =
        parsed.child("w:document").child("w:body").child("w:p");
    const auto anchor = paragraph.child("w:r");
    const auto insertion = anchor.next_sibling();
    REQUIRE_EQ(std::string_view{insertion.name()}, "w:ins");
    CHECK_EQ(std::string_view{insertion.child("w:r").child("w:t").text().get()},
             utf8_from_u8(u8"书签内修订😀"));
    CHECK_EQ(std::string_view{insertion.next_sibling().name()},
             "w:bookmarkEnd");
}

TEST_CASE("revision story transactions retire exactly the published handles") {
    for (const auto &scenario : revision_scenarios) {
        CAPTURE(scenario.name);
        revision_test_paths paths{scenario.name};
        create_canonical_fixture(paths);

        featherdoc::Document document(paths.source());
        REQUIRE_FALSE(document.open());
        auto handles = capture_handles(document);
        check_all_handles_valid(handles);
        REQUIRE(execute_revision_mutation(scenario, document));
        CHECK_FALSE(document.last_error());
        check_success_handle_retirement(scenario, handles);
    }
}

TEST_CASE("revision transactions roll back every pugixml allocation failure") {
    for (const auto &scenario : revision_scenarios) {
        CAPTURE(scenario.name);
        revision_test_paths paths{scenario.name};
        create_canonical_fixture(paths);
        const auto before = capture_snapshot(paths.source());

        auto successful_allocation_count = std::size_t{0U};
        {
            featherdoc::Document document(paths.source());
            REQUIRE_FALSE(document.open());
            auto handles = capture_handles(document);
            {
                pugi_failure_guard guard;
                REQUIRE(execute_revision_mutation(scenario, document));
                successful_allocation_count = observed_pugi_allocation_calls;
            }
            check_success_handle_retirement(scenario, handles);
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            featherdoc::Document document(paths.source());
            REQUIRE_FALSE(document.open());
            auto handles = capture_handles(document);
            check_all_handles_valid(handles);

            auto succeeded = false;
            {
                pugi_failure_guard guard;
                pugi_allocation_failure_call = failure_call;
                succeeded = execute_revision_mutation(scenario, document);
            }
            REQUIRE_FALSE(succeeded);
            check_failure_state(scenario, document, handles, before,
                                paths.output());
            check_retry(scenario, document, handles);
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "revision transactions roll back every global allocation failure") {
    for (const auto &scenario : revision_scenarios) {
        CAPTURE(scenario.name);
        revision_test_paths paths{scenario.name};
        create_canonical_fixture(paths);
        const auto before = capture_snapshot(paths.source());

        auto successful_allocation_count = std::size_t{0U};
        {
            pugi_global_new_guard pugi_memory;
            featherdoc::Document document(paths.source());
            REQUIRE_FALSE(document.open());
            auto handles = capture_handles(document);
            {
                global_allocation_window tracking{0U};
                REQUIRE(execute_revision_mutation(scenario, document));
            }
            successful_allocation_count = observed_global_allocation_calls.load(
                std::memory_order_relaxed);
            check_success_handle_retirement(scenario, handles);
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            pugi_global_new_guard pugi_memory;
            featherdoc::Document document(paths.source());
            REQUIRE_FALSE(document.open());
            auto handles = capture_handles(document);
            check_all_handles_valid(handles);

            auto succeeded = false;
            auto escaped_bad_alloc = false;
            try {
                global_allocation_window tracking{failure_call};
                succeeded = execute_revision_mutation(scenario, document);
            } catch (const std::bad_alloc &) {
                escaped_bad_alloc = true;
            }
            CAPTURE(escaped_bad_alloc);
            REQUIRE_FALSE(escaped_bad_alloc);
            REQUIRE_FALSE(succeeded);
            check_failure_state(scenario, document, handles, before,
                                paths.output());
            check_retry(scenario, document, handles);
        }
    }
}
