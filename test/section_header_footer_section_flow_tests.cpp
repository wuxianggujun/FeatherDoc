#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "doctest.h"
#include "allocation_failure_test_case.hpp"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

namespace {

pugi::allocation_function section_move_delegated_allocate = nullptr;
std::size_t section_move_allocation_calls = 0U;
std::size_t section_move_failure_call = 0U;

auto controlled_section_move_allocate(std::size_t size) -> void * {
    ++section_move_allocation_calls;
    if (section_move_failure_call != 0U &&
        section_move_allocation_calls == section_move_failure_call) {
        return nullptr;
    }
    return section_move_delegated_allocate(size);
}

class section_move_pugi_allocator_guard final {
  public:
    section_move_pugi_allocator_guard()
        : previous_allocate_(pugi::get_memory_allocation_function()),
          previous_deallocate_(pugi::get_memory_deallocation_function()) {
        section_move_delegated_allocate = this->previous_allocate_;
        section_move_allocation_calls = 0U;
        section_move_failure_call = 0U;
        pugi::set_memory_management_functions(
            controlled_section_move_allocate, this->previous_deallocate_);
    }

    section_move_pugi_allocator_guard(
        const section_move_pugi_allocator_guard &) = delete;
    auto operator=(const section_move_pugi_allocator_guard &)
        -> section_move_pugi_allocator_guard & = delete;

    ~section_move_pugi_allocator_guard() {
        pugi::set_memory_management_functions(this->previous_allocate_,
                                              this->previous_deallocate_);
        section_move_delegated_allocate = nullptr;
        section_move_allocation_calls = 0U;
        section_move_failure_call = 0U;
    }

  private:
    pugi::allocation_function previous_allocate_;
    pugi::deallocation_function previous_deallocate_;
};

void append_section_body_paragraph(featherdoc::Document &document,
                                   const std::string &text) {
    auto paragraph = document.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }
    REQUIRE(paragraph.insert_paragraph_after(text).has_next());
}

auto make_section_move_allocation_fixture() -> featherdoc::Document {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    const auto large_text = std::string(70U * 1024U, 'x');
    REQUIRE(document.paragraphs().add_run("section zero " + large_text)
                .has_next());
    auto header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.has_next());
    REQUIRE(header.add_run("retained header").has_next());

    REQUIRE(document.append_section(false));
    append_section_body_paragraph(document, "section one " + large_text);
    REQUIRE(document.append_section(false));
    append_section_body_paragraph(document, "section two " + large_text);
    return document;
}

auto section_body_texts(featherdoc::Document &document)
    -> std::vector<std::string> {
    std::vector<std::string> result;
    for (auto paragraph = document.paragraphs(); paragraph.has_next();
         paragraph.next()) {
        std::string text;
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            text += run.get_text();
        }
        if (!text.empty()) {
            result.push_back(std::move(text));
        }
    }
    return result;
}

enum class section_lifecycle_operation {
    append,
    insert,
    remove_final,
};

auto section_lifecycle_operation_name(section_lifecycle_operation operation)
    -> std::string_view {
    switch (operation) {
    case section_lifecycle_operation::append:
        return "append";
    case section_lifecycle_operation::insert:
        return "insert";
    case section_lifecycle_operation::remove_final:
        return "remove-final";
    }
    return "unknown";
}

auto perform_section_lifecycle_operation(
    featherdoc::Document &document, section_lifecycle_operation operation)
    -> bool {
    switch (operation) {
    case section_lifecycle_operation::append:
        return document.append_section(false);
    case section_lifecycle_operation::insert:
        return document.insert_section(0U, false);
    case section_lifecycle_operation::remove_final:
        return document.remove_section(document.section_count() - 1U);
    }
    return false;
}

} // namespace

TEST_CASE("move header and footer parts reorders logical indices and persists after reopen") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "move_header_footer_parts.docx";
    fs::remove(target);

    const std::string content_types_xml =
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
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
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
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(doc.footer_paragraphs(1).runs().get_text(), "section 2 footer");

    CHECK(doc.move_header_part(1, 0));
    CHECK(doc.move_footer_part(1, 0));
    CHECK_EQ(doc.header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.footer_paragraphs(0).runs().get_text(), "section 2 footer");
    CHECK_EQ(doc.footer_paragraphs(1).runs().get_text(), "section 1 footer");

    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(doc.section_footer_paragraphs(1).runs().get_text(), "section 2 footer");
    CHECK_FALSE(doc.save());

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK(saved_relationships.find("Target=\"header2.xml\"") <
          saved_relationships.find("Target=\"header1.xml\""));
    CHECK(saved_relationships.find("Target=\"footer2.xml\"") <
          saved_relationships.find("Target=\"footer1.xml\""));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(reopened.header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.footer_paragraphs(0).runs().get_text(), "section 2 footer");
    CHECK_EQ(reopened.footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(reopened.section_footer_paragraphs(0).runs().get_text(), "section 1 footer");
    CHECK_EQ(reopened.section_footer_paragraphs(1).runs().get_text(), "section 2 footer");

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.move_header_part(0, 0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    featherdoc::Document invalid_range(target);
    CHECK_FALSE(invalid_range.open());
    CHECK_FALSE(invalid_range.move_header_part(2, 0));
    CHECK_EQ(invalid_range.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_range.move_footer_part(0, 2));
    CHECK_EQ(invalid_range.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("copying section header and footer references replaces target layout") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "copy_section_header_footer_refs.docx";
    fs::remove(target);

    const std::string content_types_xml =
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
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/settings.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rId2"/>
          <w:footerReference w:type="default" r:id="rId3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId4"/>
      <w:headerReference w:type="even" r:id="rId4"/>
      <w:footerReference w:type="default" r:id="rId5"/>
      <w:footerReference w:type="first" r:id="rId5"/>
      <w:titlePg w:val="0"/>
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
  <Relationship Id="rId4"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rId5"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
  <Relationship Id="rId6"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings"
                Target="settings.xml"/>
</Relationships>
)";

    const std::string header1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string header2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 header</w:t></w:r></w:p>
</w:hdr>
)";
    const std::string footer1_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string footer2_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 2 footer</w:t></w:r></w:p>
</w:ftr>
)";
    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:evenAndOddHeaders w:val="0"/>
</w:settings>
)";

    write_test_archive_entries(
        target,
        {
            {test_content_types_xml_entry, content_types_xml},
            {test_relationships_xml_entry, test_relationships_xml},
            {test_document_xml_entry, document_xml},
            {"word/_rels/document.xml.rels", document_relationships_xml},
            {"word/header1.xml", header1_xml},
            {"word/header2.xml", header2_xml},
            {"word/footer1.xml", footer1_xml},
            {"word/footer2.xml", footer2_xml},
            {"word/settings.xml", settings_xml},
        });

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.copy_section_header_references(0, 1));
    CHECK(doc.copy_section_footer_references(0, 1));
    CHECK_FALSE(doc.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto section1_properties =
        saved_document.child("w:document").child("w:body").child("w:sectPr");
    REQUIRE(section1_properties != pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type",
                                                         "default")
                 .attribute("r:id")
                 .value(),
             std::string{"rId2"});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "default")
                 .attribute("r:id")
                 .value(),
             std::string{"rId3"});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section1_properties.find_child_by_attribute("w:footerReference", "w:type",
                                                         "first"),
             pugi::xml_node{});
    CHECK(section1_properties.child("w:titlePg") != pugi::xml_node{});
    CHECK_EQ(std::string_view{section1_properties.child("w:titlePg")
                                  .attribute("w:val")
                                  .value()},
             "0");

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});
    CHECK_EQ(std::string_view{saved_settings.child("w:settings")
                                  .child("w:evenAndOddHeaders")
                                  .attribute("w:val")
                                  .value()},
             "0");

    const auto saved_relationships =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(saved_relationships.find("Target=\"header1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"header2.xml\""), std::string::npos);
    CHECK_NE(saved_relationships.find("Target=\"footer1.xml\""), std::string::npos);
    CHECK_EQ(saved_relationships.find("Target=\"footer2.xml\""), std::string::npos);

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK(test_docx_entry_exists(target, "word/footer1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer2.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(1).runs().get_text(), "section 1 footer");
    CHECK_FALSE(reopened.section_header_paragraphs(
        1, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.copy_section_header_references(0, 1));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("append section can inherit or reset header and footer references") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "append_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(default_header.has_next());
    CHECK(default_header.add_run("default header").has_next());

    auto even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.has_next());
    CHECK(even_header.add_run("even header").has_next());

    auto first_footer = doc.ensure_section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_footer.has_next());
    CHECK(first_footer.add_run("first footer").has_next());

    CHECK(doc.append_section());
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "default header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");

    CHECK(doc.append_section(false));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    const auto linked_section = doc.inspect_section(2U);
    REQUIRE(linked_section.has_value());
    CHECK(linked_section->different_first_page_enabled);
    CHECK(linked_section->footer.first_linked_to_previous);
    REQUIRE(linked_section->footer.resolved_first_section_index.has_value());
    CHECK_EQ(*linked_section->footer.resolved_first_section_index, 1U);
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 3);

    CHECK(section_nodes[1].child("w:titlePg") != pugi::xml_node{});
    CHECK(section_nodes[2].child("w:titlePg") != pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type",
                                                      "default"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:footerReference", "w:type",
                                                      "first"),
             pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 3);
    CHECK_EQ(reopened.header_count(), 2);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "default header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "even header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 1, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "first footer");
    CHECK_FALSE(reopened.section_header_paragraphs(2).has_next());
    CHECK_FALSE(reopened.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    const auto reopened_linked_section = reopened.inspect_section(2U);
    REQUIRE(reopened_linked_section.has_value());
    CHECK(reopened_linked_section->different_first_page_enabled);
    CHECK(reopened_linked_section->footer.first_linked_to_previous);

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.append_section());
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("insert section can split layout inheritance or reset references") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto section0_default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.add_run("section 0 header").has_next());

    auto section0_even_header = doc.ensure_section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page);
    REQUIRE(section0_even_header.has_next());
    CHECK(section0_even_header.add_run("section 0 even header").has_next());

    CHECK(doc.append_section(false));
    CHECK_EQ(doc.section_count(), 2);

    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 1 header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.insert_section(0));
    CHECK_EQ(doc.section_count(), 3);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 0 even header");
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK(doc.insert_section(1, false));
    CHECK_EQ(doc.section_count(), 4);
    CHECK_FALSE(doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(doc.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(doc.section_header_paragraphs(3).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 3, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK(doc.insert_section(3));
    CHECK_EQ(doc.section_count(), 5);
    CHECK_EQ(doc.section_header_paragraphs(4).runs().get_text(), "section 1 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 4, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 5);

    CHECK(section_nodes[2].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type",
                                                      "default"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[2].find_child_by_attribute("w:footerReference", "w:type",
                                                      "first"),
             pugi::xml_node{});
    CHECK(section_nodes[3].child("w:titlePg") != pugi::xml_node{});
    CHECK(section_nodes[4].child("w:titlePg") != pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 5);
    CHECK_EQ(reopened.header_count(), 3);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 0 even header");
    CHECK_FALSE(reopened.section_header_paragraphs(2).has_next());
    CHECK_FALSE(reopened.section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_EQ(reopened.section_header_paragraphs(3).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 3, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_EQ(reopened.section_header_paragraphs(4).runs().get_text(), "section 1 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 4, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.insert_section(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("section lifecycle publishes a new body and preserves related part handles") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto old_body_paragraph = document.paragraphs();
    REQUIRE(old_body_paragraph.valid());
    auto old_body_run = old_body_paragraph.add_run("body text");
    REQUIRE(old_body_run.valid());
    auto body_template = document.body_template();
    auto old_body_table = body_template.append_table(1U, 1U);
    REQUIRE(old_body_table.valid());

    auto retained_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(retained_header.valid());
    auto retained_header_run = retained_header.add_run("first header");
    REQUIRE(retained_header_run.valid());

    REQUIRE(document.append_section(false));
    CHECK_FALSE(old_body_paragraph.valid());
    CHECK_FALSE(old_body_run.valid());
    CHECK_FALSE(old_body_table.valid());
    CHECK(body_template);
    CHECK(retained_header.valid());
    CHECK(retained_header_run.valid());

    const auto appended = document.inspect_section(1U);
    REQUIRE(appended.has_value());
    CHECK(appended->different_first_page_enabled);
    CHECK(appended->header.first_linked_to_previous);

    old_body_paragraph = document.paragraphs();
    old_body_run = old_body_paragraph.runs();
    REQUIRE(old_body_paragraph.valid());
    REQUIRE(old_body_run.valid());
    REQUIRE(document.insert_section(0U, false));
    CHECK_FALSE(old_body_paragraph.valid());
    CHECK_FALSE(old_body_run.valid());
    CHECK(retained_header.valid());
    CHECK(retained_header_run.valid());

    old_body_paragraph = document.paragraphs();
    old_body_run = old_body_paragraph.runs();
    REQUIRE(old_body_paragraph.valid());
    REQUIRE(old_body_run.valid());
    REQUIRE(document.remove_section(document.section_count() - 1U));
    CHECK_FALSE(old_body_paragraph.valid());
    CHECK_FALSE(old_body_run.valid());
    CHECK(retained_header.valid());
    CHECK(retained_header_run.valid());
    CHECK_EQ(retained_header_run.get_text(), "first header");
    CHECK(body_template);
}

TEST_CASE("section lifecycle preserves caller-owned title page and even odd markers") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_header.valid());
    REQUIRE(first_header.add_run("first header").valid());
    auto even_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::even_page);
    REQUIRE(even_header.valid());
    REQUIRE(even_header.add_run("even header").valid());

    REQUIRE(document.append_section(false));
    REQUIRE(document.insert_section(0U, false));
    REQUIRE_EQ(document.section_count(), 3U);

    REQUIRE(document.remove_section(0U));
    REQUIRE_EQ(document.section_count(), 2U);
    for (auto section_index = std::size_t{0U};
         section_index < document.section_count(); ++section_index) {
        const auto section = document.inspect_section(section_index);
        REQUIRE(section.has_value());
        CHECK(section->different_first_page_enabled);
    }
    auto inspection = document.inspect_sections();
    REQUIRE(inspection.even_and_odd_headers_enabled.has_value());
    CHECK(*inspection.even_and_odd_headers_enabled);

    REQUIRE(document.remove_section(0U));
    REQUIRE_EQ(document.section_count(), 1U);
    const auto final_section = document.inspect_section(0U);
    REQUIRE(final_section.has_value());
    CHECK(final_section->different_first_page_enabled);
    inspection = document.inspect_sections();
    REQUIRE(inspection.even_and_odd_headers_enabled.has_value());
    CHECK(*inspection.even_and_odd_headers_enabled);
}

TEST_CASE("remove section merges boundaries and prunes orphaned header footer parts on save") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "remove_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto section0_default_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_default_header.has_next());
    CHECK(section0_default_header.add_run("section 0 header").has_next());

    CHECK(doc.append_section(false));
    auto section1_default_header = doc.ensure_section_header_paragraphs(1);
    REQUIRE(section1_default_header.has_next());
    CHECK(section1_default_header.add_run("section 1 header").has_next());

    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.append_section(false));
    auto section2_default_header = doc.ensure_section_header_paragraphs(2);
    REQUIRE(section2_default_header.has_next());
    CHECK(section2_default_header.add_run("section 2 header").has_next());

    auto section2_even_header = doc.ensure_section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page);
    REQUIRE(section2_even_header.has_next());
    CHECK(section2_even_header.add_run("section 2 even header").has_next());

    CHECK(doc.remove_section(1));
    CHECK_EQ(doc.section_count(), 2);
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 1, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_FALSE(doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page)
                    .has_next());

    CHECK(doc.remove_section(1));
    CHECK_EQ(doc.section_count(), 1);
    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 0 header");
    CHECK_FALSE(doc.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(doc.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());
    CHECK_FALSE(doc.remove_section(0));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});

    std::vector<pugi::xml_node> section_nodes;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (const auto section_properties = paragraph.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            section_nodes.push_back(section_properties);
        }
    }
    section_nodes.push_back(body.child("w:sectPr"));
    REQUIRE(section_nodes.size() == 1);
    CHECK(section_nodes[0].child("w:titlePg") == pugi::xml_node{});
    CHECK_EQ(section_nodes[0].find_child_by_attribute("w:headerReference", "w:type", "even"),
             pugi::xml_node{});
    CHECK_EQ(section_nodes[0].find_child_by_attribute("w:footerReference", "w:type", "first"),
             pugi::xml_node{});

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    CHECK(test_docx_entry_exists(target, "word/header1.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header2.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/header3.xml"));
    CHECK_FALSE(test_docx_entry_exists(target, "word/footer1.xml"));

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(reopened.section_count(), 1);
    CHECK_EQ(reopened.header_count(), 1);
    CHECK_EQ(reopened.footer_count(), 0);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 0 header");
    CHECK_FALSE(reopened.section_header_paragraphs(
        0, featherdoc::section_reference_kind::even_page)
                    .has_next());
    CHECK_FALSE(reopened.section_footer_paragraphs(
        0, featherdoc::section_reference_kind::first_page)
                    .has_next());

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.remove_section(0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("move section reorders body content and keeps header footer layouts attached") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "move_section_header_footer.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK(doc.paragraphs().add_run("section 0 body").has_next());
    auto section0_header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK(section0_header.add_run("section 0 header").has_next());

    CHECK(doc.append_section(false));

    auto append_body_paragraph = [](featherdoc::Document &document, const char *text) {
        auto paragraph = document.paragraphs();
        while (paragraph.has_next()) {
            paragraph.next();
        }

        const auto inserted = paragraph.insert_paragraph_after(text);
        REQUIRE(inserted.has_next());
    };

    append_body_paragraph(doc, "section 1 body");
    auto section1_first_footer = doc.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(doc.append_section(false));
    append_body_paragraph(doc, "section 2 body");

    auto section2_header = doc.ensure_section_header_paragraphs(2);
    REQUIRE(section2_header.has_next());
    CHECK(section2_header.add_run("section 2 header").has_next());

    auto section2_even_header = doc.ensure_section_header_paragraphs(
        2, featherdoc::section_reference_kind::even_page);
    REQUIRE(section2_even_header.has_next());
    CHECK(section2_even_header.add_run("section 2 even header").has_next());

    auto stale_body_paragraph = doc.paragraphs();
    auto stale_body_run = stale_body_paragraph.runs();
    auto retained_header_paragraph = doc.section_header_paragraphs(2);
    auto retained_header_run = retained_header_paragraph.runs();
    auto retained_footer_paragraph = doc.section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    auto body_template = doc.body_template();
    auto stale_body_table = body_template.append_table(1U, 1U);
    REQUIRE(stale_body_paragraph.valid());
    REQUIRE(stale_body_run.valid());
    REQUIRE(retained_header_paragraph.valid());
    REQUIRE(retained_header_run.valid());
    REQUIRE(retained_footer_paragraph.valid());
    REQUIRE(body_template);
    REQUIRE(stale_body_table.valid());

    CHECK(doc.move_section(2, 2));
    CHECK(stale_body_paragraph.valid());
    CHECK(stale_body_run.valid());
    CHECK(stale_body_table.valid());
    CHECK(retained_header_paragraph.valid());
    CHECK(retained_header_run.valid());
    CHECK(retained_footer_paragraph.valid());
    CHECK(body_template);

    CHECK_FALSE(doc.move_section(3, 0));
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK(stale_body_paragraph.valid());
    CHECK(stale_body_run.valid());
    CHECK(stale_body_table.valid());
    CHECK(retained_header_paragraph.valid());
    CHECK(retained_header_run.valid());
    CHECK(retained_footer_paragraph.valid());
    CHECK(body_template);

    CHECK(doc.move_section(2, 0));
    CHECK_FALSE(stale_body_paragraph.valid());
    CHECK_FALSE(stale_body_run.valid());
    CHECK_FALSE(stale_body_table.valid());
    CHECK(retained_header_paragraph.valid());
    CHECK(retained_header_run.valid());
    CHECK(retained_footer_paragraph.valid());
    CHECK(body_template);
    CHECK_EQ(doc.section_count(), 3);
    CHECK_EQ(doc.section_header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(doc.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_EQ(doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_FALSE(doc.move_section(3, 0));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto collect_non_empty_document_text = [](featherdoc::Document &document) {
        std::ostringstream stream;
        for (auto paragraph = document.paragraphs(); paragraph.has_next(); paragraph.next()) {
            std::string text;
            for (auto run = paragraph.runs(); run.has_next(); run.next()) {
                text += run.get_text();
            }

            if (!text.empty()) {
                stream << text << '\n';
            }
        }
        return stream.str();
    };

    CHECK_EQ(collect_non_empty_document_text(reopened),
             "section 2 body\nsection 0 body\nsection 1 body\n");
    CHECK_EQ(reopened.section_count(), 3);
    CHECK_EQ(reopened.header_count(), 3);
    CHECK_EQ(reopened.footer_count(), 1);
    CHECK_EQ(reopened.section_header_paragraphs(0).runs().get_text(), "section 2 header");
    CHECK_EQ(reopened.section_header_paragraphs(
                 0, featherdoc::section_reference_kind::even_page)
                 .runs()
                 .get_text(),
             "section 2 even header");
    CHECK_EQ(reopened.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(reopened.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    const auto saved_settings_xml = read_test_docx_entry(target, "word/settings.xml");
    pugi::xml_document saved_settings;
    REQUIRE(saved_settings.load_buffer(saved_settings_xml.data(), saved_settings_xml.size()));
    CHECK(saved_settings.child("w:settings").child("w:evenAndOddHeaders") !=
          pugi::xml_node{});

    featherdoc::Document invalid(target);
    CHECK_FALSE(invalid.move_section(0, 0));
    CHECK_EQ(invalid.last_error().code, featherdoc::document_errc::document_not_open);

    fs::remove(target);
}

TEST_CASE("move section preserves unrelated title page and settings metadata") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "move_section_preserves_unrelated_metadata.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/settings.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"/>
</Types>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings" Target="settings.xml"/>
</Relationships>
)";
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr><w:sectPr><w:titlePg/></w:sectPr></w:pPr>
      <w:r><w:t>first section</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>second section</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    const std::string settings_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:settings xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:evenAndOddHeaders w:val="1"/>
</w:settings>
)";

    write_test_archive_entries(
        target,
        {{test_content_types_xml_entry, content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"word/_rels/document.xml.rels", document_relationships_xml},
         {"word/settings.xml", settings_xml}});

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    REQUIRE_EQ(document.section_count(), 2U);
    auto old_body_paragraph = document.paragraphs();
    auto reusable_body_template = document.body_template();
    REQUIRE(old_body_paragraph.valid());
    REQUIRE(reusable_body_template);

    REQUIRE(document.move_section(0U, 1U));
    CHECK_FALSE(old_body_paragraph.valid());
    CHECK(reusable_body_template);
    REQUIRE_FALSE(document.save());

    const auto saved_document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(),
                                       saved_document_xml.size()));
    const auto saved_body =
        saved_document.child("w:document").child("w:body");
    REQUIRE(saved_body != pugi::xml_node{});
    CHECK(saved_body.child("w:sectPr").child("w:titlePg") !=
          pugi::xml_node{});

    std::vector<std::string> paragraph_texts;
    for (auto paragraph = saved_body.child("w:p");
         paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        std::string text;
        for (auto run = paragraph.child("w:r"); run != pugi::xml_node{};
             run = run.next_sibling("w:r")) {
            text += run.child("w:t").text().get();
        }
        if (!text.empty()) {
            paragraph_texts.push_back(std::move(text));
        }
    }
    CHECK(paragraph_texts ==
          std::vector<std::string>{"second section", "first section"});
    CHECK_EQ(read_test_docx_entry(target, "word/settings.xml"), settings_xml);

    fs::remove(target);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "move section preserves the live document for every pugixml allocation "
    "failure") {
    std::size_t successful_allocation_count = 0U;
    {
        auto document = make_section_move_allocation_fixture();
        auto table = document.body_template().append_table(1U, 1U);
        REQUIRE(table.valid());

        section_move_pugi_allocator_guard guard;
        REQUIRE(document.move_section(2U, 0U));
        successful_allocation_count = section_move_allocation_calls;
    }
    REQUIRE_GT(successful_allocation_count, 2U);

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_allocation_count;
         ++current_failure_call) {
        auto document = make_section_move_allocation_fixture();
        auto old_body_paragraph = document.paragraphs();
        auto old_body_run = old_body_paragraph.runs();
        auto body_template = document.body_template();
        auto old_body_table = body_template.append_table(1U, 1U);
        auto retained_header = document.section_header_paragraphs(0U);
        auto retained_header_run = retained_header.runs();
        REQUIRE(old_body_paragraph.valid());
        REQUIRE(old_body_run.valid());
        REQUIRE(old_body_table.valid());
        REQUIRE(body_template);
        REQUIRE(retained_header.valid());
        REQUIRE(retained_header_run.valid());
        const auto expected_body_texts = section_body_texts(document);

        bool moved = false;
        {
            section_move_pugi_allocator_guard guard;
            section_move_failure_call = current_failure_call;
            moved = document.move_section(2U, 0U);
            CAPTURE(current_failure_call);
            CAPTURE(successful_allocation_count);
            CAPTURE(section_move_allocation_calls);
            CHECK_FALSE(moved);
        }

        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK(old_body_paragraph.valid());
        CHECK(old_body_run.valid());
        CHECK(old_body_table.valid());
        CHECK(body_template);
        CHECK(retained_header.valid());
        CHECK(retained_header_run.valid());
        CHECK_EQ(retained_header_run.get_text(), "retained header");
        CHECK_EQ(document.section_count(), 3U);
        CHECK(section_body_texts(document) == expected_body_texts);
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "append insert and remove section preserve the live body for every "
    "pugixml allocation failure") {
    constexpr auto operations = std::array{
        section_lifecycle_operation::append,
        section_lifecycle_operation::insert,
        section_lifecycle_operation::remove_final,
    };

    for (const auto operation : operations) {
        CAPTURE(section_lifecycle_operation_name(operation));
        auto successful_allocation_count = std::size_t{0U};
        {
            auto document = make_section_move_allocation_fixture();
            auto table = document.body_template().append_table(1U, 1U);
            REQUIRE(table.valid());

            section_move_pugi_allocator_guard guard;
            REQUIRE(perform_section_lifecycle_operation(document, operation));
            successful_allocation_count = section_move_allocation_calls;
        }
        REQUIRE_GT(successful_allocation_count, 0U);

        for (auto current_failure_call = std::size_t{1U};
             current_failure_call <= successful_allocation_count;
             ++current_failure_call) {
            auto document = make_section_move_allocation_fixture();
            auto old_body_paragraph = document.paragraphs();
            auto old_body_run = old_body_paragraph.runs();
            auto body_template = document.body_template();
            auto old_body_table = body_template.append_table(1U, 1U);
            auto retained_header = document.section_header_paragraphs(0U);
            auto retained_header_run = retained_header.runs();
            REQUIRE(old_body_paragraph.valid());
            REQUIRE(old_body_run.valid());
            REQUIRE(old_body_table.valid());
            REQUIRE(body_template);
            REQUIRE(retained_header.valid());
            REQUIRE(retained_header_run.valid());
            const auto expected_section_count = document.section_count();
            const auto expected_body_texts = section_body_texts(document);

            auto changed = false;
            {
                section_move_pugi_allocator_guard guard;
                section_move_failure_call = current_failure_call;
                changed =
                    perform_section_lifecycle_operation(document, operation);
                CAPTURE(current_failure_call);
                CAPTURE(successful_allocation_count);
                CAPTURE(section_move_allocation_calls);
                CHECK_FALSE(changed);
            }

            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            CHECK(old_body_paragraph.valid());
            CHECK(old_body_run.valid());
            CHECK(old_body_table.valid());
            CHECK(body_template);
            CHECK(retained_header.valid());
            CHECK(retained_header_run.valid());
            CHECK_EQ(retained_header_run.get_text(), "retained header");
            CHECK_EQ(document.section_count(), expected_section_count);
            CHECK(section_body_texts(document) == expected_body_texts);
        }
    }
}
