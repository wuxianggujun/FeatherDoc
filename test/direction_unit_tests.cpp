#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("run rtl clear preserves unrelated run metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_rtl_preserve_metadata_roundtrip.docx";
    const std::string run_text = "rtl mixed metadata preserve";
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
    CHECK(run.set_rtl());
    CHECK_FALSE(doc.save());

    auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document document_xml;
    REQUIRE(document_xml.load_string(saved_document_xml.c_str()));
    auto run_properties = document_xml.child("w:document")
                              .child("w:body")
                              .child("w:p")
                              .child("w:r")
                              .child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    REQUIRE(run_properties.child("w:rFonts") != pugi::xml_node{});
    REQUIRE(run_properties.child("w:lang") != pugi::xml_node{});
    REQUIRE(run_properties.child("w:rtl") != pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK(reopened_run.clear_rtl());
    CHECK_FALSE(reopened.save());

    saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document cleared_document_xml;
    REQUIRE(cleared_document_xml.load_string(saved_document_xml.c_str()));
    run_properties = cleared_document_xml.child("w:document")
                         .child("w:body")
                         .child("w:p")
                         .child("w:r")
                         .child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});

    const auto run_fonts = run_properties.child("w:rFonts");
    REQUIRE(run_fonts != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_fonts.attribute("w:ascii").value()}, "Segoe UI");
    CHECK_EQ(std::string_view{run_fonts.attribute("w:eastAsia").value()},
             "Microsoft YaHei");

    const auto run_language = run_properties.child("w:lang");
    REQUIRE(run_language != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_language.attribute("w:val").value()}, "en-US");
    CHECK_EQ(std::string_view{run_language.attribute("w:eastAsia").value()}, "zh-CN");
    CHECK_EQ(std::string_view{run_language.attribute("w:bidi").value()}, "ar-SA");
    CHECK(run_properties.child("w:rtl") == pugi::xml_node{});

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());

    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());
    CHECK_FALSE(cleared_run.rtl().has_value());

    const auto font_family = cleared_run.font_family();
    REQUIRE(font_family.has_value());
    CHECK_EQ(*font_family, "Segoe UI");

    const auto east_asia_font_family = cleared_run.east_asia_font_family();
    REQUIRE(east_asia_font_family.has_value());
    CHECK_EQ(*east_asia_font_family, "Microsoft YaHei");

    const auto language = cleared_run.language();
    REQUIRE(language.has_value());
    CHECK_EQ(*language, "en-US");

    const auto east_asia_language = cleared_run.east_asia_language();
    REQUIRE(east_asia_language.has_value());
    CHECK_EQ(*east_asia_language, "zh-CN");

    const auto bidi_language = cleared_run.bidi_language();
    REQUIRE(bidi_language.has_value());
    CHECK_EQ(*bidi_language, "ar-SA");
    CHECK_EQ(cleared_run.get_text(), run_text);

    fs::remove(target);
}

TEST_CASE("default run rtl clear preserves unrelated docDefaults metadata") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "default_run_rtl_preserve_metadata_roundtrip.docx";
    const std::string run_text = "default rtl metadata preserve";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.set_default_run_font_family("Segoe UI"));
    CHECK(doc.set_default_run_east_asia_font_family("Microsoft YaHei"));
    CHECK(doc.set_default_run_language("en-US"));
    CHECK(doc.set_default_run_east_asia_language("zh-CN"));
    CHECK(doc.set_default_run_bidi_language("ar-SA"));
    CHECK(doc.set_default_run_rtl());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());
    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    auto run_properties = styles_document.child("w:styles")
                              .child("w:docDefaults")
                              .child("w:rPrDefault")
                              .child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    REQUIRE(run_properties.child("w:rFonts") != pugi::xml_node{});
    REQUIRE(run_properties.child("w:lang") != pugi::xml_node{});
    REQUIRE(run_properties.child("w:rtl") != pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.clear_default_run_rtl());
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document cleared_styles_document;
    REQUIRE(cleared_styles_document.load_string(saved_styles_xml.c_str()));
    run_properties = cleared_styles_document.child("w:styles")
                         .child("w:docDefaults")
                         .child("w:rPrDefault")
                         .child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});

    const auto run_fonts = run_properties.child("w:rFonts");
    REQUIRE(run_fonts != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_fonts.attribute("w:ascii").value()}, "Segoe UI");
    CHECK_EQ(std::string_view{run_fonts.attribute("w:eastAsia").value()},
             "Microsoft YaHei");

    const auto run_language = run_properties.child("w:lang");
    REQUIRE(run_language != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_language.attribute("w:val").value()}, "en-US");
    CHECK_EQ(std::string_view{run_language.attribute("w:eastAsia").value()}, "zh-CN");
    CHECK_EQ(std::string_view{run_language.attribute("w:bidi").value()}, "ar-SA");
    CHECK(run_properties.child("w:rtl") == pugi::xml_node{});

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.default_run_rtl().has_value());

    const auto default_font_family = cleared.default_run_font_family();
    REQUIRE(default_font_family.has_value());
    CHECK_EQ(*default_font_family, "Segoe UI");

    const auto default_east_asia_font_family = cleared.default_run_east_asia_font_family();
    REQUIRE(default_east_asia_font_family.has_value());
    CHECK_EQ(*default_east_asia_font_family, "Microsoft YaHei");

    const auto default_language = cleared.default_run_language();
    REQUIRE(default_language.has_value());
    CHECK_EQ(*default_language, "en-US");

    const auto default_east_asia_language = cleared.default_run_east_asia_language();
    REQUIRE(default_east_asia_language.has_value());
    CHECK_EQ(*default_east_asia_language, "zh-CN");

    const auto default_bidi_language = cleared.default_run_bidi_language();
    REQUIRE(default_bidi_language.has_value());
    CHECK_EQ(*default_bidi_language, "ar-SA");
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("paragraph bidi clear preserves unrelated paragraph properties") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "paragraph_bidi_preserve_properties_roundtrip.docx";
    const std::string paragraph_text = "paragraph bidi keeps style";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "Heading1"));
    CHECK(paragraph.add_run(paragraph_text).has_next());
    CHECK(paragraph.set_bidi(false));
    CHECK_FALSE(doc.save());

    auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document document_xml;
    REQUIRE(document_xml.load_string(saved_document_xml.c_str()));
    auto paragraph_properties =
        document_xml.child("w:document").child("w:body").child("w:p").child("w:pPr");
    REQUIRE(paragraph_properties != pugi::xml_node{});
    REQUIRE(paragraph_properties.child("w:pStyle") != pugi::xml_node{});
    REQUIRE(paragraph_properties.child("w:bidi") != pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    const auto bidi = reopened_paragraph.bidi();
    REQUIRE(bidi.has_value());
    CHECK_FALSE(*bidi);
    CHECK(reopened_paragraph.clear_bidi());
    CHECK_FALSE(reopened.save());

    saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document cleared_document_xml;
    REQUIRE(cleared_document_xml.load_string(saved_document_xml.c_str()));
    paragraph_properties =
        cleared_document_xml.child("w:document").child("w:body").child("w:p").child("w:pPr");
    REQUIRE(paragraph_properties != pugi::xml_node{});

    const auto paragraph_style = paragraph_properties.child("w:pStyle");
    REQUIRE(paragraph_style != pugi::xml_node{});
    CHECK_EQ(std::string_view{paragraph_style.attribute("w:val").value()}, "Heading1");
    CHECK(paragraph_properties.child("w:bidi") == pugi::xml_node{});

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), paragraph_text + "\n");

    auto cleared_paragraph = cleared.paragraphs();
    REQUIRE(cleared_paragraph.has_next());
    CHECK_FALSE(cleared_paragraph.bidi().has_value());

    fs::remove(target);
}

TEST_CASE("default paragraph bidi clear preserves unrelated docDefaults paragraph markup") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "default_paragraph_bidi_preserve_markup.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults>
    <w:pPrDefault>
      <w:pPr>
        <w:spacing w:after="120"/>
        <w:bidi w:val="1"/>
      </w:pPr>
    </w:pPrDefault>
  </w:docDefaults>
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="character" w:default="1" w:styleId="DefaultParagraphFont">
    <w:name w:val="Default Paragraph Font"/>
  </w:style>
</w:styles>
)";

    write_test_docx_with_styles(target, document_xml, styles_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto default_bidi = doc.default_paragraph_bidi();
    REQUIRE(default_bidi.has_value());
    CHECK(*default_bidi);

    CHECK(doc.clear_default_paragraph_bidi());
    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto paragraph_properties = styles_document.child("w:styles")
                                        .child("w:docDefaults")
                                        .child("w:pPrDefault")
                                        .child("w:pPr");
    REQUIRE(paragraph_properties != pugi::xml_node{});

    const auto spacing = paragraph_properties.child("w:spacing");
    REQUIRE(spacing != pugi::xml_node{});
    CHECK_EQ(std::string_view{spacing.attribute("w:after").value()}, "120");
    CHECK(paragraph_properties.child("w:bidi") == pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.default_paragraph_bidi().has_value());
    CHECK_EQ(collect_document_text(reopened), "seed\n");

    fs::remove(target);
}

TEST_CASE("style run rtl clear preserves unrelated style run metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_run_rtl_preserve_metadata.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r>
        <w:rPr><w:rStyle w:val="AccentStrong"/></w:rPr>
        <w:t>seed</w:t>
      </w:r>
    </w:p>
  </w:body>
</w:document>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="character" w:default="1" w:styleId="DefaultParagraphFont">
    <w:name w:val="Default Paragraph Font"/>
  </w:style>
  <w:style w:type="character" w:styleId="AccentStrong" w:customStyle="1">
    <w:name w:val="Accent Strong"/>
    <w:basedOn w:val="DefaultParagraphFont"/>
    <w:rPr>
      <w:b/>
      <w:rFonts w:ascii="Segoe UI" w:hAnsi="Segoe UI" w:cs="Segoe UI"
                w:eastAsia="Microsoft YaHei"/>
      <w:lang w:val="en-US" w:eastAsia="zh-CN" w:bidi="ar-SA"/>
      <w:rtl w:val="1"/>
    </w:rPr>
  </w:style>
</w:styles>
)";

    write_test_docx_with_styles(target, document_xml, styles_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto style_rtl = doc.style_run_rtl("AccentStrong");
    REQUIRE(style_rtl.has_value());
    CHECK(*style_rtl);

    CHECK(doc.clear_style_run_rtl("AccentStrong"));
    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto style = find_style_xml_node(styles_document.child("w:styles"), "AccentStrong");
    REQUIRE(style != pugi::xml_node{});
    const auto run_properties = style.child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    CHECK(run_properties.child("w:b") != pugi::xml_node{});

    const auto run_fonts = run_properties.child("w:rFonts");
    REQUIRE(run_fonts != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_fonts.attribute("w:ascii").value()}, "Segoe UI");
    CHECK_EQ(std::string_view{run_fonts.attribute("w:eastAsia").value()},
             "Microsoft YaHei");

    const auto run_language = run_properties.child("w:lang");
    REQUIRE(run_language != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_language.attribute("w:val").value()}, "en-US");
    CHECK_EQ(std::string_view{run_language.attribute("w:eastAsia").value()}, "zh-CN");
    CHECK_EQ(std::string_view{run_language.attribute("w:bidi").value()}, "ar-SA");
    CHECK(run_properties.child("w:rtl") == pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.style_run_rtl("AccentStrong").has_value());

    const auto style_font = reopened.style_run_font_family("AccentStrong");
    REQUIRE(style_font.has_value());
    CHECK_EQ(*style_font, "Segoe UI");

    const auto style_east_asia_font = reopened.style_run_east_asia_font_family("AccentStrong");
    REQUIRE(style_east_asia_font.has_value());
    CHECK_EQ(*style_east_asia_font, "Microsoft YaHei");

    const auto style_language = reopened.style_run_language("AccentStrong");
    REQUIRE(style_language.has_value());
    CHECK_EQ(*style_language, "en-US");

    const auto style_east_asia_language =
        reopened.style_run_east_asia_language("AccentStrong");
    REQUIRE(style_east_asia_language.has_value());
    CHECK_EQ(*style_east_asia_language, "zh-CN");

    const auto style_bidi_language = reopened.style_run_bidi_language("AccentStrong");
    REQUIRE(style_bidi_language.has_value());
    CHECK_EQ(*style_bidi_language, "ar-SA");

    fs::remove(target);
}

TEST_CASE("style paragraph bidi clear preserves unrelated style paragraph and run markup") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "style_paragraph_bidi_preserve_markup.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr><w:pStyle w:val="CustomHeading"/></w:pPr>
      <w:r><w:t>seed</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)";
    const std::string styles_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="character" w:default="1" w:styleId="DefaultParagraphFont">
    <w:name w:val="Default Paragraph Font"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="CustomHeading" w:customStyle="1">
    <w:name w:val="Custom Heading"/>
    <w:basedOn w:val="Normal"/>
    <w:pPr>
      <w:keepNext/>
      <w:outlineLvl w:val="2"/>
      <w:bidi w:val="1"/>
    </w:pPr>
    <w:rPr>
      <w:b/>
      <w:lang w:val="en-US"/>
    </w:rPr>
  </w:style>
</w:styles>
)";

    write_test_docx_with_styles(target, document_xml, styles_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto style_bidi = doc.style_paragraph_bidi("CustomHeading");
    REQUIRE(style_bidi.has_value());
    CHECK(*style_bidi);

    CHECK(doc.clear_style_paragraph_bidi("CustomHeading"));
    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto style = find_style_xml_node(styles_document.child("w:styles"), "CustomHeading");
    REQUIRE(style != pugi::xml_node{});

    const auto paragraph_properties = style.child("w:pPr");
    REQUIRE(paragraph_properties != pugi::xml_node{});
    CHECK(paragraph_properties.child("w:keepNext") != pugi::xml_node{});
    const auto outline_level = paragraph_properties.child("w:outlineLvl");
    REQUIRE(outline_level != pugi::xml_node{});
    CHECK_EQ(std::string_view{outline_level.attribute("w:val").value()}, "2");
    CHECK(paragraph_properties.child("w:bidi") == pugi::xml_node{});

    const auto run_properties = style.child("w:rPr");
    REQUIRE(run_properties != pugi::xml_node{});
    CHECK(run_properties.child("w:b") != pugi::xml_node{});
    const auto run_language = run_properties.child("w:lang");
    REQUIRE(run_language != pugi::xml_node{});
    CHECK_EQ(std::string_view{run_language.attribute("w:val").value()}, "en-US");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_FALSE(reopened.style_paragraph_bidi("CustomHeading").has_value());
    CHECK_EQ(collect_document_text(reopened), "seed\n");

    fs::remove(target);
}

TEST_CASE("run rtl APIs write w:rtl and clear removes empty run properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_rtl_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"مرحبا RTL 123");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());

    CHECK_FALSE(run.rtl().has_value());
    CHECK(run.set_rtl());

    const auto rtl = run.rtl();
    REQUIRE(rtl.has_value());
    CHECK(*rtl);

    CHECK_FALSE(doc.save());

    auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:rtl"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:rtl w:val=\"1\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_run = reopened.paragraphs().runs();
    REQUIRE(reopened_run.has_next());
    CHECK_EQ(reopened_run.get_text(), run_text);

    const auto reopened_rtl = reopened_run.rtl();
    REQUIRE(reopened_rtl.has_value());
    CHECK(*reopened_rtl);

    CHECK(reopened_run.clear_rtl());
    CHECK_FALSE(reopened.save());

    saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rtl"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rPr>"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    auto cleared_run = cleared.paragraphs().runs();
    REQUIRE(cleared_run.has_next());
    CHECK_FALSE(cleared_run.rtl().has_value());

    fs::remove(target);
}

TEST_CASE("paragraph bidi APIs write w:bidi and clear removes empty paragraph properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_bidi_roundtrip.docx";
    const auto paragraph_text = utf8_from_u8(u8"فقرة عربية مع 123");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(paragraph_text).has_next());

    CHECK_FALSE(paragraph.bidi().has_value());
    CHECK(paragraph.set_bidi());

    auto bidi = paragraph.bidi();
    REQUIRE(bidi.has_value());
    CHECK(*bidi);

    CHECK_FALSE(doc.save());

    auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:bidi"), std::string::npos);
    CHECK_NE(saved_document_xml.find("<w:bidi w:val=\"1\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    CHECK_EQ(collect_document_text(reopened), paragraph_text + "\n");

    bidi = reopened_paragraph.bidi();
    REQUIRE(bidi.has_value());
    CHECK(*bidi);

    CHECK(reopened_paragraph.set_bidi(false));
    CHECK_FALSE(reopened.save());

    saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:bidi w:val=\"0\""), std::string::npos);

    featherdoc::Document reopened_false(target);
    CHECK_FALSE(reopened_false.open());
    auto false_paragraph = reopened_false.paragraphs();
    REQUIRE(false_paragraph.has_next());
    bidi = false_paragraph.bidi();
    REQUIRE(bidi.has_value());
    CHECK_FALSE(*bidi);

    CHECK(false_paragraph.clear_bidi());
    CHECK_FALSE(reopened_false.save());

    saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:bidi"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pPr>"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_EQ(collect_document_text(cleared), paragraph_text + "\n");
    CHECK_FALSE(cleared.paragraphs().bidi().has_value());

    fs::remove(target);
}

TEST_CASE("default direction APIs edit docDefaults and round-trip through styles.xml") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "default_direction_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"默认方向写入");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.default_run_rtl().has_value());
    CHECK_FALSE(doc.default_paragraph_bidi().has_value());
    CHECK(doc.set_default_run_rtl());
    CHECK(doc.set_default_paragraph_bidi());

    auto default_run_rtl = doc.default_run_rtl();
    REQUIRE(default_run_rtl.has_value());
    CHECK(*default_run_rtl);

    auto default_paragraph_bidi = doc.default_paragraph_bidi();
    REQUIRE(default_paragraph_bidi.has_value());
    CHECK(*default_paragraph_bidi);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.add_run(run_text).has_next());

    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:rtl w:val=\"1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:bidi w:val=\"1\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    default_run_rtl = reopened.default_run_rtl();
    REQUIRE(default_run_rtl.has_value());
    CHECK(*default_run_rtl);

    default_paragraph_bidi = reopened.default_paragraph_bidi();
    REQUIRE(default_paragraph_bidi.has_value());
    CHECK(*default_paragraph_bidi);

    CHECK_EQ(collect_document_text(reopened), run_text + "\n");
    CHECK(reopened.set_default_run_rtl(false));
    CHECK(reopened.set_default_paragraph_bidi(false));
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:rtl w:val=\"0\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:bidi w:val=\"0\""), std::string::npos);

    featherdoc::Document reopened_false(target);
    CHECK_FALSE(reopened_false.open());
    default_run_rtl = reopened_false.default_run_rtl();
    REQUIRE(default_run_rtl.has_value());
    CHECK_FALSE(*default_run_rtl);
    default_paragraph_bidi = reopened_false.default_paragraph_bidi();
    REQUIRE(default_paragraph_bidi.has_value());
    CHECK_FALSE(*default_paragraph_bidi);

    CHECK(reopened_false.clear_default_run_rtl());
    CHECK(reopened_false.clear_default_paragraph_bidi());
    CHECK_FALSE(reopened_false.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "<w:rtl"), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "<w:bidi"), 0);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.default_run_rtl().has_value());
    CHECK_FALSE(cleared.default_paragraph_bidi().has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}

TEST_CASE("style direction APIs edit styles.xml and preserve unrelated style markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_direction_roundtrip.docx";
    const auto run_text = utf8_from_u8(u8"样式方向写入");
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_FALSE(doc.set_style_run_rtl(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_style_run_rtl("MissingStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.style_run_rtl("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.set_style_paragraph_bidi(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_style_paragraph_bidi("MissingStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.style_paragraph_bidi("MissingStyle").has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_style_run_rtl("Emphasis"));
    CHECK(doc.set_style_paragraph_bidi("Heading1"));

    auto style_run_rtl = doc.style_run_rtl("Emphasis");
    REQUIRE(style_run_rtl.has_value());
    CHECK(*style_run_rtl);

    auto style_paragraph_bidi = doc.style_paragraph_bidi("Heading1");
    REQUIRE(style_paragraph_bidi.has_value());
    CHECK(*style_paragraph_bidi);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    auto run = paragraph.add_run(run_text);
    REQUIRE(run.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "Heading1"));
    CHECK(doc.set_run_style(run, "Emphasis"));

    CHECK_FALSE(doc.save());

    auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Emphasis\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("w:styleId=\"Heading1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:rtl w:val=\"1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:bidi w:val=\"1\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:i"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:outlineLvl"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), run_text + "\n");

    style_run_rtl = reopened.style_run_rtl("Emphasis");
    REQUIRE(style_run_rtl.has_value());
    CHECK(*style_run_rtl);

    style_paragraph_bidi = reopened.style_paragraph_bidi("Heading1");
    REQUIRE(style_paragraph_bidi.has_value());
    CHECK(*style_paragraph_bidi);

    CHECK(reopened.set_style_run_rtl("Emphasis", false));
    CHECK(reopened.set_style_paragraph_bidi("Heading1", false));
    CHECK_FALSE(reopened.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_NE(saved_styles_xml.find("<w:rtl w:val=\"0\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:bidi w:val=\"0\""), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:i"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:outlineLvl"), std::string::npos);

    featherdoc::Document reopened_false(target);
    CHECK_FALSE(reopened_false.open());
    style_run_rtl = reopened_false.style_run_rtl("Emphasis");
    REQUIRE(style_run_rtl.has_value());
    CHECK_FALSE(*style_run_rtl);
    style_paragraph_bidi = reopened_false.style_paragraph_bidi("Heading1");
    REQUIRE(style_paragraph_bidi.has_value());
    CHECK_FALSE(*style_paragraph_bidi);

    CHECK(reopened_false.clear_style_run_rtl("Emphasis"));
    CHECK(reopened_false.clear_style_paragraph_bidi("Heading1"));
    CHECK_FALSE(reopened_false.save());

    saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "<w:rtl"), 0);
    CHECK_EQ(count_substring_occurrences(saved_styles_xml, "<w:bidi w:val=\"0\""), 0);
    CHECK_NE(saved_styles_xml.find("<w:i"), std::string::npos);
    CHECK_NE(saved_styles_xml.find("<w:outlineLvl"), std::string::npos);

    featherdoc::Document cleared(target);
    CHECK_FALSE(cleared.open());
    CHECK_FALSE(cleared.style_run_rtl("Emphasis").has_value());
    CHECK_FALSE(cleared.style_paragraph_bidi("Heading1").has_value());
    CHECK_EQ(collect_document_text(cleared), run_text + "\n");

    fs::remove(target);
}
