#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

namespace {

void write_numbering_exhaustion_document(
    const std::filesystem::path &path, std::string_view numbering_xml,
    bool include_numbering_content_type = true) {
    constexpr auto content_types = std::string_view{R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>)"};
    constexpr auto document_relationships = std::string_view{R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)"};
    constexpr auto styles_xml = std::string_view{R"(<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:styleId="BoundaryStyle"><w:name w:val="Boundary Style"/></w:style>
</w:styles>)"};

    auto content_types_xml = std::string{content_types};
    if (!include_numbering_content_type) {
        constexpr auto numbering_override = std::string_view{
            "  <Override PartName=\"/word/numbering.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml\"/>\n"};
        if (const auto position = content_types_xml.find(numbering_override);
            position != std::string::npos) {
            content_types_xml.erase(position, numbering_override.size());
        }
    }

    write_test_archive_entries(
        path,
        {{test_content_types_xml_entry, std::move(content_types_xml)},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry,
          R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p/></w:body></w:document>)"},
         {"word/_rels/document.xml.rels",
          std::string{document_relationships}},
         {"word/numbering.xml", std::string{numbering_xml}},
         {"word/styles.xml", std::string{styles_xml}}});
}

auto boundary_numbering_definition(std::string name)
    -> featherdoc::numbering_definition {
    auto definition = featherdoc::numbering_definition{};
    definition.name = std::move(name);
    definition.levels = {featherdoc::numbering_level_definition{
        featherdoc::list_kind::decimal, 1U, 0U, "%1."}};
    return definition;
}

auto contains_numbering_definition(
    const std::vector<featherdoc::numbering_definition_summary> &definitions,
    std::string_view name) -> bool {
    for (const auto &definition : definitions) {
        if (definition.name == name) {
            return true;
        }
    }
    return false;
}

auto catalog_definition(std::string name, std::size_t instance_count = 0U)
    -> featherdoc::numbering_catalog_definition {
    auto definition = featherdoc::numbering_catalog_definition{};
    definition.definition = boundary_numbering_definition(std::move(name));
    definition.instances.resize(instance_count);
    return definition;
}

} // namespace

TEST_CASE(
    "custom numbering definition APIs validate inputs and reject missing definitions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "custom_numbering_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto invalid_definition = featherdoc::numbering_definition{};
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "EmptyLevels";
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "ReservedName";
    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 0U, 0U, "%1."},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 9U, "%1."},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, ""},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 0U, "o"},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    invalid_definition.name = "FeatherDocBulletList";
    invalid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::bullet, 1U, 0U, "o"},
    };
    CHECK_FALSE(doc.ensure_numbering_definition(invalid_definition).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto valid_definition = featherdoc::numbering_definition{};
    valid_definition.name = "ValidOutline";
    valid_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(valid_definition);
    REQUIRE(numbering_id.has_value());

    CHECK_FALSE(doc.set_paragraph_numbering(featherdoc::Paragraph{}, *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.find_numbering_instance(9999U).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, 9999U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, *numbering_id, 1U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.set_paragraph_numbering(paragraph, *numbering_id, 9U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("numbering ID allocation rejects UINT32_MAX and ignores invalid unsigned IDs") {
    namespace fs = std::filesystem;

    const auto make_numbering_document = [](const fs::path &path,
                                             std::string_view numbering_xml) {
        const auto content_types = std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
</Types>)"};
        const auto document_relationships = std::string{R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/>
</Relationships>)"};
        write_test_archive_entries(
            path,
            {{test_content_types_xml_entry, content_types},
             {test_relationships_xml_entry, test_relationships_xml},
             {test_document_xml_entry,
              R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p/></w:body></w:document>)"},
             {"word/_rels/document.xml.rels", document_relationships},
             {"word/numbering.xml", std::string{numbering_xml}}});
    };

    auto definition = featherdoc::numbering_definition{};
    definition.name = "BoundaryDefinition";
    definition.levels = {featherdoc::numbering_level_definition{
        featherdoc::list_kind::decimal, 1U, 0U, "%1."}};

    const auto exhausted_path = fs::current_path() / "numbering_id_exhausted.docx";
    make_numbering_document(
        exhausted_path,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:abstractNum w:abstractNumId="4294967295"><w:name w:val="Existing"/></w:abstractNum></w:numbering>)");
    featherdoc::Document exhausted(exhausted_path);
    REQUIRE_FALSE(exhausted.open());
    CHECK_FALSE(exhausted.ensure_numbering_definition(definition).has_value());
    CHECK_EQ(exhausted.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);
    fs::remove(exhausted_path);

    const auto instance_exhausted_path =
        fs::current_path() / "numbering_instance_id_exhausted.docx";
    make_numbering_document(
        instance_exhausted_path,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:abstractNum w:abstractNumId="1"><w:name w:val="FeatherDocBulletList"/></w:abstractNum><w:abstractNum w:abstractNumId="2"><w:name w:val="Other"/></w:abstractNum><w:num w:numId="4294967295"><w:abstractNumId w:val="2"/></w:num></w:numbering>)");
    featherdoc::Document instance_exhausted(instance_exhausted_path);
    REQUIRE_FALSE(instance_exhausted.open());
    auto target_paragraph = instance_exhausted.paragraphs();
    REQUIRE(target_paragraph.has_next());
    CHECK_FALSE(instance_exhausted.set_paragraph_list(
        target_paragraph, featherdoc::list_kind::bullet));
    CHECK_EQ(instance_exhausted.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);
    fs::remove(instance_exhausted_path);

    const auto invalid_path = fs::current_path() / "numbering_invalid_ids.docx";
    make_numbering_document(
        invalid_path,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:abstractNum w:abstractNumId="-1"><w:name w:val="Negative"/></w:abstractNum><w:abstractNum w:abstractNumId="4294967296"><w:name w:val="Overflow"/></w:abstractNum><w:abstractNum w:abstractNumId="7x"><w:name w:val="Trailing"/></w:abstractNum></w:numbering>)");
    featherdoc::Document invalid_ids(invalid_path);
    REQUIRE_FALSE(invalid_ids.open());
    const auto allocated_id = invalid_ids.ensure_numbering_definition(definition);
    REQUIRE(allocated_id.has_value());
    CHECK_EQ(*allocated_id, 1U);
    fs::remove(invalid_path);
}

TEST_CASE("numbering catalog reserves every abstract ID before mutating the package") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "numbering_catalog_abstract_id_batch_exhausted.docx";
    fs::remove(target);
    write_numbering_exhaustion_document(
        target,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="4294967294"><w:name w:val="Existing"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum>
</w:numbering>)");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());

    auto catalog = featherdoc::numbering_catalog{};
    catalog.definitions = {catalog_definition("RejectedFirst"),
                           catalog_definition("RejectedSecond")};
    const auto rejected = document.import_numbering_catalog(catalog);
    CHECK_EQ(rejected.imported_definition_count, 0U);
    CHECK_EQ(rejected.imported_instance_count, 0U);
    CHECK(rejected.definitions.empty());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);

    const auto definitions_after_failure = document.list_numbering_definitions();
    CHECK_FALSE(contains_numbering_definition(definitions_after_failure,
                                              "RejectedFirst"));
    CHECK_FALSE(contains_numbering_definition(definitions_after_failure,
                                              "RejectedSecond"));

    const auto recovery_id = document.ensure_numbering_definition(
        boundary_numbering_definition("Existing"));
    REQUIRE(recovery_id.has_value());
    CHECK_EQ(*recovery_id, 4294967294U);
    REQUIRE_FALSE(document.save());

    const auto saved_numbering = read_test_docx_entry(target, "word/numbering.xml");
    CHECK_EQ(saved_numbering.find("RejectedFirst"), std::string::npos);
    CHECK_EQ(saved_numbering.find("RejectedSecond"), std::string::npos);
    CHECK_EQ(saved_numbering.find("w:abstractNumId=\"4294967295\""),
             std::string::npos);

    fs::remove(target);
}

TEST_CASE("numbering definition reserves its ID before attaching the package part") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "numbering_definition_attach_before_exhaustion.docx";
    fs::remove(target);
    write_numbering_exhaustion_document(
        target,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="4294967295"><w:name w:val="Existing"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum>
</w:numbering>)",
        false);

    const auto original_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    const auto original_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    const auto original_numbering =
        read_test_docx_entry(target, "word/numbering.xml");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    CHECK_FALSE(document
                    .ensure_numbering_definition(
                        boundary_numbering_definition("RejectedDefinition"))
                    .has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);

    REQUIRE_FALSE(document.save());
    CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry),
             original_content_types);
    CHECK_EQ(read_test_docx_entry(target, "word/_rels/document.xml.rels"),
             original_relationships);
    CHECK_EQ(read_test_docx_entry(target, "word/numbering.xml"),
             original_numbering);

    fs::remove(target);
}

TEST_CASE("numbering catalog reserves every instance ID before mutating the package") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "numbering_catalog_instance_id_batch_exhausted.docx";
    fs::remove(target);
    write_numbering_exhaustion_document(
        target,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="1"><w:name w:val="Existing"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum>
  <w:num w:numId="4294967294"><w:abstractNumId w:val="1"/></w:num>
</w:numbering>)");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());

    auto catalog = featherdoc::numbering_catalog{};
    catalog.definitions = {catalog_definition("RejectedInstances", 2U)};
    const auto rejected = document.import_numbering_catalog(catalog);
    CHECK_EQ(rejected.imported_definition_count, 0U);
    CHECK_EQ(rejected.imported_instance_count, 0U);
    CHECK(rejected.definitions.empty());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);

    const auto definitions_after_failure = document.list_numbering_definitions();
    CHECK_FALSE(contains_numbering_definition(definitions_after_failure,
                                              "RejectedInstances"));

    const auto recovery_id = document.ensure_numbering_definition(
        boundary_numbering_definition("Existing"));
    REQUIRE(recovery_id.has_value());
    CHECK_EQ(*recovery_id, 1U);
    REQUIRE_FALSE(document.save());

    const auto saved_numbering = read_test_docx_entry(target, "word/numbering.xml");
    CHECK_EQ(saved_numbering.find("RejectedInstances"), std::string::npos);
    CHECK_EQ(saved_numbering.find("w:numId=\"4294967295\""),
             std::string::npos);

    fs::remove(target);
}

TEST_CASE("numbering lookups compare valid numeric IDs including leading zeroes") {
    namespace fs = std::filesystem;

    const auto target = fs::current_path() / "numbering_leading_zero_ids.docx";
    fs::remove(target);
    write_numbering_exhaustion_document(
        target,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="0007"><w:name w:val="LeadingZero"/><w:lvl w:ilvl="01"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1.%2."/></w:lvl></w:abstractNum>
  <w:num w:numId="0009"><w:abstractNumId w:val="0007"/></w:num>
</w:numbering>)");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());

    const auto definition = document.find_numbering_definition(7U);
    REQUIRE(definition.has_value());
    CHECK_EQ(definition->definition_id, 7U);
    CHECK_EQ(definition->name, "LeadingZero");

    const auto instance = document.find_numbering_instance(9U);
    REQUIRE(instance.has_value());
    CHECK_EQ(instance->definition_id, 7U);
    CHECK_EQ(instance->instance.instance_id, 9U);

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(document.set_paragraph_numbering(paragraph, 7U, 1U));
    const auto inspected = document.inspect_paragraph(0U);
    REQUIRE(inspected.has_value());
    REQUIRE(inspected->numbering.has_value());
    REQUIRE(inspected->numbering->num_id.has_value());
    CHECK_EQ(*inspected->numbering->num_id, 9U);
    REQUIRE(inspected->numbering->level.has_value());
    CHECK_EQ(*inspected->numbering->level, 1U);

    fs::remove(target);
}

TEST_CASE("numbering instance reuse skips malformed candidates before a valid match") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "numbering_malformed_first_instance.docx";
    fs::remove(target);
    write_numbering_exhaustion_document(
        target,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="3"><w:name w:val="Reusable"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum>
  <w:num w:numId="invalid"><w:abstractNumId w:val="3"/></w:num>
  <w:num w:numId="27"><w:abstractNumId w:val="3"/></w:num>
</w:numbering>)");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(document.set_paragraph_numbering(paragraph, 3U, 0U));

    const auto inspected = document.inspect_paragraph(0U);
    REQUIRE(inspected.has_value());
    REQUIRE(inspected->numbering.has_value());
    REQUIRE(inspected->numbering->num_id.has_value());
    CHECK_EQ(*inspected->numbering->num_id, 27U);

    REQUIRE_FALSE(document.save());
    const auto saved_numbering = read_test_docx_entry(target, "word/numbering.xml");
    CHECK_EQ(count_substring_occurrences(saved_numbering, "<w:num "), 2U);
    CHECK_EQ(saved_numbering.find("w:numId=\"28\""), std::string::npos);

    fs::remove(target);
}

TEST_CASE("numbering instance ID exhaustion is reported by direct paragraph APIs") {
    namespace fs = std::filesystem;

    const auto numbering_xml = std::string_view{R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="1"><w:name w:val="Target"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum>
  <w:abstractNum w:abstractNumId="2"><w:name w:val="Occupied"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum>
  <w:num w:numId="4294967295"><w:abstractNumId w:val="2"/></w:num>
</w:numbering>)"};

    SUBCASE("paragraph numbering") {
        const auto target =
            fs::current_path() / "paragraph_numbering_num_id_exhausted.docx";
        fs::remove(target);
        write_numbering_exhaustion_document(target, numbering_xml);

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        CHECK_FALSE(document.set_paragraph_numbering(paragraph, 1U, 0U));
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::identifier_space_exhausted);
        const auto inspected_paragraph = document.inspect_paragraph(0U);
        REQUIRE(inspected_paragraph.has_value());
        CHECK_FALSE(inspected_paragraph->numbering.has_value());

        fs::remove(target);
    }

    SUBCASE("paragraph style numbering") {
        const auto target =
            fs::current_path() / "style_numbering_num_id_exhausted.docx";
        fs::remove(target);
        write_numbering_exhaustion_document(target, numbering_xml);

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        CHECK_FALSE(
            document.set_paragraph_style_numbering("BoundaryStyle", 1U, 0U));
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::identifier_space_exhausted);
        const auto style = document.find_style("BoundaryStyle");
        REQUIRE(style.has_value());
        CHECK_FALSE(style->numbering.has_value());

        fs::remove(target);
    }
}

TEST_CASE("style-linked numbering reserves IDs before mutating styles or numbering") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "style_linked_numbering_num_id_exhausted.docx";
    fs::remove(target);
    write_numbering_exhaustion_document(
        target,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="2"><w:name w:val="Occupied"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum>
  <w:num w:numId="4294967295"><w:abstractNumId w:val="2"/></w:num>
</w:numbering>)");

    {
        featherdoc::Document normalize_package(target);
        REQUIRE_FALSE(normalize_package.open());
        REQUIRE_FALSE(normalize_package.save());
    }

    const auto original_numbering =
        read_test_docx_entry(target, "word/numbering.xml");
    const auto original_styles = read_test_docx_entry(target, "word/styles.xml");
    const auto original_content_types =
        read_test_docx_entry(target, test_content_types_xml_entry);
    const auto original_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    const auto definition =
        boundary_numbering_definition("RejectedStyleLinkedDefinition");
    CHECK_FALSE(
        document
            .ensure_style_linked_numbering(
                definition,
                {featherdoc::paragraph_style_numbering_link{"BoundaryStyle",
                                                             0U}})
            .has_value());
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);

    const auto definitions = document.list_numbering_definitions();
    CHECK_FALSE(contains_numbering_definition(
        definitions, "RejectedStyleLinkedDefinition"));
    const auto style = document.find_style("BoundaryStyle");
    REQUIRE(style.has_value());
    CHECK_FALSE(style->numbering.has_value());

    REQUIRE_FALSE(document.save());
    CHECK_EQ(read_test_docx_entry(target, "word/numbering.xml"),
             original_numbering);
    CHECK_EQ(read_test_docx_entry(target, "word/styles.xml"), original_styles);
    CHECK_EQ(read_test_docx_entry(target, test_content_types_xml_entry),
             original_content_types);
    CHECK_EQ(read_test_docx_entry(target, "word/_rels/document.xml.rels"),
             original_relationships);

    fs::remove(target);
}

TEST_CASE("managed list creation reserves abstract and instance IDs atomically") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "managed_list_num_id_exhausted_atomic.docx";
    fs::remove(target);
    write_numbering_exhaustion_document(
        target,
        R"(<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="2"><w:name w:val="Occupied"/><w:lvl w:ilvl="0"><w:start w:val="1"/><w:numFmt w:val="decimal"/><w:lvlText w:val="%1."/></w:lvl></w:abstractNum>
  <w:num w:numId="4294967295"><w:abstractNumId w:val="2"/></w:num>
</w:numbering>)");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK_FALSE(document.set_paragraph_list(
        paragraph, featherdoc::list_kind::bullet, 0U));
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);

    auto definitions = document.list_numbering_definitions();
    CHECK_FALSE(
        contains_numbering_definition(definitions, "FeatherDocBulletList"));
    const auto inspected_paragraph = document.inspect_paragraph(0U);
    REQUIRE(inspected_paragraph.has_value());
    CHECK_FALSE(inspected_paragraph->numbering.has_value());

    const auto recovery_definition =
        boundary_numbering_definition("RecoveryDefinition");
    const auto recovery_id =
        document.ensure_numbering_definition(recovery_definition);
    REQUIRE(recovery_id.has_value());
    CHECK_EQ(*recovery_id, 3U);
    REQUIRE_FALSE(document.save());

    const auto saved_numbering =
        read_test_docx_entry(target, "word/numbering.xml");
    CHECK_EQ(saved_numbering.find("FeatherDocBulletList"), std::string::npos);
    CHECK_NE(saved_numbering.find("RecoveryDefinition"), std::string::npos);

    fs::remove(target);
}

TEST_CASE(
    "set_paragraph_style_numbering links custom numbering definitions to paragraph styles") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_numbering_roundtrip.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Legal Heading";
    paragraph_style.based_on = std::string{"Heading1"};
    paragraph_style.paragraph_bidi = false;
    CHECK(doc.ensure_paragraph_style("LegalHeading", paragraph_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "LegalHeadingOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 1U, "%1.%2."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());
    CHECK(doc.set_paragraph_style_numbering("LegalHeading", *numbering_id, 1U));

    const auto style_summary = doc.find_style("LegalHeading");
    REQUIRE(style_summary.has_value());
    REQUIRE(style_summary->numbering.has_value());
    REQUIRE(style_summary->numbering->num_id.has_value());
    REQUIRE(style_summary->numbering->level.has_value());
    CHECK_EQ(*style_summary->numbering->level, 1U);
    REQUIRE(style_summary->numbering->definition_id.has_value());
    CHECK_EQ(*style_summary->numbering->definition_id, *numbering_id);
    REQUIRE(style_summary->numbering->definition_name.has_value());
    CHECK_EQ(*style_summary->numbering->definition_name, "LegalHeadingOutline");
    REQUIRE(style_summary->numbering->instance.has_value());
    CHECK_EQ(style_summary->numbering->instance->instance_id,
             *style_summary->numbering->num_id);
    CHECK(style_summary->numbering->instance->level_overrides.empty());

    const auto styles = doc.list_styles();
    const auto *listed_style = find_style_summary(styles, "LegalHeading");
    REQUIRE(listed_style != nullptr);
    REQUIRE(listed_style->numbering.has_value());
    REQUIRE(listed_style->numbering->instance.has_value());
    CHECK_EQ(listed_style->numbering->instance->instance_id,
             *style_summary->numbering->num_id);

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(doc.set_paragraph_style(paragraph, "LegalHeading"));
    CHECK(paragraph.add_run("styled numbered heading").has_next());

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto style = find_style_xml_node(styles_root, "LegalHeading");
    REQUIRE(style != pugi::xml_node{});
    const auto style_num_pr = style.child("w:pPr").child("w:numPr");
    REQUIRE(style_num_pr != pugi::xml_node{});
    CHECK_EQ(std::string_view{style_num_pr.child("w:ilvl").attribute("w:val").value()}, "1");
    CHECK_NE(std::string_view{style_num_pr.child("w:numId").attribute("w:val").value()}, "");
    CHECK(style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document document_xml;
    REQUIRE(document_xml.load_string(saved_document_xml.c_str()));
    const auto body = document_xml.child("w:document").child("w:body");
    const auto first_paragraph = body.child("w:p");
    REQUIRE(first_paragraph != pugi::xml_node{});
    CHECK(first_paragraph.child("w:pPr").child("w:pStyle") != pugi::xml_node{});
    CHECK(first_paragraph.child("w:pPr").child("w:numPr") == pugi::xml_node{});

    const auto numbering_xml = read_test_docx_entry(target, "word/numbering.xml");
    pugi::xml_document numbering_document;
    REQUIRE(numbering_document.load_string(numbering_xml.c_str()));
    const auto numbering_root = numbering_document.child("w:numbering");
    REQUIRE(numbering_root != pugi::xml_node{});
    CHECK_EQ(count_substring_occurrences(numbering_xml, "<w:num "), 1U);
    CHECK(find_numbering_abstract_xml_node(numbering_root, "LegalHeadingOutline") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_style = reopened.find_style("LegalHeading");
    REQUIRE(reopened_style.has_value());
    REQUIRE(reopened_style->numbering.has_value());
    REQUIRE(reopened_style->numbering->num_id.has_value());
    CHECK_EQ(*reopened_style->numbering->num_id, *style_summary->numbering->num_id);
    REQUIRE(reopened_style->numbering->level.has_value());
    CHECK_EQ(*reopened_style->numbering->level, 1U);
    REQUIRE(reopened_style->numbering->definition_id.has_value());
    CHECK_EQ(*reopened_style->numbering->definition_id, *numbering_id);
    REQUIRE(reopened_style->numbering->definition_name.has_value());
    CHECK_EQ(*reopened_style->numbering->definition_name, "LegalHeadingOutline");
    REQUIRE(reopened_style->numbering->instance.has_value());
    CHECK_EQ(reopened_style->numbering->instance->instance_id,
             *reopened_style->numbering->num_id);
    CHECK(reopened_style->numbering->instance->level_overrides.empty());
    auto inserted = reopened.paragraphs().insert_paragraph_after("");
    REQUIRE(inserted.has_next());
    CHECK(reopened.set_paragraph_style(inserted, "LegalHeading"));
    CHECK(inserted.add_run("reopened styled heading").has_next());
    CHECK_FALSE(reopened.save());

    fs::remove(target);
}

TEST_CASE(
    "paragraph style numbering APIs validate inputs and clear numbering without removing unrelated markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "style_numbering_validation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph_style = featherdoc::paragraph_style_definition{};
    paragraph_style.name = "Body Numbered";
    paragraph_style.paragraph_bidi = true;
    CHECK(doc.ensure_paragraph_style("BodyNumbered", paragraph_style));

    auto numbering_definition = featherdoc::numbering_definition{};
    numbering_definition.name = "BodyOutline";
    numbering_definition.levels = {
        featherdoc::numbering_level_definition{
            featherdoc::list_kind::decimal, 1U, 0U, "%1."},
    };

    const auto numbering_id = doc.ensure_numbering_definition(numbering_definition);
    REQUIRE(numbering_id.has_value());

    CHECK_FALSE(doc.set_paragraph_style_numbering("", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("MissingStyle", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("Emphasis", *numbering_id));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("BodyNumbered", 9999U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.set_paragraph_style_numbering("BodyNumbered", *numbering_id, 1U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK(doc.set_paragraph_style_numbering("BodyNumbered", *numbering_id));
    CHECK(doc.clear_paragraph_style_numbering("BodyNumbered"));
    CHECK_FALSE(doc.clear_paragraph_style_numbering(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.clear_paragraph_style_numbering("MissingStyle"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.clear_paragraph_style_numbering("Emphasis"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    CHECK_FALSE(doc.save());

    const auto saved_styles_xml = read_test_docx_entry(target, "word/styles.xml");
    pugi::xml_document styles_document;
    REQUIRE(styles_document.load_string(saved_styles_xml.c_str()));
    const auto styles_root = styles_document.child("w:styles");
    REQUIRE(styles_root != pugi::xml_node{});

    const auto style = find_style_xml_node(styles_root, "BodyNumbered");
    REQUIRE(style != pugi::xml_node{});
    CHECK(style.child("w:pPr").child("w:numPr") == pugi::xml_node{});
    CHECK(style.child("w:pPr").child("w:bidi") != pugi::xml_node{});

    fs::remove(target);
}
