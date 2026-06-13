#include <filesystem>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <featherdoc.hpp>
TEST_CASE("inspect sections returns header footer layout flags and entry names") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "inspect_sections_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &section0_default_header = doc.ensure_section_header_paragraphs(0U);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.set_text("section 0 default header"));

    auto &section0_even_header = doc.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(section0_even_header.has_next());
    CHECK(section0_even_header.set_text("section 0 even header"));

    auto &section0_first_footer = doc.ensure_section_footer_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(section0_first_footer.has_next());
    CHECK(section0_first_footer.set_text("section 0 first footer"));

    CHECK(doc.append_section(false));

    auto &section1_default_header = doc.ensure_section_header_paragraphs(1U);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.set_text("section 1 default header"));

    auto &section1_default_footer = doc.ensure_section_footer_paragraphs(1U);
    REQUIRE(section1_default_footer.has_next());
    CHECK(section1_default_footer.set_text("section 1 default footer"));

    CHECK(doc.append_section(false));

    const auto sections = doc.inspect_sections();
    CHECK_EQ(sections.section_count, 3U);
    CHECK_EQ(sections.header_count, 3U);
    CHECK_EQ(sections.footer_count, 2U);
    REQUIRE(sections.even_and_odd_headers_enabled.has_value());
    CHECK(*sections.even_and_odd_headers_enabled);
    REQUIRE(sections.sections.size() == 3U);

    CHECK_EQ(sections.sections[0].index, 0U);
    REQUIRE(sections.sections[0].even_and_odd_headers_enabled.has_value());
    CHECK(*sections.sections[0].even_and_odd_headers_enabled);
    CHECK(sections.sections[0].different_first_page_enabled);
    CHECK(sections.sections[0].header.has_default);
    CHECK_FALSE(sections.sections[0].header.has_first);
    CHECK(sections.sections[0].header.has_even);
    CHECK_FALSE(sections.sections[0].header.default_linked_to_previous);
    CHECK_FALSE(sections.sections[0].header.even_linked_to_previous);
    REQUIRE(sections.sections[0].header.default_entry_name.has_value());
    CHECK_EQ(*sections.sections[0].header.default_entry_name, "word/header1.xml");
    REQUIRE(sections.sections[0].header.even_entry_name.has_value());
    CHECK_EQ(*sections.sections[0].header.even_entry_name, "word/header2.xml");
    CHECK_FALSE(sections.sections[0].header.first_entry_name.has_value());
    REQUIRE(sections.sections[0].header.resolved_default_entry_name.has_value());
    CHECK_EQ(*sections.sections[0].header.resolved_default_entry_name, "word/header1.xml");
    REQUIRE(sections.sections[0].header.resolved_default_section_index.has_value());
    CHECK_EQ(*sections.sections[0].header.resolved_default_section_index, 0U);
    REQUIRE(sections.sections[0].header.resolved_even_entry_name.has_value());
    CHECK_EQ(*sections.sections[0].header.resolved_even_entry_name, "word/header2.xml");
    REQUIRE(sections.sections[0].header.resolved_even_section_index.has_value());
    CHECK_EQ(*sections.sections[0].header.resolved_even_section_index, 0U);
    CHECK_FALSE(sections.sections[0].footer.has_default);
    CHECK(sections.sections[0].footer.has_first);
    CHECK_FALSE(sections.sections[0].footer.has_even);
    CHECK_FALSE(sections.sections[0].footer.first_linked_to_previous);
    REQUIRE(sections.sections[0].footer.first_entry_name.has_value());
    CHECK_EQ(*sections.sections[0].footer.first_entry_name, "word/footer1.xml");
    REQUIRE(sections.sections[0].footer.resolved_first_entry_name.has_value());
    CHECK_EQ(*sections.sections[0].footer.resolved_first_entry_name, "word/footer1.xml");
    REQUIRE(sections.sections[0].footer.resolved_first_section_index.has_value());
    CHECK_EQ(*sections.sections[0].footer.resolved_first_section_index, 0U);

    CHECK_EQ(sections.sections[1].index, 1U);
    REQUIRE(sections.sections[1].even_and_odd_headers_enabled.has_value());
    CHECK(*sections.sections[1].even_and_odd_headers_enabled);
    CHECK_FALSE(sections.sections[1].different_first_page_enabled);
    CHECK(sections.sections[1].header.has_default);
    CHECK_FALSE(sections.sections[1].header.has_first);
    CHECK_FALSE(sections.sections[1].header.has_even);
    CHECK_FALSE(sections.sections[1].header.default_linked_to_previous);
    CHECK(sections.sections[1].header.even_linked_to_previous);
    REQUIRE(sections.sections[1].header.default_entry_name.has_value());
    CHECK_EQ(*sections.sections[1].header.default_entry_name, "word/header3.xml");
    REQUIRE(sections.sections[1].header.resolved_default_entry_name.has_value());
    CHECK_EQ(*sections.sections[1].header.resolved_default_entry_name, "word/header3.xml");
    REQUIRE(sections.sections[1].header.resolved_default_section_index.has_value());
    CHECK_EQ(*sections.sections[1].header.resolved_default_section_index, 1U);
    REQUIRE(sections.sections[1].header.resolved_even_entry_name.has_value());
    CHECK_EQ(*sections.sections[1].header.resolved_even_entry_name, "word/header2.xml");
    REQUIRE(sections.sections[1].header.resolved_even_section_index.has_value());
    CHECK_EQ(*sections.sections[1].header.resolved_even_section_index, 0U);
    CHECK(sections.sections[1].footer.has_default);
    CHECK_FALSE(sections.sections[1].footer.has_first);
    CHECK_FALSE(sections.sections[1].footer.has_even);
    CHECK_FALSE(sections.sections[1].footer.default_linked_to_previous);
    CHECK(sections.sections[1].footer.first_linked_to_previous);
    REQUIRE(sections.sections[1].footer.default_entry_name.has_value());
    CHECK_EQ(*sections.sections[1].footer.default_entry_name, "word/footer2.xml");
    REQUIRE(sections.sections[1].footer.resolved_default_entry_name.has_value());
    CHECK_EQ(*sections.sections[1].footer.resolved_default_entry_name, "word/footer2.xml");
    REQUIRE(sections.sections[1].footer.resolved_default_section_index.has_value());
    CHECK_EQ(*sections.sections[1].footer.resolved_default_section_index, 1U);
    REQUIRE(sections.sections[1].footer.resolved_first_entry_name.has_value());
    CHECK_EQ(*sections.sections[1].footer.resolved_first_entry_name, "word/footer1.xml");
    REQUIRE(sections.sections[1].footer.resolved_first_section_index.has_value());
    CHECK_EQ(*sections.sections[1].footer.resolved_first_section_index, 0U);

    CHECK_EQ(sections.sections[2].index, 2U);
    REQUIRE(sections.sections[2].even_and_odd_headers_enabled.has_value());
    CHECK(*sections.sections[2].even_and_odd_headers_enabled);
    CHECK_FALSE(sections.sections[2].different_first_page_enabled);
    CHECK_FALSE(sections.sections[2].header.has_default);
    CHECK_FALSE(sections.sections[2].header.has_first);
    CHECK_FALSE(sections.sections[2].header.has_even);
    CHECK(sections.sections[2].header.default_linked_to_previous);
    CHECK(sections.sections[2].header.even_linked_to_previous);
    CHECK_FALSE(sections.sections[2].footer.has_default);
    CHECK_FALSE(sections.sections[2].footer.has_first);
    CHECK_FALSE(sections.sections[2].footer.has_even);
    CHECK(sections.sections[2].footer.default_linked_to_previous);
    CHECK(sections.sections[2].footer.first_linked_to_previous);
    CHECK_FALSE(sections.sections[2].header.default_entry_name.has_value());
    CHECK_FALSE(sections.sections[2].header.first_entry_name.has_value());
    CHECK_FALSE(sections.sections[2].header.even_entry_name.has_value());
    CHECK_FALSE(sections.sections[2].footer.default_entry_name.has_value());
    CHECK_FALSE(sections.sections[2].footer.first_entry_name.has_value());
    CHECK_FALSE(sections.sections[2].footer.even_entry_name.has_value());
    REQUIRE(sections.sections[2].header.resolved_default_entry_name.has_value());
    CHECK_EQ(*sections.sections[2].header.resolved_default_entry_name, "word/header3.xml");
    REQUIRE(sections.sections[2].header.resolved_default_section_index.has_value());
    CHECK_EQ(*sections.sections[2].header.resolved_default_section_index, 1U);
    REQUIRE(sections.sections[2].header.resolved_even_entry_name.has_value());
    CHECK_EQ(*sections.sections[2].header.resolved_even_entry_name, "word/header2.xml");
    REQUIRE(sections.sections[2].header.resolved_even_section_index.has_value());
    CHECK_EQ(*sections.sections[2].header.resolved_even_section_index, 0U);
    REQUIRE(sections.sections[2].footer.resolved_default_entry_name.has_value());
    CHECK_EQ(*sections.sections[2].footer.resolved_default_entry_name, "word/footer2.xml");
    REQUIRE(sections.sections[2].footer.resolved_default_section_index.has_value());
    CHECK_EQ(*sections.sections[2].footer.resolved_default_section_index, 1U);
    REQUIRE(sections.sections[2].footer.resolved_first_entry_name.has_value());
    CHECK_EQ(*sections.sections[2].footer.resolved_first_entry_name, "word/footer1.xml");
    REQUIRE(sections.sections[2].footer.resolved_first_section_index.has_value());
    CHECK_EQ(*sections.sections[2].footer.resolved_first_section_index, 0U);

    const auto inspected_section = doc.inspect_section(1U);
    REQUIRE(inspected_section.has_value());
    CHECK_EQ(inspected_section->index, 1U);
    REQUIRE(inspected_section->even_and_odd_headers_enabled.has_value());
    CHECK(*inspected_section->even_and_odd_headers_enabled);
    CHECK_FALSE(inspected_section->different_first_page_enabled);
    CHECK(inspected_section->header.has_default);
    CHECK(inspected_section->footer.has_default);
    REQUIRE(inspected_section->header.resolved_even_entry_name.has_value());
    CHECK_EQ(*inspected_section->header.resolved_even_entry_name, "word/header2.xml");
    REQUIRE(inspected_section->footer.resolved_first_entry_name.has_value());
    CHECK_EQ(*inspected_section->footer.resolved_first_entry_name, "word/footer1.xml");
    CHECK_FALSE(doc.inspect_section(9U).has_value());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_sections = reopened.inspect_sections();
    CHECK_EQ(reopened_sections.section_count, 3U);
    CHECK_EQ(reopened_sections.header_count, 3U);
    CHECK_EQ(reopened_sections.footer_count, 2U);
    REQUIRE(reopened_sections.even_and_odd_headers_enabled.has_value());
    CHECK(*reopened_sections.even_and_odd_headers_enabled);
    REQUIRE(reopened_sections.sections.size() == 3U);
    REQUIRE(reopened_sections.sections[0].even_and_odd_headers_enabled.has_value());
    CHECK(*reopened_sections.sections[0].even_and_odd_headers_enabled);
    CHECK(reopened_sections.sections[0].different_first_page_enabled);
    REQUIRE(reopened_sections.sections[0].header.default_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[0].header.default_entry_name, "word/header1.xml");
    REQUIRE(reopened_sections.sections[0].header.even_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[0].header.even_entry_name, "word/header2.xml");
    REQUIRE(reopened_sections.sections[0].footer.first_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[0].footer.first_entry_name, "word/footer1.xml");
    REQUIRE(reopened_sections.sections[1].header.resolved_even_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[1].header.resolved_even_entry_name,
             "word/header2.xml");
    REQUIRE(reopened_sections.sections[1].footer.resolved_first_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[1].footer.resolved_first_entry_name,
             "word/footer1.xml");
    REQUIRE(reopened_sections.sections[1].header.default_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[1].header.default_entry_name, "word/header3.xml");
    REQUIRE(reopened_sections.sections[1].footer.default_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[1].footer.default_entry_name, "word/footer2.xml");
    CHECK_FALSE(reopened_sections.sections[2].header.has_default);
    CHECK_FALSE(reopened_sections.sections[2].footer.has_default);
    REQUIRE(reopened_sections.sections[2].header.resolved_default_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[2].header.resolved_default_entry_name,
             "word/header3.xml");
    REQUIRE(reopened_sections.sections[2].header.resolved_even_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[2].header.resolved_even_entry_name,
             "word/header2.xml");
    REQUIRE(reopened_sections.sections[2].footer.resolved_default_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[2].footer.resolved_default_entry_name,
             "word/footer2.xml");
    REQUIRE(reopened_sections.sections[2].footer.resolved_first_entry_name.has_value());
    CHECK_EQ(*reopened_sections.sections[2].footer.resolved_first_entry_name,
             "word/footer1.xml");

    fs::remove(target);
}

TEST_CASE("inspect sections reports even-and-odd header setting without settings part") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "inspect_sections_without_settings_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto sections = doc.inspect_sections();
    CHECK_EQ(sections.section_count, 1U);
    CHECK_EQ(sections.header_count, 0U);
    CHECK_EQ(sections.footer_count, 0U);
    REQUIRE(sections.even_and_odd_headers_enabled.has_value());
    CHECK_FALSE(*sections.even_and_odd_headers_enabled);
    REQUIRE(sections.sections.size() == 1U);
    REQUIRE(sections.sections[0].even_and_odd_headers_enabled.has_value());
    CHECK_FALSE(*sections.sections[0].even_and_odd_headers_enabled);

    const auto section = doc.inspect_section(0U);
    REQUIRE(section.has_value());
    REQUIRE(section->even_and_odd_headers_enabled.has_value());
    CHECK_FALSE(*section->even_and_odd_headers_enabled);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_sections = reopened.inspect_sections();
    CHECK_EQ(reopened_sections.section_count, 1U);
    REQUIRE(reopened_sections.even_and_odd_headers_enabled.has_value());
    CHECK_FALSE(*reopened_sections.even_and_odd_headers_enabled);
    REQUIRE(reopened_sections.sections.size() == 1U);
    REQUIRE(reopened_sections.sections[0].even_and_odd_headers_enabled.has_value());
    CHECK_FALSE(*reopened_sections.sections[0].even_and_odd_headers_enabled);

    fs::remove(target);
}
