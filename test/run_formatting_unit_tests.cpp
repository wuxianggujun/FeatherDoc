#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("run font family APIs write rFonts and clear removes empty run properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_font_family_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"\u4E2D\u6587 CJK smoke");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());

    CHECK_FALSE(run.set_font_family(""));
    CHECK_FALSE(run.set_east_asia_font_family(""));
    CHECK(run.set_font_family("Segoe UI"));
    CHECK(run.set_east_asia_font_family("Microsoft YaHei"));

    const auto font_family = run.font_family();
    REQUIRE(font_family.has_value());
    CHECK_EQ(*font_family, "Segoe UI");

    const auto east_asia_font = run.east_asia_font_family();
    REQUIRE(east_asia_font.has_value());
    CHECK_EQ(*east_asia_font, "Microsoft YaHei");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("w:rFonts"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:hAnsi=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:cs=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:eastAsia=\"Microsoft YaHei\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK_EQ(reopened_run.get_text(), run_text);

    const auto reopened_font_family = reopened_run.font_family();
    REQUIRE(reopened_font_family.has_value());
    CHECK_EQ(*reopened_font_family, "Segoe UI");

    const auto reopened_east_asia_font = reopened_run.east_asia_font_family();
    REQUIRE(reopened_east_asia_font.has_value());
    CHECK_EQ(*reopened_east_asia_font, "Microsoft YaHei");

    CHECK(reopened_run.clear_font_family());
    CHECK_FALSE(reopened.save());

    const auto cleared_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(cleared_document_xml, "<w:rFonts"), 0);
    CHECK_EQ(count_substring_occurrences(cleared_document_xml, "<w:rPr>"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());
    CHECK_FALSE(cleared_run.font_family().has_value());
    CHECK_FALSE(cleared_run.east_asia_font_family().has_value());

    fs::remove(target);
}

TEST_CASE("default run font APIs edit docDefaults and round-trip through styles.xml") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "default_run_font_family_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"\u9ED8\u8BA4\u5B57\u4F53\u5199\u5165");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_default_run_font_family(""));
    CHECK_FALSE(doc.set_default_run_east_asia_font_family(""));
    CHECK_FALSE(doc.default_run_font_family().has_value());
    CHECK_FALSE(doc.default_run_east_asia_font_family().has_value());

    CHECK(doc.set_default_run_font_family("Segoe UI"));
    CHECK(doc.set_default_run_east_asia_font_family("Microsoft YaHei"));

    const auto default_font_family = doc.default_run_font_family();
    REQUIRE(default_font_family.has_value());
    CHECK_EQ(*default_font_family, "Segoe UI");

    const auto default_east_asia_font = doc.default_run_east_asia_font_family();
    REQUIRE(default_east_asia_font.has_value());
    CHECK_EQ(*default_east_asia_font, "Microsoft YaHei");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());

    CHECK_FALSE(doc.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:docDefaults>"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:hAnsi=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:cs=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"Microsoft YaHei\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_default_font_family = reopened.default_run_font_family();
    REQUIRE(reopened_default_font_family.has_value());
    CHECK_EQ(*reopened_default_font_family, "Segoe UI");

    const auto reopened_default_east_asia_font = reopened.default_run_east_asia_font_family();
    REQUIRE(reopened_default_east_asia_font.has_value());
    CHECK_EQ(*reopened_default_east_asia_font, "Microsoft YaHei");

    CHECK_EQ(collect_document_text(reopened), run_text + "\n");
    CHECK(reopened.clear_default_run_font_family());
    CHECK_FALSE(reopened.save());

    const auto cleared_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:eastAsia=\"Microsoft YaHei\""), 0);
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:ascii=\"Segoe UI\""), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.default_run_font_family().has_value());
    CHECK_FALSE(cleared.default_run_east_asia_font_family().has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("style run font APIs edit styles.xml and preserve unrelated style markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_run_font_family_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"\u6837\u5F0F\u5B57\u4F53\u5199\u5165");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_style_run_font_family("", "Segoe UI"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_style_run_font_family("MissingStyle", "Segoe UI"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.style_run_font_family("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_style_run_font_family("Strong", "Segoe UI"));
    CHECK(doc.set_style_run_east_asia_font_family("Strong", "Microsoft YaHei"));

    const auto style_font_family = doc.style_run_font_family("Strong");
    REQUIRE(style_font_family.has_value());
    CHECK_EQ(*style_font_family, "Segoe UI");

    const auto style_east_asia_font = doc.style_run_east_asia_font_family("Strong");
    REQUIRE(style_east_asia_font.has_value());
    CHECK_EQ(*style_east_asia_font, "Microsoft YaHei");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "Strong"));

    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Strong\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"Microsoft YaHei\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), run_text + "\n");

    const auto reopened_style_font_family = reopened.style_run_font_family("Strong");
    REQUIRE(reopened_style_font_family.has_value());
    CHECK_EQ(*reopened_style_font_family, "Segoe UI");

    const auto reopened_style_east_asia_font = reopened.style_run_east_asia_font_family("Strong");
    REQUIRE(reopened_style_east_asia_font.has_value());
    CHECK_EQ(*reopened_style_east_asia_font, "Microsoft YaHei");

    CHECK(reopened.clear_style_run_font_family("Strong"));
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:eastAsia=\"Microsoft YaHei\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:ascii=\"Segoe UI\""), 0);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.style_run_font_family("Strong").has_value());
    CHECK_FALSE(cleared.style_run_east_asia_font_family("Strong").has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("run language APIs write w:lang and clear removes empty run properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_language_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"中文 language smoke");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());

    CHECK_FALSE(run.set_language(""));
    CHECK_FALSE(run.set_east_asia_language(""));
    CHECK_FALSE(run.set_bidi_language(""));
    CHECK(run.set_language("en-US"));
    CHECK(run.set_east_asia_language("zh-CN"));
    CHECK(run.set_bidi_language("ar-SA"));

    const auto language = run.language();
    REQUIRE(language.has_value());
    CHECK_EQ(*language, "en-US");

    const auto east_asia_language = run.east_asia_language();
    REQUIRE(east_asia_language.has_value());
    CHECK_EQ(*east_asia_language, "zh-CN");

    const auto bidi_language = run.bidi_language();
    REQUIRE(bidi_language.has_value());
    CHECK_EQ(*bidi_language, "ar-SA");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:lang"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:bidi=\"ar-SA\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK_EQ(reopened_run.get_text(), run_text);

    const auto reopened_language = reopened_run.language();
    REQUIRE(reopened_language.has_value());
    CHECK_EQ(*reopened_language, "en-US");

    const auto reopened_east_asia_language = reopened_run.east_asia_language();
    REQUIRE(reopened_east_asia_language.has_value());
    CHECK_EQ(*reopened_east_asia_language, "zh-CN");

    const auto reopened_bidi_language = reopened_run.bidi_language();
    REQUIRE(reopened_bidi_language.has_value());
    CHECK_EQ(*reopened_bidi_language, "ar-SA");

    CHECK(reopened_run.clear_language());
    CHECK_FALSE(reopened.save());

    const auto cleared_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(cleared_document_xml, "<w:lang"), 0);
    CHECK_EQ(count_substring_occurrences(cleared_document_xml, "<w:rPr>"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());
    CHECK_FALSE(cleared_run.language().has_value());
    CHECK_FALSE(cleared_run.east_asia_language().has_value());
    CHECK_FALSE(cleared_run.bidi_language().has_value());

    fs::remove(target);
}

TEST_CASE("default run language APIs edit docDefaults and round-trip through styles.xml") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "default_run_language_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"默认语言写入");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_default_run_language(""));
    CHECK_FALSE(doc.set_default_run_east_asia_language(""));
    CHECK_FALSE(doc.set_default_run_bidi_language(""));
    CHECK_FALSE(doc.default_run_language().has_value());
    CHECK_FALSE(doc.default_run_east_asia_language().has_value());
    CHECK_FALSE(doc.default_run_bidi_language().has_value());

    CHECK(doc.set_default_run_language("en-US"));
    CHECK(doc.set_default_run_east_asia_language("zh-CN"));
    CHECK(doc.set_default_run_bidi_language("ar-SA"));

    const auto default_language = doc.default_run_language();
    REQUIRE(default_language.has_value());
    CHECK_EQ(*default_language, "en-US");

    const auto default_east_asia_language = doc.default_run_east_asia_language();
    REQUIRE(default_east_asia_language.has_value());
    CHECK_EQ(*default_east_asia_language, "zh-CN");

    const auto default_bidi_language = doc.default_run_bidi_language();
    REQUIRE(default_bidi_language.has_value());
    CHECK_EQ(*default_bidi_language, "ar-SA");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());

    CHECK_FALSE(doc.save());
    CHECK(test_docx_entry_exists(target, "word/styles.xml"));

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:docDefaults>"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:lang"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:bidi=\"ar-SA\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_default_language = reopened.default_run_language();
    REQUIRE(reopened_default_language.has_value());
    CHECK_EQ(*reopened_default_language, "en-US");

    const auto reopened_default_east_asia_language =
        reopened.default_run_east_asia_language();
    REQUIRE(reopened_default_east_asia_language.has_value());
    CHECK_EQ(*reopened_default_east_asia_language, "zh-CN");

    const auto reopened_default_bidi_language = reopened.default_run_bidi_language();
    REQUIRE(reopened_default_bidi_language.has_value());
    CHECK_EQ(*reopened_default_bidi_language, "ar-SA");

    CHECK_EQ(collect_document_text(reopened), run_text + "\n");
    CHECK(reopened.clear_default_run_language());
    CHECK_FALSE(reopened.save());

    const auto cleared_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:eastAsia=\"zh-CN\""), 0);
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:val=\"en-US\""), 0);
    CHECK_EQ(count_substring_occurrences(cleared_styles_xml, "w:bidi=\"ar-SA\""), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.default_run_language().has_value());
    CHECK_FALSE(cleared.default_run_east_asia_language().has_value());
    CHECK_FALSE(cleared.default_run_bidi_language().has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("style run language APIs edit styles.xml and preserve unrelated style markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_run_language_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"样式语言写入");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_style_run_language("", "en-US"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_style_run_language("MissingStyle", "en-US"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.style_run_language("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_style_run_language("Strong", "en-US"));
    CHECK(doc.set_style_run_east_asia_language("Strong", "zh-CN"));
    CHECK(doc.set_style_run_bidi_language("Strong", "ar-SA"));

    const auto style_language = doc.style_run_language("Strong");
    REQUIRE(style_language.has_value());
    CHECK_EQ(*style_language, "en-US");

    const auto style_east_asia_language = doc.style_run_east_asia_language("Strong");
    REQUIRE(style_east_asia_language.has_value());
    CHECK_EQ(*style_east_asia_language, "zh-CN");

    const auto style_bidi_language = doc.style_run_bidi_language("Strong");
    REQUIRE(style_bidi_language.has_value());
    CHECK_EQ(*style_bidi_language, "ar-SA");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "Strong"));

    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Strong\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:lang"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:bidi=\"ar-SA\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), run_text + "\n");

    const auto reopened_style_language = reopened.style_run_language("Strong");
    REQUIRE(reopened_style_language.has_value());
    CHECK_EQ(*reopened_style_language, "en-US");

    const auto reopened_style_east_asia_language =
        reopened.style_run_east_asia_language("Strong");
    REQUIRE(reopened_style_east_asia_language.has_value());
    CHECK_EQ(*reopened_style_east_asia_language, "zh-CN");

    const auto reopened_style_bidi_language = reopened.style_run_bidi_language("Strong");
    REQUIRE(reopened_style_bidi_language.has_value());
    CHECK_EQ(*reopened_style_bidi_language, "ar-SA");

    CHECK(reopened.clear_style_run_language("Strong"));
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:eastAsia=\"zh-CN\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:val=\"en-US\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:bidi=\"ar-SA\""), 0);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.style_run_language("Strong").has_value());
    CHECK_FALSE(cleared.style_run_east_asia_language("Strong").has_value());
    CHECK_FALSE(cleared.style_run_bidi_language("Strong").has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE(
    "granular run metadata clear APIs preserve unrelated font and language attributes") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "run_partial_metadata_clear_roundtrip.docx";
    const std::string run_text = "mixed rtl smoke";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());

    CHECK(run.set_font_family("Segoe UI"));
    CHECK(run.set_east_asia_font_family("Microsoft YaHei"));
    CHECK(run.set_language("en-US"));
    CHECK(run.set_east_asia_language("zh-CN"));
    CHECK(run.set_bidi_language("ar-SA"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK(reopened_run.clear_east_asia_font_family());
    CHECK(reopened_run.clear_east_asia_language());
    CHECK(reopened_run.clear_bidi_language());
    CHECK_FALSE(reopened.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:hAnsi=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:cs=\"Segoe UI\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "w:eastAsia=\"Microsoft YaHei\""),
             0);
    CHECK_NE(saved_document_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:eastAsia=\"zh-CN\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:bidi=\"ar-SA\""), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());

    const auto font_family = cleared_run.font_family();
    REQUIRE(font_family.has_value());
    CHECK_EQ(*font_family, "Segoe UI");
    CHECK_FALSE(cleared_run.east_asia_font_family().has_value());

    const auto language = cleared_run.language();
    REQUIRE(language.has_value());
    CHECK_EQ(*language, "en-US");
    CHECK_FALSE(cleared_run.east_asia_language().has_value());
    CHECK_FALSE(cleared_run.bidi_language().has_value());
    CHECK_EQ(cleared_run.get_text(), run_text);

    fs::remove(target);
}

TEST_CASE(
    "granular default run metadata clear APIs preserve unrelated docDefaults attributes") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "default_run_partial_metadata_clear_roundtrip.docx";
    const std::string run_text = "default metadata write";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.set_default_run_font_family("Segoe UI"));
    CHECK(doc.set_default_run_east_asia_font_family("Microsoft YaHei"));
    CHECK(doc.set_default_run_language("en-US"));
    CHECK(doc.set_default_run_east_asia_language("zh-CN"));
    CHECK(doc.set_default_run_bidi_language("ar-SA"));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.clear_default_run_east_asia_font_family());
    CHECK(reopened.clear_default_run_east_asia_language());
    CHECK(reopened.clear_default_run_bidi_language());
    CHECK_FALSE(reopened.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:hAnsi=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:cs=\"Segoe UI\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml,
                                         "w:eastAsia=\"Microsoft YaHei\""),
             0);
    CHECK_NE(saved_styles_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:eastAsia=\"zh-CN\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:bidi=\"ar-SA\""), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());

    const auto default_font_family = cleared.default_run_font_family();
    REQUIRE(default_font_family.has_value());
    CHECK_EQ(*default_font_family, "Segoe UI");
    CHECK_FALSE(cleared.default_run_east_asia_font_family().has_value());

    const auto default_language = cleared.default_run_language();
    REQUIRE(default_language.has_value());
    CHECK_EQ(*default_language, "en-US");
    CHECK_FALSE(cleared.default_run_east_asia_language().has_value());
    CHECK_FALSE(cleared.default_run_bidi_language().has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE(
    "granular style run metadata clear APIs preserve unrelated style markup") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "style_run_partial_metadata_clear_roundtrip.docx";
    const std::string run_text = "style partial clear";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.set_style_run_font_family("Strong", "Segoe UI"));
    CHECK(doc.set_style_run_east_asia_font_family("Strong", "Microsoft YaHei"));
    CHECK(doc.set_style_run_language("Strong", "en-US"));
    CHECK(doc.set_style_run_east_asia_language("Strong", "zh-CN"));
    CHECK(doc.set_style_run_bidi_language("Strong", "ar-SA"));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "Strong"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.clear_style_run_east_asia_font_family("Strong"));
    CHECK(reopened.clear_style_run_east_asia_language("Strong"));
    CHECK(reopened.clear_style_run_bidi_language("Strong"));
    CHECK_FALSE(reopened.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:ascii=\"Segoe UI\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:val=\"en-US\""), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml,
                                         "w:eastAsia=\"Microsoft YaHei\""),
             0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:eastAsia=\"zh-CN\""), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:bidi=\"ar-SA\""), 0);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());

    const auto style_font_family = cleared.style_run_font_family("Strong");
    REQUIRE(style_font_family.has_value());
    CHECK_EQ(*style_font_family, "Segoe UI");
    CHECK_FALSE(cleared.style_run_east_asia_font_family("Strong").has_value());

    const auto style_language = cleared.style_run_language("Strong");
    REQUIRE(style_language.has_value());
    CHECK_EQ(*style_language, "en-US");
    CHECK_FALSE(cleared.style_run_east_asia_language("Strong").has_value());
    CHECK_FALSE(cleared.style_run_bidi_language("Strong").has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE(
    "primary run language clear preserves eastAsia and bidi language attributes") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "run_primary_language_clear_roundtrip.docx";
    const std::string run_text = "primary language clear";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());

    CHECK(run.set_language("en-US"));
    CHECK(run.set_east_asia_language("zh-CN"));
    CHECK(run.set_bidi_language("ar-SA"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK(reopened_run.clear_primary_language());
    CHECK_FALSE(reopened.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:val=\"en-US\""), 0);
    CHECK_NE(saved_document_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:bidi=\"ar-SA\""), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());

    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());
    CHECK_FALSE(cleared_run.language().has_value());

    const auto east_asia_language = cleared_run.east_asia_language();
    REQUIRE(east_asia_language.has_value());
    CHECK_EQ(*east_asia_language, "zh-CN");

    const auto bidi_language = cleared_run.bidi_language();
    REQUIRE(bidi_language.has_value());
    CHECK_EQ(*bidi_language, "ar-SA");
    CHECK_EQ(cleared_run.get_text(), run_text);

    fs::remove(target);
}

TEST_CASE(
    "primary default run language clear preserves eastAsia and bidi docDefaults attributes") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "default_run_primary_language_clear_roundtrip.docx";
    const std::string run_text = "default primary language clear";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.set_default_run_language("en-US"));
    CHECK(doc.set_default_run_east_asia_language("zh-CN"));
    CHECK(doc.set_default_run_bidi_language("ar-SA"));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.clear_default_run_primary_language());
    CHECK_FALSE(reopened.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:val=\"en-US\""), 0);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:bidi=\"ar-SA\""), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.default_run_language().has_value());

    const auto default_east_asia_language = cleared.default_run_east_asia_language();
    REQUIRE(default_east_asia_language.has_value());
    CHECK_EQ(*default_east_asia_language, "zh-CN");

    const auto default_bidi_language = cleared.default_run_bidi_language();
    REQUIRE(default_bidi_language.has_value());
    CHECK_EQ(*default_bidi_language, "ar-SA");
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE(
    "primary style run language clear preserves eastAsia and bidi style markup") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "style_run_primary_language_clear_roundtrip.docx";
    const std::string run_text = "style primary language clear";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.clear_style_run_primary_language(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.clear_style_run_primary_language("MissingStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_style_run_language("Strong", "en-US"));
    CHECK(doc.set_style_run_east_asia_language("Strong", "zh-CN"));
    CHECK(doc.set_style_run_bidi_language("Strong", "ar-SA"));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());
    CHECK(doc.set_run_style(run, "Strong"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.clear_style_run_primary_language("Strong"));
    CHECK_FALSE(reopened.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "w:val=\"en-US\""), 0);
    CHECK_NE(saved_styles_xml.find("w:eastAsia=\"zh-CN\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:bidi=\"ar-SA\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:b"), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.style_run_language("Strong").has_value());

    const auto style_east_asia_language =
        cleared.style_run_east_asia_language("Strong");
    REQUIRE(style_east_asia_language.has_value());
    CHECK_EQ(*style_east_asia_language, "zh-CN");

    const auto style_bidi_language = cleared.style_run_bidi_language("Strong");
    REQUIRE(style_bidi_language.has_value());
    CHECK_EQ(*style_bidi_language, "ar-SA");
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}
