#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("review notes comments can be appended replaced removed and saved") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_notes_comments_mutation.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    CHECK_EQ(doc.append_footnote("Footnote anchor ", "Original footnote"), 1U);
    CHECK(doc.replace_footnote(0U, "Replaced footnote"));
    CHECK_EQ(doc.append_footnote("Removed footnote anchor ", "Removed footnote"), 1U);
    CHECK(doc.remove_footnote(1U));

    CHECK_EQ(doc.append_endnote("Endnote anchor ", "Original endnote"), 1U);
    CHECK(doc.replace_endnote(0U, "Replaced endnote"));
    CHECK_EQ(doc.append_endnote("Removed endnote anchor ", "Removed endnote"), 1U);
    CHECK(doc.remove_endnote(1U));

    CHECK_EQ(doc.append_comment("Commented text", "Original comment", "Reviewer", "RV",
                                "2026-05-02T08:00:00Z"),
             1U);
    CHECK(doc.replace_comment(0U, "Replaced comment"));
    CHECK(doc.set_comment_resolved(0U, true));
    CHECK_EQ(doc.append_comment_reply(0U, "Reply comment", "Responder", "RS",
                                      "2026-05-02T08:05:00Z"),
             1U);
    CHECK_EQ(doc.append_comment("Removed comment text", "Removed comment"), 1U);
    CHECK(doc.set_comment_resolved(2U, true));
    CHECK(doc.remove_comment(2U));

    auto footnotes = doc.list_footnotes();
    REQUIRE_EQ(footnotes.size(), 1U);
    CHECK_EQ(footnotes.front().text, "Replaced footnote");
    auto endnotes = doc.list_endnotes();
    REQUIRE_EQ(endnotes.size(), 1U);
    CHECK_EQ(endnotes.front().text, "Replaced endnote");
    auto comments = doc.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments.front().anchor_text.has_value());
    CHECK_EQ(*comments.front().anchor_text, "Commented text");
    CHECK_EQ(comments.front().text, "Replaced comment");
    CHECK(comments.front().resolved);
    REQUIRE(comments.front().author.has_value());
    CHECK_EQ(*comments.front().author, "Reviewer");
    CHECK_EQ(comments.front().date,
             std::optional<std::string>{"2026-05-02T08:00:00Z"});
    CHECK_EQ(comments[1].text, "Reply comment");
    CHECK_EQ(comments[1].parent_index, std::optional<std::size_t>{0U});
    CHECK_EQ(comments[1].parent_id, std::optional<std::string>{comments[0].id});
    CHECK_EQ(comments[1].date,
             std::optional<std::string>{"2026-05-02T08:05:00Z"});
    CHECK_FALSE(comments[1].anchor_text.has_value());

    featherdoc::comment_metadata_update metadata_update;
    metadata_update.author = "Updated Reviewer";
    metadata_update.clear_initials = true;
    metadata_update.date = "2026-05-02T08:30:00Z";
    CHECK(doc.set_comment_metadata(0U, metadata_update));
    comments = doc.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    CHECK_EQ(comments.front().author,
             std::optional<std::string>{"Updated Reviewer"});
    CHECK_FALSE(comments.front().initials.has_value());
    CHECK_EQ(comments.front().date,
             std::optional<std::string>{"2026-05-02T08:30:00Z"});

    CHECK_FALSE(doc.replace_footnote(4U, "Missing"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "footnote index is out of range");

    CHECK_FALSE(doc.save());
    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:footnoteReference"), 1U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:endnoteReference"), 1U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:commentReference"), 1U);

    const auto footnotes_xml = read_test_docx_entry(target, "word/footnotes.xml");
    CHECK_NE(footnotes_xml.find("Replaced footnote"), std::string::npos);
    CHECK_EQ(footnotes_xml.find("Removed footnote"), std::string::npos);
    const auto endnotes_xml = read_test_docx_entry(target, "word/endnotes.xml");
    CHECK_NE(endnotes_xml.find("Replaced endnote"), std::string::npos);
    CHECK_EQ(endnotes_xml.find("Removed endnote"), std::string::npos);
    const auto comments_xml = read_test_docx_entry(target, "word/comments.xml");
    CHECK_NE(comments_xml.find("Replaced comment"), std::string::npos);
    CHECK_NE(comments_xml.find("Reply comment"), std::string::npos);
    CHECK_NE(comments_xml.find("w:author=\"Updated Reviewer\""),
             std::string::npos);
    CHECK_NE(comments_xml.find("w:date=\"2026-05-02T08:30:00Z\""),
             std::string::npos);
    CHECK_NE(comments_xml.find("w:date=\"2026-05-02T08:05:00Z\""),
             std::string::npos);
    CHECK_EQ(comments_xml.find("w:date=\"2026-05-02T08:00:00Z\""),
             std::string::npos);
    CHECK_EQ(comments_xml.find("Removed comment"), std::string::npos);
    CHECK_NE(comments_xml.find("w14:paraId"), std::string::npos);
    const auto comments_extended_xml =
        read_test_docx_entry(target, "word/commentsExtended.xml");
    CHECK_EQ(count_substring_occurrences(comments_extended_xml, "<w15:commentEx"), 2U);
    CHECK_NE(comments_extended_xml.find("w15:done=\"1\""), std::string::npos);
    CHECK_NE(comments_extended_xml.find("w15:paraIdParent=\""), std::string::npos);

    const auto relationships_xml =
        read_test_docx_entry(target, "word/_rels/document.xml.rels");
    CHECK_NE(relationships_xml.find("/relationships/footnotes"), std::string::npos);
    CHECK_NE(relationships_xml.find("/relationships/endnotes"), std::string::npos);
    CHECK_NE(relationships_xml.find("/relationships/comments"), std::string::npos);
    CHECK_NE(relationships_xml.find("/relationships/commentsExtended"), std::string::npos);
    const auto content_types_xml = read_test_docx_entry(target, test_content_types_xml_entry);
    CHECK_NE(content_types_xml.find("/word/footnotes.xml"), std::string::npos);
    CHECK_NE(content_types_xml.find("/word/endnotes.xml"), std::string::npos);
    CHECK_NE(content_types_xml.find("/word/comments.xml"), std::string::npos);
    CHECK_NE(content_types_xml.find("/word/commentsExtended.xml"), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    REQUIRE_EQ(reopened.list_footnotes().size(), 1U);
    REQUIRE_EQ(reopened.list_endnotes().size(), 1U);
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    CHECK(comments.front().resolved);
    CHECK_EQ(comments.front().author,
             std::optional<std::string>{"Updated Reviewer"});
    CHECK_FALSE(comments.front().initials.has_value());
    CHECK_EQ(comments.front().date,
             std::optional<std::string>{"2026-05-02T08:30:00Z"});
    CHECK_EQ(comments[1].parent_index, std::optional<std::size_t>{0U});
    CHECK_EQ(comments[1].parent_id, std::optional<std::string>{comments[0].id});
    CHECK_EQ(comments[1].date,
             std::optional<std::string>{"2026-05-02T08:05:00Z"});
    CHECK(reopened.set_comment_resolved(0U, false));
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    CHECK_FALSE(comments.front().resolved);
    CHECK_FALSE(reopened.save());
    const auto unresolved_comments_extended_xml =
        read_test_docx_entry(target, "word/commentsExtended.xml");
    CHECK_NE(unresolved_comments_extended_xml.find("w15:done=\"0\""),
             std::string::npos);
    CHECK_NE(unresolved_comments_extended_xml.find("w15:paraIdParent=\""),
             std::string::npos);

    CHECK(reopened.remove_comment(0U));
    comments = reopened.list_comments();
    CHECK(comments.empty());
    CHECK_FALSE(reopened.save());
    const auto removed_thread_comments_xml =
        read_test_docx_entry(target, "word/comments.xml");
    CHECK_EQ(removed_thread_comments_xml.find("Replaced comment"), std::string::npos);
    CHECK_EQ(removed_thread_comments_xml.find("Reply comment"), std::string::npos);
    const auto removed_thread_comments_extended_xml =
        read_test_docx_entry(target, "word/commentsExtended.xml");
    CHECK_EQ(count_substring_occurrences(removed_thread_comments_extended_xml,
                                         "<w15:commentEx"),
             0U);

    fs::remove(target);
}

TEST_CASE("review comments can target paragraph text ranges") {
    namespace fs = std::filesystem;

    const auto write_comment_range_source = [](const fs::path &path) {
        write_test_docx(path,
                        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t xml:space="preserve">Alpha </w:t></w:r>
      <w:r><w:rPr><w:b/></w:rPr><w:t>Beta</w:t></w:r>
      <w:r><w:t xml:space="preserve"> Gamma</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Middle </w:t></w:r>
      <w:r><w:rPr><w:i/></w:rPr><w:t>Text</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Gamma</w:t></w:r>
      <w:r><w:t>Delta</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");
    };

    const fs::path target =
        fs::current_path() / "review_comments_text_range.docx";
    const fs::path invalid_target =
        fs::current_path() / "review_comments_text_range_invalid.docx";
    fs::remove(target);
    fs::remove(invalid_target);

    write_comment_range_source(target);
    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.append_paragraph_text_comment(
                 0U, 0U, 3U, "Paragraph range comment", "Reviewer", "RV",
                 "2026-05-02T09:00:00Z"),
             1U);
    CHECK_EQ(doc.append_text_range_comment(
                 0U, 6U, 2U, 5U, "Cross paragraph comment", "Cross", "CP",
                 "2026-05-02T09:10:00Z"),
             1U);

    auto comments = doc.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Alp");
    CHECK_EQ(comments[0].text, "Paragraph range comment");
    REQUIRE(comments[0].author.has_value());
    CHECK_EQ(*comments[0].author, "Reviewer");
    CHECK_EQ(comments[0].date,
             std::optional<std::string>{"2026-05-02T09:00:00Z"});
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Beta GammaMiddle TextGamma");
    CHECK_EQ(comments[1].text, "Cross paragraph comment");
    CHECK_EQ(comments[1].date,
             std::optional<std::string>{"2026-05-02T09:10:00Z"});

    CHECK_FALSE(doc.save());
    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:commentRangeStart"), 2U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:commentRangeEnd"), 2U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:commentReference"), 2U);
    const auto comments_xml = read_test_docx_entry(target, "word/comments.xml");
    CHECK_NE(comments_xml.find("Paragraph range comment"), std::string::npos);
    CHECK_NE(comments_xml.find("Cross paragraph comment"), std::string::npos);
    CHECK_NE(comments_xml.find("w:date=\"2026-05-02T09:00:00Z\""),
             std::string::npos);
    CHECK_NE(comments_xml.find("w:date=\"2026-05-02T09:10:00Z\""),
             std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Beta GammaMiddle TextGamma");
    CHECK_EQ(comments[0].date,
             std::optional<std::string>{"2026-05-02T09:00:00Z"});
    CHECK_EQ(comments[1].date,
             std::optional<std::string>{"2026-05-02T09:10:00Z"});
    CHECK(reopened.set_paragraph_text_comment_range(0U, 0U, 6U, 4U));
    CHECK(reopened.set_text_range_comment_range(1U, 1U, 0U, 2U, 5U));
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Beta");
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Middle TextGamma");
    CHECK_EQ(comments[0].text, "Paragraph range comment");
    CHECK_EQ(comments[1].text, "Cross paragraph comment");
    CHECK(reopened.remove_comment(1U));
    CHECK_FALSE(reopened.save());
    const auto removed_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(removed_xml, "w:commentReference"), 1U);

    write_comment_range_source(invalid_target);
    featherdoc::Document invalid(invalid_target);
    CHECK_FALSE(invalid.open());
    CHECK_EQ(invalid.append_paragraph_text_comment(0U, 6U, 0U, "bad"), 0U);
    CHECK_EQ(invalid.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(invalid.append_text_range_comment(2U, 0U, 1U, 1U, "bad"), 0U);
    CHECK_EQ(invalid.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(invalid.append_text_range_comment(0U, 6U, 2U, 5U, ""), 0U);
    CHECK_EQ(invalid.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid.set_paragraph_text_comment_range(0U, 0U, 0U, 1U));
    CHECK_EQ(invalid.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
    fs::remove(invalid_target);
}

TEST_CASE("review comment inspection preserves nested anchor text") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "review_comments_nested_anchor_text.docx";
    fs::remove(target);

    write_test_docx(
        target,
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t xml:space="preserve">Alpha </w:t></w:r>
      <w:r><w:t>Beta</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.append_paragraph_text_comment(
                 0U, 0U, 10U, "Outer comment", "Outer", "OC",
                 "2026-05-03T14:00:00Z"),
             1U);
    CHECK_EQ(doc.append_paragraph_text_comment(
                 0U, 6U, 4U, "Inner comment", "Inner", "IC",
                 "2026-05-03T14:01:00Z"),
             1U);

    auto comments = doc.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Alpha Beta");
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Beta");
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    comments = reopened.list_comments();
    REQUIRE_EQ(comments.size(), 2U);
    REQUIRE(comments[0].anchor_text.has_value());
    CHECK_EQ(*comments[0].anchor_text, "Alpha Beta");
    REQUIRE(comments[1].anchor_text.has_value());
    CHECK_EQ(*comments[1].anchor_text, "Beta");

    fs::remove(target);
}

TEST_CASE("revisions can be accepted and rejected") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_revisions_accept_reject.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:ins w:id="1" w:author="Ada"><w:r><w:t>Accepted insertion</w:t></w:r></w:ins>
      <w:del w:id="2" w:author="Grace"><w:r><w:delText>Rejected deletion</w:delText></w:r></w:del>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    REQUIRE_EQ(doc.list_revisions().size(), 2U);
    CHECK(doc.accept_revision(0U));
    REQUIRE_EQ(doc.list_revisions().size(), 1U);
    CHECK(doc.reject_revision(0U));
    CHECK(doc.list_revisions().empty());

    CHECK_FALSE(doc.save());
    auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_xml.find("<w:ins"), std::string::npos);
    CHECK_EQ(saved_xml.find("<w:del"), std::string::npos);
    CHECK_EQ(saved_xml.find("w:delText"), std::string::npos);
    CHECK_NE(saved_xml.find("Accepted insertion"), std::string::npos);
    CHECK_NE(saved_xml.find("Rejected deletion"), std::string::npos);

    const fs::path accept_all_target = fs::current_path() / "review_revisions_accept_all.docx";
    fs::remove(accept_all_target);
    write_test_docx(accept_all_target, document_xml);
    featherdoc::Document accept_all_doc(accept_all_target);
    CHECK_FALSE(accept_all_doc.open());
    CHECK_EQ(accept_all_doc.accept_all_revisions(), 2U);
    CHECK(accept_all_doc.list_revisions().empty());
    CHECK_FALSE(accept_all_doc.save());
    saved_xml = read_test_docx_entry(accept_all_target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("Accepted insertion"), std::string::npos);
    CHECK_EQ(saved_xml.find("Rejected deletion"), std::string::npos);

    const fs::path reject_all_target = fs::current_path() / "review_revisions_reject_all.docx";
    fs::remove(reject_all_target);
    write_test_docx(reject_all_target, document_xml);
    featherdoc::Document reject_all_doc(reject_all_target);
    CHECK_FALSE(reject_all_doc.open());
    CHECK_EQ(reject_all_doc.reject_all_revisions(), 2U);
    CHECK(reject_all_doc.list_revisions().empty());
    CHECK_FALSE(reject_all_doc.save());
    saved_xml = read_test_docx_entry(reject_all_target, test_document_xml_entry);
    CHECK_EQ(saved_xml.find("Accepted insertion"), std::string::npos);
    CHECK_NE(saved_xml.find("Rejected deletion"), std::string::npos);

    fs::remove(target);
    fs::remove(accept_all_target);
    fs::remove(reject_all_target);
}

TEST_CASE("revision authoring APIs append insertion and deletion markup") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_revisions_authoring.docx";
    fs::remove(target);
    write_test_docx(target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>Revision authoring fixture</w:t></w:r></w:p></w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.append_insertion_revision("Inserted authored revision", "Ada",
                                           "2026-05-02T10:00:00Z"),
             1U);
    CHECK_EQ(doc.append_deletion_revision("Deleted authored revision", "Grace",
                                          "2026-05-02T11:00:00Z"),
             1U);

    auto revisions = doc.list_revisions();
    REQUIRE_EQ(revisions.size(), 2U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].id, "1");
    REQUIRE(revisions[0].author.has_value());
    CHECK_EQ(*revisions[0].author, "Ada");
    REQUIRE(revisions[0].date.has_value());
    CHECK_EQ(*revisions[0].date, "2026-05-02T10:00:00Z");
    CHECK_EQ(revisions[0].text, "Inserted authored revision");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[1].id, "2");
    REQUIRE(revisions[1].author.has_value());
    CHECK_EQ(*revisions[1].author, "Grace");
    CHECK_EQ(revisions[1].text, "Deleted authored revision");

    featherdoc::revision_metadata_update insertion_metadata;
    insertion_metadata.author = "Ada Updated";
    insertion_metadata.date = "2026-05-02T10:30:00Z";
    CHECK(doc.set_revision_metadata(0U, insertion_metadata));
    featherdoc::revision_metadata_update deletion_metadata;
    deletion_metadata.clear_author = true;
    deletion_metadata.date = "2026-05-02T11:30:00Z";
    CHECK(doc.set_revision_metadata(1U, deletion_metadata));
    revisions = doc.list_revisions();
    REQUIRE_EQ(revisions.size(), 2U);
    CHECK_EQ(revisions[0].author,
             std::optional<std::string>{"Ada Updated"});
    CHECK_EQ(revisions[0].date,
             std::optional<std::string>{"2026-05-02T10:30:00Z"});
    CHECK_FALSE(revisions[1].author.has_value());
    CHECK_EQ(revisions[1].date,
             std::optional<std::string>{"2026-05-02T11:30:00Z"});

    CHECK_FALSE(doc.save());
    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(R"(<w:ins w:id="1" w:author="Ada Updated" w:date="2026-05-02T10:30:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="2" w:date="2026-05-02T11:30:00Z">)"),
             std::string::npos);
    CHECK_EQ(saved_xml.find("Grace"), std::string::npos);
    CHECK_NE(saved_xml.find("<w:t>Inserted authored revision</w:t>"),
             std::string::npos);
    CHECK_NE(saved_xml.find("<w:delText>Deleted authored revision</w:delText>"),
             std::string::npos);

    featherdoc::Document invalid_doc(target);
    CHECK_FALSE(invalid_doc.open());
    CHECK_EQ(invalid_doc.append_insertion_revision(""), 0U);
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    featherdoc::revision_metadata_update empty_metadata;
    CHECK_FALSE(invalid_doc.set_revision_metadata(0U, empty_metadata));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("revision authoring APIs create in-place run revisions") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "review_revisions_run_authoring.docx";
    fs::remove(target);
    write_test_docx(target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Alpha </w:t></w:r>
      <w:r><w:rPr><w:b/></w:rPr><w:t>Beta</w:t></w:r>
      <w:r><w:t> Gamma</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.insert_run_revision_after(0U, 0U, "Inserted run", "Ada",
                                        "2026-05-02T14:00:00Z"));
    CHECK(doc.delete_run_revision(0U, 1U, "Grace",
                                  "2026-05-02T15:00:00Z"));
    CHECK(doc.replace_run_revision(0U, 1U, "Replacement run", "Linus",
                                   "2026-05-02T16:00:00Z"));

    auto revisions = doc.list_revisions();
    REQUIRE_EQ(revisions.size(), 4U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].text, "Inserted run");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[1].text, "Beta");
    CHECK_EQ(revisions[2].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[2].text, " Gamma");
    CHECK_EQ(revisions[3].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[3].text, "Replacement run");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_EQ(revisions[3].id, "4");

    CHECK_FALSE(doc.save());
    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(R"(<w:ins w:id="1" w:author="Ada" w:date="2026-05-02T14:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="2" w:author="Grace" w:date="2026-05-02T15:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="3" w:author="Linus" w:date="2026-05-02T16:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:ins w:id="4" w:author="Linus" w:date="2026-05-02T16:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find("<w:delText>Beta</w:delText>"), std::string::npos);
    CHECK_NE(saved_xml.find("<w:t>Replacement run</w:t>"), std::string::npos);

    featherdoc::Document accepted(target);
    CHECK_FALSE(accepted.open());
    CHECK_EQ(accepted.accept_all_revisions(), 4U);
    CHECK_FALSE(accepted.save());
    featherdoc::Document accepted_reopened(target);
    CHECK_FALSE(accepted_reopened.open());
    CHECK_EQ(collect_document_text(accepted_reopened),
             "Alpha Inserted runReplacement run\n");

    const fs::path reject_target = fs::current_path() / "review_revisions_run_authoring_reject.docx";
    fs::remove(reject_target);
    write_test_docx(reject_target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>One</w:t></w:r><w:r><w:t>Two</w:t></w:r></w:p></w:body>
</w:document>
)");
    featherdoc::Document rejected(reject_target);
    CHECK_FALSE(rejected.open());
    CHECK(rejected.insert_run_revision_after(0U, 0U, "Add"));
    CHECK(rejected.delete_run_revision(0U, 1U));
    CHECK_EQ(rejected.reject_all_revisions(), 2U);
    CHECK_FALSE(rejected.save());
    featherdoc::Document rejected_reopened(reject_target);
    CHECK_FALSE(rejected_reopened.open());
    CHECK_EQ(collect_document_text(rejected_reopened), "OneTwo\n");

    featherdoc::Document invalid_doc(target);
    CHECK_FALSE(invalid_doc.open());
    CHECK_FALSE(invalid_doc.insert_run_revision_after(9U, 0U, "bad"));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.delete_run_revision(0U, 9U));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.replace_run_revision(0U, 0U, ""));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
    fs::remove(reject_target);
}

TEST_CASE("revision authoring APIs create paragraph text range revisions") {
    namespace fs = std::filesystem;

    const auto write_range_source = [](const fs::path &path) {
        write_test_docx(path,
                        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Alpha </w:t></w:r>
      <w:r><w:rPr><w:b/></w:rPr><w:t>Beta</w:t></w:r>
      <w:r><w:t> Gamma</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");
    };

    const fs::path insert_target =
        fs::current_path() / "review_revisions_paragraph_text_insert.docx";
    const fs::path delete_target =
        fs::current_path() / "review_revisions_paragraph_text_delete.docx";
    const fs::path replace_target =
        fs::current_path() / "review_revisions_paragraph_text_replace.docx";
    const fs::path reject_target =
        fs::current_path() / "review_revisions_paragraph_text_reject.docx";
    const fs::path guard_target =
        fs::current_path() / "review_revisions_paragraph_text_guard.docx";
    const fs::path invalid_target =
        fs::current_path() / "review_revisions_paragraph_text_invalid.docx";

    fs::remove(insert_target);
    fs::remove(delete_target);
    fs::remove(replace_target);
    fs::remove(reject_target);
    fs::remove(guard_target);
    fs::remove(invalid_target);

    write_range_source(insert_target);
    featherdoc::Document inserted(insert_target);
    CHECK_FALSE(inserted.open());
    CHECK(inserted.insert_paragraph_text_revision(
        0U, 6U, "Inserted ", "Ada", "2026-05-02T20:00:00Z"));
    auto revisions = inserted.list_revisions();
    REQUIRE_EQ(revisions.size(), 1U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].text, "Inserted ");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_FALSE(inserted.save());
    featherdoc::Document accepted_insert(insert_target);
    CHECK_FALSE(accepted_insert.open());
    CHECK_EQ(accepted_insert.accept_all_revisions(), 1U);
    CHECK_FALSE(accepted_insert.save());
    featherdoc::Document accepted_insert_reopened(insert_target);
    CHECK_FALSE(accepted_insert_reopened.open());
    CHECK_EQ(collect_document_text(accepted_insert_reopened),
             "Alpha Inserted Beta Gamma\n");

    write_range_source(delete_target);
    featherdoc::Document deleted(delete_target);
    CHECK_FALSE(deleted.open());
    featherdoc::revision_text_range_options delete_options;
    delete_options.author = "Grace";
    delete_options.date = "2026-05-02T21:00:00Z";
    delete_options.expected_text = "ha Beta";
    CHECK(deleted.delete_paragraph_text_revision(0U, 3U, 7U,
                                                 delete_options));
    revisions = deleted.list_revisions();
    REQUIRE_EQ(revisions.size(), 1U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[0].text, "ha Beta");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_FALSE(deleted.save());
    auto saved_xml = read_test_docx_entry(delete_target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="1" w:author="Grace" w:date="2026-05-02T21:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:delText xml:space="preserve">ha </w:delText>)"),
             std::string::npos);
    CHECK_NE(saved_xml.find("<w:delText>Beta</w:delText>"), std::string::npos);
    featherdoc::Document accepted_delete(delete_target);
    CHECK_FALSE(accepted_delete.open());
    CHECK_EQ(accepted_delete.accept_all_revisions(), 1U);
    CHECK_FALSE(accepted_delete.save());
    featherdoc::Document accepted_delete_reopened(delete_target);
    CHECK_FALSE(accepted_delete_reopened.open());
    CHECK_EQ(collect_document_text(accepted_delete_reopened), "Alp Gamma\n");

    write_range_source(replace_target);
    featherdoc::Document replaced(replace_target);
    CHECK_FALSE(replaced.open());
    featherdoc::revision_text_range_options replace_options;
    replace_options.author = "Linus";
    replace_options.date = "2026-05-02T22:00:00Z";
    replace_options.expected_text = "ha Beta";
    CHECK(replaced.replace_paragraph_text_revision(0U, 3U, 7U, "Range",
                                                   replace_options));
    revisions = replaced.list_revisions();
    REQUIRE_EQ(revisions.size(), 2U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[0].text, "ha Beta");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[1].text, "Range");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_EQ(revisions[1].id, "2");
    CHECK_FALSE(replaced.save());
    featherdoc::Document accepted_replace(replace_target);
    CHECK_FALSE(accepted_replace.open());
    CHECK_EQ(accepted_replace.accept_all_revisions(), 2U);
    CHECK_FALSE(accepted_replace.save());
    featherdoc::Document accepted_replace_reopened(replace_target);
    CHECK_FALSE(accepted_replace_reopened.open());
    CHECK_EQ(collect_document_text(accepted_replace_reopened),
             "AlpRange Gamma\n");

    write_range_source(reject_target);
    featherdoc::Document rejected(reject_target);
    CHECK_FALSE(rejected.open());
    CHECK(rejected.replace_paragraph_text_revision(0U, 3U, 7U, "Range"));
    CHECK_EQ(rejected.reject_all_revisions(), 2U);
    CHECK_FALSE(rejected.save());
    featherdoc::Document rejected_reopened(reject_target);
    CHECK_FALSE(rejected_reopened.open());
    CHECK_EQ(collect_document_text(rejected_reopened), "Alpha Beta Gamma\n");

    write_range_source(guard_target);
    featherdoc::Document guarded(guard_target);
    CHECK_FALSE(guarded.open());
    featherdoc::revision_text_range_options guard_options;
    guard_options.author = "Noop";
    guard_options.expected_text = "Wrong";
    CHECK_FALSE(guarded.delete_paragraph_text_revision(0U, 3U, 7U,
                                                       guard_options));
    CHECK_EQ(guarded.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(guarded.last_error().detail.find(
                 "expected text did not match selected text"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("expected: Wrong"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("actual: ha Beta"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("start_paragraph_index: 0"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("text_offset=3"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("text_length=7"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("text=ha Beta"),
             std::string::npos);
    CHECK_EQ(guarded.list_revisions().size(), 0U);
    CHECK_EQ(collect_document_text(guarded), "Alpha Beta Gamma\n");

    write_range_source(invalid_target);
    featherdoc::Document invalid_doc(invalid_target);
    CHECK_FALSE(invalid_doc.open());
    CHECK_FALSE(invalid_doc.insert_paragraph_text_revision(0U, 99U, "bad"));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.delete_paragraph_text_revision(0U, 0U, 0U));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.replace_paragraph_text_revision(0U, 0U, 99U, "bad"));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(insert_target);
    fs::remove(delete_target);
    fs::remove(replace_target);
    fs::remove(reject_target);
    fs::remove(guard_target);
    fs::remove(invalid_target);
}

TEST_CASE("revision authoring APIs create cross-paragraph text range revisions") {
    namespace fs = std::filesystem;

    const auto write_cross_paragraph_source = [](const fs::path &path) {
        write_test_docx(path,
                        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t xml:space="preserve">Alpha </w:t></w:r>
      <w:r><w:rPr><w:b/></w:rPr><w:t>Beta</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Middle </w:t></w:r>
      <w:r><w:rPr><w:i/></w:rPr><w:t>Text</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Gamma</w:t></w:r>
      <w:r><w:t>Delta</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");
    };

    const fs::path insert_target =
        fs::current_path() / "review_revisions_text_range_insert.docx";
    const fs::path delete_target =
        fs::current_path() / "review_revisions_text_range_delete.docx";
    const fs::path replace_target =
        fs::current_path() / "review_revisions_text_range_replace.docx";
    const fs::path reject_target =
        fs::current_path() / "review_revisions_text_range_reject.docx";
    const fs::path guard_target =
        fs::current_path() / "review_revisions_text_range_guard.docx";
    const fs::path invalid_target =
        fs::current_path() / "review_revisions_text_range_invalid.docx";

    fs::remove(insert_target);
    fs::remove(delete_target);
    fs::remove(replace_target);
    fs::remove(reject_target);
    fs::remove(guard_target);
    fs::remove(invalid_target);

    write_cross_paragraph_source(insert_target);
    featherdoc::Document inserted(insert_target);
    CHECK_FALSE(inserted.open());
    CHECK(inserted.insert_text_range_revision(1U, 7U, "Inserted ", "Ada",
                                              "2026-05-02T23:00:00Z"));
    auto revisions = inserted.list_revisions();
    REQUIRE_EQ(revisions.size(), 1U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[0].text, "Inserted ");
    CHECK_FALSE(inserted.save());
    featherdoc::Document accepted_insert(insert_target);
    CHECK_FALSE(accepted_insert.open());
    CHECK_EQ(accepted_insert.accept_all_revisions(), 1U);
    CHECK_FALSE(accepted_insert.save());
    featherdoc::Document accepted_insert_reopened(insert_target);
    CHECK_FALSE(accepted_insert_reopened.open());
    CHECK_EQ(collect_document_text(accepted_insert_reopened),
             "Alpha Beta\nMiddle Inserted Text\nGammaDelta\n");

    write_cross_paragraph_source(delete_target);
    featherdoc::Document deleted(delete_target);
    CHECK_FALSE(deleted.open());
    const auto preview =
        deleted.preview_text_range(0U, 6U, 2U, 5U);
    REQUIRE(preview.has_value());
    CHECK_EQ(preview->start_paragraph_index, 0U);
    CHECK_EQ(preview->start_text_offset, 6U);
    CHECK_EQ(preview->end_paragraph_index, 2U);
    CHECK_EQ(preview->end_text_offset, 5U);
    CHECK_EQ(preview->text_length, 20U);
    CHECK(preview->plain_text_runs_supported);
    CHECK_EQ(preview->text, "BetaMiddle TextGamma");
    REQUIRE_EQ(preview->segments.size(), 3U);
    CHECK_EQ(preview->segments[0].paragraph_index, 0U);
    CHECK_EQ(preview->segments[0].text, "Beta");
    CHECK_EQ(preview->segments[1].paragraph_index, 1U);
    CHECK_EQ(preview->segments[1].text, "Middle Text");
    CHECK_EQ(preview->segments[2].paragraph_index, 2U);
    CHECK_EQ(preview->segments[2].text, "Gamma");
    featherdoc::revision_text_range_options delete_options;
    delete_options.author = "Grace";
    delete_options.date = "2026-05-03T00:00:00Z";
    delete_options.expected_text = "BetaMiddle TextGamma";
    CHECK(deleted.delete_text_range_revision(0U, 6U, 2U, 5U,
                                             delete_options));
    revisions = deleted.list_revisions();
    REQUIRE_EQ(revisions.size(), 3U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[0].text, "Beta");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[1].text, "Middle Text");
    CHECK_EQ(revisions[2].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[2].text, "Gamma");
    CHECK_EQ(revisions[0].id, "1");
    CHECK_EQ(revisions[2].id, "3");
    CHECK_FALSE(deleted.save());
    auto saved_xml = read_test_docx_entry(delete_target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(R"(<w:del w:id="1" w:author="Grace" w:date="2026-05-03T00:00:00Z">)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:delText>Beta</w:delText>)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:delText xml:space="preserve">Middle </w:delText>)"),
             std::string::npos);
    CHECK_NE(saved_xml.find(R"(<w:delText>Gamma</w:delText>)"),
             std::string::npos);
    featherdoc::Document accepted_delete(delete_target);
    CHECK_FALSE(accepted_delete.open());
    CHECK_EQ(accepted_delete.accept_all_revisions(), 3U);
    CHECK_FALSE(accepted_delete.save());
    featherdoc::Document accepted_delete_reopened(delete_target);
    CHECK_FALSE(accepted_delete_reopened.open());
    CHECK_EQ(collect_document_text(accepted_delete_reopened),
             "Alpha \n\nDelta\n");

    write_cross_paragraph_source(replace_target);
    featherdoc::Document replaced(replace_target);
    CHECK_FALSE(replaced.open());
    featherdoc::revision_text_range_options replace_options;
    replace_options.author = "Linus";
    replace_options.date = "2026-05-03T01:00:00Z";
    replace_options.expected_text = "BetaMiddle TextGamma";
    CHECK(replaced.replace_text_range_revision(0U, 6U, 2U, 5U, "Range ",
                                               replace_options));
    revisions = replaced.list_revisions();
    REQUIRE_EQ(revisions.size(), 4U);
    CHECK_EQ(revisions[0].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[0].text, "Beta");
    CHECK_EQ(revisions[1].kind, featherdoc::revision_kind::insertion);
    CHECK_EQ(revisions[1].text, "Range ");
    CHECK_EQ(revisions[2].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[2].text, "Middle Text");
    CHECK_EQ(revisions[3].kind, featherdoc::revision_kind::deletion);
    CHECK_EQ(revisions[3].text, "Gamma");
    CHECK_FALSE(replaced.save());
    featherdoc::Document accepted_replace(replace_target);
    CHECK_FALSE(accepted_replace.open());
    CHECK_EQ(accepted_replace.accept_all_revisions(), 4U);
    CHECK_FALSE(accepted_replace.save());
    featherdoc::Document accepted_replace_reopened(replace_target);
    CHECK_FALSE(accepted_replace_reopened.open());
    CHECK_EQ(collect_document_text(accepted_replace_reopened),
             "Alpha Range \n\nDelta\n");

    write_cross_paragraph_source(reject_target);
    featherdoc::Document rejected(reject_target);
    CHECK_FALSE(rejected.open());
    CHECK(rejected.replace_text_range_revision(0U, 6U, 2U, 5U, "Range "));
    CHECK_EQ(rejected.reject_all_revisions(), 4U);
    CHECK_FALSE(rejected.save());
    featherdoc::Document rejected_reopened(reject_target);
    CHECK_FALSE(rejected_reopened.open());
    CHECK_EQ(collect_document_text(rejected_reopened),
             "Alpha Beta\nMiddle Text\nGammaDelta\n");

    write_cross_paragraph_source(guard_target);
    featherdoc::Document guarded(guard_target);
    CHECK_FALSE(guarded.open());
    featherdoc::revision_text_range_options guard_options;
    guard_options.author = "Noop";
    guard_options.expected_text = "Wrong";
    CHECK_FALSE(guarded.replace_text_range_revision(0U, 6U, 2U, 5U,
                                                    "Range ", guard_options));
    CHECK_EQ(guarded.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(guarded.last_error().detail.find(
                 "expected text did not match selected text"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("expected: Wrong"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find(
                 "actual: BetaMiddle TextGamma"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("start_paragraph_index: 0"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("end_paragraph_index: 2"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("paragraph_index=1"),
             std::string::npos);
    CHECK_NE(guarded.last_error().detail.find("text=Middle Text"),
             std::string::npos);
    CHECK_EQ(guarded.list_revisions().size(), 0U);
    CHECK_EQ(collect_document_text(guarded),
             "Alpha Beta\nMiddle Text\nGammaDelta\n");

    write_cross_paragraph_source(invalid_target);
    featherdoc::Document invalid_doc(invalid_target);
    CHECK_FALSE(invalid_doc.open());
    CHECK_FALSE(invalid_doc.preview_text_range(2U, 0U, 1U, 1U).has_value());
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.delete_text_range_revision(2U, 0U, 1U, 1U));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.delete_text_range_revision(0U, 11U, 1U, 1U));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.replace_text_range_revision(0U, 10U, 1U, 0U,
                                                        "bad"));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_FALSE(invalid_doc.replace_text_range_revision(0U, 6U, 2U, 5U, ""));
    CHECK_EQ(invalid_doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(insert_target);
    fs::remove(delete_target);
    fs::remove(replace_target);
    fs::remove(reject_target);
    fs::remove(guard_target);
    fs::remove(invalid_target);
}

TEST_CASE("find text ranges locates body text across paragraphs") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "find_text_ranges_body.docx";
    fs::remove(target);
    write_test_docx(target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t xml:space="preserve">Alpha </w:t></w:r>
      <w:r><w:t>Beta</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Middle </w:t></w:r>
      <w:r><w:t>Text</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Beta</w:t></w:r>
      <w:r><w:t xml:space="preserve"> Tail</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");

    featherdoc::Document document(target);
    CHECK_FALSE(document.open());

    const auto beta_matches = document.find_text_ranges("Beta");
    REQUIRE_EQ(beta_matches.size(), 2U);
    CHECK_EQ(beta_matches[0].start_paragraph_index, 0U);
    CHECK_EQ(beta_matches[0].start_text_offset, 6U);
    CHECK_EQ(beta_matches[0].end_paragraph_index, 0U);
    CHECK_EQ(beta_matches[0].end_text_offset, 10U);
    CHECK_EQ(beta_matches[0].text, "Beta");
    CHECK_EQ(beta_matches[1].start_paragraph_index, 2U);
    CHECK_EQ(beta_matches[1].start_text_offset, 0U);
    CHECK_EQ(beta_matches[1].end_paragraph_index, 2U);
    CHECK_EQ(beta_matches[1].end_text_offset, 4U);
    CHECK_EQ(beta_matches[1].text, "Beta");

    const auto cross_match = document.find_text_ranges("BetaMiddle TextBeta");
    REQUIRE_EQ(cross_match.size(), 1U);
    CHECK_EQ(cross_match[0].start_paragraph_index, 0U);
    CHECK_EQ(cross_match[0].start_text_offset, 6U);
    CHECK_EQ(cross_match[0].end_paragraph_index, 2U);
    CHECK_EQ(cross_match[0].end_text_offset, 4U);
    CHECK_EQ(cross_match[0].text, "BetaMiddle TextBeta");
    REQUIRE_EQ(cross_match[0].segments.size(), 3U);
    CHECK_EQ(cross_match[0].segments[0].text, "Beta");
    CHECK_EQ(cross_match[0].segments[1].text, "Middle Text");
    CHECK_EQ(cross_match[0].segments[2].text, "Beta");

    CHECK(document.find_text_ranges("Missing").empty());
    CHECK_FALSE(document.last_error().code);
    CHECK(document.find_text_ranges("").empty());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}
