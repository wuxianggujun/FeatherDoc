#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "allocation_failure_test_case.hpp"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <new>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <pugixml.hpp>

namespace {

enum class mutation_kind {
    paragraph_set_text,
    run_set_text,
    paragraph_add_run,
    paragraph_add_run_empty_cursor,
    run_insert_before,
    run_insert_after,
    run_insert_like_before,
    run_insert_like_after,
    paragraph_insert_before,
    paragraph_insert_after,
    paragraph_insert_like_before,
    paragraph_insert_like_after,
    template_part_append_paragraph,
};

constexpr auto superscript_formatting =
    featherdoc::formatting_flag::bold |
    featherdoc::formatting_flag::italic |
    featherdoc::formatting_flag::underline |
    featherdoc::formatting_flag::strikethrough |
    featherdoc::formatting_flag::superscript |
    featherdoc::formatting_flag::smallcaps |
    featherdoc::formatting_flag::shadow;

constexpr auto subscript_formatting =
    featherdoc::formatting_flag::bold |
    featherdoc::formatting_flag::italic |
    featherdoc::formatting_flag::underline |
    featherdoc::formatting_flag::strikethrough |
    featherdoc::formatting_flag::subscript |
    featherdoc::formatting_flag::smallcaps |
    featherdoc::formatting_flag::shadow;

struct mutation_scenario final {
    mutation_kind kind;
    std::string_view name;
    featherdoc::formatting_flag formatting;
    featherdoc::formatting_flag expected_formatting;
};

constexpr auto mutation_scenarios = std::array{
    mutation_scenario{mutation_kind::paragraph_set_text,
                      "paragraph-set-text", featherdoc::formatting_flag::none,
                      featherdoc::formatting_flag::none},
    mutation_scenario{mutation_kind::run_set_text, "run-set-text",
                      featherdoc::formatting_flag::none,
                      superscript_formatting},
    mutation_scenario{mutation_kind::paragraph_add_run, "paragraph-add-run",
                      superscript_formatting, superscript_formatting},
    mutation_scenario{mutation_kind::paragraph_add_run_empty_cursor,
                      "paragraph-add-run-empty-cursor",
                      subscript_formatting, subscript_formatting},
    mutation_scenario{mutation_kind::run_insert_before, "run-insert-before",
                      subscript_formatting, subscript_formatting},
    mutation_scenario{mutation_kind::run_insert_after, "run-insert-after",
                      superscript_formatting, superscript_formatting},
    mutation_scenario{mutation_kind::run_insert_like_before,
                      "run-insert-like-before",
                      featherdoc::formatting_flag::none,
                      superscript_formatting},
    mutation_scenario{mutation_kind::run_insert_like_after,
                      "run-insert-like-after",
                      featherdoc::formatting_flag::none,
                      superscript_formatting},
    mutation_scenario{mutation_kind::paragraph_insert_before,
                      "paragraph-insert-before", subscript_formatting,
                      subscript_formatting},
    mutation_scenario{mutation_kind::paragraph_insert_after,
                      "paragraph-insert-after", superscript_formatting,
                      superscript_formatting},
    mutation_scenario{mutation_kind::paragraph_insert_like_before,
                      "paragraph-insert-like-before",
                      featherdoc::formatting_flag::none,
                      featherdoc::formatting_flag::none},
    mutation_scenario{mutation_kind::paragraph_insert_like_after,
                      "paragraph-insert-like-after",
                      featherdoc::formatting_flag::none,
                      featherdoc::formatting_flag::none},
    mutation_scenario{mutation_kind::template_part_append_paragraph,
                      "template-part-append-paragraph",
                      superscript_formatting, superscript_formatting},
};

struct mutation_handles final {
    featherdoc::Paragraph paragraph;
    featherdoc::Paragraph empty_paragraph_cursor;
    featherdoc::Run first_run;
    featherdoc::Run second_run;
    featherdoc::TemplatePart template_part;
};

struct mutation_outcome final {
    bool succeeded{false};
    featherdoc::Paragraph paragraph;
    featherdoc::Run run;
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

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
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

class scoped_test_path final {
  public:
    scoped_test_path(std::string_view scenario, std::size_t failure_call,
                     std::string_view allocator_kind) {
        auto directory_name =
            std::u8string{u8"FeatherDoc-文本事务-中文-😀-"};
        const auto append_ascii = [&](std::string_view text) {
            for (const auto byte : text) {
                directory_name.push_back(static_cast<char8_t>(
                    static_cast<unsigned char>(byte)));
            }
        };
        append_ascii(allocator_kind);
        directory_name.push_back(u8'-');
        append_ascii(scenario);
        directory_name.push_back(u8'-');
        append_ascii(std::to_string(failure_call));

        this->directory_ = std::filesystem::temp_directory_path() /
                           std::filesystem::path{directory_name};
        this->path_ =
            this->directory_ / std::filesystem::path{u8"保存-文档-😀.docx"};

        std::error_code ignored;
        std::filesystem::remove_all(this->directory_, ignored);
        ignored.clear();
        std::filesystem::create_directories(this->directory_, ignored);
        if (ignored) {
            throw std::filesystem::filesystem_error{
                "failed to create text transaction test directory",
                this->directory_, ignored};
        }
    }

    scoped_test_path(const scoped_test_path &) = delete;
    auto operator=(const scoped_test_path &) -> scoped_test_path & = delete;

    ~scoped_test_path() {
        std::error_code ignored;
        std::filesystem::remove_all(this->directory_, ignored);
    }

    [[nodiscard]] auto path() const -> const std::filesystem::path & {
        return this->path_;
    }

  private:
    std::filesystem::path directory_;
    std::filesystem::path path_;
};

[[nodiscard]] auto large_property_value() -> std::string {
    constexpr auto alphabet = std::string_view{
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
    auto value = std::string(68U * 1024U, 'a');
    auto state = std::uint32_t{0x5B34A921U};
    for (auto &character : value) {
        state = state * 1664525U + 1013904223U;
        character = alphabet[(state >> 24U) % alphabet.size()];
    }
    return value;
}

[[nodiscard]] auto fixture_xml() -> std::string {
    const auto large_value = large_property_value();
    auto xml = utf8_from_u8(
        u8R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr><w:pStyle w:val=")");
    xml += large_value;
    xml += utf8_from_u8(u8R"("/><w:jc w:val="left"/></w:pPr>
      <w:r>
        <w:rPr>
          <w:rStyle w:val=")");
    xml += large_value;
    xml += utf8_from_u8(u8R"("/>
          <w:b/><w:i/><w:u w:val="single"/><w:strike w:val="true"/>
          <w:vertAlign w:val="superscript"/>
          <w:smallCaps w:val="true"/><w:shadow w:val="true"/>
        </w:rPr>
        <w:t>原始锚点😀</w:t>
      </w:r>
      <w:r><w:t>原始尾部中文</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>不受影响的段落</w:t></w:r></w:p>
  </w:body>
</w:document>
)");
    return xml;
}

[[nodiscard]] auto mutation_text() -> std::string {
    auto text = utf8_from_u8(u8"  事务中文😀-");
    constexpr auto alphabet = std::string_view{
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
    text.reserve(70U * 1024U);
    auto state = std::uint32_t{0x8D12A3E5U};
    for (std::size_t index = 0U; index < 68U * 1024U; ++index) {
        state = state * 1664525U + 1013904223U;
        text.push_back(alphabet[(state >> 24U) % alphabet.size()]);
    }
    text += utf8_from_u8(u8"\n第二行中文与 emoji 🪶  ");
    return text;
}

[[nodiscard]] auto capture_handles(featherdoc::Document &document)
    -> mutation_handles {
    mutation_handles handles;
    handles.paragraph = document.paragraphs();
    handles.empty_paragraph_cursor = handles.paragraph;
    while (handles.empty_paragraph_cursor.valid()) {
        handles.empty_paragraph_cursor.next();
    }
    handles.first_run = handles.paragraph.runs();
    handles.second_run = handles.first_run;
    handles.second_run.next();
    handles.template_part = document.body_template();
    return handles;
}

[[nodiscard]] auto execute_mutation(const mutation_scenario &scenario,
                                    mutation_handles &handles,
                                    const std::string &text)
    -> mutation_outcome {
    mutation_outcome outcome;
    switch (scenario.kind) {
    case mutation_kind::paragraph_set_text:
        outcome.succeeded = handles.paragraph.set_text(text);
        break;
    case mutation_kind::run_set_text:
        outcome.succeeded = handles.first_run.set_text(text);
        outcome.run = handles.first_run;
        break;
    case mutation_kind::paragraph_add_run:
        outcome.run = handles.paragraph.add_run(text, scenario.formatting);
        outcome.succeeded = outcome.run.valid();
        break;
    case mutation_kind::paragraph_add_run_empty_cursor:
        outcome.run = handles.empty_paragraph_cursor.add_run(
            text, scenario.formatting);
        outcome.succeeded = outcome.run.valid();
        break;
    case mutation_kind::run_insert_before:
        outcome.run =
            handles.first_run.insert_run_before(text, scenario.formatting);
        outcome.succeeded = outcome.run.valid();
        break;
    case mutation_kind::run_insert_after:
        outcome.run =
            handles.first_run.insert_run_after(text, scenario.formatting);
        outcome.succeeded = outcome.run.valid();
        break;
    case mutation_kind::run_insert_like_before:
        outcome.run = handles.first_run.insert_run_like_before();
        outcome.succeeded = outcome.run.valid();
        break;
    case mutation_kind::run_insert_like_after:
        outcome.run = handles.first_run.insert_run_like_after();
        outcome.succeeded = outcome.run.valid();
        break;
    case mutation_kind::paragraph_insert_before:
        outcome.paragraph = handles.paragraph.insert_paragraph_before(
            text, scenario.formatting);
        outcome.succeeded = outcome.paragraph.valid();
        break;
    case mutation_kind::paragraph_insert_after:
        outcome.paragraph = handles.paragraph.insert_paragraph_after(
            text, scenario.formatting);
        outcome.succeeded = outcome.paragraph.valid();
        break;
    case mutation_kind::paragraph_insert_like_before:
        outcome.paragraph = handles.paragraph.insert_paragraph_like_before();
        outcome.succeeded = outcome.paragraph.valid();
        break;
    case mutation_kind::paragraph_insert_like_after:
        outcome.paragraph = handles.paragraph.insert_paragraph_like_after();
        outcome.succeeded = outcome.paragraph.valid();
        break;
    case mutation_kind::template_part_append_paragraph:
        outcome.paragraph = handles.template_part.append_paragraph(
            text, scenario.formatting);
        outcome.succeeded = outcome.paragraph.valid();
        break;
    }
    return outcome;
}

void check_original_handles(const mutation_handles &handles) {
    CHECK(handles.paragraph.valid());
    CHECK_FALSE(handles.empty_paragraph_cursor.valid());
    CHECK(handles.first_run.valid());
    CHECK(handles.second_run.valid());
    CHECK(static_cast<bool>(handles.template_part));
    CHECK_EQ(handles.first_run.get_text(), utf8_from_u8(u8"原始锚点😀"));
    CHECK_EQ(handles.second_run.get_text(), utf8_from_u8(u8"原始尾部中文"));
}

[[nodiscard]] auto collect_run_text(pugi::xml_node run) -> std::string {
    auto text = std::string{};
    for (auto child = run.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:t") {
            text.append(child.text().get());
        } else if (name == "w:br") {
            text.push_back('\n');
        }
    }
    return text;
}

[[nodiscard]] auto find_run_with_text(pugi::xml_node body,
                                      std::string_view expected_text)
    -> pugi::xml_node {
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        for (auto run = paragraph.child("w:r"); run != pugi::xml_node{};
             run = run.next_sibling("w:r")) {
            if (collect_run_text(run) == expected_text) {
                return run;
            }
        }
    }
    return {};
}

[[nodiscard]] auto count_body_runs(pugi::xml_node body) -> std::size_t {
    auto count = std::size_t{0U};
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        count += count_named_children(paragraph, "w:r");
    }
    return count;
}

void check_formatted_run(const pugi::xml_node run,
                         featherdoc::formatting_flag formatting,
                         std::string_view expected_text) {
    const auto properties = run.child("w:rPr");
    if (formatting == featherdoc::formatting_flag::none) {
        CHECK_EQ(properties, pugi::xml_node{});
    } else {
        REQUIRE(properties != pugi::xml_node{});
        CHECK_EQ(count_named_children(properties, "w:b"), 1U);
        CHECK_EQ(count_named_children(properties, "w:i"), 1U);
        CHECK_EQ(count_named_children(properties, "w:u"), 1U);
        CHECK_EQ(std::string_view{properties.child("w:u").attribute("w:val").value()},
                 "single");
        CHECK_EQ(count_named_children(properties, "w:strike"), 1U);
        CHECK_EQ(std::string_view{
                     properties.child("w:strike").attribute("w:val").value()},
                 "true");
        CHECK_EQ(count_named_children(properties, "w:smallCaps"), 1U);
        CHECK_EQ(std::string_view{properties.child("w:smallCaps")
                                      .attribute("w:val")
                                      .value()},
                 "true");
        CHECK_EQ(count_named_children(properties, "w:shadow"), 1U);
        CHECK_EQ(std::string_view{properties.child("w:shadow")
                                      .attribute("w:val")
                                      .value()},
                 "true");

        const auto vertical_align = properties.child("w:vertAlign");
        REQUIRE(vertical_align != pugi::xml_node{});
        if (featherdoc::has_flag(
                formatting, featherdoc::formatting_flag::superscript)) {
            CHECK_EQ(std::string_view{
                         vertical_align.attribute("w:val").value()},
                     "superscript");
        } else {
            REQUIRE(featherdoc::has_flag(
                formatting, featherdoc::formatting_flag::subscript));
            CHECK_EQ(std::string_view{
                         vertical_align.attribute("w:val").value()},
                     "subscript");
        }
    }

    const auto newline = expected_text.find('\n');
    REQUIRE_NE(newline, std::string_view::npos);
    const auto first_text = properties == pugi::xml_node{}
                                ? run.first_child()
                                : properties.next_sibling();
    REQUIRE(first_text != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_text.name()}, "w:t");
    CHECK_EQ(std::string_view{first_text.text().get()},
             expected_text.substr(0U, newline));
    CHECK_EQ(std::string_view{first_text.attribute("xml:space").value()},
             "preserve");

    const auto line_break = first_text.next_sibling();
    REQUIRE(line_break != pugi::xml_node{});
    CHECK_EQ(std::string_view{line_break.name()}, "w:br");

    const auto second_text = line_break.next_sibling();
    REQUIRE(second_text != pugi::xml_node{});
    CHECK_EQ(std::string_view{second_text.name()}, "w:t");
    CHECK_EQ(std::string_view{second_text.text().get()},
             expected_text.substr(newline + 1U));
    CHECK_EQ(std::string_view{second_text.attribute("xml:space").value()},
             "preserve");
    CHECK_EQ(second_text.next_sibling(), pugi::xml_node{});
}

void check_successful_retry(const mutation_scenario &scenario,
                            const mutation_handles &handles,
                            mutation_outcome &outcome,
                            const std::filesystem::path &path,
                            const std::string &text) {
    CHECK(handles.paragraph.valid());
    if (scenario.kind == mutation_kind::paragraph_add_run_empty_cursor) {
        CHECK(handles.empty_paragraph_cursor.valid());
    }
    if (scenario.kind == mutation_kind::paragraph_set_text) {
        CHECK_FALSE(handles.first_run.valid());
        CHECK_FALSE(handles.second_run.valid());
    } else {
        CHECK(handles.first_run.valid());
        CHECK(handles.second_run.valid());
    }

    if (scenario.kind == mutation_kind::run_set_text) {
        CHECK(outcome.run.valid());
        CHECK_EQ(handles.first_run.get_text(), text);
        CHECK_EQ(handles.second_run.get_text(),
                 utf8_from_u8(u8"原始尾部中文"));
    } else if (scenario.kind == mutation_kind::paragraph_add_run ||
               scenario.kind ==
                   mutation_kind::paragraph_add_run_empty_cursor ||
               scenario.kind == mutation_kind::run_insert_before ||
               scenario.kind == mutation_kind::run_insert_after ||
               scenario.kind == mutation_kind::run_insert_like_before ||
               scenario.kind == mutation_kind::run_insert_like_after) {
        CHECK(outcome.run.valid());
        CHECK_EQ(outcome.run.get_text(), text);
    } else if (scenario.kind == mutation_kind::paragraph_insert_before ||
               scenario.kind == mutation_kind::paragraph_insert_after ||
               scenario.kind == mutation_kind::paragraph_insert_like_before ||
               scenario.kind == mutation_kind::paragraph_insert_like_after ||
               scenario.kind ==
                   mutation_kind::template_part_append_paragraph) {
        CHECK(outcome.paragraph.valid());
        CHECK(outcome.paragraph.runs().valid());
        CHECK_EQ(outcome.paragraph.runs().get_text(), text);
    }

    const auto saved_xml =
        read_test_docx_entry(path, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_buffer(saved_xml.data(), saved_xml.size()));
    const auto body = xml_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    const auto mutated_run = find_run_with_text(body, text);
    REQUIRE(mutated_run != pugi::xml_node{});
    check_formatted_run(mutated_run, scenario.expected_formatting, text);

    if (scenario.kind == mutation_kind::run_set_text ||
        scenario.kind == mutation_kind::run_insert_like_before ||
        scenario.kind == mutation_kind::run_insert_like_after) {
        CHECK_EQ(std::string_view{mutated_run.child("w:rPr")
                                      .child("w:rStyle")
                                      .attribute("w:val")
                                      .value()},
                 large_property_value());
    }
    if (scenario.kind == mutation_kind::paragraph_insert_like_before ||
        scenario.kind == mutation_kind::paragraph_insert_like_after) {
        const auto properties = mutated_run.parent().child("w:pPr");
        REQUIRE(properties != pugi::xml_node{});
        CHECK_EQ(std::string_view{properties.child("w:pStyle")
                                      .attribute("w:val")
                                      .value()},
                 large_property_value());
        CHECK_EQ(std::string_view{
                     properties.child("w:jc").attribute("w:val").value()},
                 "left");
    }

    const auto paragraph_insertion =
        scenario.kind == mutation_kind::paragraph_add_run_empty_cursor ||
        scenario.kind == mutation_kind::paragraph_insert_before ||
        scenario.kind == mutation_kind::paragraph_insert_after ||
        scenario.kind == mutation_kind::paragraph_insert_like_before ||
        scenario.kind == mutation_kind::paragraph_insert_like_after ||
        scenario.kind == mutation_kind::template_part_append_paragraph;
    CHECK_EQ(count_named_children(body, "w:p"),
             paragraph_insertion ? 3U : 2U);

    auto expected_run_count = std::size_t{3U};
    if (scenario.kind == mutation_kind::paragraph_set_text) {
        expected_run_count = 2U;
    } else if (scenario.kind != mutation_kind::run_set_text) {
        expected_run_count = 4U;
    }
    CHECK_EQ(count_body_runs(body), expected_run_count);
}

void check_failure_and_retry(const mutation_scenario &scenario,
                             featherdoc::Document &document,
                             mutation_handles &handles,
                             const std::filesystem::path &path,
                             std::string_view xml_before,
                             const std::string &text) {
    check_original_handles(handles);
    if (scenario.kind == mutation_kind::template_part_append_paragraph) {
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.last_error().entry_name, test_document_xml_entry);
        CHECK_FALSE(document.last_error().detail.empty());
    }
    REQUIRE_FALSE(document.save());
    CHECK_EQ(read_test_docx_entry(path, test_document_xml_entry), xml_before);

    auto retry = execute_mutation(scenario, handles, text);
    REQUIRE(retry.succeeded);
    if (scenario.kind == mutation_kind::run_insert_like_before ||
        scenario.kind == mutation_kind::run_insert_like_after) {
        REQUIRE(retry.run.set_text(text));
    } else if (scenario.kind ==
                   mutation_kind::paragraph_insert_like_before ||
               scenario.kind ==
                   mutation_kind::paragraph_insert_like_after) {
        REQUIRE(retry.paragraph.set_text(text));
    }
    REQUIRE_FALSE(document.save());
    check_successful_retry(scenario, handles, retry, path, text);
}

} // namespace

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
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
void operator delete[](void *memory, std::size_t,
                       std::align_val_t) noexcept {
    std::free(memory);
}
#endif

TEST_CASE("run after insertion stays before intervening bookmark and proofing markers") {
    const auto source_xml = utf8_from_u8(
        u8R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="7" w:name="范围_中文_😀"/>
      <w:r><w:t>锚点一</w:t></w:r>
      <w:bookmarkEnd w:id="7"/>
      <w:r><w:t>尾部一</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:rPr><w:b/></w:rPr><w:t>锚点二</w:t></w:r>
      <w:proofErr w:type="spellStart"/>
      <w:r><w:t>尾部二</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)" );
    scoped_test_path path{"run-direct-sibling-boundary", 0U, "normal"};
    write_test_docx(path.path(), source_xml);

    featherdoc::Document document(path.path());
    REQUIRE_FALSE(document.open());

    auto first_paragraph = document.paragraphs();
    auto first_anchor = first_paragraph.runs();
    auto inserted_text = first_anchor.insert_run_after(
        utf8_from_u8(u8"书签内中文😀"));
    REQUIRE(inserted_text.valid());

    auto second_paragraph = first_paragraph;
    second_paragraph.next();
    auto second_anchor = second_paragraph.runs();
    auto inserted_like = second_anchor.insert_run_like_after();
    REQUIRE(inserted_like.valid());
    REQUIRE(inserted_like.set_text(utf8_from_u8(u8"校对标记前中文🪶")));
    REQUIRE_FALSE(document.save());

    const auto saved_xml =
        read_test_docx_entry(path.path(), test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_buffer(saved_xml.data(), saved_xml.size()));
    auto paragraph =
        xml_document.child("w:document").child("w:body").child("w:p");
    REQUIRE(paragraph != pugi::xml_node{});

    auto anchor = paragraph.child("w:r");
    auto inserted = anchor.next_sibling();
    REQUIRE_EQ(std::string_view{inserted.name()}, "w:r");
    CHECK_EQ(std::string_view{inserted.child("w:t").text().get()},
             utf8_from_u8(u8"书签内中文😀"));
    REQUIRE_EQ(std::string_view{inserted.next_sibling().name()},
               "w:bookmarkEnd");

    paragraph = paragraph.next_sibling("w:p");
    REQUIRE(paragraph != pugi::xml_node{});
    anchor = paragraph.child("w:r");
    inserted = anchor.next_sibling();
    REQUIRE_EQ(std::string_view{inserted.name()}, "w:r");
    CHECK_EQ(std::string_view{inserted.child("w:t").text().get()},
             utf8_from_u8(u8"校对标记前中文🪶"));
    REQUIRE_EQ(std::string_view{inserted.next_sibling().name()}, "w:proofErr");
}

TEST_CASE("text mutations roll back every pugixml allocation failure") {
    const auto source_xml = fixture_xml();
    const auto text = mutation_text();

    for (const auto &scenario : mutation_scenarios) {
        CAPTURE(scenario.name);
        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{scenario.name, 0U, "pugi-baseline"};
            write_test_docx(path.path(), source_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto handles = capture_handles(document);
            check_original_handles(handles);

            mutation_outcome outcome;
            {
                pugi_failure_guard guard;
                outcome = execute_mutation(scenario, handles, text);
                successful_allocation_count =
                    observed_pugi_allocation_calls;
            }
            REQUIRE(outcome.succeeded);
            REQUIRE_GT(successful_allocation_count, 0U);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{scenario.name, failure_call, "pugi"};
            write_test_docx(path.path(), source_xml);
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);
            auto handles = capture_handles(document);
            check_original_handles(handles);

            auto failed_outcome = mutation_outcome{};
            {
                pugi_failure_guard guard;
                pugi_allocation_failure_call = failure_call;
                failed_outcome = execute_mutation(scenario, handles, text);
            }

            REQUIRE_FALSE(failed_outcome.succeeded);
            check_failure_and_retry(scenario, document, handles, path.path(),
                                    xml_before, text);
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "text mutations preserve XML and handles for every global allocation "
    "failure") {
    const auto source_xml = fixture_xml();
    const auto text = mutation_text();

    for (const auto &scenario : mutation_scenarios) {
        CAPTURE(scenario.name);
        auto successful_allocation_count = std::size_t{0U};
        {
            scoped_test_path path{scenario.name, 0U, "global-baseline"};
            write_test_docx(path.path(), source_xml);
            pugi_global_new_guard pugi_memory;
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            auto handles = capture_handles(document);
            check_original_handles(handles);

            auto outcome = mutation_outcome{};
            {
                global_allocation_window tracking{0U};
                outcome = execute_mutation(scenario, handles, text);
            }
            successful_allocation_count =
                observed_global_allocation_calls.load(
                    std::memory_order_relaxed);
            REQUIRE(outcome.succeeded);
            REQUIRE_GT(successful_allocation_count, 0U);
        }

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);
            scoped_test_path path{scenario.name, failure_call, "global"};
            write_test_docx(path.path(), source_xml);
            pugi_global_new_guard pugi_memory;
            featherdoc::Document document(path.path());
            REQUIRE_FALSE(document.open());
            REQUIRE_FALSE(document.save());
            const auto xml_before =
                read_test_docx_entry(path.path(), test_document_xml_entry);
            auto handles = capture_handles(document);
            check_original_handles(handles);

            auto failed_outcome = mutation_outcome{};
            auto observed_bad_alloc = false;
            try {
                global_allocation_window tracking{failure_call};
                failed_outcome = execute_mutation(scenario, handles, text);
            } catch (const std::bad_alloc &) {
                observed_bad_alloc = true;
            }

            CAPTURE(observed_bad_alloc);
            REQUIRE_FALSE(failed_outcome.succeeded);
            check_failure_and_retry(scenario, document, handles, path.path(),
                                    xml_before, text);
        }
    }
}
