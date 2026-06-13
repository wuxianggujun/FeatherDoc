#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

TEST_CASE("documents can be compared with semantic diff summaries") {
    namespace fs = std::filesystem;

    const fs::path left_path = fs::current_path() / "semantic_diff_left.docx";
    const fs::path right_path = fs::current_path() / "semantic_diff_right.docx";

    create_semantic_diff_fixture(left_path, "Quarterly report", "draft",
                                 "Ada Lovelace", "$120");
    create_semantic_diff_fixture(right_path, "Quarterly report", "approved",
                                 "Grace Hopper", "$150");

    featherdoc::Document left(left_path);
    REQUIRE_FALSE(left.open());
    featherdoc::Document right(right_path);
    REQUIRE_FALSE(right.open());

    const auto result = left.compare_semantic(right);
    REQUIRE(result.has_value());
    CHECK(result->different());
    CHECK_EQ(result->change_count(), 3U);
    CHECK_EQ(result->sections.left_count, 1U);
    CHECK_EQ(result->sections.right_count, 1U);
    CHECK_EQ(result->sections.change_count(), 0U);

    CHECK_EQ(result->paragraphs.left_count, 2U);
    CHECK_EQ(result->paragraphs.right_count, 2U);
    CHECK_EQ(result->paragraphs.changed_count, 1U);
    REQUIRE_EQ(result->paragraph_changes.size(), 1U);
    CHECK_EQ(result->paragraph_changes[0].kind,
             featherdoc::document_semantic_diff_change_kind::changed);
    CHECK_EQ(result->paragraph_changes[0].left_index,
             std::optional<std::size_t>{1U});
    CHECK_NE(result->paragraph_changes[0].left_value.find("Status: draft"),
             std::string::npos);
    CHECK_NE(result->paragraph_changes[0].right_value.find("Status: approved"),
             std::string::npos);
    REQUIRE_EQ(result->paragraph_changes[0].field_changes.size(), 1U);
    CHECK_EQ(result->paragraph_changes[0].field_changes[0].field_path, "text");
    CHECK_EQ(result->paragraph_changes[0].field_changes[0].left_value, "Status: draft");
    CHECK_EQ(result->paragraph_changes[0].field_changes[0].right_value,
             "Status: approved");

    CHECK_EQ(result->tables.left_count, 1U);
    CHECK_EQ(result->tables.right_count, 1U);
    CHECK_EQ(result->tables.changed_count, 1U);
    REQUIRE_EQ(result->table_changes.size(), 1U);
    CHECK_NE(result->table_changes[0].left_value.find("$120"), std::string::npos);
    CHECK_NE(result->table_changes[0].right_value.find("$150"), std::string::npos);
    CHECK_FALSE(result->table_changes[0].field_changes.empty());
    CHECK_EQ(result->table_changes[0].field_changes[0].field_path, "text");

    CHECK_EQ(result->content_controls.left_count, 1U);
    CHECK_EQ(result->content_controls.right_count, 1U);
    CHECK_EQ(result->content_controls.changed_count, 1U);
    REQUIRE_EQ(result->content_control_changes.size(), 1U);
    CHECK_NE(result->content_control_changes[0].left_value.find("Ada Lovelace"),
             std::string::npos);
    CHECK_NE(result->content_control_changes[0].right_value.find("Grace Hopper"),
             std::string::npos);
    REQUIRE_FALSE(result->content_control_changes[0].field_changes.empty());
    CHECK_EQ(result->content_control_changes[0].field_changes.back().field_path, "text");
    CHECK_EQ(result->content_control_changes[0].field_changes.back().left_value,
             "Ada Lovelace");
    CHECK_EQ(result->content_control_changes[0].field_changes.back().right_value,
             "Grace Hopper");

    CHECK_EQ(result->images.left_count, 0U);
    CHECK_EQ(result->images.right_count, 0U);
    CHECK_EQ(result->images.change_count(), 0U);

    auto options = featherdoc::document_semantic_diff_options{};
    options.compare_tables = false;
    options.compare_images = false;
    options.compare_content_controls = false;
    const auto text_only = left.compare_semantic(right, options);
    REQUIRE(text_only.has_value());
    CHECK_EQ(text_only->change_count(), 1U);
    CHECK_EQ(text_only->tables.left_count, 0U);
    CHECK_EQ(text_only->content_controls.left_count, 0U);

    fs::remove(left_path);
    fs::remove(right_path);
}





TEST_CASE("semantic diff reports TOC REF and SEQ field changes") {
    namespace fs = std::filesystem;

    const fs::path left_path = fs::current_path() / "semantic_diff_fields_left.docx";
    const fs::path right_path = fs::current_path() / "semantic_diff_fields_right.docx";
    fs::remove(left_path);
    fs::remove(right_path);

    const auto make_document = [](const fs::path &path, std::uint32_t toc_max_level,
                                  std::string_view reference_result,
                                  std::uint32_t sequence_restart,
                                  std::string_view sequence_result) {
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.create_empty());
        auto body = document.body_template();
        REQUIRE(static_cast<bool>(body));
        REQUIRE(body.paragraphs().has_next());
        CHECK(body.paragraphs().set_text("Semantic field diff fixture"));

        auto toc_options = featherdoc::table_of_contents_field_options{};
        toc_options.min_outline_level = 1U;
        toc_options.max_outline_level = toc_max_level;
        CHECK(body.append_table_of_contents_field(toc_options, "TOC placeholder"));
        CHECK(body.append_reference_field("target_bookmark", {}, reference_result));

        auto sequence_options = featherdoc::sequence_field_options{};
        sequence_options.restart_value = sequence_restart;
        CHECK(body.append_sequence_field("Figure", sequence_options, sequence_result));
        CHECK_FALSE(document.save());
    };

    make_document(left_path, 2U, "Referenced heading", 1U, "Figure 1");
    make_document(right_path, 3U, "Approved heading", 2U, "Figure 2");

    featherdoc::Document left(left_path);
    REQUIRE_FALSE(left.open());
    featherdoc::Document right(right_path);
    REQUIRE_FALSE(right.open());

    auto options = featherdoc::document_semantic_diff_options{};
    options.compare_paragraphs = false;
    options.compare_tables = false;
    options.compare_images = false;
    options.compare_content_controls = false;
    options.compare_sections = false;

    const auto result = left.compare_semantic(right, options);
    REQUIRE(result.has_value());
    CHECK_EQ(result->change_count(), 3U);
    CHECK_EQ(result->fields.left_count, 3U);
    CHECK_EQ(result->fields.right_count, 3U);
    CHECK_EQ(result->fields.changed_count, 3U);
    CHECK_EQ(result->template_parts.change_count(), 0U);
    REQUIRE_EQ(result->field_changes.size(), 3U);
    CHECK_EQ(result->field_changes[0].field, "field");
    CHECK_NE(result->field_changes[0].left_value.find("kind=table_of_contents"),
             std::string::npos);
    CHECK_NE(result->field_changes[1].left_value.find("kind=reference"),
             std::string::npos);
    CHECK_NE(result->field_changes[2].left_value.find("kind=sequence"),
             std::string::npos);

    const auto toc_instruction_changed = std::any_of(
        result->field_changes[0].field_changes.begin(),
        result->field_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "instruction" &&
                   field_change.left_value.find("\\o \"1-2\"") !=
                       std::string::npos &&
                   field_change.right_value.find("\\o \"1-3\"") !=
                       std::string::npos;
        });
    CHECK(toc_instruction_changed);

    const auto reference_result_changed = std::any_of(
        result->field_changes[1].field_changes.begin(),
        result->field_changes[1].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "result_text" &&
                   field_change.left_value == "Referenced heading" &&
                   field_change.right_value == "Approved heading";
        });
    CHECK(reference_result_changed);

    const auto sequence_instruction_changed = std::any_of(
        result->field_changes[2].field_changes.begin(),
        result->field_changes[2].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "instruction" &&
                   field_change.left_value.find("\\r 1") != std::string::npos &&
                   field_change.right_value.find("\\r 2") != std::string::npos;
        });
    CHECK(sequence_instruction_changed);

    REQUIRE_EQ(result->template_part_results.size(), 1U);
    CHECK_EQ(result->template_part_results[0].target.part,
             featherdoc::template_schema_part_kind::body);
    CHECK_EQ(result->template_part_results[0].fields.changed_count, 3U);
    CHECK_EQ(result->template_part_results[0].field_changes.size(), 3U);

    options.compare_fields = false;
    const auto disabled = left.compare_semantic(right, options);
    REQUIRE(disabled.has_value());
    CHECK_EQ(disabled->change_count(), 0U);
    CHECK_EQ(disabled->fields.left_count, 0U);
    CHECK(disabled->field_changes.empty());

    fs::remove(left_path);
    fs::remove(right_path);
}



TEST_CASE("semantic diff reports style and numbering summary changes") {
    namespace fs = std::filesystem;

    const fs::path left_path =
        fs::current_path() / "semantic_diff_style_numbering_left.docx";
    const fs::path right_path =
        fs::current_path() / "semantic_diff_style_numbering_right.docx";
    fs::remove(left_path);
    fs::remove(right_path);

    const auto make_document = [](
        const fs::path &path, std::string_view style_name,
        std::string_view based_on, std::string_view numbering_name,
        featherdoc::list_kind numbering_kind, std::uint32_t numbering_start,
        std::string_view numbering_pattern) {
        featherdoc::Document document(path);
        REQUIRE_FALSE(document.create_empty());

        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        CHECK(paragraph.set_text("Semantic style and numbering diff fixture"));

        auto style = featherdoc::paragraph_style_definition{};
        style.name = std::string{style_name};
        style.based_on = std::string{based_on};
        style.is_quick_format = true;
        CHECK(document.ensure_paragraph_style("SemanticDiffHeading", style));

        auto numbering = featherdoc::numbering_definition{};
        numbering.name = std::string{numbering_name};
        numbering.levels = {featherdoc::numbering_level_definition{
            numbering_kind, numbering_start, 0U, std::string{numbering_pattern}}};
        const auto numbering_id = document.ensure_numbering_definition(numbering);
        REQUIRE(numbering_id.has_value());
        CHECK(document.set_paragraph_style_numbering("SemanticDiffHeading",
                                                     *numbering_id, 0U));
        CHECK_FALSE(document.save());
    };

    make_document(left_path, "Draft Heading", "Heading1", "DraftOutline",
                  featherdoc::list_kind::decimal, 1U, "%1.");
    make_document(right_path, "Approved Heading", "Title", "ApprovedOutline",
                  featherdoc::list_kind::bullet, 3U, "o");

    featherdoc::Document left(left_path);
    REQUIRE_FALSE(left.open());
    featherdoc::Document right(right_path);
    REQUIRE_FALSE(right.open());

    auto options = featherdoc::document_semantic_diff_options{};
    options.compare_paragraphs = false;
    options.compare_tables = false;
    options.compare_images = false;
    options.compare_content_controls = false;
    options.compare_fields = false;
    options.compare_sections = false;
    options.compare_template_parts = false;

    const auto result = left.compare_semantic(right, options);
    REQUIRE(result.has_value());
    CHECK_EQ(result->change_count(), 2U);
    CHECK_EQ(result->styles.changed_count, 1U);
    CHECK_EQ(result->numbering.changed_count, 1U);
    REQUIRE_EQ(result->style_changes.size(), 1U);
    REQUIRE_EQ(result->numbering_changes.size(), 1U);
    CHECK_EQ(result->style_changes[0].field, "style");
    CHECK_EQ(result->numbering_changes[0].field, "numbering");
    CHECK_NE(result->style_changes[0].left_value.find("name=Draft Heading"),
             std::string::npos);
    CHECK_NE(result->style_changes[0].right_value.find("name=Approved Heading"),
             std::string::npos);
    CHECK_NE(result->numbering_changes[0].left_value.find("name=DraftOutline"),
             std::string::npos);
    CHECK_NE(result->numbering_changes[0].right_value.find("kind=bullet"),
             std::string::npos);

    const auto style_name_changed = std::any_of(
        result->style_changes[0].field_changes.begin(),
        result->style_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "name" &&
                   field_change.left_value == "Draft Heading" &&
                   field_change.right_value == "Approved Heading";
        });
    CHECK(style_name_changed);

    const auto numbering_kind_changed = std::any_of(
        result->numbering_changes[0].field_changes.begin(),
        result->numbering_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "levels.kind" &&
                   field_change.left_value == "decimal" &&
                   field_change.right_value == "bullet";
        });
    CHECK(numbering_kind_changed);

    options.compare_styles = false;
    options.compare_numbering = false;
    const auto disabled = left.compare_semantic(right, options);
    REQUIRE(disabled.has_value());
    CHECK_EQ(disabled->change_count(), 0U);
    CHECK_EQ(disabled->styles.left_count, 0U);
    CHECK_EQ(disabled->numbering.left_count, 0U);
    CHECK(disabled->style_changes.empty());
    CHECK(disabled->numbering_changes.empty());

    fs::remove(left_path);
    fs::remove(right_path);
}

TEST_CASE("semantic diff reports review note comment and revision changes") {
    namespace fs = std::filesystem;

    const fs::path left_path = fs::current_path() / "semantic_diff_review_left.docx";
    const fs::path right_path = fs::current_path() / "semantic_diff_review_right.docx";
    fs::remove(left_path);
    fs::remove(right_path);

    const auto make_document = [](const fs::path &path, std::string_view footnote_text,
                                  std::string_view endnote_text,
                                  std::string_view comment_text,
                                  std::string_view revision_text,
                                  std::string_view revision_author) {
        const std::string content_types_xml =
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/footnotes.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"/>
  <Override PartName="/word/endnotes.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"/>
  <Override PartName="/word/comments.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"/>
</Types>
)";
        const std::string document_relationships_xml =
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rFootnotes"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes"
                Target="footnotes.xml"/>
  <Relationship Id="rEndnotes"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes"
                Target="endnotes.xml"/>
  <Relationship Id="rComments"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments"
                Target="comments.xml"/>
</Relationships>
)";

        auto comment_anchor_text = std::string{revision_author == "Ada"
                                                   ? "Original commented text"
                                                   : "Approved commented text"};
        auto document_xml = std::string{R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:r><w:t>Footnote mark</w:t></w:r>
      <w:r><w:footnoteReference w:id="2"/></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Endnote mark</w:t></w:r>
      <w:r><w:endnoteReference w:id="3"/></w:r>
    </w:p>
    <w:p>
      <w:commentRangeStart w:id="4"/>
      <w:r><w:t>)xml"};
        document_xml += comment_anchor_text;
        document_xml += R"xml(</w:t></w:r>
      <w:commentRangeEnd w:id="4"/>
      <w:r><w:commentReference w:id="4"/></w:r>
    </w:p>
    <w:p>
      <w:ins w:id="5" w:author=")xml";
        document_xml += revision_author;
        document_xml += R"xml(" w:date="2026-05-01T10:00:00Z"><w:r><w:t>)xml";
        document_xml += revision_text;
        document_xml += R"xml(</w:t></w:r></w:ins>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)xml";

        auto footnotes_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="2"><w:p><w:r><w:t>)"};
        footnotes_xml += footnote_text;
        footnotes_xml += R"(</w:t></w:r></w:p></w:footnote>
</w:footnotes>
)";

        auto endnotes_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:endnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:endnote w:id="3"><w:p><w:r><w:t>)"};
        endnotes_xml += endnote_text;
        endnotes_xml += R"(</w:t></w:r></w:p></w:endnote>
</w:endnotes>
)";

        auto comments_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-05-01T12:00:00Z">
    <w:p><w:r><w:t>)"};
        comments_xml += comment_text;
        comments_xml += R"(</w:t></w:r></w:p>
  </w:comment>
</w:comments>
)";

        write_test_archive_entries(
            path,
            {{test_content_types_xml_entry, content_types_xml},
             {test_relationships_xml_entry, test_relationships_xml},
             {test_document_xml_entry, document_xml},
             {"word/_rels/document.xml.rels", document_relationships_xml},
             {"word/footnotes.xml", footnotes_xml},
             {"word/endnotes.xml", endnotes_xml},
             {"word/comments.xml", comments_xml}});
    };

    make_document(left_path, "Original footnote", "Original endnote",
                  "Original comment", "Inserted draft text", "Ada");
    make_document(right_path, "Approved footnote", "Approved endnote",
                  "Approved comment", "Inserted approved text", "Grace");

    featherdoc::Document left(left_path);
    REQUIRE_FALSE(left.open());
    featherdoc::Document right(right_path);
    REQUIRE_FALSE(right.open());

    auto options = featherdoc::document_semantic_diff_options{};
    options.compare_paragraphs = false;
    options.compare_tables = false;
    options.compare_images = false;
    options.compare_content_controls = false;
    options.compare_fields = false;
    options.compare_styles = false;
    options.compare_numbering = false;
    options.compare_sections = false;
    options.compare_template_parts = false;

    const auto result = left.compare_semantic(right, options);
    REQUIRE(result.has_value());
    CHECK_EQ(result->change_count(), 4U);
    CHECK_EQ(result->footnotes.changed_count, 1U);
    CHECK_EQ(result->endnotes.changed_count, 1U);
    CHECK_EQ(result->comments.changed_count, 1U);
    CHECK_EQ(result->revisions.changed_count, 1U);
    REQUIRE_EQ(result->footnote_changes.size(), 1U);
    REQUIRE_EQ(result->endnote_changes.size(), 1U);
    REQUIRE_EQ(result->comment_changes.size(), 1U);
    REQUIRE_EQ(result->revision_changes.size(), 1U);
    CHECK_EQ(result->footnote_changes[0].field, "footnote");
    CHECK_EQ(result->endnote_changes[0].field, "endnote");
    CHECK_EQ(result->comment_changes[0].field, "comment");
    CHECK_EQ(result->revision_changes[0].field, "revision");

    const auto footnote_text_changed = std::any_of(
        result->footnote_changes[0].field_changes.begin(),
        result->footnote_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "text" &&
                   field_change.left_value == "Original footnote" &&
                   field_change.right_value == "Approved footnote";
        });
    CHECK(footnote_text_changed);

    const auto endnote_text_changed = std::any_of(
        result->endnote_changes[0].field_changes.begin(),
        result->endnote_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "text" &&
                   field_change.left_value == "Original endnote" &&
                   field_change.right_value == "Approved endnote";
        });
    CHECK(endnote_text_changed);

    const auto comment_text_changed = std::any_of(
        result->comment_changes[0].field_changes.begin(),
        result->comment_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "text" &&
                   field_change.left_value == "Original comment" &&
                   field_change.right_value == "Approved comment";
        });
    CHECK(comment_text_changed);

    const auto comment_anchor_text_changed = std::any_of(
        result->comment_changes[0].field_changes.begin(),
        result->comment_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "anchor_text" &&
                   field_change.left_value == "Original commented text" &&
                   field_change.right_value == "Approved commented text";
        });
    CHECK(comment_anchor_text_changed);

    const auto revision_author_changed = std::any_of(
        result->revision_changes[0].field_changes.begin(),
        result->revision_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "author" &&
                   field_change.left_value == "Ada" &&
                   field_change.right_value == "Grace";
        });
    CHECK(revision_author_changed);

    options.compare_footnotes = false;
    options.compare_endnotes = false;
    options.compare_comments = false;
    options.compare_revisions = false;
    const auto disabled = left.compare_semantic(right, options);
    REQUIRE(disabled.has_value());
    CHECK_EQ(disabled->change_count(), 0U);
    CHECK_EQ(disabled->footnotes.left_count, 0U);
    CHECK_EQ(disabled->endnotes.left_count, 0U);
    CHECK_EQ(disabled->comments.left_count, 0U);
    CHECK_EQ(disabled->revisions.left_count, 0U);
    CHECK(disabled->footnote_changes.empty());
    CHECK(disabled->endnote_changes.empty());
    CHECK(disabled->comment_changes.empty());
    CHECK(disabled->revision_changes.empty());

    fs::remove(left_path);
    fs::remove(right_path);
}


TEST_CASE("semantic diff reports section page setup changes") {
    namespace fs = std::filesystem;

    const fs::path left_path = fs::current_path() / "semantic_diff_section_left.docx";
    const fs::path right_path = fs::current_path() / "semantic_diff_section_right.docx";
    fs::remove(left_path);
    fs::remove(right_path);

    const auto left_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Section setup smoke</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440" w:header="720" w:footer="720"/>
      <w:pgNumType w:start="1"/>
    </w:sectPr>
  </w:body>
</w:document>
)"};
    const auto right_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Section setup smoke</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/>
      <w:pgMar w:top="720" w:right="1800" w:bottom="1080" w:left="1800" w:header="360" w:footer="540"/>
      <w:pgNumType w:start="5"/>
    </w:sectPr>
  </w:body>
</w:document>
)"};
    write_test_docx(left_path, left_xml);
    write_test_docx(right_path, right_xml);

    featherdoc::Document left(left_path);
    REQUIRE_FALSE(left.open());
    featherdoc::Document right(right_path);
    REQUIRE_FALSE(right.open());

    auto options = featherdoc::document_semantic_diff_options{};
    options.compare_paragraphs = false;
    options.compare_tables = false;
    options.compare_images = false;
    options.compare_content_controls = false;
    const auto result = left.compare_semantic(right, options);
    REQUIRE(result.has_value());
    CHECK_EQ(result->change_count(), 1U);
    CHECK_EQ(result->sections.left_count, 1U);
    CHECK_EQ(result->sections.right_count, 1U);
    CHECK_EQ(result->sections.changed_count, 1U);
    REQUIRE_EQ(result->section_changes.size(), 1U);
    CHECK_EQ(result->section_changes[0].kind,
             featherdoc::document_semantic_diff_change_kind::changed);
    CHECK_NE(result->section_changes[0].left_value.find("orientation=portrait"),
             std::string::npos);
    CHECK_NE(result->section_changes[0].right_value.find("orientation=landscape"),
             std::string::npos);
    CHECK_NE(result->section_changes[0].right_value.find("page_number_start=5"),
             std::string::npos);
    const auto has_orientation_field = std::any_of(
        result->section_changes[0].field_changes.begin(),
        result->section_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "page_setup.orientation" &&
                   field_change.left_value == "portrait" &&
                   field_change.right_value == "landscape";
        });
    CHECK(has_orientation_field);
    const auto has_page_number_field = std::any_of(
        result->section_changes[0].field_changes.begin(),
        result->section_changes[0].field_changes.end(),
        [](const featherdoc::document_semantic_diff_field_change &field_change) {
            return field_change.field_path == "page_setup.page_number_start" &&
                   field_change.left_value == "1" &&
                   field_change.right_value == "5";
        });
    CHECK(has_page_number_field);

    options.compare_sections = false;
    const auto disabled = left.compare_semantic(right, options);
    REQUIRE(disabled.has_value());
    CHECK_EQ(disabled->change_count(), 0U);

    fs::remove(left_path);
    fs::remove(right_path);
}

TEST_CASE("semantic diff reports header and footer template part changes") {
    namespace fs = std::filesystem;

    const fs::path left_path = fs::current_path() / "semantic_diff_part_left.docx";
    const fs::path right_path = fs::current_path() / "semantic_diff_part_right.docx";
    fs::remove(left_path);
    fs::remove(right_path);

    write_test_docx_with_header_footer(left_path, "Body stable", "Header draft",
                                       "Footer draft");
    write_test_docx_with_header_footer(right_path, "Body stable", "Header approved",
                                       "Footer approved");

    featherdoc::Document left(left_path);
    REQUIRE_FALSE(left.open());
    featherdoc::Document right(right_path);
    REQUIRE_FALSE(right.open());

    const auto result = left.compare_semantic(right);
    REQUIRE(result.has_value());
    CHECK(result->different());
    CHECK_EQ(result->change_count(), 2U);
    CHECK_EQ(result->paragraphs.change_count(), 0U);
    CHECK_EQ(result->sections.change_count(), 0U);
    CHECK_EQ(result->template_parts.left_count, 2U);
    CHECK_EQ(result->template_parts.right_count, 2U);
    CHECK_EQ(result->template_parts.changed_count, 2U);
    REQUIRE_EQ(result->template_part_results.size(), 5U);

    const auto find_part = [&result](
                               featherdoc::template_schema_part_kind kind,
                               std::optional<std::size_t> section_index = std::nullopt)
        -> const featherdoc::document_semantic_diff_part_result * {
        for (const auto &part_result : result->template_part_results) {
            if (part_result.target.part == kind &&
                part_result.target.section_index == section_index) {
                return &part_result;
            }
        }
        return nullptr;
    };

    const auto *body_part = find_part(featherdoc::template_schema_part_kind::body);
    REQUIRE(body_part != nullptr);
    CHECK_FALSE(body_part->different());
    CHECK_EQ(body_part->entry_name, "word/document.xml");

    const auto *header_part = find_part(featherdoc::template_schema_part_kind::header);
    REQUIRE(header_part != nullptr);
    CHECK_EQ(header_part->target.part_index, std::optional<std::size_t>{0U});
    CHECK_EQ(header_part->entry_name, "word/header1.xml");
    REQUIRE_EQ(header_part->paragraph_changes.size(), 1U);
    REQUIRE_EQ(header_part->paragraph_changes[0].field_changes.size(), 1U);
    CHECK_EQ(header_part->paragraph_changes[0].field_changes[0].field_path, "text");
    CHECK_EQ(header_part->paragraph_changes[0].field_changes[0].left_value,
             "Header draft");
    CHECK_EQ(header_part->paragraph_changes[0].field_changes[0].right_value,
             "Header approved");

    const auto *footer_part = find_part(featherdoc::template_schema_part_kind::footer);
    REQUIRE(footer_part != nullptr);
    CHECK_EQ(footer_part->target.part_index, std::optional<std::size_t>{0U});
    CHECK_EQ(footer_part->entry_name, "word/footer1.xml");
    REQUIRE_EQ(footer_part->paragraph_changes.size(), 1U);
    REQUIRE_EQ(footer_part->paragraph_changes[0].field_changes.size(), 1U);
    CHECK_EQ(footer_part->paragraph_changes[0].field_changes[0].field_path, "text");
    CHECK_EQ(footer_part->paragraph_changes[0].field_changes[0].left_value,
             "Footer draft");
    CHECK_EQ(footer_part->paragraph_changes[0].field_changes[0].right_value,
             "Footer approved");

    const auto *section_header_part = find_part(
        featherdoc::template_schema_part_kind::section_header, 0U);
    REQUIRE(section_header_part != nullptr);
    CHECK_EQ(section_header_part->target.reference_kind,
             featherdoc::section_reference_kind::default_reference);
    CHECK_EQ(section_header_part->entry_name, "word/header1.xml");
    CHECK_EQ(section_header_part->left_resolved_from_section_index,
             std::optional<std::size_t>{0U});
    CHECK_EQ(section_header_part->right_resolved_from_section_index,
             std::optional<std::size_t>{0U});
    REQUIRE_EQ(section_header_part->paragraph_changes.size(), 1U);
    CHECK_EQ(section_header_part->paragraph_changes[0].field_changes[0].left_value,
             "Header draft");

    const auto *section_footer_part = find_part(
        featherdoc::template_schema_part_kind::section_footer, 0U);
    REQUIRE(section_footer_part != nullptr);
    CHECK_EQ(section_footer_part->target.reference_kind,
             featherdoc::section_reference_kind::default_reference);
    CHECK_EQ(section_footer_part->entry_name, "word/footer1.xml");
    CHECK_EQ(section_footer_part->left_resolved_from_section_index,
             std::optional<std::size_t>{0U});
    CHECK_EQ(section_footer_part->right_resolved_from_section_index,
             std::optional<std::size_t>{0U});
    REQUIRE_EQ(section_footer_part->paragraph_changes.size(), 1U);
    CHECK_EQ(section_footer_part->paragraph_changes[0].field_changes[0].right_value,
             "Footer approved");

    auto physical_only_options = featherdoc::document_semantic_diff_options{};
    physical_only_options.compare_resolved_section_template_parts = false;
    const auto physical_only = left.compare_semantic(right, physical_only_options);
    REQUIRE(physical_only.has_value());
    CHECK_EQ(physical_only->change_count(), 2U);
    CHECK_EQ(physical_only->template_part_results.size(), 3U);

    auto body_only_options = featherdoc::document_semantic_diff_options{};
    body_only_options.compare_template_parts = false;
    const auto body_only = left.compare_semantic(right, body_only_options);
    REQUIRE(body_only.has_value());
    CHECK_EQ(body_only->change_count(), 0U);

    fs::remove(left_path);
    fs::remove(right_path);
}

TEST_CASE("semantic diff aligns inserted paragraphs by content") {
    namespace fs = std::filesystem;

    const fs::path left_path = fs::current_path() / "semantic_diff_align_left.docx";
    const fs::path right_path = fs::current_path() / "semantic_diff_align_right.docx";
    fs::remove(left_path);
    fs::remove(right_path);

    const auto left_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Intro</w:t></w:r></w:p>
    <w:p><w:r><w:t>Scope</w:t></w:r></w:p>
    <w:p><w:r><w:t>Total</w:t></w:r></w:p>
  </w:body>
</w:document>
)"};
    const auto right_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Intro</w:t></w:r></w:p>
    <w:p><w:r><w:t>Inserted approval note</w:t></w:r></w:p>
    <w:p><w:r><w:t>Scope</w:t></w:r></w:p>
    <w:p><w:r><w:t>Total</w:t></w:r></w:p>
  </w:body>
</w:document>
)"};
    write_test_docx(left_path, left_xml);
    write_test_docx(right_path, right_xml);

    featherdoc::Document left(left_path);
    REQUIRE_FALSE(left.open());
    featherdoc::Document right(right_path);
    REQUIRE_FALSE(right.open());

    const auto aligned = left.compare_semantic(right);
    REQUIRE(aligned.has_value());
    CHECK_EQ(aligned->paragraphs.unchanged_count, 3U);
    CHECK_EQ(aligned->paragraphs.added_count, 1U);
    CHECK_EQ(aligned->paragraphs.changed_count, 0U);
    REQUIRE_EQ(aligned->paragraph_changes.size(), 1U);
    CHECK_EQ(aligned->paragraph_changes[0].kind,
             featherdoc::document_semantic_diff_change_kind::added);
    CHECK_EQ(aligned->paragraph_changes[0].right_index,
             std::optional<std::size_t>{1U});
    CHECK_NE(aligned->paragraph_changes[0].right_value.find("Inserted approval note"),
             std::string::npos);

    auto index_options = featherdoc::document_semantic_diff_options{};
    index_options.align_sequences_by_content = false;
    const auto index_aligned = left.compare_semantic(right, index_options);
    REQUIRE(index_aligned.has_value());
    CHECK_EQ(index_aligned->paragraphs.unchanged_count, 1U);
    CHECK_EQ(index_aligned->paragraphs.added_count, 1U);
    CHECK_EQ(index_aligned->paragraphs.changed_count, 2U);

    fs::remove(left_path);
    fs::remove(right_path);
}
