#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

#include <zip.h>

namespace {
void create_cli_review_inspection_fixture(const fs::path &path) {
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
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math">
  <w:body>
    <w:p>
      <w:hyperlink r:id="rHyperlink">
        <w:r><w:t>Open docs</w:t></w:r>
      </w:hyperlink>
    </w:p>
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
      <w:ins w:id="5" w:author="Ada" w:date="2026-04-01T00:00:00Z">
        <w:r><w:t>Inserted review text</w:t></w:r>
      </w:ins>
    </w:p>
    <w:p>
      <w:del w:id="6" w:author="Ada" w:date="2026-04-02T00:00:00Z">
        <w:r><w:delText>Deleted review text</w:delText></w:r>
      </w:del>
    </w:p>
    <w:p>
      <m:oMath><m:r><m:t>x+1</m:t></m:r></m:oMath>
    </w:p>
    <m:oMathPara>
      <m:oMath><m:r><m:t>y=2</m:t></m:r></m:oMath>
    </m:oMathPara>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rHyperlink"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink"
                Target="https://example.com/docs"
                TargetMode="External"/>
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
    constexpr auto footnotes_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="2">
    <w:p><w:r><w:t>Footnote body</w:t></w:r></w:p>
  </w:footnote>
</w:footnotes>
)";
    constexpr auto endnotes_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:endnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:endnote w:id="3">
    <w:p><w:r><w:t>Endnote body</w:t></w:r></w:p>
  </w:endnote>
</w:endnotes>
)";
    constexpr auto comments_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-04-03T00:00:00Z">
    <w:p><w:r><w:t>Reviewer note</w:t></w:r></w:p>
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

TEST_CASE("cli inspect-hyperlinks lists document hyperlinks") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_inspect_hyperlinks_source.docx";
    const fs::path text_output =
        working_directory / "cli_inspect_hyperlinks.txt";
    const fs::path json_output =
        working_directory / "cli_inspect_hyperlinks.json";

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-hyperlinks", source.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("hyperlinks: 1"), std::string::npos);
    CHECK_NE(text.find("Open docs"), std::string::npos);
    CHECK_NE(text.find("https://example.com/docs"), std::string::npos);
    CHECK_NE(text.find("external=yes"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-hyperlinks", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("text":"Open docs")"), std::string::npos);
    CHECK_NE(json.find(R"("relationship_id":"rHyperlink")"),
             std::string::npos);
    CHECK_NE(json.find(R"("target":"https://example.com/docs")"),
             std::string::npos);
    CHECK_NE(json.find(R"("external":true)"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-review lists notes comments and revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_inspect_review_source.docx";
    const fs::path text_output = working_directory / "cli_inspect_review.txt";
    const fs::path json_output = working_directory / "cli_inspect_review.json";

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-review", source.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("footnotes: 1"), std::string::npos);
    CHECK_NE(text.find("Footnote body"), std::string::npos);
    CHECK_NE(text.find("endnotes: 1"), std::string::npos);
    CHECK_NE(text.find("Endnote body"), std::string::npos);
    CHECK_NE(text.find("comments: 1"), std::string::npos);
    CHECK_NE(text.find("Reviewer note"), std::string::npos);
    CHECK_NE(text.find("anchor_text=Commented text"), std::string::npos);
    CHECK_NE(text.find("revisions: 2"), std::string::npos);
    CHECK_NE(text.find("kind=insertion"), std::string::npos);
    CHECK_NE(text.find("kind=deletion"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("footnotes_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("endnotes_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("comments_count":1)"), std::string::npos);
    CHECK_NE(json.find(R"("revisions_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("kind":"comment")"), std::string::npos);
    CHECK_NE(json.find(R"("author":"Reviewer")"), std::string::npos);
    CHECK_NE(json.find(R"("anchor_text":"Commented text")"),
             std::string::npos);
    CHECK_NE(json.find(R"("text":"Inserted review text")"),
             std::string::npos);
    CHECK_NE(json.find(R"("text":"Deleted review text")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
}

TEST_CASE("cli inspect-omml lists inline and display formulas") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_inspect_omml_source.docx";
    const fs::path text_output = working_directory / "cli_inspect_omml.txt";
    const fs::path json_output = working_directory / "cli_inspect_omml.json";

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"inspect-omml", source.string()}, text_output), 0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("formulas: 2"), std::string::npos);
    CHECK_NE(text.find("display=no"), std::string::npos);
    CHECK_NE(text.find("display=yes"), std::string::npos);
    CHECK_NE(text.find("x+1"), std::string::npos);
    CHECK_NE(text.find("y=2"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-omml", source.string(), "--json"}, json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("display":false)"), std::string::npos);
    CHECK_NE(json.find(R"("display":true)"), std::string::npos);
    CHECK_NE(json.find(R"("text":"x+1")"), std::string::npos);
    CHECK_NE(json.find(R"("text":"y=2")"), std::string::npos);
    CHECK_NE(json.find("<m:oMath"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(text_output);
    remove_if_exists(json_output);
}

TEST_CASE("cli hyperlink mutation appends replaces and removes links") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_hyperlink_mutation_source.docx";
    const fs::path appended = working_directory / "cli_hyperlink_mutation_appended.docx";
    const fs::path replaced = working_directory / "cli_hyperlink_mutation_replaced.docx";
    const fs::path removed = working_directory / "cli_hyperlink_mutation_removed.docx";
    const fs::path output = working_directory / "cli_hyperlink_mutation.json";
    const fs::path inspect_output = working_directory / "cli_hyperlink_mutation_inspect.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(replaced);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-hyperlink",
                      source.string(),
                      "--text",
                      "Added link",
                      "--target",
                      "https://example.com/added",
                      "--output",
                      appended.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-hyperlink",
                      appended.string(),
                      "0",
                      "--text",
                      "Changed docs",
                      "--target",
                      "https://example.com/changed",
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("command":"replace-hyperlink")"),
             std::string::npos);

    CHECK_EQ(run_cli({"remove-hyperlink",
                      replaced.string(),
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-hyperlinks", removed.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Changed docs")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("target":"https://example.com/changed")"),
             std::string::npos);
    CHECK_EQ(inspect_json.find("https://example.com/added"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(replaced);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli omml mutation appends replaces and removes formulas") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_omml_mutation_source.docx";
    const fs::path appended = working_directory / "cli_omml_mutation_appended.docx";
    const fs::path replaced = working_directory / "cli_omml_mutation_replaced.docx";
    const fs::path removed = working_directory / "cli_omml_mutation_removed.docx";
    const fs::path output = working_directory / "cli_omml_mutation.json";
    const fs::path inspect_output = working_directory / "cli_omml_mutation_inspect.json";

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(replaced);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    const std::string appended_xml =
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>z=3</m:t></m:r></m:oMath>)";
    const std::string replacement_xml =
        R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>q=4</m:t></m:r></m:oMath>)";

    CHECK_EQ(run_cli({"append-omml",
                      source.string(),
                      "--xml",
                      appended_xml,
                      "--output",
                      appended.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-omml",
                      appended.string(),
                      "0",
                      "--xml",
                      replacement_xml,
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"remove-omml",
                      replaced.string(),
                      "1",
                      "--output",
                      removed.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-omml", removed.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"q=4")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"z=3")"), std::string::npos);
    CHECK_EQ(inspect_json.find(R"("text":"y=2")"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(appended);
    remove_if_exists(replaced);
    remove_if_exists(removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli revision authoring appends insertion and deletion revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_revision_authoring_source.docx";
    const fs::path inserted = working_directory / "cli_revision_authoring_inserted.docx";
    const fs::path deleted = working_directory / "cli_revision_authoring_deleted.docx";
    const fs::path metadata_updated =
        working_directory / "cli_revision_authoring_metadata_updated.docx";
    const fs::path metadata_cleared =
        working_directory / "cli_revision_authoring_metadata_cleared.docx";
    const fs::path output = working_directory / "cli_revision_authoring.json";
    const fs::path inspect_output = working_directory / "cli_revision_authoring_inspect.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(metadata_updated);
    remove_if_exists(metadata_cleared);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-insertion-revision",
                      source.string(),
                      "--text",
                      "CLI inserted revision",
                      "--author",
                      "CLI Author",
                      "--date",
                      "2026-05-02T12:00:00Z",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"append-deletion-revision",
                      inserted.string(),
                      "--text",
                      "CLI deleted revision",
                      "--author",
                      "CLI Reviewer",
                      "--date",
                      "2026-05-02T13:00:00Z",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", deleted.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("revisions_count":4)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("kind":"insertion")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("kind":"deletion")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Author")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Reviewer")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI inserted revision")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI deleted revision")"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-revision-metadata",
                      deleted.string(),
                      "2",
                      "--author",
                      "CLI Updated Author",
                      "--date",
                      "2026-05-02T12:30:00Z",
                      "--output",
                      metadata_updated.string(),
                      "--json"},
                     output),
             0);
    const auto metadata_output_json = read_text_file(output);
    CHECK_NE(metadata_output_json.find(R"("affected":1)"), std::string::npos);
    CHECK_NE(metadata_output_json.find(R"("revision_index":2)"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-revision-metadata",
                      metadata_updated.string(),
                      "3",
                      "--clear-author",
                      "--date",
                      "2026-05-02T13:30:00Z",
                      "--output",
                      metadata_cleared.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", metadata_cleared.string(), "--json"},
                     inspect_output),
             0);
    const auto metadata_inspect_json = read_text_file(inspect_output);
    CHECK_NE(metadata_inspect_json.find(R"("revisions_count":4)"),
             std::string::npos);
    CHECK_NE(metadata_inspect_json.find(R"("author":"CLI Updated Author")"),
             std::string::npos);
    CHECK_NE(metadata_inspect_json.find(R"("date":"2026-05-02T12:30:00Z")"),
             std::string::npos);
    CHECK_NE(metadata_inspect_json.find(R"("date":"2026-05-02T13:30:00Z")"),
             std::string::npos);
    CHECK_EQ(metadata_inspect_json.find(R"("author":"CLI Reviewer")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(metadata_updated);
    remove_if_exists(metadata_cleared);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli run revision authoring creates in-place revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_run_revision_authoring_source.docx";
    const fs::path inserted = working_directory / "cli_run_revision_authoring_inserted.docx";
    const fs::path deleted = working_directory / "cli_run_revision_authoring_deleted.docx";
    const fs::path replaced = working_directory / "cli_run_revision_authoring_replaced.docx";
    const fs::path output = working_directory / "cli_run_revision_authoring.json";
    const fs::path inspect_output = working_directory / "cli_run_revision_authoring_inspect.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto source_paragraph = source_document.paragraphs();
    REQUIRE(source_paragraph.has_next());
    REQUIRE(source_paragraph.add_run("Left ").has_next());
    REQUIRE(source_paragraph.add_run("Middle").has_next());
    REQUIRE(source_paragraph.add_run(" Right").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"insert-run-revision-after",
                      source.string(),
                      "0",
                      "0",
                      "--text",
                      "CLI in-place insertion",
                      "--author",
                      "CLI Run Author",
                      "--date",
                      "2026-05-02T17:00:00Z",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(read_text_file(output).find(R"("run_index":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-run-revision",
                      inserted.string(),
                      "0",
                      "1",
                      "--author",
                      "CLI Run Reviewer",
                      "--date",
                      "2026-05-02T18:00:00Z",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"replace-run-revision",
                      deleted.string(),
                      "0",
                      "1",
                      "--text",
                      "CLI replacement",
                      "--author",
                      "CLI Run Editor",
                      "--date",
                      "2026-05-02T19:00:00Z",
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", replaced.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("revisions_count":4)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI in-place insertion")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":" Right")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI replacement")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Run Author")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Run Reviewer")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Run Editor")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli paragraph text revision authoring creates range revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_paragraph_text_revision_authoring_source.docx";
    const fs::path inserted =
        working_directory / "cli_paragraph_text_revision_authoring_inserted.docx";
    const fs::path deleted =
        working_directory / "cli_paragraph_text_revision_authoring_deleted.docx";
    const fs::path replaced =
        working_directory / "cli_paragraph_text_revision_authoring_replaced.docx";
    const fs::path output =
        working_directory / "cli_paragraph_text_revision_authoring.json";
    const fs::path inspect_output =
        working_directory / "cli_paragraph_text_revision_authoring_inspect.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto source_paragraph = source_document.paragraphs();
    REQUIRE(source_paragraph.has_next());
    REQUIRE(source_paragraph.add_run("Left ").has_next());
    REQUIRE(source_paragraph.add_run("Middle").has_next());
    REQUIRE(source_paragraph.add_run(" Right").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"insert-paragraph-text-revision",
                      source.string(),
                      "0",
                      "5",
                      "--text",
                      "CLI range insertion ",
                      "--author",
                      "CLI Range Author",
                      "--date",
                      "2026-05-02T20:00:00Z",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("paragraph_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("text_offset":5)"), std::string::npos);

    CHECK_EQ(run_cli({"insert-paragraph-text-revision",
                      source.string(),
                      "0",
                      "5",
                      "--text",
                      "CLI range insertion ",
                      "--expected-text",
                      "Middle",
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("insert-paragraph-text-revision does not accept --expected-text"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-paragraph-text-revision",
                      inserted.string(),
                      "0",
                      "5",
                      "6",
                      "--expected-text",
                      "Middle",
                      "--expected-text",
                      "Middle",
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("duplicate --expected-text option"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-paragraph-text-revision",
                      inserted.string(),
                      "0",
                      "5",
                      "6",
                      "--expected-text"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("missing value after --expected-text"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-paragraph-text-revision",
                      inserted.string(),
                      "0",
                      "5",
                      "6",
                      "--expected-text",
                      "Wrong paragraph text",
                      "--output",
                      deleted.string(),
                     "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"validate")"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_text":"Wrong paragraph text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Middle")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("preview":{)"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text_length":6)"), std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":0,"text_offset":5,"text_length":6,"text":"Middle")"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-paragraph-text-revision",
                      inserted.string(),
                      "0",
                      "5",
                      "6",
                      "--expected-text",
                      "Middle",
                      "--author",
                      "CLI Range Reviewer",
                      "--date",
                      "2026-05-02T21:00:00Z",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("text_length":6)"), std::string::npos);

    CHECK_EQ(run_cli({"replace-paragraph-text-revision",
                      deleted.string(),
                      "0",
                      "5",
                      "6",
                      "--text",
                      "CLI range replacement",
                      "--author",
                      "CLI Range Editor",
                      "--date",
                      "2026-05-02T22:00:00Z",
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", replaced.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("revisions_count":4)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI range insertion ")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":" Right")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI range replacement")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Range Author")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Range Reviewer")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Range Editor")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli text range revision authoring creates cross-paragraph revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_text_range_revision_authoring_source.docx";
    const fs::path inserted =
        working_directory / "cli_text_range_revision_authoring_inserted.docx";
    const fs::path deleted =
        working_directory / "cli_text_range_revision_authoring_deleted.docx";
    const fs::path replaced =
        working_directory / "cli_text_range_revision_authoring_replaced.docx";
    const fs::path output =
        working_directory / "cli_text_range_revision_authoring.json";
    const fs::path inspect_output =
        working_directory / "cli_text_range_revision_authoring_inspect.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Gamma ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Delta").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"preview-text-range",
                      source.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"preview-text-range")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text_length":20)"), std::string::npos);
    CHECK_NE(output_json.find(R"("plain_text_runs_supported":true)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text":"BetaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":1,"text_offset":0,"text_length":11,"text":"Middle Text")"),
             std::string::npos);

    CHECK_EQ(run_cli({"preview-text-range",
                      source.string(),
                      "2",
                      "0",
                      "1",
                      "1",
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"preview")"), std::string::npos);
    CHECK_NE(output_json.find("text range start must not be after end"),
             std::string::npos);

    CHECK_EQ(run_cli({"insert-text-range-revision",
                      source.string(),
                      "1",
                      "7",
                      "--text",
                      "CLI cross insertion ",
                      "--expected-text",
                      "Text",
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("insert-text-range-revision does not accept --expected-text"),
             std::string::npos);

    CHECK_EQ(run_cli({"insert-text-range-revision",
                      source.string(),
                      "1",
                      "7",
                      "--text",
                      "CLI cross insertion ",
                      "--author",
                      "CLI Cross Author",
                      "--date",
                      "2026-05-03T02:00:00Z",
                      "--output",
                      inserted.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("start_paragraph_index":1)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":7)"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-text-range-revision",
                      source.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--expected-text",
                      "Wrong selected text",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"validate")"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_text":"Wrong selected text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"BetaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("preview":{)"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":1,"text_offset":0,"text_length":11,"text":"Middle Text")"),
             std::string::npos);

    CHECK_EQ(run_cli({"delete-text-range-revision",
                      source.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--expected-text",
                      "BetaMiddle TextGamma",
                      "--author",
                      "CLI Cross Reviewer",
                      "--date",
                      "2026-05-03T03:00:00Z",
                      "--output",
                      deleted.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":5)"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-text-range-revision",
                      source.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--text",
                      "CLI cross replacement ",
                      "--expected-text",
                      "BetaMiddle TextGamma",
                      "--author",
                      "CLI Cross Editor",
                      "--date",
                      "2026-05-03T04:00:00Z",
                      "--output",
                      replaced.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", replaced.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("revisions_count":4)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Beta")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI cross replacement ")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle Text")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Gamma")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"CLI Cross Editor")"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(deleted);
    remove_if_exists(replaced);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli find text ranges reports paragraph offsets") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_find_text_ranges_source.docx";
    const fs::path output =
        working_directory / "cli_find_text_ranges_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Beta ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Tail").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"find-text-ranges",
                      source.string(),
                      "--text",
                      "Beta",
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"find-text-ranges")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("query":"Beta")"), std::string::npos);
    CHECK_NE(output_json.find(R"("matches_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":6)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"find-text-ranges",
                      source.string(),
                      "--text",
                      "BetaMiddle TextBeta",
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("matches_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":4)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":1,"text_offset":0,"text_length":11,"text":"Middle Text")"),
             std::string::npos);

    CHECK_EQ(run_cli({"find-text-ranges", source.string(), "--json"}, output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("missing --text"), std::string::npos);

    CHECK_EQ(run_cli({"find-text-ranges",
                      source.string(),
                      "--text",
                      "",
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"find")"), std::string::npos);
    CHECK_NE(output_json.find("search text must not be empty"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli run-recipe executes batch replace") {
    const fs::path working_directory = fs::current_path();
    const fs::path source_dir =
        working_directory / "cli_run_recipe_batch_replace_source";
    const fs::path output_dir =
        working_directory / "cli_run_recipe_batch_replace_output";
    const fs::path source = source_dir / "input.docx";
    const fs::path replaced = output_dir / "input_replaced.docx";
    const fs::path recipe =
        working_directory / "cli_run_recipe_batch_replace_recipe.json";
    const fs::path inputs =
        working_directory / "cli_run_recipe_batch_replace_inputs.json";
    const fs::path output =
        working_directory / "cli_run_recipe_batch_replace_result.json";
    const fs::path find_output =
        working_directory / "cli_run_recipe_batch_replace_find.json";

    std::error_code cleanup_error;
    fs::remove_all(source_dir, cleanup_error);
    fs::remove_all(output_dir, cleanup_error);
    remove_if_exists(recipe);
    remove_if_exists(inputs);
    remove_if_exists(output);
    remove_if_exists(find_output);

    REQUIRE(fs::create_directories(source_dir));

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Beta ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Tail").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(recipe, R"({"id":"batch_replace"})");
    write_binary_file(
        inputs,
        R"({"inputs":{"source_dir":")" + json_escape_text(source_dir.string()) +
            R"(","find_text":"Beta","replace_text":"Omega"}})");

    CHECK_EQ(run_cli({"run-recipe",
                      "--recipe",
                      recipe.string(),
                      "--inputs",
                      inputs.string(),
                      "--output",
                      output_dir.string(),
                      "--json"},
                     output),
             0);

    const auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"run-recipe")"), std::string::npos);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("recipe_id":"batch_replace")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("documents_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("changed_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("replacements_count":2)"),
             std::string::npos);
    CHECK(fs::exists(replaced));

    CHECK_EQ(run_cli({"find-text-ranges",
                      replaced.string(),
                      "--text",
                      "Omega",
                      "--json"},
                     find_output),
             0);
    const auto find_json = read_text_file(find_output);
    CHECK_NE(find_json.find(R"("matches_count":2)"), std::string::npos);

    fs::remove_all(source_dir, cleanup_error);
    fs::remove_all(output_dir, cleanup_error);
    remove_if_exists(recipe);
    remove_if_exists(inputs);
    remove_if_exists(output);
    remove_if_exists(find_output);
}

TEST_CASE("cli builds review mutation plans from found text") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_build_review_mutation_plan_source.docx";
    const fs::path request =
        working_directory / "cli_build_review_mutation_plan_request.json";
    const fs::path missing_request =
        working_directory / "cli_build_review_mutation_plan_missing.json";
    const fs::path context_request =
        working_directory / "cli_build_review_mutation_plan_context.json";
    const fs::path context_missing_request =
        working_directory /
        "cli_build_review_mutation_plan_context_missing.json";
    const fs::path unique_request =
        working_directory / "cli_build_review_mutation_plan_unique.json";
    const fs::path insert_request =
        working_directory / "cli_build_review_mutation_plan_insert.json";
    const fs::path comment_request =
        working_directory / "cli_build_review_mutation_plan_comment.json";
    const fs::path paragraph_request =
        working_directory / "cli_build_review_mutation_plan_paragraph.json";
    const fs::path plan =
        working_directory / "cli_build_review_mutation_plan_output.json";
    const fs::path context_plan =
        working_directory / "cli_build_review_mutation_plan_context_output.json";
    const fs::path insert_plan =
        working_directory / "cli_build_review_mutation_plan_insert_output.json";
    const fs::path comment_plan =
        working_directory / "cli_build_review_mutation_plan_comment_output.json";
    const fs::path applied =
        working_directory / "cli_build_review_mutation_plan_applied.docx";
    const fs::path insert_applied =
        working_directory / "cli_build_review_mutation_plan_insert_applied.docx";
    const fs::path comment_applied =
        working_directory / "cli_build_review_mutation_plan_comment_applied.docx";
    const fs::path output =
        working_directory / "cli_build_review_mutation_plan_result.json";
    const fs::path inspect_output =
        working_directory / "cli_build_review_mutation_plan_inspect.json";
    const fs::path insert_inspect_output =
        working_directory /
        "cli_build_review_mutation_plan_insert_inspect.json";
    const fs::path comment_inspect_output =
        working_directory /
        "cli_build_review_mutation_plan_comment_inspect.json";

    remove_if_exists(source);
    remove_if_exists(request);
    remove_if_exists(missing_request);
    remove_if_exists(context_request);
    remove_if_exists(context_missing_request);
    remove_if_exists(unique_request);
    remove_if_exists(insert_request);
    remove_if_exists(comment_request);
    remove_if_exists(paragraph_request);
    remove_if_exists(plan);
    remove_if_exists(context_plan);
    remove_if_exists(insert_plan);
    remove_if_exists(comment_plan);
    remove_if_exists(applied);
    remove_if_exists(insert_applied);
    remove_if_exists(comment_applied);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
    remove_if_exists(insert_inspect_output);
    remove_if_exists(comment_inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Beta ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Tail").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(
        request,
        R"({"operations":[)"
        R"({"kind":"replace_text_range_revision","find_text":"BetaMiddle TextBeta","occurrence":0,"text":"Range ","author":"Plan Builder","date":"2026-05-03T11:00:00Z"},)"
        R"({"kind":"delete_text_range_revision","find_text":"Tail","occurrence":0,"author":"Plan Cleaner","date":"2026-05-03T11:05:00Z"}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      request.string(),
                      "--output-plan",
                      plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"build-review-mutation-plan")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("operations_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("output_plan_path":")"), std::string::npos);
    CHECK_NE(output_json.find(R"("find_text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("matches_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":4)"), std::string::npos);
    CHECK(fs::exists(plan));
    const auto plan_json = read_text_file(plan);
    CHECK_NE(plan_json.find(R"("kind":"replace_text_range_revision")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("expected_text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(plan_json.find(R"("text":"Range ")"), std::string::npos);
    CHECK_NE(plan_json.find(R"("author":"Plan Cleaner")"),
             std::string::npos);

    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      plan.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("text":"Range ")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Beta")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Tail")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Builder")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Cleaner")"),
             std::string::npos);

    write_binary_file(
        context_request,
        R"({"operations":[{"kind":"delete_text_range_revision","find_text":"Beta","before_text":"Middle Text","after_text":" Tail","require_unique":true,"occurrence":0,"author":"Context Picker","date":"2026-05-03T11:10:00Z"}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      context_request.string(),
                      "--output-plan",
                      context_plan.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("raw_matches_count":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("matches_count":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("selected_match_index":1)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("require_unique":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("before_text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("after_text":" Tail")"), std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":4)"), std::string::npos);
    const auto context_plan_json = read_text_file(context_plan);
    CHECK_NE(context_plan_json.find(R"("expected_text":"Beta")"),
             std::string::npos);
    CHECK_NE(context_plan_json.find(R"("author":"Context Picker")"),
             std::string::npos);

    write_binary_file(
        insert_request,
        R"({"operations":[)"
        R"({"kind":"insert_text_range_revision","find_text":"Beta","before_text":"Middle Text","after_text":" Tail","require_unique":true,"insert_after_match":true,"text":" inserted-after ","author":"Insert Builder","date":"2026-05-03T12:40:00Z"},)"
        R"({"kind":"insert_paragraph_text_revision","find_text":"Alpha","require_unique":true,"insert_after_match":false,"text":"Start ","author":"Paragraph Inserter","date":"2026-05-03T12:41:00Z"}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      insert_request.string(),
                      "--output-plan",
                      insert_plan.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("operations_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"insert_text_range_revision")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"insert_paragraph_text_revision")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("insert_after_match":true)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("start_text_offset":4)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("text_offset":0)"), std::string::npos);
    CHECK(fs::exists(insert_plan));
    const auto insert_plan_json = read_text_file(insert_plan);
    CHECK_NE(insert_plan_json.find(R"("kind":"insert_text_range_revision")"),
             std::string::npos);
    CHECK_NE(
        insert_plan_json.find(R"("kind":"insert_paragraph_text_revision")"),
        std::string::npos);
    CHECK_NE(insert_plan_json.find(R"("text":" inserted-after ")"),
             std::string::npos);
    CHECK_NE(insert_plan_json.find(R"("text":"Start ")"), std::string::npos);
    CHECK_EQ(insert_plan_json.find(R"("expected_text")"), std::string::npos);
    CHECK_EQ(insert_plan_json.find(R"("text_length")"), std::string::npos);
    CHECK_EQ(insert_plan_json.find(R"("end_paragraph_index")"),
             std::string::npos);
    CHECK_EQ(insert_plan_json.find(R"("end_text_offset")"), std::string::npos);

    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      insert_plan.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      insert_plan.string(),
                      "--output",
                      insert_applied.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", insert_applied.string(), "--json"},
                     insert_inspect_output),
             0);
    const auto insert_inspect_json = read_text_file(insert_inspect_output);
    CHECK_NE(insert_inspect_json.find(R"("text":"Start ")"),
             std::string::npos);
    CHECK_NE(insert_inspect_json.find(R"("text":" inserted-after ")"),
             std::string::npos);
    CHECK_NE(insert_inspect_json.find(R"("author":"Paragraph Inserter")"),
             std::string::npos);
    CHECK_NE(insert_inspect_json.find(R"("author":"Insert Builder")"),
             std::string::npos);

    write_binary_file(
        comment_request,
        R"({"operations":[)"
        R"({"kind":"append_text_range_comment","find_text":"BetaMiddle TextBeta","comment_text":"Review this cross-paragraph span.","author":"Comment Builder","initials":"CB","date":"2026-05-03T12:45:00Z"},)"
        R"({"kind":"append_paragraph_text_comment","find_text":"Tail","comment_text":"Check final tail.","author":"Paragraph Commenter","initials":"PC","date":"2026-05-03T12:46:00Z"}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      comment_request.string(),
                      "--output-plan",
                      comment_plan.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"append_text_range_comment")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"append_paragraph_text_comment")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text":"Tail")"), std::string::npos);
    CHECK(fs::exists(comment_plan));
    const auto comment_plan_json = read_text_file(comment_plan);
    CHECK_NE(comment_plan_json.find(R"("comment_text":"Review this cross-paragraph span.")"),
             std::string::npos);
    CHECK_NE(comment_plan_json.find(R"("comment_text":"Check final tail.")"),
             std::string::npos);
    CHECK_NE(comment_plan_json.find(R"("expected_text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(comment_plan_json.find(R"("expected_text":"Tail")"),
             std::string::npos);
    CHECK_NE(comment_plan_json.find(R"("initials":"CB")"),
             std::string::npos);
    CHECK_EQ(comment_plan_json.find(R"("insert_after_match")"),
             std::string::npos);

    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      comment_plan.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      comment_plan.string(),
                      "--output",
                      comment_applied.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", comment_applied.string(), "--json"},
                     comment_inspect_output),
             0);
    const auto comment_inspect_json = read_text_file(comment_inspect_output);
    CHECK_NE(comment_inspect_json.find(R"("comments_count":2)"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("text":"Review this cross-paragraph span.")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("text":"Check final tail.")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("anchor_text":"BetaMiddle TextBeta")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("anchor_text":"Tail")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("initials":"CB")"),
             std::string::npos);
    CHECK_NE(comment_inspect_json.find(R"("initials":"PC")"),
             std::string::npos);

    write_binary_file(
        context_missing_request,
        R"({"operations":[{"kind":"delete_text_range_revision","find_text":"Beta","before_text":"Missing context","occurrence":0}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      context_missing_request.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("matches_count":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("raw_matches_count":2)"),
             std::string::npos);
    CHECK_NE(output_json.find("requested text occurrence was not found"),
             std::string::npos);

    write_binary_file(
        unique_request,
        R"({"operations":[{"kind":"delete_text_range_revision","find_text":"Beta","require_unique":true}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      unique_request.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("matches_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("raw_matches_count":2)"),
             std::string::npos);
    CHECK_NE(output_json.find("requested text did not resolve to a unique match"),
             std::string::npos);

    write_binary_file(
        missing_request,
        R"({"operations":[{"kind":"delete_text_range_revision","find_text":"Missing","occurrence":0}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      missing_request.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"resolve")"), std::string::npos);
    CHECK_NE(output_json.find(R"("matches_count":0)"), std::string::npos);
    CHECK_NE(output_json.find("requested text occurrence was not found"),
             std::string::npos);

    write_binary_file(
        paragraph_request,
        R"({"operations":[{"kind":"replace_paragraph_text_revision","find_text":"BetaMiddle TextBeta","text":"Range "}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      paragraph_request.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("matched text crosses paragraphs"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(request);
    remove_if_exists(missing_request);
    remove_if_exists(context_request);
    remove_if_exists(context_missing_request);
    remove_if_exists(unique_request);
    remove_if_exists(insert_request);
    remove_if_exists(comment_request);
    remove_if_exists(paragraph_request);
    remove_if_exists(plan);
    remove_if_exists(context_plan);
    remove_if_exists(insert_plan);
    remove_if_exists(comment_plan);
    remove_if_exists(applied);
    remove_if_exists(insert_applied);
    remove_if_exists(comment_applied);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
    remove_if_exists(insert_inspect_output);
    remove_if_exists(comment_inspect_output);
}

TEST_CASE("cli review mutation plan previews expected text guards") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_preview_source.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_preview_success.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_preview_mismatch.json";
    const fs::path invalid_plan =
        working_directory / "cli_review_mutation_plan_preview_invalid.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_preview_output.json";

    remove_if_exists(source);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(invalid_plan);
    remove_if_exists(output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Gamma ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Delta").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(
        success_plan,
        R"({"operations":[)"
        R"({"kind":"delete_paragraph_text_revision","paragraph_index":1,"text_offset":0,"text_length":11,"expected_text":"Middle Text"},)"
        R"({"kind":"replace-text-range-revision","start_paragraph_index":0,"start_text_offset":6,"end_paragraph_index":2,"end_text_offset":5,"text":"Range ","expected_text":"BetaMiddle TextGamma"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"preview-review-mutation-plan")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("operations_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"delete_paragraph_text_revision")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"replace_text_range_revision")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("expected_text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"BetaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("paragraph_index":1,"text_offset":0,"text_length":11,"text":"Middle Text")"),
             std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[)"
        R"({"kind":"delete_text_range_revision","start_paragraph_index":0,"start_text_offset":6,"end_paragraph_index":2,"end_text_offset":5,"expected_text":"Wrong selected text"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":1)"), std::string::npos);
    CHECK_NE(output_json.find("expected text did not match selected text"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("expected_text":"Wrong selected text")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"BetaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("preview":{)"), std::string::npos);

    write_binary_file(
        invalid_plan,
        R"({"operations":[{"kind":"replace_paragraph_text_revision","paragraph_index":0,"text_offset":0,"text_length":5}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      invalid_plan.string(),
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"parse")"), std::string::npos);
    CHECK_NE(output_json.find("replace_paragraph_text_revision operation must contain"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(invalid_plan);
    remove_if_exists(output);
}

TEST_CASE("cli review mutation plan applies guarded revisions atomically") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_apply_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_apply_output.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_apply_mismatch.docx";
    const fs::path overlap_output =
        working_directory / "cli_review_mutation_plan_apply_overlap.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_apply_success.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_apply_mismatch.json";
    const fs::path overlap_plan =
        working_directory / "cli_review_mutation_plan_apply_overlap.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_apply_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_apply_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(overlap_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(overlap_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Gamma ");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Delta").has_next());
    auto fourth = third.insert_paragraph_after("Tail ");
    REQUIRE(fourth.has_next());
    REQUIRE(fourth.add_run("End").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(
        success_plan,
        R"({"operations":[)"
        R"({"kind":"replace_paragraph_text_revision","paragraph_index":0,"text_offset":6,"text_length":4,"text":"BETA","expected_text":"Beta","author":"Plan Editor","date":"2026-05-03T10:00:00Z"},)"
        R"({"kind":"delete-paragraph-text-revision","paragraph_index":1,"text_offset":0,"text_length":11,"expected_text":"Middle Text","author":"Plan Reviewer","date":"2026-05-03T10:05:00Z"},)"
        R"({"kind":"replace_text_range_revision","start_paragraph_index":2,"start_text_offset":0,"end_paragraph_index":3,"end_text_offset":4,"text":"Cross ","expected_text":"Gamma DeltaTail","author":"Plan Cross Editor","date":"2026-05-03T10:10:00Z"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("command":"apply-review-mutation-plan")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("operations_count":3)"), std::string::npos);
    CHECK_NE(output_json.find(R"("applied_count":3)"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":0)"), std::string::npos);
    CHECK(fs::exists(applied));

    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("text":"Beta")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"BETA")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Middle Text")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Gamma Delta")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Tail")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Cross ")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Editor")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Reviewer")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Cross Editor")"),
             std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"replace_paragraph_text_revision","paragraph_index":0,"text_offset":6,"text_length":4,"text":"BETA","expected_text":"Wrong"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--output",
                      mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"preflight")"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":1)"), std::string::npos);
    CHECK_NE(output_json.find("expected text did not match selected text"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        overlap_plan,
        R"({"operations":[)"
        R"({"kind":"delete_paragraph_text_revision","paragraph_index":1,"text_offset":0,"text_length":11,"expected_text":"Middle Text"},)"
        R"({"kind":"replace_text_range_revision","start_paragraph_index":0,"start_text_offset":6,"end_paragraph_index":2,"end_text_offset":5,"text":"Range ","expected_text":"BetaMiddle TextGamma"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      overlap_plan.string(),
                      "--output",
                      overlap_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"validate")"), std::string::npos);
    CHECK_NE(output_json.find("operation ranges overlap"), std::string::npos);
    CHECK_FALSE(fs::exists(overlap_output));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(overlap_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(overlap_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review mutation plan allows overlapping comment anchors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_overlap_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_overlap_output.docx";
    const fs::path plan =
        working_directory / "cli_review_mutation_plan_comment_overlap.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_overlap_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_overlap_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto paragraph = source_document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Alpha ").has_next());
    REQUIRE(paragraph.add_run("Beta").has_next());
    REQUIRE_FALSE(source_document.save());

    write_binary_file(
        plan,
        R"({"operations":[)"
        R"({"kind":"append_paragraph_text_comment","paragraph_index":0,"text_offset":0,"text_length":10,"comment_text":"Check the full phrase.","expected_text":"Alpha Beta","author":"Outer Commenter","initials":"OC","date":"2026-05-03T14:00:00Z"},)"
        R"({"kind":"append_paragraph_text_comment","paragraph_index":0,"text_offset":6,"text_length":4,"comment_text":"Check the nested word.","expected_text":"Beta","author":"Inner Commenter","initials":"IC","date":"2026-05-03T14:01:00Z"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(output_json.find(R"("operations_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("applied_count":2)"), std::string::npos);
    CHECK_NE(output_json.find(R"("failed_count":0)"), std::string::npos);
    CHECK(fs::exists(applied));

    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Beta")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Check the full phrase.")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Check the nested word.")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("initials":"OC")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("initials":"IC")"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review mutation plan sets comment resolved state") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_resolved_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_resolved_output.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_resolved_mismatch.docx";
    const fs::path state_output =
        working_directory / "cli_review_mutation_plan_comment_resolved_state.docx";
    const fs::path bounds_output =
        working_directory / "cli_review_mutation_plan_comment_resolved_bounds.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_comment_resolved.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_resolved_mismatch.json";
    const fs::path state_plan =
        working_directory / "cli_review_mutation_plan_comment_resolved_state.json";
    const fs::path bounds_plan =
        working_directory / "cli_review_mutation_plan_comment_resolved_bounds.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_resolved_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_resolved_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(state_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(state_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto paragraph = source_document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Alpha ").has_next());
    REQUIRE(paragraph.add_run("Beta").has_next());
    REQUIRE_FALSE(source_document.save());
    REQUIRE_EQ(run_cli({"append-paragraph-text-comment",
                        source.string(),
                        "0",
                        "0",
                        "10",
                        "--comment-text",
                        "Resolve this comment.",
                        "--author",
                        "Plan Reviewer",
                        "--initials",
                        "PR",
                        "--date",
                        "2026-05-03T15:00:00Z",
                        "--json"},
                       output),
               0);

    write_binary_file(
        success_plan,
        R"({"operations":[{"kind":"set_comment_resolved","comment_index":0,"resolved":true,"expected_resolved":false,"expected_text":"Alpha Beta"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"set_comment_resolved")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_resolved":false)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("applied_count":1)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("resolved":true)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alpha Beta")"),
             std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"set-comment-resolved","comment_index":0,"resolved":true,"expected_resolved":false,"expected_text":"Wrong anchor"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--output",
                      mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"preflight")"), std::string::npos);
    CHECK_NE(output_json.find("expected text did not match comment anchor text"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        state_plan,
        R"({"operations":[{"kind":"set_comment_resolved","comment_index":0,"resolved":true,"expected_resolved":true,"expected_text":"Alpha Beta"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      state_plan.string(),
                      "--output",
                      state_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected resolved state did not match comment state"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);
    CHECK_FALSE(fs::exists(state_output));

    write_binary_file(
        bounds_plan,
        R"({"operations":[{"kind":"set_comment_resolved","comment_index":5,"resolved":true}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      bounds_plan.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("comment index is out of range"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":5)"), std::string::npos);
    CHECK_FALSE(fs::exists(bounds_output));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(state_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(state_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review mutation plan appends comment replies") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_reply_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_reply_output.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_reply_mismatch.docx";
    const fs::path state_output =
        working_directory / "cli_review_mutation_plan_comment_reply_state.docx";
    const fs::path bounds_output =
        working_directory / "cli_review_mutation_plan_comment_reply_bounds.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_comment_reply.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_reply_mismatch.json";
    const fs::path state_plan =
        working_directory / "cli_review_mutation_plan_comment_reply_state.json";
    const fs::path bounds_plan =
        working_directory / "cli_review_mutation_plan_comment_reply_bounds.json";
    const fs::path build_request =
        working_directory / "cli_review_mutation_plan_comment_reply_build_request.json";
    const fs::path built_plan =
        working_directory / "cli_review_mutation_plan_comment_reply_built.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_reply_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_reply_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(state_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(state_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(build_request);
    remove_if_exists(built_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto paragraph = source_document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Alpha ").has_next());
    REQUIRE(paragraph.add_run("Beta").has_next());
    REQUIRE_FALSE(source_document.save());
    REQUIRE_EQ(run_cli({"append-paragraph-text-comment",
                        source.string(),
                        "0",
                        "0",
                        "10",
                        "--comment-text",
                        "Parent comment.",
                        "--author",
                        "Plan Reviewer",
                        "--initials",
                        "PR",
                        "--date",
                        "2026-05-03T15:30:00Z",
                        "--json"},
                       output),
               0);

    write_binary_file(
        success_plan,
        R"({"operations":[{"kind":"append_comment_reply","comment_index":0,"comment_text":"Reply from plan.","author":"Reply Reviewer","initials":"RR","date":"2026-05-03T16:00:00Z","expected_resolved":false,"expected_text":"Alpha Beta"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"append_comment_reply")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_resolved":false)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("applied_count":1)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Reply from plan.")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Reply Reviewer")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("initials":"RR")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("date":"2026-05-03T16:00:00Z")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("parent_index":0)"), std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"append-comment-reply","comment_index":0,"comment_text":"Reply from mismatch plan.","expected_resolved":false,"expected_text":"Wrong anchor"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--output",
                      mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"preflight")"), std::string::npos);
    CHECK_NE(output_json.find("expected text did not match comment anchor text"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        state_plan,
        R"({"operations":[{"kind":"append_comment_reply","comment_index":0,"comment_text":"Reply from state plan.","expected_resolved":true,"expected_text":"Alpha Beta"}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      state_plan.string(),
                      "--output",
                      state_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected resolved state did not match comment state"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);
    CHECK_FALSE(fs::exists(state_output));

    write_binary_file(
        bounds_plan,
        R"({"operations":[{"kind":"append_comment_reply","comment_index":5,"comment_text":"Out of range reply."}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      bounds_plan.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("comment index is out of range"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":5)"), std::string::npos);
    CHECK_FALSE(fs::exists(bounds_output));

    write_binary_file(
        build_request,
        R"({"operations":[{"kind":"append_comment_reply","find_text":"Alpha Beta","comment_text":"Reply from build request."}]})");
    CHECK_EQ(run_cli({"build-review-mutation-plan",
                      source.string(),
                      "--request-file",
                      build_request.string(),
                      "--output-plan",
                      built_plan.string(),
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(
                 "does not support direct comment-index operations"),
             std::string::npos);
    CHECK_FALSE(fs::exists(built_plan));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(state_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(state_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(build_request);
    remove_if_exists(built_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review mutation plan updates comment metadata") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_metadata_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_metadata_output.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_metadata_mismatch.docx";
    const fs::path parent_output =
        working_directory / "cli_review_mutation_plan_comment_metadata_parent.docx";
    const fs::path bounds_output =
        working_directory / "cli_review_mutation_plan_comment_metadata_bounds.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata_mismatch.json";
    const fs::path parent_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata_parent.json";
    const fs::path conflict_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata_conflict.json";
    const fs::path bounds_plan =
        working_directory / "cli_review_mutation_plan_comment_metadata_bounds.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_metadata_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_metadata_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(parent_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(parent_plan);
    remove_if_exists(conflict_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto paragraph = source_document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Alpha ").has_next());
    REQUIRE(paragraph.add_run("Beta").has_next());
    REQUIRE_FALSE(source_document.save());
    REQUIRE_EQ(run_cli({"append-paragraph-text-comment",
                        source.string(),
                        "0",
                        "0",
                        "10",
                        "--comment-text",
                        "Parent metadata comment.",
                        "--author",
                        "Parent Reviewer",
                        "--initials",
                        "PR",
                        "--date",
                        "2026-05-03T16:30:00Z",
                        "--json"},
                       output),
               0);
    REQUIRE_EQ(run_cli({"append-comment-reply",
                        source.string(),
                        "0",
                        "--comment-text",
                        "Reply metadata body.",
                        "--author",
                        "Reply Reviewer",
                        "--initials",
                        "RR",
                        "--date",
                        "2026-05-03T16:40:00Z",
                        "--json"},
                       output),
               0);

    write_binary_file(
        success_plan,
        R"({"operations":[{"kind":"set_comment_metadata","comment_index":1,"author":"Plan Updated Responder","clear_initials":true,"date":"2026-05-03T17:10:00Z","expected_comment_text":"Reply metadata body.","expected_parent_index":0,"expected_resolved":false}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"set_comment_metadata")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("expected_comment_text":"Reply metadata body.")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_comment_text":"Reply metadata body.")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("expected_parent_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_parent_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_resolved":false)"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("applied_count":1)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Reply metadata body.")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("author":"Plan Updated Responder")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("initials":null)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("date":"2026-05-03T17:10:00Z")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("parent_index":0)"), std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"set-comment-metadata","comment_index":1,"author":"Wrong target","expected_comment_text":"Wrong reply body.","expected_parent_index":0}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--output",
                      mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("stage":"preflight")"), std::string::npos);
    CHECK_NE(output_json.find("expected comment text did not match comment body"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_comment_text":"Reply metadata body.")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        parent_plan,
        R"({"operations":[{"kind":"set_comment_metadata","comment_index":1,"clear_author":true,"expected_comment_text":"Reply metadata body.","expected_parent_index":3}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      parent_plan.string(),
                      "--output",
                      parent_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected parent index did not match comment parent"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_parent_index":0)"),
             std::string::npos);
    CHECK_FALSE(fs::exists(parent_output));

    write_binary_file(
        conflict_plan,
        R"({"operations":[{"kind":"set_comment_metadata","comment_index":1,"author":"Conflicting","clear_author":true}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      conflict_plan.string(),
                      "--json"},
                     output),
             2);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(
                 "cannot set and clear the same metadata field"),
             std::string::npos);

    write_binary_file(
        bounds_plan,
        R"({"operations":[{"kind":"set_comment_metadata","comment_index":9,"author":"Out of range"}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      bounds_plan.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("comment index is out of range"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("comment_index":9)"), std::string::npos);
    CHECK_FALSE(fs::exists(bounds_output));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(mismatch_output);
    remove_if_exists(parent_output);
    remove_if_exists(bounds_output);
    remove_if_exists(success_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(parent_plan);
    remove_if_exists(conflict_plan);
    remove_if_exists(bounds_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review mutation plan replaces and removes comments") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_review_mutation_plan_comment_body_source.docx";
    const fs::path applied =
        working_directory / "cli_review_mutation_plan_comment_body_output.docx";
    const fs::path parent_removed =
        working_directory / "cli_review_mutation_plan_comment_body_parent_removed.docx";
    const fs::path mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_body_mismatch.docx";
    const fs::path parent_mismatch_output =
        working_directory / "cli_review_mutation_plan_comment_body_parent_mismatch.docx";
    const fs::path success_plan =
        working_directory / "cli_review_mutation_plan_comment_body.json";
    const fs::path parent_remove_plan =
        working_directory / "cli_review_mutation_plan_comment_body_parent_remove.json";
    const fs::path mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_body_mismatch.json";
    const fs::path parent_mismatch_plan =
        working_directory / "cli_review_mutation_plan_comment_body_parent_mismatch.json";
    const fs::path output =
        working_directory / "cli_review_mutation_plan_comment_body_output.json";
    const fs::path inspect_output =
        working_directory / "cli_review_mutation_plan_comment_body_inspect.json";

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(parent_removed);
    remove_if_exists(mismatch_output);
    remove_if_exists(parent_mismatch_output);
    remove_if_exists(success_plan);
    remove_if_exists(parent_remove_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(parent_mismatch_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto paragraph = source_document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Alpha ").has_next());
    REQUIRE(paragraph.add_run("Beta").has_next());
    REQUIRE_FALSE(source_document.save());
    REQUIRE_EQ(run_cli({"append-paragraph-text-comment",
                        source.string(),
                        "0",
                        "0",
                        "10",
                        "--comment-text",
                        "Original parent body.",
                        "--author",
                        "Parent Reviewer",
                        "--initials",
                        "PR",
                        "--date",
                        "2026-05-03T17:40:00Z",
                        "--json"},
                       output),
               0);
    REQUIRE_EQ(run_cli({"append-comment-reply",
                        source.string(),
                        "0",
                        "--comment-text",
                        "Reply body to remove.",
                        "--author",
                        "Reply Reviewer",
                        "--initials",
                        "RR",
                        "--date",
                        "2026-05-03T17:45:00Z",
                        "--json"},
                       output),
               0);

    write_binary_file(
        success_plan,
        R"({"operations":[{"kind":"replace_comment","comment_index":0,"comment_text":"Plan replaced parent body.","expected_text":"Alpha Beta","expected_comment_text":"Original parent body.","expected_resolved":false},{"kind":"remove_comment","comment_index":1,"expected_comment_text":"Reply body to remove.","expected_parent_index":0}]})");
    CHECK_EQ(run_cli({"preview-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("kind":"replace_comment")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("kind":"remove_comment")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_comment_text":"Original parent body.")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_comment_text":"Reply body to remove.")"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_parent_index":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      success_plan.string(),
                      "--output",
                      applied.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("applied_count":2)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", applied.string(), "--json"},
                     inspect_output),
             0);
    auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Plan replaced parent body.")"),
             std::string::npos);
    CHECK_EQ(inspect_json.find(R"("text":"Reply body to remove.")"),
             std::string::npos);

    write_binary_file(
        parent_remove_plan,
        R"({"operations":[{"kind":"remove_comment","comment_index":0,"expected_text":"Alpha Beta","expected_comment_text":"Original parent body.","expected_resolved":false}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      parent_remove_plan.string(),
                      "--output",
                      parent_removed.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("applied_count":1)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", parent_removed.string(), "--json"},
                     inspect_output),
             0);
    inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":0)"), std::string::npos);

    write_binary_file(
        mismatch_plan,
        R"({"operations":[{"kind":"replace-comment","comment_index":0,"comment_text":"Wrong replacement target.","expected_text":"Wrong anchor","expected_comment_text":"Original parent body."}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      mismatch_plan.string(),
                      "--output",
                      mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected text did not match comment anchor text"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_text":"Alpha Beta")"),
             std::string::npos);
    CHECK_FALSE(fs::exists(mismatch_output));

    write_binary_file(
        parent_mismatch_plan,
        R"({"operations":[{"kind":"remove_comment","comment_index":1,"expected_comment_text":"Reply body to remove.","expected_parent_index":5}]})");
    CHECK_EQ(run_cli({"apply-review-mutation-plan",
                      source.string(),
                      "--plan-file",
                      parent_mismatch_plan.string(),
                      "--output",
                      parent_mismatch_output.string(),
                      "--json"},
                     output),
             1);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find("expected parent index did not match comment parent"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("actual_parent_index":0)"),
             std::string::npos);
    CHECK_FALSE(fs::exists(parent_mismatch_output));

    remove_if_exists(source);
    remove_if_exists(applied);
    remove_if_exists(parent_removed);
    remove_if_exists(mismatch_output);
    remove_if_exists(parent_mismatch_output);
    remove_if_exists(success_plan);
    remove_if_exists(parent_remove_plan);
    remove_if_exists(mismatch_plan);
    remove_if_exists(parent_mismatch_plan);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli revision mutation accepts and rejects revisions") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_revision_mutation_source.docx";
    const fs::path accepted = working_directory / "cli_revision_mutation_accepted.docx";
    const fs::path clean = working_directory / "cli_revision_mutation_clean.docx";
    const fs::path output = working_directory / "cli_revision_mutation.json";
    const fs::path inspect_output = working_directory / "cli_revision_mutation_inspect.json";

    remove_if_exists(source);
    remove_if_exists(accepted);
    remove_if_exists(clean);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"accept-revision",
                      source.string(),
                      "0",
                      "--output",
                      accepted.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", accepted.string(), "--json"},
                     inspect_output),
             0);
    CHECK_NE(read_text_file(inspect_output).find(R"("revisions_count":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"reject-all-revisions",
                      accepted.string(),
                      "--output",
                      clean.string(),
                      "--json"},
                     output),
             0);
    CHECK_NE(read_text_file(output).find(R"("affected":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", clean.string(), "--json"},
                     inspect_output),
             0);
    CHECK_NE(read_text_file(inspect_output).find(R"("revisions_count":0)"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(accepted);
    remove_if_exists(clean);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli review note mutation updates notes and comments") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_review_note_mutation_source.docx";
    const fs::path footnote_added = working_directory / "cli_review_note_footnote_added.docx";
    const fs::path footnote_replaced = working_directory / "cli_review_note_footnote_replaced.docx";
    const fs::path footnote_removed = working_directory / "cli_review_note_footnote_removed.docx";
    const fs::path endnote_added = working_directory / "cli_review_note_endnote_added.docx";
    const fs::path endnote_replaced = working_directory / "cli_review_note_endnote_replaced.docx";
    const fs::path endnote_removed = working_directory / "cli_review_note_endnote_removed.docx";
    const fs::path comment_added = working_directory / "cli_review_note_comment_added.docx";
    const fs::path comment_replaced = working_directory / "cli_review_note_comment_replaced.docx";
    const fs::path final_doc = working_directory / "cli_review_note_final.docx";
    const fs::path output = working_directory / "cli_review_note_mutation.json";
    const fs::path inspect_output = working_directory / "cli_review_note_inspect.json";

    remove_if_exists(source);
    remove_if_exists(footnote_added);
    remove_if_exists(footnote_replaced);
    remove_if_exists(footnote_removed);
    remove_if_exists(endnote_added);
    remove_if_exists(endnote_replaced);
    remove_if_exists(endnote_removed);
    remove_if_exists(comment_added);
    remove_if_exists(comment_replaced);
    remove_if_exists(final_doc);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    create_cli_review_inspection_fixture(source);

    CHECK_EQ(run_cli({"append-footnote",
                      source.string(),
                      "--reference-text",
                      "Added footnote mark ",
                      "--note-text",
                      "Added footnote body",
                      "--output",
                      footnote_added.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"replace-footnote",
                      footnote_added.string(),
                      "0",
                      "--note-text",
                      "Changed footnote body",
                      "--output",
                      footnote_replaced.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"remove-footnote",
                      footnote_replaced.string(),
                      "1",
                      "--output",
                      footnote_removed.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"append-endnote",
                      footnote_removed.string(),
                      "--reference-text",
                      "Added endnote mark ",
                      "--note-text",
                      "Added endnote body",
                      "--output",
                      endnote_added.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"replace-endnote",
                      endnote_added.string(),
                      "0",
                      "--note-text",
                      "Changed endnote body",
                      "--output",
                      endnote_replaced.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"remove-endnote",
                      endnote_replaced.string(),
                      "1",
                      "--output",
                      endnote_removed.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"append-comment",
                      endnote_removed.string(),
                      "--selected-text",
                      "New commented text",
                      "--comment-text",
                      "Added comment body",
                      "--author",
                      "CLI Reviewer",
                      "--initials",
                      "CR",
                      "--date",
                      "2026-05-02T11:00:00Z",
                      "--output",
                      comment_added.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", comment_added.string(), "--json"},
                     inspect_output),
             0);
    const auto appended_comment_json = read_text_file(inspect_output);
    CHECK_NE(appended_comment_json.find(R"("comments_count":2)"),
             std::string::npos);
    CHECK_NE(appended_comment_json.find(R"("anchor_text":"New commented text")"),
             std::string::npos);
    CHECK_NE(appended_comment_json.find(R"("text":"Added comment body")"),
             std::string::npos);
    CHECK_NE(appended_comment_json.find(R"("date":"2026-05-02T11:00:00Z")"),
             std::string::npos);
    CHECK_EQ(run_cli({"replace-comment",
                      comment_added.string(),
                      "0",
                      "--comment-text",
                      "Changed comment body",
                      "--output",
                      comment_replaced.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"remove-comment",
                      comment_replaced.string(),
                      "1",
                      "--output",
                      final_doc.string(),
                      "--json"},
                     output),
             0);

    CHECK_EQ(run_cli({"inspect-review", final_doc.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("footnotes_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("endnotes_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("comments_count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Changed footnote body")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Changed endnote body")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Commented text")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Changed comment body")"),
             std::string::npos);
    CHECK_EQ(inspect_json.find("Added footnote body"), std::string::npos);
    CHECK_EQ(inspect_json.find("Added endnote body"), std::string::npos);
    CHECK_EQ(inspect_json.find("Added comment body"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(footnote_added);
    remove_if_exists(footnote_replaced);
    remove_if_exists(footnote_removed);
    remove_if_exists(endnote_added);
    remove_if_exists(endnote_replaced);
    remove_if_exists(endnote_removed);
    remove_if_exists(comment_added);
    remove_if_exists(comment_replaced);
    remove_if_exists(final_doc);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli comment range authoring creates in-place comments") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_comment_range_source.docx";
    const fs::path paragraph_comment =
        working_directory / "cli_comment_range_paragraph.docx";
    const fs::path cross_comment =
        working_directory / "cli_comment_range_cross.docx";
    const fs::path paragraph_moved =
        working_directory / "cli_comment_range_paragraph_moved.docx";
    const fs::path range_moved =
        working_directory / "cli_comment_range_moved.docx";
    const fs::path resolved =
        working_directory / "cli_comment_range_resolved.docx";
    const fs::path threaded =
        working_directory / "cli_comment_range_threaded.docx";
    const fs::path metadata_updated =
        working_directory / "cli_comment_range_metadata.docx";
    const fs::path thread_removed =
        working_directory / "cli_comment_range_thread_removed.docx";
    const fs::path output = working_directory / "cli_comment_range.json";
    const fs::path inspect_output =
        working_directory / "cli_comment_range_inspect.json";

    remove_if_exists(source);
    remove_if_exists(paragraph_comment);
    remove_if_exists(cross_comment);
    remove_if_exists(paragraph_moved);
    remove_if_exists(range_moved);
    remove_if_exists(resolved);
    remove_if_exists(threaded);
    remove_if_exists(metadata_updated);
    remove_if_exists(thread_removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);

    featherdoc::Document source_document(source);
    REQUIRE_FALSE(source_document.create_empty());
    auto first = source_document.paragraphs();
    REQUIRE(first.has_next());
    REQUIRE(first.add_run("Alpha ").has_next());
    REQUIRE(first.add_run("Beta").has_next());
    REQUIRE(first.add_run(" Gamma").has_next());
    auto second = first.insert_paragraph_after("Middle ");
    REQUIRE(second.has_next());
    REQUIRE(second.add_run("Text").has_next());
    auto third = second.insert_paragraph_after("Gamma");
    REQUIRE(third.has_next());
    REQUIRE(third.add_run("Delta").has_next());
    REQUIRE_FALSE(source_document.save());

    CHECK_EQ(run_cli({"append-paragraph-text-comment",
                      source.string(),
                      "0",
                      "0",
                      "3",
                      "--comment-text",
                      "CLI paragraph range comment",
                      "--author",
                      "CLI Commenter",
                      "--initials",
                      "CC",
                      "--date",
                      "2026-05-02T10:00:00Z",
                      "--output",
                      paragraph_comment.string(),
                      "--json"},
                     output),
             0);
    auto output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("start_paragraph_index":0)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("text_length":3)"), std::string::npos);

    CHECK_EQ(run_cli({"append-text-range-comment",
                      paragraph_comment.string(),
                      "0",
                      "6",
                      "2",
                      "5",
                      "--comment-text",
                      "CLI cross paragraph comment",
                      "--author",
                      "CLI Cross Commenter",
                      "--initials",
                      "CX",
                      "--date",
                      "2026-05-02T10:10:00Z",
                      "--output",
                      cross_comment.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);
    CHECK_NE(output_json.find(R"("end_text_offset":5)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", cross_comment.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("anchor_text":"Alp")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(
                 R"("anchor_text":"Beta GammaMiddle TextGamma")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI paragraph range comment")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"CLI cross paragraph comment")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("date":"2026-05-02T10:00:00Z")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("date":"2026-05-02T10:10:00Z")"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-paragraph-text-comment-range",
                      cross_comment.string(),
                      "0",
                      "0",
                      "6",
                      "4",
                      "--output",
                      paragraph_moved.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("comment_index":0)"), std::string::npos);
    CHECK_NE(output_json.find(R"("text_length":4)"), std::string::npos);

    CHECK_EQ(run_cli({"set-text-range-comment-range",
                      paragraph_moved.string(),
                      "1",
                      "1",
                      "0",
                      "2",
                      "5",
                      "--output",
                      range_moved.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("end_paragraph_index":2)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", range_moved.string(), "--json"},
                     inspect_output),
             0);
    const auto moved_json = read_text_file(inspect_output);
    CHECK_NE(moved_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(moved_json.find(R"("anchor_text":"Beta")"),
             std::string::npos);
    CHECK_NE(moved_json.find(R"("anchor_text":"Middle TextGamma")"),
             std::string::npos);
    CHECK_NE(moved_json.find(R"("resolved":false)"), std::string::npos);

    CHECK_EQ(run_cli({"set-comment-resolved",
                      range_moved.string(),
                      "1",
                      "true",
                      "--output",
                      resolved.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("comment_index":1)"), std::string::npos);
    CHECK_NE(output_json.find(R"("resolved":true)"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", resolved.string(), "--json"},
                     inspect_output),
             0);
    const auto resolved_json = read_text_file(inspect_output);
    CHECK_NE(resolved_json.find(R"("comments_count":2)"), std::string::npos);
    CHECK_NE(resolved_json.find(R"("anchor_text":"Middle TextGamma")"),
             std::string::npos);
    CHECK_NE(resolved_json.find(R"("resolved":true)"), std::string::npos);

    CHECK_EQ(run_cli({"append-comment-reply",
                      resolved.string(),
                      "1",
                      "--comment-text",
                      "CLI threaded reply",
                      "--author",
                      "CLI Responder",
                      "--initials",
                      "RS",
                      "--date",
                      "2026-05-02T10:20:00Z",
                      "--output",
                      threaded.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("parent_comment_index":1)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-review", threaded.string(), "--json"},
                     inspect_output),
             0);
    const auto threaded_json = read_text_file(inspect_output);
    CHECK_NE(threaded_json.find(R"("comments_count":3)"), std::string::npos);
    CHECK_NE(threaded_json.find(R"("text":"CLI threaded reply")"),
             std::string::npos);
    CHECK_NE(threaded_json.find(R"("date":"2026-05-02T10:20:00Z")"),
             std::string::npos);
    CHECK_NE(threaded_json.find(R"("parent_index":1)"), std::string::npos);
    CHECK_NE(threaded_json.find(R"("parent_id":")"), std::string::npos);

    CHECK_EQ(run_cli({"set-comment-metadata",
                      threaded.string(),
                      "2",
                      "--author",
                      "CLI Updated Responder",
                      "--clear-initials",
                      "--date",
                      "2026-05-02T10:25:00Z",
                      "--output",
                      metadata_updated.string(),
                      "--json"},
                     output),
             0);
    output_json = read_text_file(output);
    CHECK_NE(output_json.find(R"("comment_index":2)"), std::string::npos);
    CHECK_EQ(run_cli({"inspect-review", metadata_updated.string(), "--json"},
                     inspect_output),
             0);
    const auto metadata_json = read_text_file(inspect_output);
    CHECK_NE(metadata_json.find(R"("author":"CLI Updated Responder")"),
             std::string::npos);
    CHECK_NE(metadata_json.find(R"("date":"2026-05-02T10:25:00Z")"),
             std::string::npos);
    CHECK_NE(metadata_json.find(R"("initials":null)"), std::string::npos);

    CHECK_EQ(run_cli({"remove-comment",
                      metadata_updated.string(),
                      "1",
                      "--output",
                      thread_removed.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(run_cli({"inspect-review", thread_removed.string(), "--json"},
                     inspect_output),
             0);
    const auto thread_removed_json = read_text_file(inspect_output);
    CHECK_NE(thread_removed_json.find(R"("comments_count":1)"),
             std::string::npos);
    CHECK_EQ(thread_removed_json.find("CLI threaded reply"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(paragraph_comment);
    remove_if_exists(cross_comment);
    remove_if_exists(paragraph_moved);
    remove_if_exists(range_moved);
    remove_if_exists(resolved);
    remove_if_exists(threaded);
    remove_if_exists(metadata_updated);
    remove_if_exists(thread_removed);
    remove_if_exists(output);
    remove_if_exists(inspect_output);
}

} // namespace
