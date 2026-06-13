#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("document and template part can inspect append replace and remove OMML") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "omml_inspect_append_replace_remove.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math">
  <w:body>
    <w:p><w:r><w:t>Before equation </w:t></w:r><m:oMath><m:r><m:t>x+1</m:t></m:r></m:oMath></w:p>
    <m:oMathPara><m:oMath><m:r><m:t>y=2</m:t></m:r></m:oMath></m:oMathPara>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    auto summaries = doc.list_omml();
    REQUIRE(summaries.size() == 2U);
    CHECK_FALSE(summaries[0].display);
    CHECK_EQ(summaries[0].text, "x+1");
    CHECK(summaries[0].xml.find("m:oMath") != std::string::npos);
    CHECK(summaries[1].display);
    CHECK_EQ(summaries[1].text, "y=2");

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.append_omml(
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>z=3</m:t></m:r></m:oMath>)"));
    CHECK(doc.replace_omml(
        1U,
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>y=4</m:t></m:r></m:oMath>)"));
    CHECK(doc.remove_omml(0U));

    summaries = body_template.list_omml();
    REQUIRE(summaries.size() == 2U);
    CHECK(summaries[0].display);
    CHECK_EQ(summaries[0].text, "y=4");
    CHECK_FALSE(summaries[1].display);
    CHECK_EQ(summaries[1].text, "z=3");

    CHECK_FALSE(doc.save());

    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\""),
             std::string::npos);
    CHECK_EQ(saved_xml.find("x+1"), std::string::npos);
    CHECK_NE(saved_xml.find("y=4"), std::string::npos);
    CHECK_NE(saved_xml.find("z=3"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_summaries = reopened.list_omml();
    REQUIRE(reopened_summaries.size() == 2U);
    CHECK_EQ(reopened_summaries[0].text, "y=4");
    CHECK_EQ(reopened_summaries[1].text, "z=3");

    fs::remove(target);
}

TEST_CASE("OMML APIs validate fragments indexes and replacement shape") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "omml_validation_errors.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());
    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    CHECK_FALSE(body_template.append_omml(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "OMML XML must not be empty");

    CHECK_FALSE(body_template.append_omml("<w:p/>"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "OMML XML root must be m:oMath or m:oMathPara");

    CHECK_FALSE(body_template.replace_omml(
        7U,
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>a</m:t></m:r></m:oMath>)"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "OMML index is out of range");

    CHECK(body_template.append_omml(
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>a</m:t></m:r></m:oMath>)"));
    CHECK_FALSE(body_template.replace_omml(
        0U,
        R"(<m:oMathPara xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:oMath><m:r><m:t>b</m:t></m:r></m:oMath></m:oMathPara>)"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "display OMML cannot replace an inline OMML target");

    CHECK_FALSE(doc.remove_omml(9U));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "OMML index is out of range");

    fs::remove(target);
}

TEST_CASE("document can inspect footnotes endnotes comments and revisions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_notes_and_revisions.docx";
    fs::remove(target);

    const std::string content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/footnotes.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"/>
  <Override PartName="/word/endnotes.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"/>
  <Override PartName="/word/comments.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"/>
</Types>
)";
    const std::string document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rFootnotes" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes" Target="footnotes.xml"/>
  <Relationship Id="rEndnotes" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes" Target="endnotes.xml"/>
  <Relationship Id="rComments" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments" Target="comments.xml"/>
</Relationships>
)";
    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Body with footnote</w:t></w:r><w:r><w:footnoteReference w:id="2"/></w:r></w:p>
    <w:p><w:r><w:t>Body with endnote</w:t></w:r><w:r><w:endnoteReference w:id="3"/></w:r></w:p>
    <w:p>
      <w:commentRangeStart w:id="4"/>
      <w:r><w:t>Commented text</w:t></w:r>
      <w:commentRangeEnd w:id="4"/>
      <w:r><w:commentReference w:id="4"/></w:r>
    </w:p>
    <w:p>
      <w:ins w:id="5" w:author="Ada" w:date="2026-05-01T10:00:00Z"><w:r><w:t>Inserted text</w:t></w:r></w:ins>
      <w:del w:id="6" w:author="Grace" w:date="2026-05-01T11:00:00Z"><w:r><w:delText>Deleted text</w:delText></w:r></w:del>
    </w:p>
  </w:body>
</w:document>
)";
    const std::string footnotes_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="2"><w:p><w:r><w:t>Footnote body text</w:t></w:r></w:p></w:footnote>
</w:footnotes>
)";
    const std::string endnotes_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:endnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:endnote w:id="3"><w:p><w:r><w:t>Endnote body text</w:t></w:r></w:p></w:endnote>
</w:endnotes>
)";
    const std::string comments_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-05-01T12:00:00Z"><w:p><w:r><w:t>Comment body text</w:t></w:r></w:p></w:comment>
</w:comments>
)";
    write_test_archive_entries(
        target,
        {{test_content_types_xml_entry, content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"word/_rels/document.xml.rels", document_relationships_xml},
         {"word/footnotes.xml", footnotes_xml},
         {"word/endnotes.xml", endnotes_xml},
         {"word/comments.xml", comments_xml}});

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto footnotes = doc.list_footnotes();
    REQUIRE(footnotes.size() == 1U);
    CHECK_EQ(footnotes[0].kind, featherdoc::review_note_kind::footnote);
    CHECK_EQ(footnotes[0].id, "2");
    CHECK_EQ(footnotes[0].text, "Footnote body text");

    const auto endnotes = doc.list_endnotes();
    REQUIRE(endnotes.size() == 1U);
    CHECK_EQ(endnotes[0].kind, featherdoc::review_note_kind::endnote);
    CHECK_EQ(endnotes[0].id, "3");
    CHECK_EQ(endnotes[0].text, "Endnote body text");

    const auto comments = doc.list_comments();
    REQUIRE(comments.size() == 1U);
    CHECK_EQ(comments[0].kind, featherdoc::review_note_kind::comment);
    CHECK_EQ(comments[0].id, "4");
    REQUIRE(comments[0].author.has_value());
    CHECK_EQ(*comments[0].author, "Reviewer");
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Commented text");
    CHECK_EQ(comments[0].text, "Comment body text");

    const auto revisions = doc.list_revisions();
    REQUIRE(revisions.size() == 2U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].id, "5");
    CHECK_EQ(revisions[0].text, "Inserted text");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[1].id, "6");
    CHECK_EQ(revisions[1].text, "Deleted text");

    fs::remove(target);
}

TEST_CASE("document review inspection returns empty lists when optional parts are absent") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_notes_absent.docx";
    fs::remove(target);

    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>No review metadata</w:t></w:r></w:p></w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.list_footnotes().empty());
    CHECK_FALSE(doc.last_error());
    CHECK(doc.list_endnotes().empty());
    CHECK_FALSE(doc.last_error());
    CHECK(doc.list_comments().empty());
    CHECK_FALSE(doc.last_error());
    CHECK(doc.list_revisions().empty());
    CHECK_FALSE(doc.last_error());

    fs::remove(target);
}

TEST_CASE("template part page number fields report unavailable parts") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "template_part_page_number_fields_error.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto missing_header_template = doc.section_header_template(1);
    CHECK_FALSE(static_cast<bool>(missing_header_template));
    CHECK_FALSE(missing_header_template.append_page_number_field());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "template part is not available");

    auto missing_footer_template = doc.section_footer_template(1);
    CHECK_FALSE(static_cast<bool>(missing_footer_template));
    CHECK_FALSE(missing_footer_template.append_total_pages_field());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "template part is not available");

    fs::remove(target);
}

TEST_CASE("header template part tables can remove a middle table and keep the wrapper usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_table_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto header_paragraph = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header_paragraph.has_next());
    CHECK(header_paragraph.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto first_table = header_template.append_table(2, 2);
    REQUIRE(first_table.has_next());
    auto row = first_table.rows();
    REQUIRE(row.has_next());
    auto cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Section"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Status"));
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Retained"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Header table"));

    auto removable_table = header_template.append_table(1, 1);
    REQUIRE(removable_table.has_next());
    CHECK(removable_table.rows().cells().set_text("temporary middle table"));

    auto final_table = header_template.append_table(2, 2);
    REQUIRE(final_table.has_next());
    row = final_table.rows();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Final"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("State"));
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Pending"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Will be updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    auto selected_table = header_template.tables();
    REQUIRE(selected_table.has_next());
    selected_table.next();
    REQUIRE(selected_table.has_next());
    CHECK(selected_table.remove());
    CHECK(selected_table.has_next());

    row = selected_table.rows();
    row.next();
    REQUIRE(row.has_next());
    cell = row.cells();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Final"));
    cell.next();
    REQUIRE(cell.has_next());
    CHECK(cell.set_text("Reached after middle-table removal"));

    CHECK_FALSE(reopened.save());

    const auto saved_header = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document header_document;
    REQUIRE(header_document.load_string(saved_header.c_str()));
    const auto header_root = header_document.child("w:hdr");
    REQUIRE(header_root != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_root, "w:tbl"), 2U);

    featherdoc::Document reopened_again(target);
    CHECK_FALSE(reopened_again.open());

    header_template = reopened_again.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\n");
    CHECK_EQ(collect_template_part_table_text(header_template),
             "Section\nStatus\nRetained\nHeader table\nFinal\nState\nFinal\nReached after middle-table removal\n");

    fs::remove(target);
}
