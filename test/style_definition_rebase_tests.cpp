#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("rebase_paragraph_style_based_on preserves resolved inherited values while switching parent") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "rebase_paragraph_style_based_on_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto base_a = featherdoc::paragraph_style_definition{};
    base_a.name = "Base A";
    base_a.based_on = std::string{"Normal"};
    base_a.run_font_family = std::string{"Segoe UI"};
    base_a.run_east_asia_language = std::string{"zh-CN"};
    base_a.run_rtl = true;
    CHECK(doc.ensure_paragraph_style("BaseA", base_a));

    auto base_b = featherdoc::paragraph_style_definition{};
    base_b.name = "Base B";
    base_b.based_on = std::string{"Normal"};
    base_b.run_font_family = std::string{"Arial"};
    base_b.run_east_asia_language = std::string{"ja-JP"};
    base_b.run_rtl = false;
    CHECK(doc.ensure_paragraph_style("BaseB", base_b));

    auto child_definition = featherdoc::paragraph_style_definition{};
    child_definition.name = "Child Style";
    child_definition.based_on = std::string{"BaseA"};
    child_definition.run_language = std::string{"en-US"};
    CHECK(doc.ensure_paragraph_style("ChildStyle", child_definition));

    const auto before = doc.resolve_style_properties("ChildStyle");
    REQUIRE(before.has_value());
    REQUIRE(before->run_font_family.value.has_value());
    CHECK_EQ(*before->run_font_family.value, "Segoe UI");
    REQUIRE(before->run_font_family.source_style_id.has_value());
    CHECK_EQ(*before->run_font_family.source_style_id, "BaseA");

    CHECK(doc.rebase_paragraph_style_based_on("ChildStyle", "BaseB"));

    const auto summary = doc.find_style("ChildStyle");
    REQUIRE(summary.has_value());
    REQUIRE(summary->based_on.has_value());
    CHECK_EQ(*summary->based_on, "BaseB");

    const auto after = doc.resolve_style_properties("ChildStyle");
    REQUIRE(after.has_value());
    REQUIRE(after->based_on.has_value());
    CHECK_EQ(*after->based_on, "BaseB");
    REQUIRE(after->run_font_family.value.has_value());
    REQUIRE(after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*after->run_font_family.value, "Segoe UI");
    CHECK_EQ(*after->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(after->run_east_asia_language.value.has_value());
    REQUIRE(after->run_east_asia_language.source_style_id.has_value());
    CHECK_EQ(*after->run_east_asia_language.value, "zh-CN");
    CHECK_EQ(*after->run_east_asia_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_language.value.has_value());
    REQUIRE(after->run_language.source_style_id.has_value());
    CHECK_EQ(*after->run_language.value, "en-US");
    CHECK_EQ(*after->run_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_rtl.value.has_value());
    REQUIRE(after->run_rtl.source_style_id.has_value());
    CHECK(*after->run_rtl.value);
    CHECK_EQ(*after->run_rtl.source_style_id, "ChildStyle");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_summary = reopened.find_style("ChildStyle");
    REQUIRE(reopened_summary.has_value());
    REQUIRE(reopened_summary->based_on.has_value());
    CHECK_EQ(*reopened_summary->based_on, "BaseB");
    const auto reopened_after = reopened.resolve_style_properties("ChildStyle");
    REQUIRE(reopened_after.has_value());
    REQUIRE(reopened_after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*reopened_after->run_font_family.source_style_id, "ChildStyle");

    fs::remove(target);
}

TEST_CASE("rebase_character_style_based_on preserves resolved inherited values while switching parent") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "rebase_character_style_based_on_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto base_a = featherdoc::character_style_definition{};
    base_a.name = "Base A";
    base_a.based_on = std::string{"DefaultParagraphFont"};
    base_a.run_font_family = std::string{"Segoe UI"};
    base_a.run_east_asia_language = std::string{"zh-CN"};
    base_a.run_strikethrough = true;
    base_a.run_superscript = true;
    base_a.run_rtl = true;
    CHECK(doc.ensure_character_style("BaseA", base_a));

    auto base_b = featherdoc::character_style_definition{};
    base_b.name = "Base B";
    base_b.based_on = std::string{"DefaultParagraphFont"};
    base_b.run_font_family = std::string{"Arial"};
    base_b.run_east_asia_language = std::string{"ja-JP"};
    base_b.run_strikethrough = false;
    base_b.run_subscript = true;
    base_b.run_rtl = false;
    CHECK(doc.ensure_character_style("BaseB", base_b));

    auto child_definition = featherdoc::character_style_definition{};
    child_definition.name = "Child Style";
    child_definition.based_on = std::string{"BaseA"};
    child_definition.run_language = std::string{"en-US"};
    CHECK(doc.ensure_character_style("ChildStyle", child_definition));

    const auto before = doc.resolve_style_properties("ChildStyle");
    REQUIRE(before.has_value());
    REQUIRE(before->run_font_family.value.has_value());
    CHECK_EQ(*before->run_font_family.value, "Segoe UI");
    REQUIRE(before->run_font_family.source_style_id.has_value());
    CHECK_EQ(*before->run_font_family.source_style_id, "BaseA");
    REQUIRE(before->run_strikethrough.value.has_value());
    CHECK(*before->run_strikethrough.value);
    REQUIRE(before->run_strikethrough.source_style_id.has_value());
    CHECK_EQ(*before->run_strikethrough.source_style_id, "BaseA");
    REQUIRE(before->run_superscript.value.has_value());
    CHECK(*before->run_superscript.value);
    REQUIRE(before->run_superscript.source_style_id.has_value());
    CHECK_EQ(*before->run_superscript.source_style_id, "BaseA");

    CHECK(doc.rebase_character_style_based_on("ChildStyle", "BaseB"));

    const auto summary = doc.find_style("ChildStyle");
    REQUIRE(summary.has_value());
    REQUIRE(summary->based_on.has_value());
    CHECK_EQ(*summary->based_on, "BaseB");

    const auto after = doc.resolve_style_properties("ChildStyle");
    REQUIRE(after.has_value());
    REQUIRE(after->based_on.has_value());
    CHECK_EQ(*after->based_on, "BaseB");
    REQUIRE(after->run_font_family.value.has_value());
    REQUIRE(after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*after->run_font_family.value, "Segoe UI");
    CHECK_EQ(*after->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(after->run_east_asia_language.value.has_value());
    REQUIRE(after->run_east_asia_language.source_style_id.has_value());
    CHECK_EQ(*after->run_east_asia_language.value, "zh-CN");
    CHECK_EQ(*after->run_east_asia_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_language.value.has_value());
    REQUIRE(after->run_language.source_style_id.has_value());
    CHECK_EQ(*after->run_language.value, "en-US");
    CHECK_EQ(*after->run_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_strikethrough.value.has_value());
    REQUIRE(after->run_strikethrough.source_style_id.has_value());
    CHECK(*after->run_strikethrough.value);
    CHECK_EQ(*after->run_strikethrough.source_style_id, "ChildStyle");
    REQUIRE(after->run_superscript.value.has_value());
    REQUIRE(after->run_superscript.source_style_id.has_value());
    CHECK(*after->run_superscript.value);
    CHECK_EQ(*after->run_superscript.source_style_id, "ChildStyle");
    REQUIRE(after->run_subscript.value.has_value());
    CHECK_FALSE(*after->run_subscript.value);
    REQUIRE(after->run_rtl.value.has_value());
    REQUIRE(after->run_rtl.source_style_id.has_value());
    CHECK(*after->run_rtl.value);
    CHECK_EQ(*after->run_rtl.source_style_id, "ChildStyle");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_summary = reopened.find_style("ChildStyle");
    REQUIRE(reopened_summary.has_value());
    REQUIRE(reopened_summary->based_on.has_value());
    CHECK_EQ(*reopened_summary->based_on, "BaseB");
    const auto reopened_after = reopened.resolve_style_properties("ChildStyle");
    REQUIRE(reopened_after.has_value());
    REQUIRE(reopened_after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*reopened_after->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(reopened_after->run_strikethrough.source_style_id.has_value());
    CHECK_EQ(*reopened_after->run_strikethrough.source_style_id, "ChildStyle");
    REQUIRE(reopened_after->run_superscript.source_style_id.has_value());
    CHECK_EQ(*reopened_after->run_superscript.source_style_id, "ChildStyle");

    fs::remove(target);
}

TEST_CASE("paragraph style property mutators update metadata without clearing direct run properties") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "paragraph_style_property_mutators_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto definition = featherdoc::paragraph_style_definition{};
    definition.name = "Working Style";
    definition.based_on = std::string{"Normal"};
    definition.next_style = std::string{"WorkingStyle"};
    definition.run_font_family = std::string{"Consolas"};
    definition.run_strikethrough = true;
    CHECK(doc.ensure_paragraph_style("WorkingStyle", definition));

    REQUIRE(doc.paragraph_style_next_style("WorkingStyle").has_value());
    CHECK_EQ(*doc.paragraph_style_next_style("WorkingStyle"), "WorkingStyle");
    CHECK_FALSE(doc.paragraph_style_outline_level("WorkingStyle").has_value());
    REQUIRE(doc.style_run_font_family("WorkingStyle").has_value());
    CHECK_EQ(*doc.style_run_font_family("WorkingStyle"), "Consolas");
    REQUIRE(doc.style_run_strikethrough("WorkingStyle").has_value());
    CHECK(*doc.style_run_strikethrough("WorkingStyle"));

    CHECK(doc.set_paragraph_style_based_on("WorkingStyle", "Heading1"));
    CHECK(doc.set_paragraph_style_next_style("WorkingStyle", "BodyText"));
    CHECK(doc.set_paragraph_style_outline_level("WorkingStyle", 2U));

    const auto summary = doc.find_style("WorkingStyle");
    REQUIRE(summary.has_value());
    REQUIRE(summary->based_on.has_value());
    CHECK_EQ(*summary->based_on, "Heading1");
    REQUIRE(doc.paragraph_style_next_style("WorkingStyle").has_value());
    CHECK_EQ(*doc.paragraph_style_next_style("WorkingStyle"), "BodyText");
    REQUIRE(doc.paragraph_style_outline_level("WorkingStyle").has_value());
    CHECK_EQ(*doc.paragraph_style_outline_level("WorkingStyle"), 2U);
    REQUIRE(doc.style_run_font_family("WorkingStyle").has_value());
    CHECK_EQ(*doc.style_run_font_family("WorkingStyle"), "Consolas");
    REQUIRE(doc.style_run_strikethrough("WorkingStyle").has_value());
    CHECK(*doc.style_run_strikethrough("WorkingStyle"));

    CHECK(doc.clear_paragraph_style_based_on("WorkingStyle"));
    CHECK(doc.clear_paragraph_style_next_style("WorkingStyle"));
    CHECK(doc.clear_paragraph_style_outline_level("WorkingStyle"));

    const auto cleared_summary = doc.find_style("WorkingStyle");
    REQUIRE(cleared_summary.has_value());
    CHECK_FALSE(cleared_summary->based_on.has_value());
    CHECK_FALSE(doc.paragraph_style_next_style("WorkingStyle").has_value());
    CHECK_FALSE(doc.paragraph_style_outline_level("WorkingStyle").has_value());
    REQUIRE(doc.style_run_font_family("WorkingStyle").has_value());
    CHECK_EQ(*doc.style_run_font_family("WorkingStyle"), "Consolas");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_summary = reopened.find_style("WorkingStyle");
    REQUIRE(reopened_summary.has_value());
    CHECK_FALSE(reopened_summary->based_on.has_value());
    CHECK_FALSE(reopened.paragraph_style_next_style("WorkingStyle").has_value());
    CHECK_FALSE(reopened.paragraph_style_outline_level("WorkingStyle").has_value());
    REQUIRE(reopened.style_run_font_family("WorkingStyle").has_value());
    CHECK_EQ(*reopened.style_run_font_family("WorkingStyle"), "Consolas");
    REQUIRE(reopened.style_run_strikethrough("WorkingStyle").has_value());
    CHECK(*reopened.style_run_strikethrough("WorkingStyle"));

    fs::remove(target);
}
