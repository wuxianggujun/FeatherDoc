#include "allocation_failure_test_case.hpp"
#include "basic_docx_archive_test_support.hpp"
#include "doctest.h"

#include <featherdoc.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <new>
#include <string>
#include <string_view>
#include <vector>

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY && defined(_WIN32)
#include <malloc.h>
#endif

namespace {

std::atomic_bool section_text_allocation_window_enabled{false};
std::atomic_size_t section_text_allocation_calls{0U};
std::atomic_size_t section_text_failure_call{0U};
pugi::allocation_function delegated_section_text_pugi_allocate = nullptr;

auto should_fail_section_text_allocation() noexcept -> bool {
    if (!section_text_allocation_window_enabled.load(
            std::memory_order_relaxed)) {
        return false;
    }
    const auto call = section_text_allocation_calls.fetch_add(
                          1U, std::memory_order_relaxed) +
                      1U;
    const auto failure_call =
        section_text_failure_call.load(std::memory_order_relaxed);
    return failure_call != 0U && call == failure_call;
}

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
[[nodiscard]] auto allocate_section_text_unaligned(std::size_t size)
    -> void * {
    if (should_fail_section_text_allocation()) {
        throw std::bad_alloc{};
    }
    if (void *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

[[nodiscard]] auto allocate_section_text_aligned(std::size_t size,
                                                 std::size_t alignment)
    -> void * {
    if (should_fail_section_text_allocation()) {
        throw std::bad_alloc{};
    }
#if defined(_WIN32)
    if (void *memory = _aligned_malloc(size == 0U ? 1U : size, alignment)) {
        return memory;
    }
#else
    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0U ? 1U : size) == 0) {
        return memory;
    }
#endif
    throw std::bad_alloc{};
}

void free_section_text_aligned(void *memory) noexcept {
#if defined(_WIN32)
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}
#endif

auto controlled_section_text_pugi_allocate(std::size_t size) -> void * {
    if (should_fail_section_text_allocation()) {
        return nullptr;
    }
    return delegated_section_text_pugi_allocate != nullptr
               ? delegated_section_text_pugi_allocate(size)
               : nullptr;
}

struct section_text_pugi_memory_guard final {
    pugi::allocation_function allocation{
        pugi::get_memory_allocation_function()};
    pugi::deallocation_function deallocation{
        pugi::get_memory_deallocation_function()};

    section_text_pugi_memory_guard() {
        delegated_section_text_pugi_allocate = this->allocation;
        pugi::set_memory_management_functions(
            controlled_section_text_pugi_allocate, this->deallocation);
    }

    ~section_text_pugi_memory_guard() {
        section_text_allocation_window_enabled.store(false,
                                                     std::memory_order_relaxed);
        pugi::set_memory_management_functions(this->allocation,
                                              this->deallocation);
        delegated_section_text_pugi_allocate = nullptr;
        section_text_allocation_calls.store(0U, std::memory_order_relaxed);
        section_text_failure_call.store(0U, std::memory_order_relaxed);
    }
};

struct section_text_allocation_window final {
    explicit section_text_allocation_window(std::size_t failure_call) {
        section_text_allocation_calls.store(0U, std::memory_order_relaxed);
        section_text_failure_call.store(failure_call,
                                        std::memory_order_relaxed);
        section_text_allocation_window_enabled.store(
            true, std::memory_order_release);
    }

    ~section_text_allocation_window() {
        section_text_allocation_window_enabled.store(false,
                                                     std::memory_order_release);
    }
};

enum class section_text_part_kind { header, footer };

auto ensure_section_text_part(featherdoc::Document &document,
                              section_text_part_kind kind)
    -> featherdoc::Paragraph {
    return kind == section_text_part_kind::header
               ? document.ensure_section_header_paragraphs(0U)
               : document.ensure_section_footer_paragraphs(0U);
}

auto section_text_part(featherdoc::Document &document,
                       section_text_part_kind kind) -> featherdoc::Paragraph {
    return kind == section_text_part_kind::header
               ? document.section_header_paragraphs(0U)
               : document.section_footer_paragraphs(0U);
}

auto replace_section_text_part(featherdoc::Document &document,
                               section_text_part_kind kind,
                               std::string_view text) -> bool {
    return kind == section_text_part_kind::header
               ? document.replace_section_header_text(0U, text)
               : document.replace_section_footer_text(0U, text);
}

auto other_section_text_part(featherdoc::Document &document,
                             section_text_part_kind kind)
    -> featherdoc::Paragraph {
    return kind == section_text_part_kind::header
               ? document.section_footer_paragraphs(0U)
               : document.section_header_paragraphs(0U);
}

void prepare_existing_section_text_parts(featherdoc::Document &document) {
    REQUIRE_FALSE(document.create_empty());
    auto header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.valid());
    REQUIRE(header.add_run("old header").valid());
    auto footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.valid());
    REQUIRE(footer.add_run("old footer").valid());
}

[[nodiscard]] auto original_section_page_setup()
    -> featherdoc::section_page_setup {
    auto setup = featherdoc::section_page_setup{};
    setup.orientation = featherdoc::page_orientation::portrait;
    setup.width_twips = 12240U;
    setup.height_twips = 15840U;
    setup.margins.top_twips = 1440U;
    setup.margins.bottom_twips = 1440U;
    setup.margins.left_twips = 1800U;
    setup.margins.right_twips = 1800U;
    setup.margins.header_twips = 720U;
    setup.margins.footer_twips = 720U;
    return setup;
}

[[nodiscard]] auto replacement_section_page_setup()
    -> featherdoc::section_page_setup {
    auto setup = featherdoc::section_page_setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 15840U;
    setup.height_twips = 12240U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 1080U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 360U;
    setup.margins.footer_twips = 540U;
    setup.page_number_start = 9U;
    return setup;
}

void check_section_page_setup(
    featherdoc::Document &document,
    const featherdoc::section_page_setup &expected) {
    const auto actual = document.get_section_page_setup(0U);
    REQUIRE(actual.has_value());
    CHECK_EQ(actual->orientation, expected.orientation);
    CHECK_EQ(actual->width_twips, expected.width_twips);
    CHECK_EQ(actual->height_twips, expected.height_twips);
    CHECK_EQ(actual->margins.top_twips, expected.margins.top_twips);
    CHECK_EQ(actual->margins.bottom_twips, expected.margins.bottom_twips);
    CHECK_EQ(actual->margins.left_twips, expected.margins.left_twips);
    CHECK_EQ(actual->margins.right_twips, expected.margins.right_twips);
    CHECK_EQ(actual->margins.header_twips, expected.margins.header_twips);
    CHECK_EQ(actual->margins.footer_twips, expected.margins.footer_twips);
    CHECK_EQ(actual->page_number_start, expected.page_number_start);
}

struct section_transaction_handles final {
    featherdoc::TemplatePart body_template;
    featherdoc::Paragraph body_paragraph;
    featherdoc::Run body_run;
    featherdoc::Table body_table;
    featherdoc::TableRow body_row;
    featherdoc::TableCell body_cell;
    featherdoc::TemplatePart header_template;
    featherdoc::Paragraph header_paragraph;
    featherdoc::Run header_run;
};

[[nodiscard]] auto capture_section_transaction_handles(
    featherdoc::Document &document, std::size_t header_index,
    std::string_view expected_header_text) -> section_transaction_handles {
    auto body_paragraph = document.paragraphs();
    REQUIRE(body_paragraph.valid());
    auto body_run = body_paragraph.add_run("transaction body");
    REQUIRE(body_run.valid());

    auto body_table = document.append_table(1U, 1U);
    REQUIRE(body_table.valid());
    auto body_row = body_table.rows();
    REQUIRE(body_row.valid());
    auto body_cell = body_row.cells();
    REQUIRE(body_cell.valid());
    REQUIRE(body_cell.set_text("transaction cell"));

    auto header_paragraph = document.header_paragraphs(header_index);
    REQUIRE(header_paragraph.valid());
    auto header_run = header_paragraph.runs();
    REQUIRE(header_run.valid());
    REQUIRE_EQ(header_run.get_text(), expected_header_text);

    return section_transaction_handles{
        document.body_template(), document.paragraphs(), body_run, body_table,
        body_row, body_cell, document.header_template(header_index),
        header_paragraph, header_run};
}

void check_section_transaction_handles(
    const section_transaction_handles &handles,
    std::string_view expected_header_text) {
    CHECK(handles.body_template);
    CHECK(handles.body_paragraph.valid());
    CHECK(handles.body_run.valid());
    CHECK_EQ(handles.body_run.get_text(), "transaction body");
    CHECK(handles.body_table.valid());
    CHECK(handles.body_row.valid());
    CHECK(handles.body_cell.valid());
    CHECK_EQ(handles.body_cell.get_text(), "transaction cell");
    CHECK(handles.header_template);
    CHECK(handles.header_paragraph.valid());
    CHECK(handles.header_run.valid());
    CHECK_EQ(handles.header_run.get_text(), expected_header_text);
}

void prepare_page_setup_transaction_document(
    featherdoc::Document &document) {
    REQUIRE_FALSE(document.create_empty());
    REQUIRE(document.set_section_page_setup(0U,
                                            original_section_page_setup()));
    auto header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.valid());
    REQUIRE(header.add_run("page setup header").valid());
}

void prepare_reference_copy_transaction_document(
    featherdoc::Document &document) {
    REQUIRE_FALSE(document.create_empty());
    auto source_default = document.ensure_section_header_paragraphs(0U);
    REQUIRE(source_default.valid());
    REQUIRE(source_default.add_run("source default header").valid());
    auto source_even = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(source_even.valid());
    REQUIRE(source_even.add_run("source even header").valid());

    REQUIRE(document.append_section(false));
    auto target_default = document.ensure_section_header_paragraphs(1U);
    REQUIRE(target_default.valid());
    REQUIRE(target_default.add_run("target default header").valid());
    auto target_first = document.ensure_section_header_paragraphs(
        1U, featherdoc::section_reference_kind::first_page);
    REQUIRE(target_first.valid());
    REQUIRE(target_first.add_run("target first header").valid());

    auto footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.valid());
    REQUIRE(footer.add_run("retained footer").valid());
}

void prepare_header_move_transaction_document(
    featherdoc::Document &document) {
    REQUIRE_FALSE(document.create_empty());
    auto first = document.ensure_section_header_paragraphs(0U);
    REQUIRE(first.valid());
    REQUIRE(first.add_run("first movable header").valid());
    auto second = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(second.valid());
    REQUIRE(second.add_run("second movable header").valid());

    auto footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.valid());
    REQUIRE(footer.add_run("retained footer").valid());
}

void check_original_reference_copy_state(featherdoc::Document &document) {
    auto source_default = document.section_header_paragraphs(0U);
    auto source_even = document.section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto target_default = document.section_header_paragraphs(1U);
    auto target_even = document.section_header_paragraphs(
        1U, featherdoc::section_reference_kind::even_page);
    auto target_first = document.section_header_paragraphs(
        1U, featherdoc::section_reference_kind::first_page);

    REQUIRE(source_default.valid());
    REQUIRE(source_even.valid());
    REQUIRE(target_default.valid());
    REQUIRE(target_first.valid());
    CHECK_FALSE(target_even.valid());
    CHECK_EQ(source_default.runs().get_text(), "source default header");
    CHECK_EQ(source_even.runs().get_text(), "source even header");
    CHECK_EQ(target_default.runs().get_text(), "target default header");
    CHECK_EQ(target_first.runs().get_text(), "target first header");
}

void check_copied_reference_state(featherdoc::Document &document) {
    auto source_default = document.section_header_paragraphs(0U);
    auto source_even = document.section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto target_default = document.section_header_paragraphs(1U);
    auto target_even = document.section_header_paragraphs(
        1U, featherdoc::section_reference_kind::even_page);
    auto target_first = document.section_header_paragraphs(
        1U, featherdoc::section_reference_kind::first_page);

    REQUIRE(source_default.valid());
    REQUIRE(source_even.valid());
    REQUIRE(target_default.valid());
    REQUIRE(target_even.valid());
    CHECK_FALSE(target_first.valid());
    CHECK_EQ(source_default.runs().get_text(), "source default header");
    CHECK_EQ(source_even.runs().get_text(), "source even header");
    CHECK_EQ(target_default.runs().get_text(), "source default header");
    CHECK_EQ(target_even.runs().get_text(), "source even header");
}

void check_original_header_move_state(featherdoc::Document &document) {
    CHECK_EQ(document.header_count(), 2U);
    CHECK_EQ(document.header_paragraphs(0U).runs().get_text(),
             "first movable header");
    CHECK_EQ(document.header_paragraphs(1U).runs().get_text(),
             "second movable header");
    CHECK_EQ(document.section_header_paragraphs(0U).runs().get_text(),
             "first movable header");
    CHECK_EQ(document
                 .section_header_paragraphs(
                     0U, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "second movable header");
}

void check_moved_header_state(featherdoc::Document &document) {
    CHECK_EQ(document.header_count(), 2U);
    CHECK_EQ(document.header_paragraphs(0U).runs().get_text(),
             "second movable header");
    CHECK_EQ(document.header_paragraphs(1U).runs().get_text(),
             "first movable header");
    CHECK_EQ(document.section_header_paragraphs(0U).runs().get_text(),
             "first movable header");
    CHECK_EQ(document
                 .section_header_paragraphs(
                     0U, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "second movable header");
}

void write_multi_alias_header_fixture(const std::filesystem::path &path) {
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
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:headerReference w:type="even" r:id="rId3"/>
          <w:headerReference w:type="first" r:id="rId5"/>
          <w:titlePg w:val="1"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>first section body</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>second section body</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
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
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId6"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="settings.xml"/>
</Relationships>
)";
    constexpr auto first_header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>multi alias header</w:t></w:r></w:p>
</w:hdr>
)";
    constexpr auto second_header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>second header part</w:t></w:r></w:p>
</w:hdr>
)";
    constexpr auto settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:evenAndOddHeaders w:val="1"/>
</w:settings>
)";

    write_test_archive_entries(
        path,
        {{test_content_types_xml_entry, content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"word/_rels/document.xml.rels", document_relationships_xml},
         {"word/header1.xml", first_header_xml},
         {"word/header2.xml", second_header_xml},
         {"word/settings.xml", settings_xml}});
}

[[nodiscard]] auto has_related_part_reference(
    const featherdoc::related_part_inspection_summary &part,
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) -> bool {
    return std::ranges::any_of(
        part.references, [&](const auto &reference) {
            return reference.section_index == section_index &&
                   reference.reference_kind == reference_kind;
        });
}

void check_multi_alias_part_state(featherdoc::Document &document, bool moved,
                                  bool even_reference_present) {
    const auto parts = document.inspect_header_parts();
    REQUIRE_EQ(parts.size(), 2U);
    const auto &first_header = parts[moved ? 1U : 0U];
    const auto &second_header = parts[moved ? 0U : 1U];

    CHECK_EQ(first_header.relationship_id, "rId2");
    CHECK_EQ(first_header.entry_name, "word/header1.xml");
    CHECK_EQ(first_header.references.size(),
             even_reference_present ? 3U : 2U);
    CHECK(has_related_part_reference(
        first_header, 0U,
        featherdoc::section_reference_kind::default_reference));
    CHECK(has_related_part_reference(
        first_header, 0U,
        featherdoc::section_reference_kind::first_page));
    CHECK_EQ(has_related_part_reference(
                 first_header, 0U,
                 featherdoc::section_reference_kind::even_page),
             even_reference_present);

    CHECK_EQ(second_header.relationship_id, "rId4");
    CHECK_EQ(second_header.entry_name, "word/header2.xml");
    REQUIRE_EQ(second_header.references.size(), 1U);
    CHECK(has_related_part_reference(
        second_header, 1U,
        featherdoc::section_reference_kind::default_reference));

    CHECK_EQ(document.header_paragraphs(moved ? 1U : 0U).runs().get_text(),
             "multi alias header");
    CHECK_EQ(document.header_paragraphs(moved ? 0U : 1U).runs().get_text(),
             "second header part");
}

void check_header_reference_markers(featherdoc::Document &document,
                                    bool even_reference_present) {
    const auto first_section = document.inspect_section(0U);
    REQUIRE(first_section.has_value());
    REQUIRE(first_section->even_and_odd_headers_enabled.has_value());
    CHECK(*first_section->even_and_odd_headers_enabled);
    CHECK(first_section->different_first_page_enabled);
    CHECK(first_section->header.has_default);
    CHECK(first_section->header.has_first);
    CHECK_EQ(first_section->header.has_even, even_reference_present);

    const auto second_section = document.inspect_section(1U);
    REQUIRE(second_section.has_value());
    REQUIRE(second_section->even_and_odd_headers_enabled.has_value());
    CHECK(*second_section->even_and_odd_headers_enabled);
    CHECK(second_section->header.has_default);
    CHECK_FALSE(second_section->header.has_first);
    CHECK_FALSE(second_section->header.has_even);
}

void check_saved_header_relationships(
    const std::filesystem::path &path,
    std::initializer_list<std::string_view> expected_ids,
    std::initializer_list<std::string_view> expected_targets) {
    const auto relationships_xml =
        read_test_docx_entry(path, "word/_rels/document.xml.rels");
    pugi::xml_document relationships_document;
    REQUIRE(relationships_document.load_buffer(relationships_xml.data(),
                                               relationships_xml.size()));

    auto actual_ids = std::vector<std::string>{};
    auto actual_targets = std::vector<std::string>{};
    const auto relationships = relationships_document.child("Relationships");
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto type =
            std::string_view{relationship.attribute("Type").value()};
        if (!type.ends_with("/header")) {
            continue;
        }
        actual_ids.emplace_back(relationship.attribute("Id").value());
        actual_targets.emplace_back(
            relationship.attribute("Target").value());
    }

    REQUIRE_EQ(actual_ids.size(), expected_ids.size());
    REQUIRE_EQ(actual_targets.size(), expected_targets.size());
    auto id_index = std::size_t{0U};
    for (const auto expected_id : expected_ids) {
        CHECK_EQ(actual_ids[id_index++], expected_id);
    }
    auto target_index = std::size_t{0U};
    for (const auto expected_target : expected_targets) {
        CHECK_EQ(actual_targets[target_index++], expected_target);
    }
}

struct multi_alias_transaction_handles final {
    section_transaction_handles common;
    featherdoc::TemplatePart second_header_template;
    featherdoc::Paragraph second_header;
    featherdoc::Run second_header_run;
    featherdoc::Paragraph default_reference;
    featherdoc::Run default_reference_run;
    featherdoc::Paragraph even_reference;
    featherdoc::Run even_reference_run;
    featherdoc::Paragraph first_reference;
    featherdoc::Run first_reference_run;
    featherdoc::Paragraph second_section_reference;
    featherdoc::Run second_section_reference_run;
};

[[nodiscard]] auto capture_multi_alias_transaction_handles(
    featherdoc::Document &document) -> multi_alias_transaction_handles {
    auto common = capture_section_transaction_handles(
        document, 0U, "multi alias header");
    auto second_header = document.header_paragraphs(1U);
    auto second_header_run = second_header.runs();
    auto default_reference = document.section_header_paragraphs(0U);
    auto default_reference_run = default_reference.runs();
    auto even_reference = document.section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto even_reference_run = even_reference.runs();
    auto first_reference = document.section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto first_reference_run = first_reference.runs();
    auto second_section_reference = document.section_header_paragraphs(1U);
    auto second_section_reference_run = second_section_reference.runs();

    REQUIRE(second_header.valid());
    REQUIRE(second_header_run.valid());
    REQUIRE(default_reference.valid());
    REQUIRE(default_reference_run.valid());
    REQUIRE(even_reference.valid());
    REQUIRE(even_reference_run.valid());
    REQUIRE(first_reference.valid());
    REQUIRE(first_reference_run.valid());
    REQUIRE(second_section_reference.valid());
    REQUIRE(second_section_reference_run.valid());

    return multi_alias_transaction_handles{
        common,
        document.header_template(1U),
        second_header,
        second_header_run,
        default_reference,
        default_reference_run,
        even_reference,
        even_reference_run,
        first_reference,
        first_reference_run,
        second_section_reference,
        second_section_reference_run};
}

void check_multi_alias_transaction_handles(
    const multi_alias_transaction_handles &handles) {
    check_section_transaction_handles(handles.common, "multi alias header");
    CHECK(handles.second_header_template);
    CHECK(handles.second_header.valid());
    CHECK(handles.second_header_run.valid());
    CHECK_EQ(handles.second_header_run.get_text(), "second header part");
    CHECK(handles.default_reference.valid());
    CHECK(handles.default_reference_run.valid());
    CHECK(handles.even_reference.valid());
    CHECK(handles.even_reference_run.valid());
    CHECK(handles.first_reference.valid());
    CHECK(handles.first_reference_run.valid());
    CHECK(handles.second_section_reference.valid());
    CHECK(handles.second_section_reference_run.valid());
    CHECK_EQ(handles.default_reference_run.get_text(),
             "multi alias header");
    CHECK_EQ(handles.even_reference_run.get_text(), "multi alias header");
    CHECK_EQ(handles.first_reference_run.get_text(), "multi alias header");
    CHECK_EQ(handles.second_section_reference_run.get_text(),
             "second header part");
}

void write_lazy_settings_copy_fixture(const std::filesystem::path &path) {
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
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="even" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>lazy source section</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>lazy target section</w:t></w:r></w:p>
    <w:sectPr>
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
  <w:p><w:r><w:t>lazy even header</w:t></w:r></w:p>
</w:hdr>
)";
    constexpr auto footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>lazy retained footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        path,
        {{test_content_types_xml_entry, content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"word/_rels/document.xml.rels", document_relationships_xml},
         {"word/header1.xml", header_xml},
         {"word/footer1.xml", footer_xml}});
}

void check_lazy_copy_archive_state(const std::filesystem::path &path,
                                   bool copied) {
    const auto document_xml =
        read_test_docx_entry(path, test_document_xml_entry);
    pugi::xml_document document;
    REQUIRE(document.load_buffer(document_xml.data(), document_xml.size()));
    const auto body = document.child("w:document").child("w:body");
    const auto source_properties =
        body.child("w:p").child("w:pPr").child("w:sectPr");
    const auto target_properties = body.child("w:sectPr");
    const auto source_even = source_properties.find_child_by_attribute(
        "w:headerReference", "w:type", "even");
    const auto target_even = target_properties.find_child_by_attribute(
        "w:headerReference", "w:type", "even");
    REQUIRE(source_even != pugi::xml_node{});
    CHECK_EQ(std::string_view{source_even.attribute("r:id").value()}, "rId2");
    CHECK_EQ(target_even != pugi::xml_node{}, copied);
    if (copied) {
        CHECK_EQ(std::string_view{target_even.attribute("r:id").value()},
                 "rId2");
    }
    CHECK_EQ(std::string_view{source_properties
                                  .find_child_by_attribute(
                                      "w:footerReference", "w:type", "default")
                                  .attribute("r:id")
                                  .value()},
             "rId3");
    CHECK_EQ(std::string_view{target_properties
                                  .find_child_by_attribute(
                                      "w:footerReference", "w:type", "default")
                                  .attribute("r:id")
                                  .value()},
             "rId3");

    const auto relationships_xml =
        read_test_docx_entry(path, "word/_rels/document.xml.rels");
    pugi::xml_document relationships_document;
    REQUIRE(relationships_document.load_buffer(relationships_xml.data(),
                                               relationships_xml.size()));
    std::size_t header_relationship_count = 0U;
    std::size_t footer_relationship_count = 0U;
    std::size_t settings_relationship_count = 0U;
    for (auto relationship =
             relationships_document.child("Relationships").child(
                 "Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto type =
            std::string_view{relationship.attribute("Type").value()};
        if (type.ends_with("/header")) {
            ++header_relationship_count;
            CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                     "rId2");
            CHECK_EQ(
                std::string_view{relationship.attribute("Target").value()},
                "header1.xml");
        } else if (type.ends_with("/footer")) {
            ++footer_relationship_count;
            CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                     "rId3");
            CHECK_EQ(
                std::string_view{relationship.attribute("Target").value()},
                "footer1.xml");
        } else if (type.ends_with("/settings")) {
            ++settings_relationship_count;
            CHECK_EQ(
                std::string_view{relationship.attribute("Target").value()},
                "settings.xml");
        }
    }
    CHECK_EQ(header_relationship_count, 1U);
    CHECK_EQ(footer_relationship_count, 1U);
    CHECK_EQ(settings_relationship_count, copied ? 1U : 0U);

    const auto content_types_xml =
        read_test_docx_entry(path, test_content_types_xml_entry);
    pugi::xml_document content_types_document;
    REQUIRE(content_types_document.load_buffer(content_types_xml.data(),
                                               content_types_xml.size()));
    bool has_document_override = false;
    bool has_header_override = false;
    bool has_footer_override = false;
    std::size_t settings_override_count = 0U;
    for (auto override_node =
             content_types_document.child("Types").child("Override");
         override_node != pugi::xml_node{};
         override_node = override_node.next_sibling("Override")) {
        const auto part_name =
            std::string_view{override_node.attribute("PartName").value()};
        if (part_name == "/word/document.xml") {
            has_document_override = true;
        } else if (part_name == "/word/header1.xml") {
            has_header_override = true;
        } else if (part_name == "/word/footer1.xml") {
            has_footer_override = true;
        } else if (part_name == "/word/settings.xml") {
            ++settings_override_count;
            CHECK_EQ(
                std::string_view{
                    override_node.attribute("ContentType").value()},
                "application/vnd.openxmlformats-officedocument."
                "wordprocessingml.settings+xml");
        }
    }
    CHECK(has_document_override);
    CHECK(has_header_override);
    CHECK(has_footer_override);
    CHECK_EQ(settings_override_count, copied ? 1U : 0U);

    CHECK_EQ(test_docx_entry_exists(path, "word/settings.xml"), copied);
    if (copied) {
        const auto settings_xml =
            read_test_docx_entry(path, "word/settings.xml");
        pugi::xml_document settings_document;
        REQUIRE(settings_document.load_buffer(settings_xml.data(),
                                              settings_xml.size()));
        const auto marker = settings_document.child("w:settings").child(
            "w:evenAndOddHeaders");
        REQUIRE(marker != pugi::xml_node{});
        CHECK_EQ(std::string_view{marker.attribute("w:val").value()}, "1");
    }
}

void check_lazy_copy_live_references(featherdoc::Document &document,
                                     bool copied) {
    CHECK_EQ(document.section_count(), 2U);
    CHECK_EQ(document.header_count(), 1U);
    CHECK_EQ(document.footer_count(), 1U);

    auto source_even = document.section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto target_even = document.section_header_paragraphs(
        1U, featherdoc::section_reference_kind::even_page);
    REQUIRE(source_even.valid());
    CHECK_EQ(source_even.runs().get_text(), "lazy even header");
    CHECK_EQ(target_even.valid(), copied);
    if (copied) {
        CHECK_EQ(target_even.runs().get_text(), "lazy even header");
    }
    CHECK_FALSE(document.section_header_paragraphs(0U).valid());
    CHECK_FALSE(document.section_header_paragraphs(1U).valid());
    CHECK_FALSE(document
                    .section_header_paragraphs(
                        0U, featherdoc::section_reference_kind::first_page)
                    .valid());
    CHECK_FALSE(document
                    .section_header_paragraphs(
                        1U, featherdoc::section_reference_kind::first_page)
                    .valid());

    auto source_footer = document.section_footer_paragraphs(0U);
    auto target_footer = document.section_footer_paragraphs(1U);
    REQUIRE(source_footer.valid());
    REQUIRE(target_footer.valid());
    CHECK_EQ(source_footer.runs().get_text(), "lazy retained footer");
    CHECK_EQ(target_footer.runs().get_text(), "lazy retained footer");
}

void check_lazy_copy_even_setting_enabled(featherdoc::Document &document) {
    const auto section = document.inspect_section(1U);
    REQUIRE(section.has_value());
    REQUIRE(section->even_and_odd_headers_enabled.has_value());
    CHECK(*section->even_and_odd_headers_enabled);
}

struct lazy_copy_transaction_handles final {
    section_transaction_handles common;
    featherdoc::TemplatePart footer_template;
    featherdoc::Paragraph footer;
    featherdoc::Run footer_run;
    featherdoc::Paragraph source_even_header;
    featherdoc::Run source_even_header_run;
    featherdoc::Paragraph target_footer;
    featherdoc::Run target_footer_run;
};

[[nodiscard]] auto capture_lazy_copy_transaction_handles(
    featherdoc::Document &document) -> lazy_copy_transaction_handles {
    auto common = capture_section_transaction_handles(
        document, 0U, "lazy even header");
    auto footer = document.footer_paragraphs(0U);
    auto footer_run = footer.runs();
    auto source_even_header = document.section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    auto source_even_header_run = source_even_header.runs();
    auto target_footer = document.section_footer_paragraphs(1U);
    auto target_footer_run = target_footer.runs();
    REQUIRE(footer.valid());
    REQUIRE(footer_run.valid());
    REQUIRE(source_even_header.valid());
    REQUIRE(source_even_header_run.valid());
    REQUIRE(target_footer.valid());
    REQUIRE(target_footer_run.valid());

    return lazy_copy_transaction_handles{
        common,
        document.footer_template(0U),
        footer,
        footer_run,
        source_even_header,
        source_even_header_run,
        target_footer,
        target_footer_run};
}

void check_lazy_copy_transaction_handles(
    const lazy_copy_transaction_handles &handles) {
    check_section_transaction_handles(handles.common, "lazy even header");
    CHECK(handles.footer_template);
    CHECK(handles.footer.valid());
    CHECK(handles.footer_run.valid());
    CHECK_EQ(handles.footer_run.get_text(), "lazy retained footer");
    CHECK(handles.source_even_header.valid());
    CHECK(handles.source_even_header_run.valid());
    CHECK_EQ(handles.source_even_header_run.get_text(), "lazy even header");
    CHECK(handles.target_footer.valid());
    CHECK(handles.target_footer_run.valid());
    CHECK_EQ(handles.target_footer_run.get_text(), "lazy retained footer");
}

} // namespace

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
void *operator new(std::size_t size) {
    return allocate_section_text_unaligned(size);
}
void *operator new[](std::size_t size) {
    return allocate_section_text_unaligned(size);
}
void *operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_section_text_aligned(
        size, static_cast<std::size_t>(alignment));
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_section_text_aligned(
        size, static_cast<std::size_t>(alignment));
}
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::size_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::align_val_t) noexcept {
    free_section_text_aligned(memory);
}
void operator delete[](void *memory, std::align_val_t) noexcept {
    free_section_text_aligned(memory);
}
void operator delete(void *memory, std::size_t,
                     std::align_val_t) noexcept {
    free_section_text_aligned(memory);
}
void operator delete[](void *memory, std::size_t,
                       std::align_val_t) noexcept {
    free_section_text_aligned(memory);
}
#endif

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "section header and footer text replacement is all-or-nothing for every "
    "allocation failure") {
    const section_text_pugi_memory_guard pugi_guard;

    for (const auto kind : {section_text_part_kind::header,
                            section_text_part_kind::footer}) {
        std::size_t successful_allocation_count = 0U;
        {
            featherdoc::Document successful;
            prepare_existing_section_text_parts(successful);
            bool replacement_succeeded = false;
            {
                const section_text_allocation_window allocation_window{0U};
                replacement_succeeded = replace_section_text_part(
                    successful, kind, "replacement one\n replacement two ");
            }
            successful_allocation_count = section_text_allocation_calls.load(
                std::memory_order_relaxed);
            REQUIRE(replacement_succeeded);
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(kind);
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);

            featherdoc::Document document;
            prepare_existing_section_text_parts(document);

            auto target = section_text_part(document, kind);
            auto target_run = target.runs();
            auto other_part = other_section_text_part(document, kind);
            auto other_run = other_part.runs();
            auto body = document.paragraphs();
            auto body_run = body.add_run("body remains live");
            REQUIRE(target.valid());
            REQUIRE(target_run.valid());
            REQUIRE(other_part.valid());
            REQUIRE(other_run.valid());
            REQUIRE(body.valid());
            REQUIRE(body_run.valid());

            bool replacement_result = true;
            bool threw = false;
            try {
                const section_text_allocation_window allocation_window{
                    failure_call};
                replacement_result = replace_section_text_part(
                    document, kind, "replacement one\n replacement two ");
            } catch (...) {
                threw = true;
            }

            CHECK_FALSE(threw);
            CHECK_FALSE(replacement_result);
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            CHECK(target.valid());
            CHECK(target_run.valid());
            CHECK_EQ(target_run.get_text(),
                     kind == section_text_part_kind::header ? "old header"
                                                            : "old footer");
            CHECK(other_part.valid());
            CHECK(other_run.valid());
            CHECK_EQ(other_run.get_text(),
                     kind == section_text_part_kind::header ? "old footer"
                                                            : "old header");
            CHECK(body.valid());
            CHECK(body_run.valid());
            CHECK_EQ(body_run.get_text(), "body remains live");
            CHECK_EQ(document.header_count(), 1U);
            CHECK_EQ(document.footer_count(), 1U);

            REQUIRE(replace_section_text_part(
                document, kind, "replacement one\n replacement two "));
            CHECK_FALSE(target.valid());
            CHECK_FALSE(target_run.valid());
            CHECK(other_part.valid());
            CHECK(other_run.valid());
            CHECK(body.valid());
            CHECK(body_run.valid());
            CHECK_EQ(section_text_part(document, kind).runs().get_text(),
                     "replacement one");
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "section related part ensure and assign preserve every published state on "
    "allocation failure") {
    enum class operation_kind { ensure_even_header, assign_even_header };
    const section_text_pugi_memory_guard pugi_guard;

    const auto prepare_document = [](featherdoc::Document &document,
                                     operation_kind operation) {
        REQUIRE_FALSE(document.create_empty());
        auto footer = document.ensure_section_footer_paragraphs(0U);
        REQUIRE(footer.valid());
        REQUIRE(footer.add_run("retained footer").valid());
        if (operation == operation_kind::assign_even_header) {
            auto header = document.ensure_section_header_paragraphs(0U);
            REQUIRE(header.valid());
            REQUIRE(header.add_run("retained header").valid());
        }
    };
    const auto invoke_operation = [](featherdoc::Document &document,
                                     operation_kind operation)
        -> featherdoc::Paragraph {
        if (operation == operation_kind::ensure_even_header) {
            return document.ensure_section_header_paragraphs(
                0U, featherdoc::section_reference_kind::even_page);
        }
        return document.assign_section_header_paragraphs(
            0U, 0U, featherdoc::section_reference_kind::even_page);
    };

    for (const auto operation : {operation_kind::ensure_even_header,
                                 operation_kind::assign_even_header}) {
        std::size_t successful_allocation_count = 0U;
        {
            featherdoc::Document successful;
            prepare_document(successful, operation);
            featherdoc::Paragraph result;
            {
                const section_text_allocation_window allocation_window{0U};
                result = invoke_operation(successful, operation);
            }
            successful_allocation_count = section_text_allocation_calls.load(
                std::memory_order_relaxed);
            REQUIRE(result.valid());
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(operation);
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);

            featherdoc::Document document;
            prepare_document(document, operation);
            const auto original_header_count = document.header_count();
            const auto original_footer_count = document.footer_count();

            auto retained_footer = document.section_footer_paragraphs(0U);
            auto retained_footer_run = retained_footer.runs();
            auto retained_header = document.section_header_paragraphs(0U);
            auto retained_header_run = retained_header.runs();
            auto body = document.paragraphs();
            auto body_run = body.add_run("retained body");
            auto body_template = document.body_template();
            REQUIRE(retained_footer.valid());
            REQUIRE(retained_footer_run.valid());
            REQUIRE(body.valid());
            REQUIRE(body_run.valid());
            REQUIRE(body_template);
            if (operation == operation_kind::assign_even_header) {
                REQUIRE(retained_header.valid());
                REQUIRE(retained_header_run.valid());
            } else {
                REQUIRE_FALSE(retained_header.valid());
            }
            REQUIRE_FALSE(document
                              .section_header_paragraphs(
                                  0U, featherdoc::section_reference_kind::even_page)
                              .valid());

            featherdoc::Paragraph result;
            bool threw = false;
            try {
                const section_text_allocation_window allocation_window{
                    failure_call};
                result = invoke_operation(document, operation);
            } catch (...) {
                threw = true;
            }

            CHECK_FALSE(threw);
            CHECK_FALSE(result.valid());
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            CHECK_EQ(document.header_count(), original_header_count);
            CHECK_EQ(document.footer_count(), original_footer_count);
            CHECK_FALSE(document
                            .section_header_paragraphs(
                                0U,
                                featherdoc::section_reference_kind::even_page)
                            .valid());
            CHECK(retained_footer.valid());
            CHECK(retained_footer_run.valid());
            CHECK_EQ(retained_footer_run.get_text(), "retained footer");
            CHECK(body.valid());
            CHECK(body_run.valid());
            CHECK_EQ(body_run.get_text(), "retained body");
            CHECK(body_template);
            if (operation == operation_kind::assign_even_header) {
                CHECK(retained_header.valid());
                CHECK(retained_header_run.valid());
                CHECK_EQ(retained_header_run.get_text(), "retained header");
            }

            auto retry = invoke_operation(document, operation);
            REQUIRE(retry.valid());
            CHECK(body.valid());
            CHECK(body_run.valid());
            CHECK(body_template);
            CHECK(retained_footer.valid());
            CHECK(retained_footer_run.valid());
            if (operation == operation_kind::assign_even_header) {
                CHECK(retained_header.valid());
                CHECK(retained_header_run.valid());
                CHECK_EQ(retry.runs().get_text(), "retained header");
            } else {
                CHECK_EQ(document.header_count(), original_header_count + 1U);
            }
        }
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "section page setup preserves state and public handles for every "
    "allocation failure") {
    const section_text_pugi_memory_guard pugi_guard;
    const auto replacement_setup = replacement_section_page_setup();

    std::size_t successful_allocation_count = 0U;
    {
        featherdoc::Document successful;
        prepare_page_setup_transaction_document(successful);
        const auto handles = capture_section_transaction_handles(
            successful, 0U, "page setup header");

        bool result = false;
        {
            const section_text_allocation_window allocation_window{0U};
            result = successful.set_section_page_setup(0U,
                                                       replacement_setup);
        }
        successful_allocation_count = section_text_allocation_calls.load(
            std::memory_order_relaxed);

        REQUIRE(result);
        check_section_page_setup(successful, replacement_setup);
        check_section_transaction_handles(handles, "page setup header");
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        featherdoc::Document document;
        prepare_page_setup_transaction_document(document);
        const auto handles = capture_section_transaction_handles(
            document, 0U, "page setup header");
        const auto original_section_count = document.section_count();
        const auto original_header_count = document.header_count();
        const auto original_footer_count = document.footer_count();

        bool result = true;
        bool threw = false;
        try {
            const section_text_allocation_window allocation_window{
                failure_call};
            result =
                document.set_section_page_setup(0U, replacement_setup);
        } catch (...) {
            threw = true;
        }

        CHECK_FALSE(threw);
        CHECK_FALSE(result);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.section_count(), original_section_count);
        CHECK_EQ(document.header_count(), original_header_count);
        CHECK_EQ(document.footer_count(), original_footer_count);
        check_section_page_setup(document, original_section_page_setup());
        check_section_transaction_handles(handles, "page setup header");

        REQUIRE(document.set_section_page_setup(0U, replacement_setup));
        check_section_page_setup(document, replacement_setup);
        check_section_transaction_handles(handles, "page setup header");
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "section header reference copy preserves every published state on every "
    "allocation failure") {
    const section_text_pugi_memory_guard pugi_guard;

    std::size_t successful_allocation_count = 0U;
    {
        featherdoc::Document successful;
        prepare_reference_copy_transaction_document(successful);
        const auto handles = capture_section_transaction_handles(
            successful, 0U, "source default header");
        auto target_default = successful.section_header_paragraphs(1U);
        auto target_default_run = target_default.runs();
        auto target_first = successful.section_header_paragraphs(
            1U, featherdoc::section_reference_kind::first_page);
        auto target_first_run = target_first.runs();
        auto retained_footer = successful.section_footer_paragraphs(0U);
        auto retained_footer_run = retained_footer.runs();
        REQUIRE(target_default.valid());
        REQUIRE(target_default_run.valid());
        REQUIRE(target_first.valid());
        REQUIRE(target_first_run.valid());
        REQUIRE(retained_footer.valid());
        REQUIRE(retained_footer_run.valid());

        bool result = false;
        {
            const section_text_allocation_window allocation_window{0U};
            result = successful.copy_section_header_references(0U, 1U);
        }
        successful_allocation_count = section_text_allocation_calls.load(
            std::memory_order_relaxed);

        REQUIRE(result);
        check_copied_reference_state(successful);
        check_section_transaction_handles(handles, "source default header");
        CHECK(target_default.valid());
        CHECK(target_default_run.valid());
        CHECK_EQ(target_default_run.get_text(), "target default header");
        CHECK(target_first.valid());
        CHECK(target_first_run.valid());
        CHECK_EQ(target_first_run.get_text(), "target first header");
        CHECK(retained_footer.valid());
        CHECK(retained_footer_run.valid());
        CHECK_EQ(retained_footer_run.get_text(), "retained footer");
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        featherdoc::Document document;
        prepare_reference_copy_transaction_document(document);
        const auto handles = capture_section_transaction_handles(
            document, 0U, "source default header");
        auto target_default = document.section_header_paragraphs(1U);
        auto target_default_run = target_default.runs();
        auto target_first = document.section_header_paragraphs(
            1U, featherdoc::section_reference_kind::first_page);
        auto target_first_run = target_first.runs();
        auto retained_footer = document.section_footer_paragraphs(0U);
        auto retained_footer_run = retained_footer.runs();
        const auto original_section_count = document.section_count();
        const auto original_header_count = document.header_count();
        const auto original_footer_count = document.footer_count();
        REQUIRE(target_default.valid());
        REQUIRE(target_default_run.valid());
        REQUIRE(target_first.valid());
        REQUIRE(target_first_run.valid());
        REQUIRE(retained_footer.valid());
        REQUIRE(retained_footer_run.valid());

        bool result = true;
        bool threw = false;
        try {
            const section_text_allocation_window allocation_window{
                failure_call};
            result = document.copy_section_header_references(0U, 1U);
        } catch (...) {
            threw = true;
        }

        CHECK_FALSE(threw);
        CHECK_FALSE(result);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.section_count(), original_section_count);
        CHECK_EQ(document.header_count(), original_header_count);
        CHECK_EQ(document.footer_count(), original_footer_count);
        check_original_reference_copy_state(document);
        check_section_transaction_handles(handles, "source default header");
        CHECK(target_default.valid());
        CHECK(target_default_run.valid());
        CHECK_EQ(target_default_run.get_text(), "target default header");
        CHECK(target_first.valid());
        CHECK(target_first_run.valid());
        CHECK_EQ(target_first_run.get_text(), "target first header");
        CHECK(retained_footer.valid());
        CHECK(retained_footer_run.valid());
        CHECK_EQ(retained_footer_run.get_text(), "retained footer");

        REQUIRE(document.copy_section_header_references(0U, 1U));
        check_copied_reference_state(document);
        check_section_transaction_handles(handles, "source default header");
        CHECK(target_default.valid());
        CHECK(target_default_run.valid());
        CHECK_EQ(target_default_run.get_text(), "target default header");
        CHECK(target_first.valid());
        CHECK(target_first_run.valid());
        CHECK_EQ(target_first_run.get_text(), "target first header");
        CHECK(retained_footer.valid());
        CHECK(retained_footer_run.valid());
        CHECK_EQ(retained_footer_run.get_text(), "retained footer");
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "header part move preserves ordering and handles for every allocation "
    "failure") {
    const section_text_pugi_memory_guard pugi_guard;

    std::size_t successful_allocation_count = 0U;
    {
        featherdoc::Document successful;
        prepare_header_move_transaction_document(successful);
        const auto handles = capture_section_transaction_handles(
            successful, 0U, "first movable header");
        auto second_header = successful.header_paragraphs(1U);
        auto second_header_run = second_header.runs();
        auto second_header_template = successful.header_template(1U);
        auto retained_footer = successful.section_footer_paragraphs(0U);
        auto retained_footer_run = retained_footer.runs();
        REQUIRE(second_header.valid());
        REQUIRE(second_header_run.valid());
        REQUIRE(second_header_template);
        REQUIRE(retained_footer.valid());
        REQUIRE(retained_footer_run.valid());

        bool result = false;
        {
            const section_text_allocation_window allocation_window{0U};
            result = successful.move_header_part(0U, 1U);
        }
        successful_allocation_count = section_text_allocation_calls.load(
            std::memory_order_relaxed);

        REQUIRE(result);
        check_moved_header_state(successful);
        check_section_transaction_handles(handles, "first movable header");
        CHECK(second_header.valid());
        CHECK(second_header_run.valid());
        CHECK_EQ(second_header_run.get_text(), "second movable header");
        CHECK(second_header_template);
        CHECK(retained_footer.valid());
        CHECK(retained_footer_run.valid());
        CHECK_EQ(retained_footer_run.get_text(), "retained footer");
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        featherdoc::Document document;
        prepare_header_move_transaction_document(document);
        const auto handles = capture_section_transaction_handles(
            document, 0U, "first movable header");
        auto second_header = document.header_paragraphs(1U);
        auto second_header_run = second_header.runs();
        auto second_header_template = document.header_template(1U);
        auto retained_footer = document.section_footer_paragraphs(0U);
        auto retained_footer_run = retained_footer.runs();
        const auto original_section_count = document.section_count();
        const auto original_header_count = document.header_count();
        const auto original_footer_count = document.footer_count();
        REQUIRE(second_header.valid());
        REQUIRE(second_header_run.valid());
        REQUIRE(second_header_template);
        REQUIRE(retained_footer.valid());
        REQUIRE(retained_footer_run.valid());

        bool result = true;
        bool threw = false;
        try {
            const section_text_allocation_window allocation_window{
                failure_call};
            result = document.move_header_part(0U, 1U);
        } catch (...) {
            threw = true;
        }

        CHECK_FALSE(threw);
        CHECK_FALSE(result);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.section_count(), original_section_count);
        CHECK_EQ(document.header_count(), original_header_count);
        CHECK_EQ(document.footer_count(), original_footer_count);
        check_original_header_move_state(document);
        check_section_transaction_handles(handles, "first movable header");
        CHECK(second_header.valid());
        CHECK(second_header_run.valid());
        CHECK_EQ(second_header_run.get_text(), "second movable header");
        CHECK(second_header_template);
        CHECK(retained_footer.valid());
        CHECK(retained_footer_run.valid());
        CHECK_EQ(retained_footer_run.get_text(), "retained footer");

        REQUIRE(document.move_header_part(0U, 1U));
        check_moved_header_state(document);
        check_section_transaction_handles(handles, "first movable header");
        CHECK(second_header.valid());
        CHECK(second_header_run.valid());
        CHECK_EQ(second_header_run.get_text(), "second movable header");
        CHECK(second_header_template);
        CHECK(retained_footer.valid());
        CHECK(retained_footer_run.valid());
        CHECK_EQ(retained_footer_run.get_text(), "retained footer");
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "section header reference removal preserves markers and handles for "
    "every allocation failure") {
    const section_text_pugi_memory_guard pugi_guard;
    const auto fixture_path = std::filesystem::current_path() /
                              "section_header_reference_remove_allocation.docx";

    std::size_t successful_allocation_count = 0U;
    {
        write_multi_alias_header_fixture(fixture_path);
        featherdoc::Document successful{fixture_path};
        REQUIRE_FALSE(successful.open());
        check_multi_alias_part_state(successful, false, true);
        check_header_reference_markers(successful, true);
        const auto handles =
            capture_multi_alias_transaction_handles(successful);

        bool result = false;
        {
            const section_text_allocation_window allocation_window{0U};
            result = successful.remove_section_header_reference(
                0U, featherdoc::section_reference_kind::even_page);
        }
        successful_allocation_count = section_text_allocation_calls.load(
            std::memory_order_relaxed);

        REQUIRE(result);
        check_multi_alias_part_state(successful, false, false);
        check_header_reference_markers(successful, false);
        check_multi_alias_transaction_handles(handles);
        CHECK_FALSE(successful
                        .section_header_paragraphs(
                            0U,
                            featherdoc::section_reference_kind::even_page)
                        .valid());
        REQUIRE_FALSE(successful.save());
        check_saved_header_relationships(
            fixture_path, {"rId2", "rId4", "rId3", "rId5"},
            {"header1.xml", "header2.xml", "header1.xml", "header1.xml"});
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        write_multi_alias_header_fixture(fixture_path);
        featherdoc::Document document{fixture_path};
        REQUIRE_FALSE(document.open());
        check_multi_alias_part_state(document, false, true);
        check_header_reference_markers(document, true);
        const auto handles = capture_multi_alias_transaction_handles(document);
        const auto original_section_count = document.section_count();
        const auto original_header_count = document.header_count();
        const auto original_footer_count = document.footer_count();

        bool result = true;
        bool threw = false;
        try {
            const section_text_allocation_window allocation_window{
                failure_call};
            result = document.remove_section_header_reference(
                0U, featherdoc::section_reference_kind::even_page);
        } catch (...) {
            threw = true;
        }

        CHECK_FALSE(threw);
        CHECK_FALSE(result);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.section_count(), original_section_count);
        CHECK_EQ(document.header_count(), original_header_count);
        CHECK_EQ(document.footer_count(), original_footer_count);
        check_multi_alias_part_state(document, false, true);
        check_header_reference_markers(document, true);
        check_multi_alias_transaction_handles(handles);
        REQUIRE_FALSE(document.save());
        check_saved_header_relationships(
            fixture_path, {"rId2", "rId4", "rId3", "rId5"},
            {"header1.xml", "header2.xml", "header1.xml", "header1.xml"});

        REQUIRE(document.remove_section_header_reference(
            0U, featherdoc::section_reference_kind::even_page));
        check_multi_alias_part_state(document, false, false);
        check_header_reference_markers(document, false);
        check_multi_alias_transaction_handles(handles);
        CHECK_FALSE(document
                        .section_header_paragraphs(
                            0U,
                            featherdoc::section_reference_kind::even_page)
                        .valid());
        REQUIRE_FALSE(document.save());
        check_saved_header_relationships(
            fixture_path, {"rId2", "rId4", "rId3", "rId5"},
            {"header1.xml", "header2.xml", "header1.xml", "header1.xml"});
    }

    std::error_code remove_error;
    std::filesystem::remove(fixture_path, remove_error);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "multi-alias header part move preserves relationships and handles for "
    "every allocation failure") {
    const section_text_pugi_memory_guard pugi_guard;
    const auto fixture_path = std::filesystem::current_path() /
                              "multi_alias_header_move_allocation.docx";

    std::size_t successful_allocation_count = 0U;
    {
        write_multi_alias_header_fixture(fixture_path);
        featherdoc::Document successful{fixture_path};
        REQUIRE_FALSE(successful.open());
        check_multi_alias_part_state(successful, false, true);
        check_header_reference_markers(successful, true);
        const auto handles =
            capture_multi_alias_transaction_handles(successful);

        bool result = false;
        {
            const section_text_allocation_window allocation_window{0U};
            result = successful.move_header_part(0U, 1U);
        }
        successful_allocation_count = section_text_allocation_calls.load(
            std::memory_order_relaxed);

        REQUIRE(result);
        check_multi_alias_part_state(successful, true, true);
        check_header_reference_markers(successful, true);
        check_multi_alias_transaction_handles(handles);
        REQUIRE_FALSE(successful.save());
        check_saved_header_relationships(
            fixture_path, {"rId4", "rId2", "rId3", "rId5"},
            {"header2.xml", "header1.xml", "header1.xml", "header1.xml"});
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        write_multi_alias_header_fixture(fixture_path);
        featherdoc::Document document{fixture_path};
        REQUIRE_FALSE(document.open());
        check_multi_alias_part_state(document, false, true);
        check_header_reference_markers(document, true);
        const auto handles = capture_multi_alias_transaction_handles(document);
        const auto original_section_count = document.section_count();
        const auto original_header_count = document.header_count();
        const auto original_footer_count = document.footer_count();

        bool result = true;
        bool threw = false;
        try {
            const section_text_allocation_window allocation_window{
                failure_call};
            result = document.move_header_part(0U, 1U);
        } catch (...) {
            threw = true;
        }

        CHECK_FALSE(threw);
        CHECK_FALSE(result);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.section_count(), original_section_count);
        CHECK_EQ(document.header_count(), original_header_count);
        CHECK_EQ(document.footer_count(), original_footer_count);
        check_multi_alias_part_state(document, false, true);
        check_header_reference_markers(document, true);
        check_multi_alias_transaction_handles(handles);
        REQUIRE_FALSE(document.save());
        check_saved_header_relationships(
            fixture_path, {"rId2", "rId4", "rId3", "rId5"},
            {"header1.xml", "header2.xml", "header1.xml", "header1.xml"});

        REQUIRE(document.move_header_part(0U, 1U));
        check_multi_alias_part_state(document, true, true);
        check_header_reference_markers(document, true);
        check_multi_alias_transaction_handles(handles);
        REQUIRE_FALSE(document.save());
        check_saved_header_relationships(
            fixture_path, {"rId4", "rId2", "rId3", "rId5"},
            {"header2.xml", "header1.xml", "header1.xml", "header1.xml"});
    }

    std::error_code remove_error;
    std::filesystem::remove(fixture_path, remove_error);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "lazy settings attachment during section header reference copy is "
    "all-or-nothing for every allocation failure") {
    const section_text_pugi_memory_guard pugi_guard;
    const auto fixture_path = std::filesystem::current_path() /
                              "lazy_settings_header_copy_allocation.docx";
    const auto failure_snapshot_path =
        std::filesystem::current_path() /
        "lazy_settings_header_copy_failure_snapshot.docx";
    const auto success_snapshot_path =
        std::filesystem::current_path() /
        "lazy_settings_header_copy_success_snapshot.docx";

    std::size_t successful_allocation_count = 0U;
    {
        write_lazy_settings_copy_fixture(fixture_path);
        check_lazy_copy_archive_state(fixture_path, false);
        featherdoc::Document successful{fixture_path};
        REQUIRE_FALSE(successful.open());
        check_lazy_copy_live_references(successful, false);
        const auto handles =
            capture_lazy_copy_transaction_handles(successful);

        bool result = false;
        {
            const section_text_allocation_window allocation_window{0U};
            result = successful.copy_section_header_references(0U, 1U);
        }
        successful_allocation_count = section_text_allocation_calls.load(
            std::memory_order_relaxed);

        REQUIRE(result);
        check_lazy_copy_live_references(successful, true);
        check_lazy_copy_transaction_handles(handles);
        check_lazy_copy_even_setting_enabled(successful);
        REQUIRE_FALSE(successful.save_as(success_snapshot_path));
        check_lazy_copy_archive_state(success_snapshot_path, true);
    }
    REQUIRE_GT(successful_allocation_count, 0U);

    for (std::size_t failure_call = 1U;
         failure_call <= successful_allocation_count; ++failure_call) {
        CAPTURE(failure_call);
        CAPTURE(successful_allocation_count);

        write_lazy_settings_copy_fixture(fixture_path);
        check_lazy_copy_archive_state(fixture_path, false);
        featherdoc::Document document{fixture_path};
        REQUIRE_FALSE(document.open());
        check_lazy_copy_live_references(document, false);
        const auto handles = capture_lazy_copy_transaction_handles(document);

        bool result = true;
        bool threw = false;
        try {
            const section_text_allocation_window allocation_window{
                failure_call};
            result = document.copy_section_header_references(0U, 1U);
        } catch (...) {
            threw = true;
        }

        CHECK_FALSE(threw);
        CHECK_FALSE(result);
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        check_lazy_copy_live_references(document, false);
        check_lazy_copy_transaction_handles(handles);
        REQUIRE_FALSE(document.save_as(failure_snapshot_path));
        check_lazy_copy_archive_state(failure_snapshot_path, false);

        bool retry_result = false;
        {
            const section_text_allocation_window allocation_window{0U};
            retry_result =
                document.copy_section_header_references(0U, 1U);
        }
        const auto retry_allocation_count =
            section_text_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE(retry_result);
        CHECK_EQ(retry_allocation_count, successful_allocation_count);
        check_lazy_copy_live_references(document, true);
        check_lazy_copy_transaction_handles(handles);
        check_lazy_copy_even_setting_enabled(document);
        REQUIRE_FALSE(document.save_as(success_snapshot_path));
        check_lazy_copy_archive_state(success_snapshot_path, true);
    }

    std::error_code remove_error;
    std::filesystem::remove(fixture_path, remove_error);
    remove_error.clear();
    std::filesystem::remove(failure_snapshot_path, remove_error);
    remove_error.clear();
    std::filesystem::remove(success_snapshot_path, remove_error);
}
