#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

#include <zip.h>

namespace {
void create_cli_semantic_diff_fixture(const fs::path &path,
                                      std::string_view status,
                                      std::string_view customer,
                                      std::string_view amount) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";
    const auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Semantic diff report</w:t></w:r></w:p>
    <w:p><w:r><w:t>Status: )"} + std::string{status} + R"(</w:t></w:r></w:p>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Customer Name"/>
        <w:tag w:val="customer_name"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>)" + std::string{customer} + R"(</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
    <w:tbl>
      <w:tr><w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc><w:tc><w:p><w:r><w:t>)" + std::string{amount} + R"(</w:t></w:r></w:p></w:tc></w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}


void create_cli_semantic_diff_field_fixture(
    const fs::path &path, std::uint32_t toc_max_level,
    std::string_view reference_result, std::uint32_t sequence_restart,
    std::string_view sequence_result) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";
    const auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Semantic field diff report</w:t></w:r></w:p>
    <w:p><w:fldSimple w:instr=" TOC \o &quot;1-)"} +
                              std::to_string(toc_max_level) +
                              R"(&quot; \h \z \u "><w:r><w:t>TOC placeholder</w:t></w:r></w:fldSimple></w:p>
    <w:p><w:fldSimple w:instr=" REF target_bookmark \h \* MERGEFORMAT "><w:r><w:t>)" +
                              std::string{reference_result} +
                              R"(</w:t></w:r></w:fldSimple></w:p>
    <w:p><w:fldSimple w:instr=" SEQ Figure \* ARABIC \r )" +
                              std::to_string(sequence_restart) +
                              R"( \* MERGEFORMAT "><w:r><w:t>)" +
                              std::string{sequence_result} +
                              R"(</w:t></w:r></w:fldSimple></w:p>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}



void create_cli_semantic_diff_style_numbering_fixture(
    const fs::path &path, std::string_view style_name,
    std::string_view based_on, std::string_view numbering_name,
    featherdoc::list_kind numbering_kind, std::uint32_t numbering_start,
    std::string_view numbering_pattern) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("CLI semantic style and numbering diff fixture"));

    auto style = featherdoc::paragraph_style_definition{};
    style.name = std::string{style_name};
    style.based_on = std::string{based_on};
    style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("CliSemanticDiffHeading", style));

    auto numbering = featherdoc::numbering_definition{};
    numbering.name = std::string{numbering_name};
    numbering.levels = {featherdoc::numbering_level_definition{
        numbering_kind, numbering_start, 0U, std::string{numbering_pattern}}};
    const auto numbering_id = document.ensure_numbering_definition(numbering);
    REQUIRE(numbering_id.has_value());
    REQUIRE(document.set_paragraph_style_numbering("CliSemanticDiffHeading",
                                                   *numbering_id, 0U));
    REQUIRE_FALSE(document.save());
}


void create_cli_semantic_diff_part_fixture(
    const fs::path &path, std::string_view body_text,
    std::string_view header_text, std::string_view footer_text) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";
    constexpr auto document_relationships_xml =
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

    auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>)"};
    document_xml += body_text;
    document_xml += R"(</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    auto header_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>)"};
    header_xml += header_text;
    header_xml += R"(</w:t></w:r></w:p>
</w:hdr>
)";

    auto footer_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>)"};
    footer_xml += footer_text;
    footer_xml += R"(</w:t></w:r></w:p>
</w:ftr>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/header1.xml", header_xml));
    REQUIRE(write_archive_entry(archive, "word/footer1.xml", footer_xml));
    zip_close(archive);
}


void create_cli_semantic_diff_paragraph_fixture(
    const fs::path &path, const std::vector<std::string_view> &paragraphs) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";

    auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
)"};
    for (const auto paragraph : paragraphs) {
        document_xml += "    <w:p><w:r><w:t>";
        document_xml += paragraph;
        document_xml += "</w:t></w:r></w:p>\n";
    }
    document_xml += R"(  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}
void create_cli_semantic_diff_review_fixture(
    const fs::path &path, std::string_view footnote_text,
    std::string_view endnote_text, std::string_view comment_text,
    std::string_view revision_text, std::string_view revision_author) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
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
    constexpr auto document_relationships_xml =
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

    auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
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
      <w:r><w:t>Commented text</w:t></w:r>
      <w:commentRangeEnd w:id="4"/>
      <w:r><w:commentReference w:id="4"/></w:r>
    </w:p>
    <w:p>
      <w:ins w:id="5" w:author=")"};
    document_xml += revision_author;
    document_xml += R"(" w:date="2026-05-01T10:00:00Z"><w:r><w:t>)";
    document_xml += revision_text;
    document_xml += R"(</w:t></w:r></w:ins>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";

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

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/footnotes.xml", footnotes_xml));
    REQUIRE(write_archive_entry(archive, "word/endnotes.xml", endnotes_xml));
    REQUIRE(write_archive_entry(archive, "word/comments.xml", comments_xml));
    zip_close(archive);
}
void create_cli_page_setup_fixture(const fs::path &path) {
    remove_if_exists(path);

    constexpr auto relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)";
    constexpr auto content_types_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:pgSz w:w="12240" w:h="15840"/>
          <w:pgMar w:top="1440" w:right="1800" w:bottom="1440" w:left="1800" w:header="720" w:footer="720"/>
          <w:pgNumType w:start="5"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>portrait section</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>landscape section</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/>
      <w:pgMar w:top="720" w:right="1440" w:bottom="1080" w:left="1440" w:header="360" w:footer="540"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}
} // namespace

TEST_CASE("cli semantic-diff reports document changes and can fail on diff") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_right.docx";
    const fs::path json_output = working_directory / "cli_semantic_diff.json";
    const fs::path fail_output = working_directory / "cli_semantic_diff_fail.json";
    const fs::path text_output = working_directory / "cli_semantic_diff.txt";
    const fs::path changed_text_output =
        working_directory / "cli_semantic_diff_changed.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(fail_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_fixture(left, "draft", "Ada Lovelace", "$120");
    create_cli_semantic_diff_fixture(right, "approved", "Grace Hopper", "$150");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"semantic-diff")"), std::string::npos);
    CHECK_NE(json.find(R"("different":true)"), std::string::npos);
    CHECK_NE(json.find(R"("change_count":3)"), std::string::npos);
    CHECK_NE(json.find(R"("paragraph_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("table_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("content_control_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("section_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("sections":{"left_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("field_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"text")"), std::string::npos);
    CHECK_NE(json.find("Status: draft"), std::string::npos);
    CHECK_NE(json.find("Grace Hopper"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--fail-on-diff", "--json"},
                     fail_output),
             1);
    const auto fail_json = read_text_file(fail_output);
    CHECK_NE(fail_json.find(R"("fail_on_diff":true)"), std::string::npos);
    CHECK_NE(fail_json.find(R"("different":true)"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string()},
                     changed_text_output),
             0);
    const auto changed_text = read_text_file(changed_text_output);
    CHECK_NE(changed_text.find("paragraph_change[0].field[0]: text"),
             std::string::npos);
    CHECK_NE(changed_text.find("content_control_change[0].field[0]: text"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), left.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("different: no"), std::string::npos);
    CHECK_NE(text.find("change_count: 0"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(fail_output);
    remove_if_exists(text_output);
    remove_if_exists(changed_text_output);
}





TEST_CASE("cli semantic-diff reports TOC REF and SEQ field changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_fields_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_fields_right.docx";
    const fs::path json_output = working_directory / "cli_semantic_diff_fields.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_fields_disabled.json";
    const fs::path text_output = working_directory / "cli_semantic_diff_fields.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_field_fixture(left, 2U, "Referenced heading", 1U,
                                           "Figure 1");
    create_cli_semantic_diff_field_fixture(right, 3U, "Approved heading", 2U,
                                           "Figure 2");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-sections"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("fields":{"left_count":3)"), std::string::npos);
    CHECK_NE(json.find(R"("field_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"field")"), std::string::npos);
    CHECK_NE(json.find(R"("kind":"changed")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"instruction")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"result_text")"), std::string::npos);
    CHECK_NE(json.find("kind=table_of_contents"), std::string::npos);
    CHECK_NE(json.find("kind=reference"), std::string::npos);
    CHECK_NE(json.find("kind=sequence"), std::string::npos);
    CHECK_NE(json.find("Approved heading"), std::string::npos);
    CHECK_NE(json.find(R"("template_part_results")"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-sections", "--no-fields"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("fields":{"left_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-sections"},
                     text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("fields: left=3 right=3 added=0 removed=0 changed=3"),
             std::string::npos);
    CHECK_NE(text.find("field_change[0]: changed left=0 right=0 field=field"),
             std::string::npos);
    CHECK_NE(text.find("field_change[0].field[0]: instruction"),
             std::string::npos);
    CHECK_NE(text.find("template_part_field_change[0]"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);
}



TEST_CASE("cli semantic-diff reports style and numbering summary changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left =
        working_directory / "cli_semantic_diff_style_numbering_left.docx";
    const fs::path right =
        working_directory / "cli_semantic_diff_style_numbering_right.docx";
    const fs::path json_output =
        working_directory / "cli_semantic_diff_style_numbering.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_style_numbering_disabled.json";
    const fs::path text_output =
        working_directory / "cli_semantic_diff_style_numbering.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_style_numbering_fixture(
        left, "CLI Draft Heading", "Heading1", "CliDraftOutline",
        featherdoc::list_kind::decimal, 1U, "%1.");
    create_cli_semantic_diff_style_numbering_fixture(
        right, "CLI Approved Heading", "Title", "CliApprovedOutline",
        featherdoc::list_kind::bullet, 3U, "o");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-sections",
                      "--no-template-parts"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("change_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("styles":{")"), std::string::npos);
    CHECK_NE(json.find(R"("numbering":{")"), std::string::npos);
    CHECK_NE(json.find(R"("style_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("numbering_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"style")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"numbering")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"name")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"levels.kind")"), std::string::npos);
    CHECK_NE(json.find("CLI Approved Heading"), std::string::npos);
    CHECK_NE(json.find("kind=bullet"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-sections",
                      "--no-template-parts", "--no-styles", "--no-numbering"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("styles":{"left_count":0)"),
             std::string::npos);
    CHECK_NE(disabled_json.find(R"("numbering":{"left_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-sections",
                      "--no-template-parts"},
                     text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("styles: left="), std::string::npos);
    CHECK_NE(text.find("numbering: left="), std::string::npos);
    CHECK_NE(text.find("style_change[0]: changed"), std::string::npos);
    CHECK_NE(text.find("numbering_change[0]: changed"), std::string::npos);
    CHECK_NE(text.find("style_change[0].field[0]: name"), std::string::npos);
    CHECK_NE(text.find("numbering_change[0].field"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);
}


TEST_CASE("cli semantic-diff reports review object changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_review_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_review_right.docx";
    const fs::path json_output = working_directory / "cli_semantic_diff_review.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_review_disabled.json";
    const fs::path text_output = working_directory / "cli_semantic_diff_review.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_review_fixture(
        left, "CLI original footnote", "CLI original endnote",
        "CLI original comment", "CLI inserted draft", "Ada");
    create_cli_semantic_diff_review_fixture(
        right, "CLI approved footnote", "CLI approved endnote",
        "CLI approved comment", "CLI inserted approved", "Grace");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-styles",
                      "--no-numbering", "--no-sections", "--no-template-parts"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("change_count":4)"), std::string::npos);
    CHECK_NE(json.find(R"("footnotes":{"left_count":1,"right_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("endnotes":{"left_count":1,"right_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("comments":{"left_count":1,"right_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("revisions":{"left_count":1,"right_count":1)"),
             std::string::npos);
    CHECK_NE(json.find(R"("footnote_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("endnote_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("comment_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("revision_changes")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"footnote")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"endnote")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"comment")"), std::string::npos);
    CHECK_NE(json.find(R"("field":"revision")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"text")"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"author")"), std::string::npos);
    CHECK_NE(json.find("CLI approved footnote"), std::string::npos);
    CHECK_NE(json.find("CLI inserted approved"), std::string::npos);
    CHECK_NE(json.find("Grace"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-styles",
                      "--no-numbering", "--no-sections", "--no-template-parts",
                      "--no-footnotes", "--no-endnotes", "--no-comments",
                      "--no-revisions"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("footnotes":{"left_count":0)"),
             std::string::npos);
    CHECK_NE(disabled_json.find(R"("revisions":{"left_count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-fields", "--no-styles",
                      "--no-numbering", "--no-sections", "--no-template-parts"},
                     text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("footnotes: left=1 right=1"), std::string::npos);
    CHECK_NE(text.find("endnotes: left=1 right=1"), std::string::npos);
    CHECK_NE(text.find("comments: left=1 right=1"), std::string::npos);
    CHECK_NE(text.find("revisions: left=1 right=1"), std::string::npos);
    CHECK_NE(text.find("footnote_change[0]: changed"), std::string::npos);
    CHECK_NE(text.find("revision_change[0].field"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);
}


TEST_CASE("cli semantic-diff can isolate section page setup changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_section_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_section_right.docx";
    const fs::path output = working_directory / "cli_semantic_diff_section.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_section_disabled.json";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(output);
    remove_if_exists(disabled_output);

    create_cli_page_setup_fixture(left);
    create_cli_page_setup_fixture(right);

    featherdoc::Document right_doc(right);
    REQUIRE_FALSE(right_doc.open());
    auto setup = featherdoc::section_page_setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 15840U;
    setup.height_twips = 12240U;
    setup.margins.top_twips = 720U;
    setup.margins.right_twips = 1800U;
    setup.margins.bottom_twips = 1080U;
    setup.margins.left_twips = 1800U;
    setup.margins.header_twips = 360U;
    setup.margins.footer_twips = 540U;
    setup.page_number_start = 5U;
    REQUIRE(right_doc.set_section_page_setup(0, setup));
    REQUIRE_FALSE(right_doc.save());

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls"},
                     output),
             0);
    const auto json = read_text_file(output);
    CHECK_NE(json.find(R"("sections":{"left_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("section_changes")"), std::string::npos);
    CHECK_NE(json.find("orientation=portrait"), std::string::npos);
    CHECK_NE(json.find("orientation=landscape"), std::string::npos);
    CHECK_NE(json.find("page_number_start=5"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"page_setup.orientation")"),
             std::string::npos);
    CHECK_NE(json.find(R"("field_path":"page_setup.width")"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-paragraphs", "--no-tables", "--no-images",
                      "--no-content-controls", "--no-sections"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("sections":{"left_count":0)"),
             std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(output);
    remove_if_exists(disabled_output);
}

TEST_CASE("cli semantic-diff reports header and footer template part changes") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_part_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_part_right.docx";
    const fs::path json_output = working_directory / "cli_semantic_diff_part.json";
    const fs::path disabled_output =
        working_directory / "cli_semantic_diff_part_disabled.json";
    const fs::path text_output = working_directory / "cli_semantic_diff_part.txt";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(text_output);

    create_cli_semantic_diff_part_fixture(left, "Body stable", "Header draft",
                                          "Footer draft");
    create_cli_semantic_diff_part_fixture(right, "Body stable", "Header approved",
                                          "Footer approved");

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("change_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("template_parts":{"left_count":2)"),
             std::string::npos);
    CHECK_NE(json.find(R"("changed_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("template_part_results")"), std::string::npos);
    CHECK_NE(json.find(R"("part":"header")"), std::string::npos);
    CHECK_NE(json.find(R"("part":"section-header")"), std::string::npos);
    CHECK_NE(json.find(R"("part":"section-footer")"), std::string::npos);
    CHECK_NE(json.find(R"("part_index":0)"), std::string::npos);
    CHECK_NE(json.find(R"("section_index":0)"), std::string::npos);
    CHECK_NE(json.find(R"("reference_kind":"default")"), std::string::npos);
    CHECK_NE(json.find(R"("left_resolved_from_section_index":0)"),
             std::string::npos);
    CHECK_NE(json.find(R"("right_resolved_from_section_index":0)"),
             std::string::npos);
    CHECK_NE(json.find(R"("entry_name":"word/header1.xml")"),
             std::string::npos);
    CHECK_NE(json.find(R"("entry_name":"word/footer1.xml")"),
             std::string::npos);
    CHECK_NE(json.find("Header draft"), std::string::npos);
    CHECK_NE(json.find("Header approved"), std::string::npos);
    CHECK_NE(json.find("Footer approved"), std::string::npos);
    CHECK_NE(json.find(R"("field_path":"text")"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-template-parts"},
                     disabled_output),
             0);
    const auto disabled_json = read_text_file(disabled_output);
    CHECK_NE(disabled_json.find(R"("change_count":0)"), std::string::npos);
    CHECK_NE(disabled_json.find(R"("template_parts":{"left_count":0)"),
             std::string::npos);

    const fs::path physical_only_output =
        working_directory / "cli_semantic_diff_part_physical_only.json";
    remove_if_exists(physical_only_output);
    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json",
                      "--no-resolved-section-template-parts"},
                     physical_only_output),
             0);
    const auto physical_only_json = read_text_file(physical_only_output);
    CHECK_NE(physical_only_json.find(R"("change_count":2)"), std::string::npos);
    CHECK_EQ(physical_only_json.find(R"("part":"section-header")"),
             std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string()}, text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("template_part[1]: part=header index=0"), std::string::npos);
    CHECK_NE(text.find("template_part[2]: part=footer index=0"), std::string::npos);
    CHECK_NE(text.find("template_part[3]: part=section-header section=0 kind=default"),
             std::string::npos);
    CHECK_NE(text.find("template_part[4]: part=section-footer section=0 kind=default"),
             std::string::npos);
    CHECK_NE(text.find("template_part_paragraph_change[0].field[0]: text"),
             std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(json_output);
    remove_if_exists(disabled_output);
    remove_if_exists(physical_only_output);
    remove_if_exists(text_output);
}

TEST_CASE("cli semantic-diff aligns inserted paragraphs by content") {
    const fs::path working_directory = fs::current_path();
    const fs::path left = working_directory / "cli_semantic_diff_align_left.docx";
    const fs::path right = working_directory / "cli_semantic_diff_align_right.docx";
    const fs::path aligned_output = working_directory / "cli_semantic_diff_align.json";
    const fs::path index_output = working_directory / "cli_semantic_diff_index.json";

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(aligned_output);
    remove_if_exists(index_output);

    create_cli_semantic_diff_paragraph_fixture(left, {"Intro", "Scope", "Total"});
    create_cli_semantic_diff_paragraph_fixture(
        right, {"Intro", "Inserted approval note", "Scope", "Total"});

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(), "--json"},
                     aligned_output),
             0);
    const auto aligned_json = read_text_file(aligned_output);
    CHECK_NE(aligned_json.find(R"("align_sequences_by_content":true)"),
             std::string::npos);
    CHECK_NE(aligned_json.find(R"("added_count":1)"), std::string::npos);
    CHECK_NE(aligned_json.find(R"("changed_count":0)"), std::string::npos);
    CHECK_NE(aligned_json.find(R"("unchanged_count":3)"), std::string::npos);
    CHECK_NE(aligned_json.find("Inserted approval note"), std::string::npos);

    CHECK_EQ(run_cli({"semantic-diff", left.string(), right.string(),
                      "--index-alignment", "--json"},
                     index_output),
             0);
    const auto index_json = read_text_file(index_output);
    CHECK_NE(index_json.find(R"("align_sequences_by_content":false)"),
             std::string::npos);
    CHECK_NE(index_json.find(R"("changed_count":2)"), std::string::npos);

    remove_if_exists(left);
    remove_if_exists(right);
    remove_if_exists(aligned_output);
    remove_if_exists(index_output);
}
