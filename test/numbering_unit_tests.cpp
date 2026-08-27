#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("set_paragraph_list creates numbering parts and preserves them across reopen save") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_list_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_item = doc.paragraphs();
    CHECK(doc.set_paragraph_list(first_item, featherdoc::list_kind::bullet));
    CHECK(first_item.add_run("bullet 0").has_next());

    auto nested_item = first_item.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(nested_item, featherdoc::list_kind::bullet, 1U));
    CHECK(nested_item.add_run("bullet 1").has_next());

    auto decimal_item = nested_item.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(decimal_item, featherdoc::list_kind::decimal));
    CHECK(decimal_item.add_run("decimal 0").has_next());

    CHECK_FALSE(doc.save());

    CHECK(test_docx_entry_exists(target, "word/numbering.xml"));

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find(
                 "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering\""),
             std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"numbering.xml\""), std::string::npos);

    const auto saved_content_types = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(saved_content_types.find("PartName=\"/word/numbering.xml\""), std::string::npos);
    CHECK_NE(saved_content_types.find(
                 "application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"),
             std::string::npos);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:numPr>"), 3);

    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));
    const auto body = xml_document.child("w:document").child("w:body");
    auto first_paragraph = body.child("w:p");
    REQUIRE(first_paragraph != pugi::xml_node{});
    auto second_paragraph = first_paragraph.next_sibling("w:p");
    REQUIRE(second_paragraph != pugi::xml_node{});
    auto third_paragraph = second_paragraph.next_sibling("w:p");
    REQUIRE(third_paragraph != pugi::xml_node{});

    const auto first_num_pr = first_paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    const auto third_num_pr = third_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    REQUIRE(third_num_pr != pugi::xml_node{});

    CHECK_EQ(std::string{first_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");
    CHECK_EQ(std::string{second_num_pr.child("w:ilvl").attribute("w:val").value()}, "1");
    CHECK_EQ(std::string{third_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");
    CHECK_EQ(std::string{first_num_pr.child("w:numId").attribute("w:val").value()},
             std::string{second_num_pr.child("w:numId").attribute("w:val").value()});
    CHECK_NE(std::string{first_num_pr.child("w:numId").attribute("w:val").value()},
             std::string{third_num_pr.child("w:numId").attribute("w:val").value()});

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    CHECK_NE(numbering_xml.find("FeatherDocBulletList"), std::string::npos);
    CHECK_NE(numbering_xml.find("FeatherDocDecimalList"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK(reopened.paragraphs().add_run("tail").has_next());
    CHECK_FALSE(reopened.save());
    CHECK(test_docx_entry_exists(target, "word/numbering.xml"));

    fs::remove(target);
}

TEST_CASE("numbering mutations save back to a non-default relationship target") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "numbering_custom_target_roundtrip.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/custom/numbering.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
</Types>
)";
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>numbered item</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rNumbering"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering"
                Target="custom/numbering.xml"/>
</Relationships>
)";
    const std::string numbering_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
</w:numbering>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/custom/numbering.xml", numbering_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto invalid_definition = featherdoc::numbering_definition{};
    CHECK_FALSE(
        doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().entry_name, "word/custom/numbering.xml");

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet));

    CHECK_FALSE(doc.find_numbering_definition(9999U).has_value());
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().entry_name, "word/custom/numbering.xml");
    CHECK_NE(doc.last_error().detail.find("word/custom/numbering.xml"),
             std::string::npos);
    CHECK_EQ(doc.last_error().detail.find("'word/numbering.xml'"),
             std::string::npos);

    CHECK_FALSE(doc.save());

    CHECK_FALSE(test_docx_entry_exists(target, "word/numbering.xml"));
    const auto saved_numbering_xml =
        read_test_docx_entry(target, "word/custom/numbering.xml");
    CHECK_NE(saved_numbering_xml.find("FeatherDocBulletList"),
             std::string::npos);
    CHECK_NE(saved_numbering_xml.find("<w:abstractNum"), std::string::npos);
    CHECK_NE(saved_numbering_xml.find("<w:num "), std::string::npos);

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:numPr>"), std::string::npos);

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"custom/numbering.xml\""),
             std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"numbering.xml\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    CHECK(reopened.clear_paragraph_list(reopened_paragraph));
    CHECK_FALSE(reopened.save());
    CHECK_FALSE(test_docx_entry_exists(target, "word/numbering.xml"));

    fs::remove(target);
}

TEST_CASE("clear_paragraph_list removes numbering markup and invalid level is rejected") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_list_clear.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>seed</w:t></w:r></w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto paragraph = doc.paragraphs();
    CHECK_FALSE(doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet, 9U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_paragraph_list(paragraph, featherdoc::list_kind::bullet));
    CHECK(doc.clear_paragraph_list(paragraph));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:numPr>"), 0);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:pPr>"), 0);

    fs::remove(target);
}

TEST_CASE("numbering APIs reject paragraphs owned by another document without "
          "changing either package") {
    namespace fs = std::filesystem;

    const auto owner_baseline =
        fs::current_path() / "foreign_paragraph_owner_baseline.docx";
    const auto receiver_baseline =
        fs::current_path() / "foreign_paragraph_receiver_baseline.docx";
    fs::remove(owner_baseline);
    fs::remove(receiver_baseline);

    {
        featherdoc::Document owner(owner_baseline);
        REQUIRE_FALSE(owner.create_empty());
        auto paragraph = owner.paragraphs();
        REQUIRE(paragraph.has_next());
        REQUIRE(owner.set_paragraph_list(paragraph,
                                         featherdoc::list_kind::bullet));
        REQUIRE(paragraph.add_run("foreign owner").has_next());
        REQUIRE_FALSE(owner.save());
    }

    std::uint32_t receiver_definition_id = 0U;
    {
        featherdoc::Document receiver(receiver_baseline);
        REQUIRE_FALSE(receiver.create_empty());
        auto definition = featherdoc::numbering_definition{};
        definition.name = "ForeignParagraphReceiverDefinition";
        definition.levels = {featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."}};
        const auto definition_id =
            receiver.ensure_numbering_definition(definition);
        REQUIRE(definition_id.has_value());
        receiver_definition_id = *definition_id;
        REQUIRE_FALSE(receiver.save());
    }

    const auto verify_rejected =
        [&](std::string_view suffix, const auto &mutation) {
            CAPTURE(suffix);
            const auto owner_path =
                fs::current_path() /
                ("foreign_paragraph_owner_" + std::string{suffix} +
                 ".docx");
            const auto receiver_path =
                fs::current_path() /
                ("foreign_paragraph_receiver_" + std::string{suffix} +
                 ".docx");
            fs::remove(owner_path);
            fs::remove(receiver_path);
            REQUIRE(fs::copy_file(owner_baseline, owner_path));
            REQUIRE(fs::copy_file(receiver_baseline, receiver_path));

            const auto owner_entries_before =
                read_test_archive_entries(owner_path);
            const auto receiver_entries_before =
                read_test_archive_entries(receiver_path);

            featherdoc::Document owner(owner_path);
            featherdoc::Document receiver(receiver_path);
            REQUIRE_FALSE(owner.open());
            REQUIRE_FALSE(receiver.open());
            auto foreign_paragraph = owner.paragraphs();
            REQUIRE(foreign_paragraph.has_next());

            CHECK_FALSE(mutation(receiver, foreign_paragraph));
            CHECK_EQ(receiver.last_error().code,
                     std::make_error_code(std::errc::invalid_argument));
            CHECK(foreign_paragraph.has_next());

            REQUIRE_FALSE(owner.save());
            REQUIRE_FALSE(receiver.save());
            CHECK(read_test_archive_entries(owner_path) ==
                  owner_entries_before);
            CHECK(read_test_archive_entries(receiver_path) ==
                  receiver_entries_before);

            fs::remove(owner_path);
            fs::remove(receiver_path);
        };

    verify_rejected(
        "set_definition",
        [&](featherdoc::Document &receiver,
            featherdoc::Paragraph foreign_paragraph) {
            return receiver.set_paragraph_numbering(
                foreign_paragraph, receiver_definition_id);
        });
    verify_rejected(
        "set_list", [](featherdoc::Document &receiver,
                       featherdoc::Paragraph foreign_paragraph) {
            return receiver.set_paragraph_list(
                foreign_paragraph, featherdoc::list_kind::decimal);
        });
    verify_rejected(
        "restart_list", [](featherdoc::Document &receiver,
                           featherdoc::Paragraph foreign_paragraph) {
            return receiver.restart_paragraph_list(
                foreign_paragraph, featherdoc::list_kind::bullet);
        });
    verify_rejected(
        "clear_list", [](featherdoc::Document &receiver,
                         featherdoc::Paragraph foreign_paragraph) {
            return receiver.clear_paragraph_list(foreign_paragraph);
        });

    fs::remove(owner_baseline);
    fs::remove(receiver_baseline);
}

TEST_CASE("paragraph and style numbering canonicalize duplicate numPr nodes "
          "in schema order") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "duplicate_numbering_properties.docx";
    fs::remove(target);

    const auto content_types_xml = std::string{R"(
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>)"};
    const auto document_relationships_xml = std::string{R"(
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rIdNumbering" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/>
  <Relationship Id="rIdStyles" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)"};
    const auto document_xml = std::string{R"(
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:pStyle w:val="Normal"/>
        <w:numPr><w:ilvl w:val="7"/><w:numId w:val="71"/></w:numPr>
        <w:numPr><w:ilvl w:val="8"/><w:numId w:val="81"/></w:numPr>
        <w:bidi/>
      </w:pPr>
      <w:r><w:t>clear duplicate paragraph numbering</w:t></w:r>
    </w:p>
    <w:p>
      <w:pPr>
        <w:pStyle w:val="Normal"/>
        <w:keepNext/>
        <w:numPr><w:ilvl w:val="6"/><w:numId w:val="61"/></w:numPr>
        <w:numPr><w:ilvl w:val="5"/><w:numId w:val="51"/></w:numPr>
        <w:bidi/>
        <w:spacing w:after="120"/>
      </w:pPr>
      <w:r><w:t>set canonical paragraph numbering</w:t></w:r>
    </w:p>
  </w:body>
</w:document>)"};
    const auto numbering_xml = std::string{R"(
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="7">
    <w:multiLevelType w:val="multilevel"/>
    <w:name w:val="DuplicateOrderingDefinition"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
    </w:lvl>
  </w:abstractNum>
  <w:num w:numId="11"><w:abstractNumId w:val="7"/></w:num>
</w:numbering>)"};
    const auto styles_xml = std::string{R"(
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:styleId="SetDuplicateStyle">
    <w:name w:val="Set Duplicate Style"/>
    <w:pPr>
      <w:keepNext/>
      <w:numPr><w:ilvl w:val="4"/><w:numId w:val="41"/></w:numPr>
      <w:numPr><w:ilvl w:val="3"/><w:numId w:val="31"/></w:numPr>
      <w:bidi/>
      <w:spacing w:after="80"/>
    </w:pPr>
  </w:style>
  <w:style w:type="paragraph" w:styleId="ClearDuplicateStyle">
    <w:name w:val="Clear Duplicate Style"/>
    <w:pPr>
      <w:keepNext/>
      <w:numPr><w:ilvl w:val="2"/><w:numId w:val="21"/></w:numPr>
      <w:numPr><w:ilvl w:val="1"/><w:numId w:val="11"/></w:numPr>
      <w:bidi/>
    </w:pPr>
  </w:style>
</w:styles>)"};

    write_test_archive_entries(
        target,
        {{test_content_types_xml_entry, content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"word/_rels/document.xml.rels", document_relationships_xml},
         {"word/numbering.xml", numbering_xml},
         {"word/styles.xml", styles_xml}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());

    auto clear_paragraph = document.paragraphs();
    REQUIRE(clear_paragraph.has_next());
    CHECK(document.clear_paragraph_list(clear_paragraph));

    auto set_paragraph = document.paragraphs();
    REQUIRE(set_paragraph.has_next());
    set_paragraph.next();
    REQUIRE(set_paragraph.has_next());
    CHECK(document.set_paragraph_numbering(set_paragraph, 7U));
    CHECK(document.set_paragraph_style_numbering("SetDuplicateStyle", 7U));
    CHECK(document.clear_paragraph_style_numbering("ClearDuplicateStyle"));
    REQUIRE_FALSE(document.save());

    const auto child_names = [](pugi::xml_node parent) {
        auto names = std::vector<std::string>{};
        for (auto child = parent.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            names.emplace_back(child.name());
        }
        return names;
    };

    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(
        read_test_docx_entry(target, test_document_xml_entry).c_str()));
    auto saved_paragraph =
        saved_document.child("w:document").child("w:body").child("w:p");
    REQUIRE(saved_paragraph != pugi::xml_node{});
    const auto cleared_paragraph_properties = saved_paragraph.child("w:pPr");
    REQUIRE(cleared_paragraph_properties != pugi::xml_node{});
    CHECK_EQ(count_named_children(cleared_paragraph_properties, "w:numPr"),
             0U);
    const auto expected_cleared_paragraph_order =
        std::vector<std::string>{"w:pStyle", "w:bidi"};
    CHECK(child_names(cleared_paragraph_properties) ==
          expected_cleared_paragraph_order);

    saved_paragraph = saved_paragraph.next_sibling("w:p");
    REQUIRE(saved_paragraph != pugi::xml_node{});
    const auto set_paragraph_properties = saved_paragraph.child("w:pPr");
    REQUIRE(set_paragraph_properties != pugi::xml_node{});
    CHECK_EQ(count_named_children(set_paragraph_properties, "w:numPr"), 1U);
    const auto expected_set_paragraph_order = std::vector<std::string>{
        "w:pStyle", "w:keepNext", "w:numPr", "w:bidi", "w:spacing"};
    CHECK(child_names(set_paragraph_properties) ==
          expected_set_paragraph_order);
    const auto set_paragraph_num_pr =
        set_paragraph_properties.child("w:numPr");
    REQUIRE(set_paragraph_num_pr != pugi::xml_node{});
    CHECK_EQ(count_named_children(set_paragraph_num_pr, "w:ilvl"), 1U);
    CHECK_EQ(count_named_children(set_paragraph_num_pr, "w:numId"), 1U);
    CHECK_EQ(std::string_view{
                 set_paragraph_num_pr.child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{
                 set_paragraph_num_pr.child("w:numId").attribute("w:val").value()},
             "11");

    pugi::xml_document saved_styles;
    REQUIRE(saved_styles.load_string(
        read_test_docx_entry(target, "word/styles.xml").c_str()));
    const auto saved_styles_root = saved_styles.child("w:styles");
    REQUIRE(saved_styles_root != pugi::xml_node{});

    const auto set_style =
        find_style_xml_node(saved_styles_root, "SetDuplicateStyle");
    REQUIRE(set_style != pugi::xml_node{});
    const auto set_style_properties = set_style.child("w:pPr");
    REQUIRE(set_style_properties != pugi::xml_node{});
    CHECK_EQ(count_named_children(set_style_properties, "w:numPr"), 1U);
    const auto expected_set_style_order = std::vector<std::string>{
        "w:keepNext", "w:numPr", "w:bidi", "w:spacing"};
    CHECK(child_names(set_style_properties) == expected_set_style_order);
    const auto set_style_num_pr = set_style_properties.child("w:numPr");
    REQUIRE(set_style_num_pr != pugi::xml_node{});
    CHECK_EQ(count_named_children(set_style_num_pr, "w:ilvl"), 1U);
    CHECK_EQ(count_named_children(set_style_num_pr, "w:numId"), 1U);
    CHECK_EQ(std::string_view{
                 set_style_num_pr.child("w:ilvl").attribute("w:val").value()},
             "0");
    CHECK_EQ(std::string_view{
                 set_style_num_pr.child("w:numId").attribute("w:val").value()},
             "11");

    const auto cleared_style =
        find_style_xml_node(saved_styles_root, "ClearDuplicateStyle");
    REQUIRE(cleared_style != pugi::xml_node{});
    const auto cleared_style_properties = cleared_style.child("w:pPr");
    REQUIRE(cleared_style_properties != pugi::xml_node{});
    CHECK_EQ(count_named_children(cleared_style_properties, "w:numPr"), 0U);
    const auto expected_cleared_style_order =
        std::vector<std::string>{"w:keepNext", "w:bidi"};
    CHECK(child_names(cleared_style_properties) ==
          expected_cleared_style_order);

    fs::remove(target);
}

TEST_CASE("clearing absent style numbering does not attach a styles part") {
    namespace fs = std::filesystem;

    const auto verify_no_attachment =
        [&](std::string_view suffix, std::string_view style_id,
            bool expected_success) {
            CAPTURE(suffix);
            CAPTURE(style_id);
            const auto target =
                fs::current_path() /
                ("clear_absent_style_numbering_" + std::string{suffix} +
                 ".docx");
            fs::remove(target);
            const auto document_xml = std::string{R"(
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>no styles part</w:t></w:r></w:p></w:body>
</w:document>)"};
            write_test_docx(target, document_xml);
            const auto entries_before = read_test_archive_entries(target);
            const auto content_types_before = read_test_docx_entry(
                target, test_content_types_xml_entry);
            const auto root_relationships_before = read_test_docx_entry(
                target, test_relationships_xml_entry);
            const auto archive_entry_names = [](const auto &entries) {
                std::vector<std::string> names;
                names.reserve(entries.size());
                for (const auto &[name, content] : entries) {
                    static_cast<void>(content);
                    names.push_back(name);
                }
                std::sort(names.begin(), names.end());
                return names;
            };
            const auto entry_names_before = archive_entry_names(entries_before);

            featherdoc::Document document(target);
            REQUIRE_FALSE(document.open());
            const auto cleared =
                document.clear_paragraph_style_numbering(style_id);
            CHECK_EQ(cleared, expected_success);
            if (expected_success) {
                CHECK_FALSE(document.last_error());
            } else {
                CHECK_EQ(document.last_error().code,
                         std::make_error_code(std::errc::invalid_argument));
            }
            REQUIRE_FALSE(document.save());

            CHECK_FALSE(test_docx_entry_exists(target, "word/styles.xml"));
            CHECK_FALSE(test_docx_entry_exists(
                target, "word/_rels/document.xml.rels"));
            const auto saved_content_types =
                read_test_docx_entry(target, test_content_types_xml_entry);
            CHECK_EQ(saved_content_types, content_types_before);
            CHECK_EQ(saved_content_types.find("/word/styles.xml"),
                     std::string::npos);
            CHECK_EQ(saved_content_types.find(
                         "wordprocessingml.styles+xml"),
                     std::string::npos);
            CHECK_EQ(read_test_docx_entry(target, test_relationships_xml_entry),
                     root_relationships_before);

            const auto entries_after = read_test_archive_entries(target);
            CHECK(archive_entry_names(entries_after) == entry_names_before);

            pugi::xml_document saved_document;
            REQUIRE(saved_document.load_string(
                read_test_docx_entry(target, test_document_xml_entry).c_str()));
            const auto saved_body =
                saved_document.child("w:document").child("w:body");
            REQUIRE(saved_body != pugi::xml_node{});
            const auto saved_paragraph = saved_body.child("w:p");
            REQUIRE(saved_paragraph != pugi::xml_node{});
            CHECK(saved_paragraph.next_sibling("w:p") == pugi::xml_node{});
            CHECK(saved_paragraph.child("w:pPr") == pugi::xml_node{});
            const auto saved_run = saved_paragraph.child("w:r");
            REQUIRE(saved_run != pugi::xml_node{});
            CHECK(saved_run.next_sibling("w:r") == pugi::xml_node{});
            const auto saved_text = saved_run.child("w:t");
            REQUIRE(saved_text != pugi::xml_node{});
            CHECK_EQ(std::string_view{saved_text.text().get()},
                     "no styles part");
            CHECK(saved_text.next_sibling("w:t") == pugi::xml_node{});

            fs::remove(target);
        };

    verify_no_attachment("normal_noop", "Normal", true);
    verify_no_attachment("missing_style", "MissingStyle", false);
}

TEST_CASE("restart_paragraph_list creates a fresh numbering instance and adjacent items continue it") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_list_restart.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_item = doc.paragraphs();
    CHECK(doc.set_paragraph_list(first_item, featherdoc::list_kind::decimal));
    CHECK(first_item.add_run("first list item").has_next());

    auto second_item = first_item.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(second_item, featherdoc::list_kind::decimal));
    CHECK(second_item.add_run("first list item 2").has_next());

    auto spacer = second_item.insert_paragraph_after("restart here");
    REQUIRE(spacer.has_next());

    auto restarted_first = spacer.insert_paragraph_after("");
    CHECK(doc.restart_paragraph_list(restarted_first, featherdoc::list_kind::decimal));
    CHECK(restarted_first.add_run("second list item 1").has_next());

    auto restarted_second = restarted_first.insert_paragraph_after("");
    CHECK(doc.set_paragraph_list(restarted_second, featherdoc::list_kind::decimal));
    CHECK(restarted_second.add_run("second list item 2").has_next());

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(saved_document_xml.c_str()));

    const auto body = xml_document.child("w:document").child("w:body");
    auto paragraph = body.child("w:p");
    REQUIRE(paragraph != pugi::xml_node{});
    auto second_paragraph = paragraph.next_sibling("w:p");
    REQUIRE(second_paragraph != pugi::xml_node{});
    auto spacer_paragraph = second_paragraph.next_sibling("w:p");
    REQUIRE(spacer_paragraph != pugi::xml_node{});
    auto restarted_first_paragraph = spacer_paragraph.next_sibling("w:p");
    REQUIRE(restarted_first_paragraph != pugi::xml_node{});
    auto restarted_second_paragraph = restarted_first_paragraph.next_sibling("w:p");
    REQUIRE(restarted_second_paragraph != pugi::xml_node{});

    const auto first_num_pr = paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    const auto restarted_first_num_pr =
        restarted_first_paragraph.child("w:pPr").child("w:numPr");
    const auto restarted_second_num_pr =
        restarted_second_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    REQUIRE(restarted_first_num_pr != pugi::xml_node{});
    REQUIRE(restarted_second_num_pr != pugi::xml_node{});

    const auto first_num_id = std::string{first_num_pr.child("w:numId").attribute("w:val").value()};
    const auto second_num_id =
        std::string{second_num_pr.child("w:numId").attribute("w:val").value()};
    const auto restarted_first_num_id =
        std::string{restarted_first_num_pr.child("w:numId").attribute("w:val").value()};
    const auto restarted_second_num_id =
        std::string{restarted_second_num_pr.child("w:numId").attribute("w:val").value()};

    CHECK_EQ(first_num_id, second_num_id);
    CHECK_NE(first_num_id, restarted_first_num_id);
    CHECK_EQ(restarted_first_num_id, restarted_second_num_id);

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});

    auto restarted_numbering_instance = pugi::xml_node{};
    for (auto numbering_instance = numbering_root.child("w:num");
         numbering_instance != pugi::xml_node{};
         numbering_instance = numbering_instance.next_sibling("w:num")) {
        if (std::string_view{numbering_instance.attribute("w:numId").value()} ==
            restarted_first_num_id) {
            restarted_numbering_instance = numbering_instance;
            break;
        }
    }
    REQUIRE(restarted_numbering_instance != pugi::xml_node{});
    const auto restart_level_override = restarted_numbering_instance.child("w:lvlOverride");
    REQUIRE(restart_level_override != pugi::xml_node{});
    CHECK_EQ(std::string_view{restart_level_override.attribute("w:ilvl").value()}, "0");
    CHECK_EQ(std::string_view{
                 restart_level_override.child("w:startOverride").attribute("w:val").value()},
             "1");

    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 2);
    CHECK_EQ(count_substring_occurrences(numbering_xml, "FeatherDocDecimalList"), 1);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    reopened_paragraph.next();
    REQUIRE(reopened_paragraph.has_next());
    reopened_paragraph.next();
    REQUIRE(reopened_paragraph.has_next());
    reopened_paragraph.next();
    REQUIRE(reopened_paragraph.has_next());
    reopened_paragraph.next();
    REQUIRE(reopened_paragraph.has_next());

    CHECK(reopened.restart_paragraph_list(reopened_paragraph, featherdoc::list_kind::decimal));
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE("numbering metadata exposes instance overrides for restarted managed lists") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "managed_numbering_metadata.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto first_item = doc.paragraphs();
    REQUIRE(first_item.has_next());
    CHECK(doc.set_paragraph_list(first_item, featherdoc::list_kind::decimal));
    CHECK(first_item.add_run("first item").has_next());

    auto spacer = first_item.insert_paragraph_after("restart here");
    REQUIRE(spacer.has_next());

    auto restarted_item = spacer.insert_paragraph_after("");
    REQUIRE(restarted_item.has_next());
    CHECK(doc.restart_paragraph_list(restarted_item, featherdoc::list_kind::decimal));
    CHECK(restarted_item.add_run("restarted item").has_next());

    const auto definitions = doc.list_numbering_definitions();
    CHECK_FALSE(doc.last_error());
    const auto managed_definition =
        std::find_if(definitions.begin(), definitions.end(), [](const auto &summary) {
            return summary.name == "FeatherDocDecimalList";
        });
    REQUIRE(managed_definition != definitions.end());
    REQUIRE_EQ(managed_definition->instance_ids.size(), 2U);
    REQUIRE_EQ(managed_definition->instances.size(), 2U);
    CHECK_EQ(managed_definition->instances[0].instance_id,
             managed_definition->instance_ids[0]);
    CHECK(managed_definition->instances[0].level_overrides.empty());
    CHECK_EQ(managed_definition->instances[1].instance_id,
             managed_definition->instance_ids[1]);
    REQUIRE_EQ(managed_definition->instances[1].level_overrides.size(), 1U);
    CHECK_EQ(managed_definition->instances[1].level_overrides[0].level, 0U);
    REQUIRE(managed_definition->instances[1].level_overrides[0].start_override.has_value());
    CHECK_EQ(*managed_definition->instances[1].level_overrides[0].start_override, 1U);
    CHECK_FALSE(
        managed_definition->instances[1].level_overrides[0].level_definition.has_value());

    const auto found_instance =
        doc.find_numbering_instance(managed_definition->instance_ids[1]);
    CHECK_FALSE(doc.last_error());
    REQUIRE(found_instance.has_value());
    CHECK_EQ(found_instance->definition_id, managed_definition->definition_id);
    CHECK_EQ(found_instance->definition_name, managed_definition->name);
    CHECK_EQ(found_instance->instance.instance_id, managed_definition->instance_ids[1]);
    REQUIRE_EQ(found_instance->instance.level_overrides.size(), 1U);
    CHECK_EQ(found_instance->instance.level_overrides[0].level, 0U);
    REQUIRE(found_instance->instance.level_overrides[0].start_override.has_value());
    CHECK_EQ(*found_instance->instance.level_overrides[0].start_override, 1U);

    const auto found = doc.find_numbering_definition(managed_definition->definition_id);
    CHECK_FALSE(doc.last_error());
    REQUIRE(found.has_value());
    CHECK_EQ(found->instance_ids, managed_definition->instance_ids);
    CHECK_EQ(found->instances.size(), managed_definition->instances.size());
    REQUIRE_EQ(found->instances[1].level_overrides.size(), 1U);
    CHECK_EQ(found->instances[1].level_overrides[0].level, 0U);
    REQUIRE(found->instances[1].level_overrides[0].start_override.has_value());
    CHECK_EQ(*found->instances[1].level_overrides[0].start_override, 1U);

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_definition =
        reopened.find_numbering_definition(managed_definition->definition_id);
    CHECK_FALSE(reopened.last_error());
    REQUIRE(reopened_definition.has_value());
    CHECK_EQ(reopened_definition->instance_ids, managed_definition->instance_ids);
    CHECK_EQ(reopened_definition->instances.size(), managed_definition->instances.size());
    REQUIRE_EQ(reopened_definition->instances[1].level_overrides.size(), 1U);
    CHECK_EQ(reopened_definition->instances[1].level_overrides[0].level, 0U);
    REQUIRE(reopened_definition->instances[1].level_overrides[0].start_override.has_value());
    CHECK_EQ(*reopened_definition->instances[1].level_overrides[0].start_override, 1U);

    const auto reopened_instance =
        reopened.find_numbering_instance(managed_definition->instance_ids[1]);
    CHECK_FALSE(reopened.last_error());
    REQUIRE(reopened_instance.has_value());
    CHECK_EQ(reopened_instance->definition_id, managed_definition->definition_id);
    CHECK_EQ(reopened_instance->definition_name, managed_definition->name);
    REQUIRE_EQ(reopened_instance->instance.level_overrides.size(), 1U);
    CHECK_EQ(reopened_instance->instance.level_overrides[0].level, 0U);
    REQUIRE(reopened_instance->instance.level_overrides[0].start_override.has_value());
    CHECK_EQ(*reopened_instance->instance.level_overrides[0].start_override, 1U);

    fs::remove(target);
}

TEST_CASE("numbering catalog exports and imports definitions with instance overrides") {
    namespace fs = std::filesystem;

    const fs::path source_path = fs::current_path() / "numbering_catalog_source.docx";
    const fs::path imported_path = fs::current_path() / "numbering_catalog_imported.docx";
    fs::remove(source_path);
    fs::remove(imported_path);

    featherdoc::Document source(source_path);
    CHECK_FALSE(source.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "CatalogOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto definition_id = source.ensure_numbering_definition(definition);
    REQUIRE(definition_id.has_value());
    auto paragraph = source.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(source.set_paragraph_numbering(paragraph, *definition_id, 0U));

    auto exported_catalog = source.export_numbering_catalog();
    CHECK_FALSE(source.last_error());
    const auto exported_it = std::find_if(
        exported_catalog.definitions.begin(), exported_catalog.definitions.end(),
        [](const auto &catalog_definition) {
            return catalog_definition.definition.name == "CatalogOutline";
        });
    REQUIRE(exported_it != exported_catalog.definitions.end());
    REQUIRE_EQ(exported_it->instances.size(), 1U);
    CHECK(exported_it->instances.front().level_overrides.empty());

    auto override_definition = featherdoc::numbering_catalog_definition{};
    override_definition.definition.name = "RestartedCatalog";
    override_definition.definition.levels = definition.levels;
    auto override_instance = featherdoc::numbering_instance_summary{};
    override_instance.instance_id = 42U;
    override_instance.level_overrides.push_back(
        featherdoc::numbering_level_override_summary{0U, 3U, std::nullopt});
    override_definition.instances.push_back(override_instance);
    exported_catalog.definitions = {*exported_it, override_definition};

    featherdoc::Document imported(imported_path);
    CHECK_FALSE(imported.create_empty());
    const auto import_summary = imported.import_numbering_catalog(exported_catalog);
    CHECK_FALSE(imported.last_error());
    CHECK(import_summary);
    CHECK_EQ(import_summary.input_definition_count, 2U);
    CHECK_EQ(import_summary.imported_definition_count, 2U);
    CHECK_EQ(import_summary.imported_instance_count, 2U);
    REQUIRE_EQ(import_summary.definitions.size(), 2U);

    const auto imported_catalog_definition = std::find_if(
        import_summary.definitions.begin(), import_summary.definitions.end(),
        [](const auto &summary) { return summary.name == "CatalogOutline"; });
    REQUIRE(imported_catalog_definition != import_summary.definitions.end());
    REQUIRE_EQ(imported_catalog_definition->instance_ids.size(), 1U);

    const auto imported_definition = imported.find_numbering_definition(
        imported_catalog_definition->definition_id);
    REQUIRE(imported_definition.has_value());
    CHECK_EQ(imported_definition->name, "CatalogOutline");
    CHECK_EQ(imported_definition->levels.size(), 2U);
    REQUIRE_EQ(imported_definition->instances.size(), 1U);
    CHECK(imported_definition->instances.front().level_overrides.empty());

    const auto restarted_catalog_definition = std::find_if(
        import_summary.definitions.begin(), import_summary.definitions.end(),
        [](const auto &summary) { return summary.name == "RestartedCatalog"; });
    REQUIRE(restarted_catalog_definition != import_summary.definitions.end());
    REQUIRE_EQ(restarted_catalog_definition->instance_ids.size(), 1U);
    const auto restarted_instance = imported.find_numbering_instance(
        restarted_catalog_definition->instance_ids.front());
    REQUIRE(restarted_instance.has_value());
    CHECK_EQ(restarted_instance->definition_name, "RestartedCatalog");
    REQUIRE_EQ(restarted_instance->instance.level_overrides.size(), 1U);
    CHECK_EQ(restarted_instance->instance.level_overrides.front().level, 0U);
    REQUIRE(restarted_instance->instance.level_overrides.front().start_override.has_value());
    CHECK_EQ(*restarted_instance->instance.level_overrides.front().start_override, 3U);

    CHECK_FALSE(imported.save());
    featherdoc::Document reopened(imported_path);
    CHECK_FALSE(reopened.open());
    const auto reopened_instance = reopened.find_numbering_instance(
        restarted_catalog_definition->instance_ids.front());
    REQUIRE(reopened_instance.has_value());
    REQUIRE_EQ(reopened_instance->instance.level_overrides.size(), 1U);
    CHECK_EQ(*reopened_instance->instance.level_overrides.front().start_override, 3U);

    fs::remove(source_path);
    fs::remove(imported_path);
}

TEST_CASE(
    "ensure_numbering_definition and set_paragraph_numbering create custom numbering "
    "definitions and round-trip") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "LegalOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 3U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());

    const auto reused_numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(reused_numbering_id.has_value());
    CHECK_EQ(*reused_numbering_id, *numbering_id);

    auto first = doc.paragraphs();
    REQUIRE(first.has_next());
    CHECK(doc.set_paragraph_numbering(first, *numbering_id));
    CHECK(first.add_run("chapter 3").has_next());

    auto second = first.insert_paragraph_after("");
    REQUIRE(second.has_next());
    CHECK(doc.set_paragraph_numbering(second, *numbering_id, 1U));
    CHECK(second.add_run("chapter 3.1").has_next());

    auto third = second.insert_paragraph_after("");
    REQUIRE(third.has_next());
    CHECK(doc.set_paragraph_numbering(third, *numbering_id));
    CHECK(third.add_run("chapter 4").has_next());

    CHECK_FALSE(doc.save());

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));

    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});

    const auto abstract_num = find_numbering_abstract_xml_node(numbering_root, "LegalOutline");
    REQUIRE(abstract_num != pugi::xml_node{});
    CHECK_EQ(std::string_view{abstract_num.attribute("w:abstractNumId").value()},
             std::to_string(*numbering_id));
    CHECK_EQ(count_named_children(abstract_num, "w:lvl"), 2U);

    const auto level_zero = find_numbering_level_xml_node(abstract_num, 0U);
    const auto level_one = find_numbering_level_xml_node(abstract_num, 1U);
    REQUIRE(level_zero != pugi::xml_node{});
    REQUIRE(level_one != pugi::xml_node{});
    CHECK_EQ(std::string_view{level_zero.child("w:start").attribute("w:val").value()}, "3");
    CHECK_EQ(std::string_view{level_zero.child("w:numFmt").attribute("w:val").value()},
             "decimal");
    CHECK_EQ(std::string_view{level_zero.child("w:lvlText").attribute("w:val").value()},
             "%1.");
    CHECK_EQ(std::string_view{level_one.child("w:start").attribute("w:val").value()}, "1");
    CHECK_EQ(std::string_view{level_one.child("w:lvlText").attribute("w:val").value()},
             "%1.%2.");
    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 1U);

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document document_xml;
    REQUIRE(document_xml.load_string(saved_document_xml.c_str()));
    const auto body = document_xml.child("w:document").child("w:body");
    auto first_paragraph = body.child("w:p");
    REQUIRE(first_paragraph != pugi::xml_node{});
    auto second_paragraph = first_paragraph.next_sibling("w:p");
    REQUIRE(second_paragraph != pugi::xml_node{});
    auto third_paragraph = second_paragraph.next_sibling("w:p");
    REQUIRE(third_paragraph != pugi::xml_node{});

    const auto first_num_pr = first_paragraph.child("w:pPr").child("w:numPr");
    const auto second_num_pr = second_paragraph.child("w:pPr").child("w:numPr");
    const auto third_num_pr = third_paragraph.child("w:pPr").child("w:numPr");
    REQUIRE(first_num_pr != pugi::xml_node{});
    REQUIRE(second_num_pr != pugi::xml_node{});
    REQUIRE(third_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{first_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");
    CHECK_EQ(std::string_view{second_num_pr.child("w:ilvl").attribute("w:val").value()}, "1");
    CHECK_EQ(std::string_view{third_num_pr.child("w:ilvl").attribute("w:val").value()}, "0");

    const auto first_num_id =
        std::string{first_num_pr.child("w:numId").attribute("w:val").value()};
    CHECK_EQ(first_num_id,
             std::string{second_num_pr.child("w:numId").attribute("w:val").value()});
    CHECK_EQ(first_num_id,
             std::string{third_num_pr.child("w:numId").attribute("w:val").value()});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_numbering_id = reopened.ensure_numbering_definition(definition);
    REQUIRE(reopened_numbering_id.has_value());
    CHECK_EQ(*reopened_numbering_id, *numbering_id);

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    auto inserted = reopened_paragraph.insert_paragraph_after("");
    REQUIRE(inserted.has_next());
    CHECK(reopened.set_paragraph_numbering(inserted, *reopened_numbering_id, 1U));
    CHECK(inserted.add_run("inserted subsection").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE("ensure_numbering_definition updates existing custom numbering definitions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_update.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "PolicyOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1)"},
    };

    const auto initial_numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(initial_numbering_id.has_value());

    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 5U, 0U, "(%1)"},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 1U, "o"},
    };

    const auto updated_numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(updated_numbering_id.has_value());
    CHECK_EQ(*updated_numbering_id, *initial_numbering_id);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_numbering(paragraph, *updated_numbering_id));
    CHECK(paragraph.add_run("policy item").has_next());

    CHECK_FALSE(doc.save());

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});

    const auto abstract_num = find_numbering_abstract_xml_node(numbering_root, "PolicyOutline");
    REQUIRE(abstract_num != pugi::xml_node{});
    CHECK_EQ(std::string_view{abstract_num.attribute("w:abstractNumId").value()},
             std::to_string(*initial_numbering_id));
    CHECK_EQ(count_named_children(abstract_num, "w:lvl"), 2U);

    const auto level_zero = find_numbering_level_xml_node(abstract_num, 0U);
    const auto level_one = find_numbering_level_xml_node(abstract_num, 1U);
    REQUIRE(level_zero != pugi::xml_node{});
    REQUIRE(level_one != pugi::xml_node{});
    CHECK_EQ(std::string_view{level_zero.child("w:start").attribute("w:val").value()}, "5");
    CHECK_EQ(std::string_view{level_zero.child("w:lvlText").attribute("w:val").value()},
             "(%1)");
    CHECK_EQ(std::string_view{level_one.child("w:numFmt").attribute("w:val").value()},
             "bullet");
    CHECK_EQ(std::string_view{level_one.child("w:lvlText").attribute("w:val").value()}, "o");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());
    auto nested = reopened_paragraph.insert_paragraph_after("");
    REQUIRE(nested.has_next());
    CHECK(reopened.set_paragraph_numbering(nested, *updated_numbering_id, 1U));
    CHECK(nested.add_run("policy sub item").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE("list_numbering_definitions and find_numbering_definition expose numbering metadata") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_metadata.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto definition = featherdoc::numbering_definition{};
    definition.name = "LegalOutline";
    definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 3U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(definition);
    REQUIRE(numbering_id.has_value());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_numbering(paragraph, *numbering_id));
    CHECK(paragraph.add_run("legal heading").has_next());

    const auto definitions = doc.list_numbering_definitions();
    CHECK_FALSE(doc.last_error());
    REQUIRE_EQ(definitions.size(), 1U);

    const auto &summary = definitions.front();
    CHECK_EQ(summary.definition_id, *numbering_id);
    CHECK_EQ(summary.name, "LegalOutline");
    REQUIRE_EQ(summary.levels.size(), 2U);
    CHECK_EQ(summary.levels[0].level, 0U);
    CHECK_EQ(summary.levels[0].kind, featherdoc::list_kind::decimal);
    CHECK_EQ(summary.levels[0].start, 3U);
    CHECK_EQ(summary.levels[0].text_pattern, "%1.");
    CHECK_EQ(summary.levels[1].level, 1U);
    CHECK_EQ(summary.levels[1].kind, featherdoc::list_kind::decimal);
    CHECK_EQ(summary.levels[1].start, 1U);
    CHECK_EQ(summary.levels[1].text_pattern, "%1.%2.");
    REQUIRE_EQ(summary.instance_ids.size(), 1U);
    CHECK_NE(summary.instance_ids.front(), 0U);
    REQUIRE_EQ(summary.instances.size(), 1U);
    CHECK_EQ(summary.instances.front().instance_id, summary.instance_ids.front());
    CHECK(summary.instances.front().level_overrides.empty());

    const auto found = doc.find_numbering_definition(*numbering_id);
    CHECK_FALSE(doc.last_error());
    REQUIRE(found.has_value());
    CHECK_EQ(found->definition_id, summary.definition_id);
    CHECK_EQ(found->name, summary.name);
    CHECK_EQ(found->levels.size(), summary.levels.size());
    CHECK_EQ(found->instance_ids, summary.instance_ids);
    REQUIRE_EQ(found->instances.size(), 1U);
    CHECK_EQ(found->instances.front().instance_id, summary.instance_ids.front());
    CHECK(found->instances.front().level_overrides.empty());

    const auto found_instance = doc.find_numbering_instance(summary.instance_ids.front());
    CHECK_FALSE(doc.last_error());
    REQUIRE(found_instance.has_value());
    CHECK_EQ(found_instance->definition_id, summary.definition_id);
    CHECK_EQ(found_instance->definition_name, summary.name);
    CHECK_EQ(found_instance->instance.instance_id, summary.instance_ids.front());
    CHECK(found_instance->instance.level_overrides.empty());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_summary = reopened.find_numbering_definition(*numbering_id);
    CHECK_FALSE(reopened.last_error());
    REQUIRE(reopened_summary.has_value());
    CHECK_EQ(reopened_summary->definition_id, *numbering_id);
    CHECK_EQ(reopened_summary->name, "LegalOutline");
    REQUIRE_EQ(reopened_summary->levels.size(), 2U);
    CHECK_EQ(reopened_summary->levels[0].start, 3U);
    CHECK_EQ(reopened_summary->levels[1].text_pattern, "%1.%2.");
    CHECK_EQ(reopened_summary->instance_ids, summary.instance_ids);
    REQUIRE_EQ(reopened_summary->instances.size(), 1U);
    CHECK_EQ(reopened_summary->instances.front().instance_id, summary.instance_ids.front());
    CHECK(reopened_summary->instances.front().level_overrides.empty());

    const auto reopened_instance =
        reopened.find_numbering_instance(summary.instance_ids.front());
    CHECK_FALSE(reopened.last_error());
    REQUIRE(reopened_instance.has_value());
    CHECK_EQ(reopened_instance->definition_id, *numbering_id);
    CHECK_EQ(reopened_instance->definition_name, "LegalOutline");
    CHECK_EQ(reopened_instance->instance.instance_id, summary.instance_ids.front());
    CHECK(reopened_instance->instance.level_overrides.empty());

    const auto reopened_definitions = reopened.list_numbering_definitions();
    CHECK_FALSE(reopened.last_error());
    REQUIRE_EQ(reopened_definitions.size(), 1U);
    CHECK_EQ(reopened_definitions.front().definition_id, *numbering_id);
    CHECK_EQ(reopened_definitions.front().instance_ids, summary.instance_ids);
    REQUIRE_EQ(reopened_definitions.front().instances.size(), 1U);
    CHECK_EQ(reopened_definitions.front().instances.front().instance_id,
             summary.instance_ids.front());
    CHECK(reopened_definitions.front().instances.front().level_overrides.empty());

    fs::remove(target);
}
