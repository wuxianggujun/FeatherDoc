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

TEST_CASE("ensure style definition APIs validate definitions and reject type mismatches") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "ensure_style_definitions_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_definition = featherdoc::paragraph_style_definition{};
    paragraph_definition.name = "Body";
    CHECK_FALSE(doc.ensure_paragraph_style("", paragraph_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    paragraph_definition.name.clear();
    CHECK_FALSE(doc.ensure_paragraph_style("BodyText", paragraph_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    paragraph_definition.name = "Body";
    paragraph_definition.run_font_family = std::string{};
    CHECK_FALSE(doc.ensure_paragraph_style("BodyText", paragraph_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    paragraph_definition.run_font_family = std::string{"Segoe UI"};
    paragraph_definition.outline_level = 9U;
    CHECK_FALSE(doc.ensure_paragraph_style("BodyText", paragraph_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto table_definition = featherdoc::table_style_definition{};
    table_definition.name = "Wrong Type";
    CHECK_FALSE(doc.ensure_table_style("Heading1", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.find_table_style_definition("Heading1").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad Fill";
    table_definition.whole_table = featherdoc::table_style_region_definition{};
    table_definition.whole_table->fill_color = std::string{};
    CHECK_FALSE(doc.ensure_table_style("BadFillTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad Text Color";
    table_definition.whole_table->fill_color = std::nullopt;
    table_definition.whole_table->text_color = std::string{};
    CHECK_FALSE(doc.ensure_table_style("BadTextColorTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad Font Size";
    table_definition.whole_table->text_color = std::nullopt;
    table_definition.whole_table->font_size_points = 0U;
    CHECK_FALSE(doc.ensure_table_style("BadFontSizeTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad Font Family";
    table_definition.whole_table->font_size_points = std::nullopt;
    table_definition.whole_table->font_family = std::string{};
    CHECK_FALSE(doc.ensure_table_style("BadFontFamilyTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    table_definition.name = "Table With Bad East Asia Font Family";
    table_definition.whole_table->font_family = std::nullopt;
    table_definition.whole_table->east_asia_font_family = std::string{};
    CHECK_FALSE(doc.ensure_table_style("BadEastAsiaFontFamilyTable", table_definition));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("resolve_style_properties follows basedOn chains and reports value sources") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "resolve_style_properties_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.set_style_run_bidi_language("Normal", "ar-SA"));

    auto base_definition = featherdoc::paragraph_style_definition{};
    base_definition.name = "Base Style";
    base_definition.based_on = std::string{"Normal"};
    base_definition.run_font_family = std::string{"Segoe UI"};
    base_definition.run_east_asia_language = std::string{"zh-CN"};
    base_definition.run_rtl = true;
    CHECK(doc.ensure_paragraph_style("BaseStyle", base_definition));

    auto child_definition = featherdoc::paragraph_style_definition{};
    child_definition.name = "Child Style";
    child_definition.based_on = std::string{"BaseStyle"};
    child_definition.run_language = std::string{"en-US"};
    child_definition.paragraph_bidi = true;
    CHECK(doc.ensure_paragraph_style("ChildStyle", child_definition));

    const auto resolved = doc.resolve_style_properties("ChildStyle");
    REQUIRE(resolved.has_value());
    CHECK_EQ(resolved->style_id, "ChildStyle");
    CHECK_EQ(resolved->type_name, "paragraph");
    CHECK_EQ(resolved->kind, featherdoc::style_kind::paragraph);
    REQUIRE(resolved->based_on.has_value());
    CHECK_EQ(*resolved->based_on, "BaseStyle");
    CHECK_EQ(resolved->inheritance_chain.size(), 3U);
    CHECK_EQ(resolved->inheritance_chain[0], "ChildStyle");
    CHECK_EQ(resolved->inheritance_chain[1], "BaseStyle");
    CHECK_EQ(resolved->inheritance_chain[2], "Normal");

    REQUIRE(resolved->run_font_family.value.has_value());
    REQUIRE(resolved->run_font_family.source_style_id.has_value());
    CHECK_EQ(*resolved->run_font_family.value, "Segoe UI");
    CHECK_EQ(*resolved->run_font_family.source_style_id, "BaseStyle");

    CHECK_FALSE(resolved->run_east_asia_font_family.value.has_value());
    CHECK_FALSE(resolved->run_east_asia_font_family.source_style_id.has_value());

    REQUIRE(resolved->run_language.value.has_value());
    REQUIRE(resolved->run_language.source_style_id.has_value());
    CHECK_EQ(*resolved->run_language.value, "en-US");
    CHECK_EQ(*resolved->run_language.source_style_id, "ChildStyle");

    REQUIRE(resolved->run_east_asia_language.value.has_value());
    REQUIRE(resolved->run_east_asia_language.source_style_id.has_value());
    CHECK_EQ(*resolved->run_east_asia_language.value, "zh-CN");
    CHECK_EQ(*resolved->run_east_asia_language.source_style_id, "BaseStyle");

    REQUIRE(resolved->run_bidi_language.value.has_value());
    REQUIRE(resolved->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*resolved->run_bidi_language.value, "ar-SA");
    CHECK_EQ(*resolved->run_bidi_language.source_style_id, "Normal");

    REQUIRE(resolved->run_rtl.value.has_value());
    REQUIRE(resolved->run_rtl.source_style_id.has_value());
    CHECK(*resolved->run_rtl.value);
    CHECK_EQ(*resolved->run_rtl.source_style_id, "BaseStyle");

    REQUIRE(resolved->paragraph_bidi.value.has_value());
    REQUIRE(resolved->paragraph_bidi.source_style_id.has_value());
    CHECK(*resolved->paragraph_bidi.value);
    CHECK_EQ(*resolved->paragraph_bidi.source_style_id, "ChildStyle");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_resolved = reopened.resolve_style_properties("ChildStyle");
    REQUIRE(reopened_resolved.has_value());
    REQUIRE(reopened_resolved->run_bidi_language.value.has_value());
    REQUIRE(reopened_resolved->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*reopened_resolved->run_bidi_language.value, "ar-SA");
    CHECK_EQ(*reopened_resolved->run_bidi_language.source_style_id, "Normal");

    fs::remove(target);
}

TEST_CASE("materialize_style_run_properties freezes inherited style metadata on the child style") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "materialize_style_run_properties_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.set_style_run_bidi_language("Normal", "ar-SA"));

    auto base_definition = featherdoc::paragraph_style_definition{};
    base_definition.name = "Base Style";
    base_definition.based_on = std::string{"Normal"};
    base_definition.run_font_family = std::string{"Segoe UI"};
    base_definition.run_east_asia_language = std::string{"zh-CN"};
    base_definition.run_rtl = true;
    base_definition.paragraph_bidi = true;
    CHECK(doc.ensure_paragraph_style("BaseStyle", base_definition));

    auto child_definition = featherdoc::paragraph_style_definition{};
    child_definition.name = "Child Style";
    child_definition.based_on = std::string{"BaseStyle"};
    CHECK(doc.ensure_paragraph_style("ChildStyle", child_definition));

    const auto before = doc.resolve_style_properties("ChildStyle");
    REQUIRE(before.has_value());
    REQUIRE(before->run_font_family.source_style_id.has_value());
    CHECK_EQ(*before->run_font_family.source_style_id, "BaseStyle");
    REQUIRE(before->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*before->run_bidi_language.source_style_id, "Normal");

    CHECK(doc.materialize_style_run_properties("ChildStyle"));

    REQUIRE(doc.style_run_font_family("ChildStyle").has_value());
    CHECK_EQ(*doc.style_run_font_family("ChildStyle"), "Segoe UI");
    REQUIRE(doc.style_run_east_asia_language("ChildStyle").has_value());
    CHECK_EQ(*doc.style_run_east_asia_language("ChildStyle"), "zh-CN");
    REQUIRE(doc.style_run_bidi_language("ChildStyle").has_value());
    CHECK_EQ(*doc.style_run_bidi_language("ChildStyle"), "ar-SA");
    REQUIRE(doc.style_run_rtl("ChildStyle").has_value());
    CHECK(*doc.style_run_rtl("ChildStyle"));
    REQUIRE(doc.style_paragraph_bidi("ChildStyle").has_value());
    CHECK(*doc.style_paragraph_bidi("ChildStyle"));

    CHECK(doc.set_style_run_font_family("BaseStyle", "Arial"));
    CHECK(doc.set_style_run_east_asia_language("BaseStyle", "ja-JP"));
    CHECK(doc.set_style_run_rtl("BaseStyle", false));
    CHECK(doc.set_style_paragraph_bidi("BaseStyle", false));
    CHECK(doc.set_style_run_bidi_language("Normal", "he-IL"));

    const auto after = doc.resolve_style_properties("ChildStyle");
    REQUIRE(after.has_value());
    REQUIRE(after->run_font_family.value.has_value());
    REQUIRE(after->run_font_family.source_style_id.has_value());
    CHECK_EQ(*after->run_font_family.value, "Segoe UI");
    CHECK_EQ(*after->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(after->run_east_asia_language.value.has_value());
    REQUIRE(after->run_east_asia_language.source_style_id.has_value());
    CHECK_EQ(*after->run_east_asia_language.value, "zh-CN");
    CHECK_EQ(*after->run_east_asia_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_bidi_language.value.has_value());
    REQUIRE(after->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*after->run_bidi_language.value, "ar-SA");
    CHECK_EQ(*after->run_bidi_language.source_style_id, "ChildStyle");
    REQUIRE(after->run_rtl.value.has_value());
    REQUIRE(after->run_rtl.source_style_id.has_value());
    CHECK(*after->run_rtl.value);
    CHECK_EQ(*after->run_rtl.source_style_id, "ChildStyle");
    REQUIRE(after->paragraph_bidi.value.has_value());
    REQUIRE(after->paragraph_bidi.source_style_id.has_value());
    CHECK(*after->paragraph_bidi.value);
    CHECK_EQ(*after->paragraph_bidi.source_style_id, "ChildStyle");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_resolved = reopened.resolve_style_properties("ChildStyle");
    REQUIRE(reopened_resolved.has_value());
    REQUIRE(reopened_resolved->run_font_family.source_style_id.has_value());
    CHECK_EQ(*reopened_resolved->run_font_family.source_style_id, "ChildStyle");
    REQUIRE(reopened_resolved->run_bidi_language.source_style_id.has_value());
    CHECK_EQ(*reopened_resolved->run_bidi_language.source_style_id, "ChildStyle");

    fs::remove(target);
}
