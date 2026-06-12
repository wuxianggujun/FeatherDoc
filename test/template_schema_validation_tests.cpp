#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("template schema patch review summary writes stable json") {
    featherdoc::template_schema baseline{{
        {{}, {"customer", featherdoc::template_slot_kind::text, false}},
        {{}, {"obsolete", featherdoc::template_slot_kind::text, true}},
    }};

    featherdoc::template_schema generated{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"order_no", featherdoc::template_slot_kind::text, true}},
    }};
    generated.entries.back().requirement.source =
        featherdoc::template_slot_source_kind::content_control_tag;

    const auto summary = featherdoc::make_template_schema_patch_review_summary(
        baseline, generated);
    CHECK_EQ(summary.baseline_slot_count, 2U);
    REQUIRE(summary.generated_slot_count.has_value());
    CHECK_EQ(*summary.generated_slot_count, 2U);
    CHECK_EQ(summary.patch_upsert_slot_count, 1U);
    CHECK_EQ(summary.patch_remove_target_count, 0U);
    CHECK_EQ(summary.patch_remove_slot_count, 1U);
    CHECK_EQ(summary.patch_rename_slot_count, 0U);
    CHECK_EQ(summary.patch_update_slot_count, 1U);
    CHECK_EQ(summary.preview.removed_slots, 1U);
    CHECK_EQ(summary.preview.inserted_slots, 1U);
    CHECK_EQ(summary.preview.replaced_slots, 1U);
    CHECK(summary.changed());

    CHECK_EQ(
        featherdoc::template_schema_patch_review_json(summary),
        std::string{
            R"({"schema":"featherdoc.template_schema_patch_review.v1","baseline_slot_count":2,"generated_slot_count":2,"patch":{"upsert_slot_count":1,"remove_target_count":0,"remove_slot_count":1,"rename_slot_count":0,"update_slot_count":1},"preview":{"removed_targets":0,"removed_slots":1,"renamed_slots":0,"inserted_slots":1,"replaced_slots":1,"changed":true},"changed":true})"});

    const auto patch = featherdoc::build_template_schema_patch(baseline, generated);
    const auto patch_summary = featherdoc::make_template_schema_patch_review_summary(
        baseline, patch);
    CHECK_FALSE(patch_summary.generated_slot_count.has_value());
    CHECK_NE(featherdoc::template_schema_patch_review_json(patch_summary)
                 .find(R"("generated_slot_count":null)"),
             std::string::npos);
}

TEST_CASE("rename_template_schema_target moves slots and merges conflicts") {
    const auto find_slot = [](const featherdoc::template_schema &current,
                              featherdoc::template_schema_part_kind part,
                              std::string_view bookmark_name)
        -> const featherdoc::template_schema_entry * {
        for (const auto &entry : current.entries) {
            if (entry.target.part == part &&
                entry.requirement.bookmark_name == bookmark_name) {
                return &entry;
            }
        }
        return nullptr;
    };

    featherdoc::template_schema_part_selector header_target{};
    header_target.part = featherdoc::template_schema_part_kind::header;
    featherdoc::template_schema_part_selector footer_target{};
    footer_target.part = featherdoc::template_schema_part_kind::footer;

    featherdoc::template_schema schema{{
        {{}, {"customer", featherdoc::template_slot_kind::text, true}},
        {{}, {"line_items", featherdoc::template_slot_kind::table_rows, true}},
        {header_target, {"customer", featherdoc::template_slot_kind::text, false}},
        {header_target, {"header_title", featherdoc::template_slot_kind::text, true}},
        {footer_target, {"footer_note", featherdoc::template_slot_kind::block, true}},
    }};

    const auto summary = featherdoc::rename_template_schema_target(
        schema, featherdoc::template_schema_part_selector{}, header_target);
    CHECK_EQ(summary.removed_targets, 1U);
    CHECK_EQ(summary.inserted_slots, 1U);
    CHECK_EQ(summary.replaced_slots, 1U);

    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "customer") == nullptr);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::body,
                    "line_items") == nullptr);

    const auto *header_customer = find_slot(
        schema, featherdoc::template_schema_part_kind::header, "customer");
    REQUIRE(header_customer != nullptr);
    CHECK(header_customer->requirement.required);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::header,
                    "line_items") != nullptr);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::header,
                    "header_title") != nullptr);
    CHECK(find_slot(schema, featherdoc::template_schema_part_kind::footer,
                    "footer_note") != nullptr);

    const auto noop_summary = featherdoc::rename_template_schema_target(
        schema, header_target, header_target);
    CHECK_FALSE(noop_summary.changed());

    featherdoc::template_schema_part_selector missing_target{};
    missing_target.part = featherdoc::template_schema_part_kind::section_footer;
    const auto missing_summary = featherdoc::rename_template_schema_target(
        schema, missing_target, footer_target);
    CHECK_FALSE(missing_summary.changed());
}

TEST_CASE("validate_template_schema aggregates section header and footer validation") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_validate_parts.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml"
           ContentType="application/xml"/>
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
    <w:p><w:r><w:t>Schema Validation Fixture</w:t></w:r></w:p>
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
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Footer company A: </w:t></w:r>
    <w:bookmarkStart w:id="1" w:name="footer_company"/>
    <w:r><w:t>placeholder A</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
  <w:p>
    <w:r><w:t>Footer company B: </w:t></w:r>
    <w:bookmarkStart w:id="2" w:name="footer_company"/>
    <w:r><w:t>placeholder B</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
  <w:p>
    <w:r><w:t>Summary: prefix </w:t></w:r>
    <w:bookmarkStart w:id="3" w:name="footer_summary"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="3"/>
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

    featherdoc::template_schema schema{
        {
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt, 0U,
                 featherdoc::section_reference_kind::default_reference},
                {"header_title", featherdoc::template_slot_kind::text, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 0U,
                 featherdoc::section_reference_kind::default_reference},
                {"footer_company", featherdoc::template_slot_kind::text, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 0U,
                 featherdoc::section_reference_kind::default_reference},
                {"footer_summary", featherdoc::template_slot_kind::block, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 0U,
                 featherdoc::section_reference_kind::default_reference},
                {"footer_signature", featherdoc::template_slot_kind::text, true},
            },
        }};

    const auto result = doc.validate_template_schema(schema);

    CHECK_FALSE(doc.last_error());
    REQUIRE(result.part_results.size() == 2U);
    CHECK(result.part_results[0].available);
    CHECK_EQ(result.part_results[0].entry_name, "word/header1.xml");
    CHECK(static_cast<bool>(result.part_results[0]));
    CHECK_EQ(result.part_results[0].target.part,
             featherdoc::template_schema_part_kind::section_header);
    REQUIRE(result.part_results[0].target.section_index.has_value());
    CHECK_EQ(*result.part_results[0].target.section_index, 0U);

    CHECK(result.part_results[1].available);
    CHECK_EQ(result.part_results[1].entry_name, "word/footer1.xml");
    CHECK_FALSE(static_cast<bool>(result.part_results[1]));
    CHECK_EQ(result.part_results[1].target.part,
             featherdoc::template_schema_part_kind::section_footer);
    REQUIRE(result.part_results[1].target.section_index.has_value());
    CHECK_EQ(*result.part_results[1].target.section_index, 0U);
    REQUIRE(result.part_results[1].validation.missing_required.size() == 1U);
    CHECK_EQ(result.part_results[1].validation.missing_required.front(),
             "footer_signature");
    REQUIRE(result.part_results[1].validation.duplicate_bookmarks.size() == 1U);
    CHECK_EQ(result.part_results[1].validation.duplicate_bookmarks.front(),
             "footer_company");
    REQUIRE(result.part_results[1].validation.malformed_placeholders.size() == 1U);
    CHECK_EQ(result.part_results[1].validation.malformed_placeholders.front(),
             "footer_summary");
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template_schema resolves linked-to-previous section targets") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_validate_resolved.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml"
           ContentType="application/xml"/>
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
    <w:p><w:r><w:t>Section 0 body</w:t></w:r></w:p>
    <w:p>
      <w:r><w:t>Section break</w:t></w:r>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
    </w:p>
    <w:p><w:r><w:t>Section 1 body</w:t></w:r></w:p>
    <w:sectPr/>
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
</w:hdr>
)";

    const std::string footer_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Footer company: </w:t></w:r>
    <w:bookmarkStart w:id="1" w:name="footer_company"/>
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

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.validate_template_schema(
        {
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt, 1U,
                 featherdoc::section_reference_kind::default_reference},
                {"header_title", featherdoc::template_slot_kind::text, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 1U,
                 featherdoc::section_reference_kind::default_reference},
                {"footer_company", featherdoc::template_slot_kind::text, true},
            },
        });

    CHECK_FALSE(doc.last_error());
    REQUIRE(result.part_results.size() == 2U);
    CHECK(result.part_results[0].available);
    CHECK_EQ(result.part_results[0].entry_name, "word/header1.xml");
    CHECK(static_cast<bool>(result.part_results[0]));
    REQUIRE(result.part_results[0].target.section_index.has_value());
    CHECK_EQ(*result.part_results[0].target.section_index, 1U);

    CHECK(result.part_results[1].available);
    CHECK_EQ(result.part_results[1].entry_name, "word/footer1.xml");
    CHECK(static_cast<bool>(result.part_results[1]));
    REQUIRE(result.part_results[1].target.section_index.has_value());
    CHECK_EQ(*result.part_results[1].target.section_index, 1U);
    CHECK(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template_schema reports unavailable parts without failing optional-only groups") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_validate_unavailable.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto result = doc.validate_template_schema(
        {
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt, 1U,
                 featherdoc::section_reference_kind::default_reference},
                {"optional_header", featherdoc::template_slot_kind::text, false},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt, 1U,
                 featherdoc::section_reference_kind::default_reference},
                {"required_footer", featherdoc::template_slot_kind::text, true},
            },
        });

    CHECK_FALSE(doc.last_error());
    REQUIRE(result.part_results.size() == 2U);
    CHECK_FALSE(result.part_results[0].available);
    CHECK(result.part_results[0].entry_name.empty());
    CHECK(result.part_results[0].validation.missing_required.empty());
    CHECK(static_cast<bool>(result.part_results[0]));

    CHECK_FALSE(result.part_results[1].available);
    CHECK(result.part_results[1].entry_name.empty());
    REQUIRE(result.part_results[1].validation.missing_required.size() == 1U);
    CHECK_EQ(result.part_results[1].validation.missing_required.front(),
             "required_footer");
    CHECK_FALSE(static_cast<bool>(result.part_results[1]));
    CHECK_FALSE(static_cast<bool>(result));

    fs::remove(target);
}

TEST_CASE("validate_template_schema rejects invalid selectors") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_schema_validate_invalid.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto result = doc.validate_template_schema(
        {{
            {featherdoc::template_schema_part_kind::section_header, std::nullopt,
             std::nullopt, featherdoc::section_reference_kind::default_reference},
            {"header_title", featherdoc::template_slot_kind::text, true},
        }});

    CHECK(result.part_results.empty());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail,
             "section-header/section-footer schema target requires section_index");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}
