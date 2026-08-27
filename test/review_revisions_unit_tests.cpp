#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

namespace {

auto wrap_review_xml_deeply(std::string leaf, std::size_t depth)
    -> std::string {
    std::string xml;
    xml.reserve(leaf.size() + depth * 7U);
    for (std::size_t index = 0U; index < depth; ++index) {
        xml += "<n>";
    }
    xml += leaf;
    for (std::size_t index = 0U; index < depth; ++index) {
        xml += "</n>";
    }
    return xml;
}

} // namespace

TEST_CASE("revision traversal handles deeply nested XML iteratively") {
    namespace fs = std::filesystem;

    constexpr std::size_t nesting_depth = 25'000U;
    const auto target = fs::current_path() / "review_revisions_deep_xml.docx";
    fs::remove(target);

    const auto deletion =
        std::string{"<w:del w:id=\"41\" w:author=\"Reviewer\">"} +
        wrap_review_xml_deeply(
            "<w:r><w:delText>deep deletion</w:delText></w:r>",
            nesting_depth) +
        "</w:del>";
    const auto document_xml =
        std::string{"<w:document xmlns:w=\"http://schemas.openxmlformats.org/"
                    "wordprocessingml/2006/main\"><w:body>"
                    "<w:p><w:r><w:t>base</w:t></w:r>"} +
        deletion + "</w:p></w:body></w:document>";
    write_test_docx(target, document_xml);

    featherdoc::document_open_options options;
    options.limits.max_xml_part_bytes = 1024ULL * 1024ULL * 1024ULL;
    options.limits.max_total_uncompressed_bytes =
        2ULL * 1024ULL * 1024ULL * 1024ULL;
    options.limits.max_compression_ratio = 100'000U;
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open(options));

    auto revisions = document.list_revisions();
    REQUIRE_EQ(revisions.size(), 1U);
    CHECK_EQ(revisions.front().text, "deep deletion");
    CHECK_EQ(document.append_insertion_revision("new insertion"), 1U);
    CHECK(document.reject_revision(0U));
    revisions = document.list_revisions();
    REQUIRE_EQ(revisions.size(), 1U);
    CHECK_EQ(revisions.front().text, "new insertion");
    const auto save_error = document.save();
    CHECK_FALSE(save_error);

    const auto saved_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("deep deletion"), std::string::npos);
    CHECK_EQ(saved_xml.find("w:delText"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("review comment traversal handles deeply nested XML iteratively") {
    namespace fs = std::filesystem;

    constexpr std::size_t nesting_depth = 25'000U;
    const auto target = fs::current_path() / "review_comments_deep_xml.docx";
    fs::remove(target);

    featherdoc::Document seed(target);
    REQUIRE_FALSE(seed.create_empty());
    REQUIRE_EQ(seed.append_comment("deep anchor", "deep comment"), 1U);
    REQUIRE_FALSE(seed.save());

    auto document_xml =
        read_test_docx_entry(target, test_document_xml_entry);
    const auto range_start = document_xml.find("<w:commentRangeStart");
    REQUIRE_NE(range_start, std::string::npos);
    const auto range_content_start = document_xml.find("/>", range_start);
    REQUIRE_NE(range_content_start, std::string::npos);
    const auto range_end = document_xml.find("<w:commentRangeEnd",
                                             range_content_start + 2U);
    REQUIRE_NE(range_end, std::string::npos);
    const auto range_content = document_xml.substr(
        range_content_start + 2U, range_end - (range_content_start + 2U));
    document_xml.replace(
        range_content_start + 2U, range_end - (range_content_start + 2U),
        wrap_review_xml_deeply(range_content, nesting_depth));
    rewrite_test_docx_entry(target, test_document_xml_entry,
                            std::move(document_xml));

    featherdoc::document_open_options options;
    options.limits.max_compression_ratio = 100'000U;
    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open(options));

    const auto comments = document.list_comments();
    REQUIRE_EQ(comments.size(), 1U);
    REQUIRE(comments.front().anchor_text.has_value());
    CHECK_EQ(*comments.front().anchor_text, "deep anchor");
    CHECK(document.remove_comment(0U));
    CHECK(document.list_comments().empty());
    CHECK_FALSE(document.last_error());

    fs::remove(target);
}

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

TEST_CASE("removing review references retires only their owning run handles") {
    namespace fs = std::filesystem;

    const auto footnote_target =
        fs::current_path() / "review_footnote_reference_handle.docx";
    fs::remove(footnote_target);
    featherdoc::Document footnote_document(footnote_target);
    REQUIRE_FALSE(footnote_document.create_empty());
    REQUIRE(footnote_document.paragraphs().set_text("Unrelated"));
    auto unrelated_footnote_paragraph = footnote_document.paragraphs();
    auto unrelated_footnote_run = unrelated_footnote_paragraph.runs();
    REQUIRE_EQ(footnote_document.append_footnote("Anchor", "Footnote"), 1U);
    auto footnote_paragraph = footnote_document.paragraphs();
    footnote_paragraph.next();
    auto footnote_anchor_run = footnote_paragraph.runs();
    auto footnote_reference_run = footnote_anchor_run;
    footnote_reference_run.next();
    REQUIRE(footnote_paragraph.valid());
    REQUIRE(footnote_anchor_run.valid());
    REQUIRE(footnote_reference_run.valid());
    REQUIRE(footnote_document.remove_footnote(0U));
    CHECK(unrelated_footnote_paragraph.valid());
    CHECK(unrelated_footnote_run.valid());
    CHECK(footnote_paragraph.valid());
    CHECK(footnote_anchor_run.valid());
    CHECK_FALSE(footnote_reference_run.valid());

    const auto endnote_target =
        fs::current_path() / "review_endnote_reference_handle.docx";
    fs::remove(endnote_target);
    featherdoc::Document endnote_document(endnote_target);
    REQUIRE_FALSE(endnote_document.create_empty());
    REQUIRE(endnote_document.paragraphs().set_text("Unrelated"));
    auto unrelated_endnote_paragraph = endnote_document.paragraphs();
    auto unrelated_endnote_run = unrelated_endnote_paragraph.runs();
    REQUIRE_EQ(endnote_document.append_endnote("Anchor", "Endnote"), 1U);
    auto endnote_paragraph = endnote_document.paragraphs();
    endnote_paragraph.next();
    auto endnote_anchor_run = endnote_paragraph.runs();
    auto endnote_reference_run = endnote_anchor_run;
    endnote_reference_run.next();
    REQUIRE(endnote_paragraph.valid());
    REQUIRE(endnote_anchor_run.valid());
    REQUIRE(endnote_reference_run.valid());
    REQUIRE(endnote_document.remove_endnote(0U));
    CHECK(unrelated_endnote_paragraph.valid());
    CHECK(unrelated_endnote_run.valid());
    CHECK(endnote_paragraph.valid());
    CHECK(endnote_anchor_run.valid());
    CHECK_FALSE(endnote_reference_run.valid());

    const auto comment_target =
        fs::current_path() / "review_comment_reference_handle.docx";
    fs::remove(comment_target);
    featherdoc::Document comment_document(comment_target);
    REQUIRE_FALSE(comment_document.create_empty());
    REQUIRE(comment_document.paragraphs().set_text("Unrelated"));
    auto unrelated_comment_paragraph = comment_document.paragraphs();
    auto unrelated_comment_run = unrelated_comment_paragraph.runs();
    REQUIRE_EQ(comment_document.append_comment("Anchor", "Comment"), 1U);
    auto comment_paragraph = comment_document.paragraphs();
    comment_paragraph.next();
    auto comment_anchor_run = comment_paragraph.runs();
    auto comment_reference_run = comment_anchor_run;
    comment_reference_run.next();
    REQUIRE(comment_paragraph.valid());
    REQUIRE(comment_anchor_run.valid());
    REQUIRE(comment_reference_run.valid());
    REQUIRE(comment_document.remove_comment(0U));
    CHECK(unrelated_comment_paragraph.valid());
    CHECK(unrelated_comment_run.valid());
    CHECK(comment_paragraph.valid());
    CHECK(comment_anchor_run.valid());
    CHECK_FALSE(comment_reference_run.valid());

    fs::remove(footnote_target);
    fs::remove(endnote_target);
    fs::remove(comment_target);
}

TEST_CASE("review reference removal retires header and footer owning runs") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "review_header_footer_reference_handles.docx";
    fs::remove(target);

    std::string comment_id;
    std::string footnote_id;
    std::string endnote_id;
    {
        featherdoc::Document seed(target);
        REQUIRE_FALSE(seed.create_empty());
        REQUIRE(seed.ensure_header_paragraphs().set_text("Header sibling"));
        REQUIRE(seed.ensure_footer_paragraphs().set_text("Footer sibling"));
        REQUIRE_EQ(seed.append_comment("Body comment", "Comment"), 1U);
        REQUIRE_EQ(seed.append_footnote("Body footnote", "Footnote"), 1U);
        REQUIRE_EQ(seed.append_endnote("Body endnote", "Endnote"), 1U);
        const auto comments = seed.list_comments();
        const auto footnotes = seed.list_footnotes();
        const auto endnotes = seed.list_endnotes();
        REQUIRE_EQ(comments.size(), 1U);
        REQUIRE_EQ(footnotes.size(), 1U);
        REQUIRE_EQ(endnotes.size(), 1U);
        comment_id = comments.front().id;
        footnote_id = footnotes.front().id;
        endnote_id = endnotes.front().id;
        REQUIRE_FALSE(seed.save());
    }

    const auto append_references = [](std::string xml,
                                      std::string_view references) {
        const auto paragraph_end = xml.find("</w:p>");
        REQUIRE_NE(paragraph_end, std::string::npos);
        xml.insert(paragraph_end, references);
        return xml;
    };
    auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    header_xml = append_references(
        std::move(header_xml),
        "<w:r><w:commentReference w:id=\"" + comment_id +
            "\"/></w:r><w:r><w:endnoteReference w:id=\"" + endnote_id +
            "\"/></w:r>");
    rewrite_test_docx_entry(target, "word/header1.xml", std::move(header_xml));

    auto footer_xml = read_test_docx_entry(target, "word/footer1.xml");
    footer_xml = append_references(
        std::move(footer_xml),
        "<w:r><w:footnoteReference w:id=\"" + footnote_id +
            "\"/></w:r>");
    rewrite_test_docx_entry(target, "word/footer1.xml", std::move(footer_xml));

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    auto header_paragraph = document.header_paragraphs();
    auto header_sibling_run = header_paragraph.runs();
    auto header_comment_run = header_sibling_run;
    header_comment_run.next();
    auto header_endnote_run = header_comment_run;
    header_endnote_run.next();
    auto footer_paragraph = document.footer_paragraphs();
    auto footer_sibling_run = footer_paragraph.runs();
    auto footer_footnote_run = footer_sibling_run;
    footer_footnote_run.next();
    REQUIRE(header_sibling_run.valid());
    REQUIRE(header_comment_run.valid());
    REQUIRE(header_endnote_run.valid());
    REQUIRE(footer_sibling_run.valid());
    REQUIRE(footer_footnote_run.valid());

    REQUIRE(document.remove_comment(0U));
    CHECK(header_paragraph.valid());
    CHECK(header_sibling_run.valid());
    CHECK_FALSE(header_comment_run.valid());
    CHECK(header_endnote_run.valid());
    CHECK(footer_paragraph.valid());
    CHECK(footer_sibling_run.valid());
    CHECK(footer_footnote_run.valid());

    REQUIRE(document.remove_footnote(0U));
    CHECK(footer_paragraph.valid());
    CHECK(footer_sibling_run.valid());
    CHECK_FALSE(footer_footnote_run.valid());
    CHECK(header_endnote_run.valid());

    REQUIRE(document.remove_endnote(0U));
    CHECK(header_paragraph.valid());
    CHECK(header_sibling_run.valid());
    CHECK_FALSE(header_endnote_run.valid());

    REQUIRE_FALSE(document.save());
    CHECK_EQ(read_test_docx_entry(target, "word/header1.xml")
                 .find("Reference"),
             std::string::npos);
    CHECK_EQ(read_test_docx_entry(target, "word/footer1.xml")
                 .find("Reference"),
             std::string::npos);
    fs::remove(target);
}

TEST_CASE("text range comments retire replaced paragraphs but keep siblings") {
    namespace fs = std::filesystem;

    const auto target =
        fs::current_path() / "review_comment_range_handle_retirement.docx";
    fs::remove(target);
    write_test_docx(
        target,
        R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>First</w:t></w:r></w:p><w:p><w:r><w:t>Middle</w:t></w:r></w:p><w:p><w:r><w:t>Last</w:t></w:r></w:p></w:body></w:document>)");

    featherdoc::Document document(target);
    REQUIRE_FALSE(document.open());
    auto first_paragraph = document.paragraphs();
    auto first_run = first_paragraph.runs();
    auto middle_paragraph = document.paragraphs();
    middle_paragraph.next();
    auto middle_run = middle_paragraph.runs();
    auto last_paragraph = document.paragraphs();
    last_paragraph.next().next();
    auto last_run = last_paragraph.runs();

    REQUIRE_EQ(document.append_paragraph_text_comment(
                   1U, 1U, 3U, "Middle comment"),
               1U);
    CHECK(first_paragraph.valid());
    CHECK(first_run.valid());
    CHECK_FALSE(middle_paragraph.valid());
    CHECK_FALSE(middle_run.valid());
    CHECK(last_paragraph.valid());
    CHECK(last_run.valid());

    auto refreshed_middle = document.paragraphs();
    refreshed_middle.next();
    auto refreshed_middle_run = refreshed_middle.runs();
    REQUIRE(refreshed_middle.valid());
    REQUIRE(refreshed_middle_run.valid());
    REQUIRE(document.set_paragraph_text_comment_range(0U, 2U, 0U, 4U));
    CHECK(first_paragraph.valid());
    CHECK(first_run.valid());
    CHECK(refreshed_middle.valid());
    CHECK(refreshed_middle_run.valid());
    CHECK_FALSE(last_paragraph.valid());
    CHECK_FALSE(last_run.valid());

    auto comments = document.list_comments();
    REQUIRE_EQ(comments.size(), 1U);
    REQUIRE(comments.front().anchor_text.has_value());
    CHECK_EQ(*comments.front().anchor_text, "Last");

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

TEST_CASE("review note and comment identifier exhaustion leaves the document unchanged") {
    namespace fs = std::filesystem;

    constexpr auto maximum_review_identifier = "9223372036854775807";
    const auto source = fs::current_path() / "review_note_identifier_exhaustion.docx";
    const auto before = fs::current_path() / "review_note_identifier_exhaustion.before.docx";
    const auto after = fs::current_path() / "review_note_identifier_exhaustion.after.docx";
    fs::remove(source);
    fs::remove(before);
    fs::remove(after);

    featherdoc::Document seed(source);
    REQUIRE_FALSE(seed.create_empty());
    REQUIRE_EQ(seed.append_footnote("Footnote anchor", "Existing footnote"), 1U);
    REQUIRE_EQ(seed.append_endnote("Endnote anchor", "Existing endnote"), 1U);
    REQUIRE_EQ(seed.append_comment("Comment anchor", "Existing comment"), 1U);
    REQUIRE_FALSE(seed.save());

    auto footnotes_xml = read_test_docx_entry(source, "word/footnotes.xml");
    auto footnote_id = footnotes_xml.find("w:id=\"1\"");
    REQUIRE_NE(footnote_id, std::string::npos);
    footnotes_xml.replace(footnote_id, std::string{"w:id=\"1\""}.size(),
                          std::string{"w:id=\""} + maximum_review_identifier + "\"");
    rewrite_test_docx_entry(source, "word/footnotes.xml", footnotes_xml);

    auto endnotes_xml = read_test_docx_entry(source, "word/endnotes.xml");
    auto endnote_id = endnotes_xml.find("w:id=\"1\"");
    REQUIRE_NE(endnote_id, std::string::npos);
    endnotes_xml.replace(endnote_id, std::string{"w:id=\"1\""}.size(),
                         std::string{"w:id=\""} + maximum_review_identifier + "\"");
    rewrite_test_docx_entry(source, "word/endnotes.xml", endnotes_xml);

    auto comments_xml = read_test_docx_entry(source, "word/comments.xml");
    auto comment_id = comments_xml.find("w:id=\"0\"");
    REQUIRE_NE(comment_id, std::string::npos);
    comments_xml.replace(comment_id, std::string{"w:id=\"0\""}.size(),
                         std::string{"w:id=\""} + maximum_review_identifier + "\"");
    rewrite_test_docx_entry(source, "word/comments.xml", comments_xml);

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE_FALSE(document.save_as(before));

    CHECK_EQ(document.append_footnote("New footnote anchor", "New footnote"), 0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);
    CHECK_EQ(document.append_endnote("New endnote anchor", "New endnote"), 0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);
    CHECK_EQ(document.append_comment("New comment anchor", "New comment"), 0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);
    CHECK_EQ(document.append_comment_reply(0U, "New reply"), 0U);
    CHECK_EQ(document.last_error().code,
             featherdoc::document_errc::identifier_space_exhausted);

    REQUIRE_EQ(document.list_footnotes().size(), 1U);
    REQUIRE_EQ(document.list_endnotes().size(), 1U);
    REQUIRE_EQ(document.list_comments().size(), 1U);
    REQUIRE_FALSE(document.save_as(after));
    CHECK_EQ(read_test_docx_entry(after, test_document_xml_entry),
             read_test_docx_entry(before, test_document_xml_entry));
    CHECK_EQ(read_test_docx_entry(after, "word/footnotes.xml"),
             read_test_docx_entry(before, "word/footnotes.xml"));
    CHECK_EQ(read_test_docx_entry(after, "word/endnotes.xml"),
             read_test_docx_entry(before, "word/endnotes.xml"));
    CHECK_EQ(read_test_docx_entry(after, "word/comments.xml"),
             read_test_docx_entry(before, "word/comments.xml"));

    fs::remove(source);
    fs::remove(before);
    fs::remove(after);
}

TEST_CASE("all tracked change elements reserve review revision identifiers") {
    namespace fs = std::filesystem;

    struct tracked_change_case {
        std::string_view name;
        std::string_view body_xml;
    };
    constexpr auto tracked_changes = std::array{
        tracked_change_case{
            "section",
            R"(<w:sectPr><w:sectPrChange w:id="7"><w:sectPr/></w:sectPrChange></w:sectPr>)"},
        tracked_change_case{
            "table",
            R"(<w:tbl><w:tblPr><w:tblPrChange w:id="7"><w:tblPr/></w:tblPrChange></w:tblPr><w:tr><w:tc><w:p/></w:tc></w:tr></w:tbl>)"},
        tracked_change_case{
            "row",
            R"(<w:tbl><w:tr><w:trPr><w:trPrChange w:id="7"><w:trPr/></w:trPrChange></w:trPr><w:tc><w:p/></w:tc></w:tr></w:tbl>)"},
        tracked_change_case{
            "cell",
            R"(<w:tbl><w:tr><w:tc><w:tcPr><w:tcPrChange w:id="7"><w:tcPr/></w:tcPrChange></w:tcPr><w:p/></w:tc></w:tr></w:tbl>)"},
    };

    for (const auto &tracked_change : tracked_changes) {
        const auto target = fs::current_path() /
                            ("review_revision_" +
                             std::string{tracked_change.name} + "_id.docx");
        fs::remove(target);
        write_test_docx(
            target,
            std::string{
                R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body>)"} +
                std::string{tracked_change.body_xml} +
                R"(</w:body></w:document>)");

        featherdoc::Document document(target);
        REQUIRE_FALSE(document.open());
        REQUIRE_EQ(document.append_insertion_revision("Inserted"), 1U);
        REQUIRE_FALSE(document.save());
        CHECK_NE(read_test_docx_entry(target, test_document_xml_entry)
                     .find(R"(<w:ins w:id="8">)"),
                 std::string::npos);

        fs::remove(target);
    }
}

TEST_CASE("unlisted tracked change maximum identifier fails atomically") {
    namespace fs = std::filesystem;

    struct maximum_identifier_case {
        std::string_view name;
        std::string_view body_xml;
    };
    constexpr auto maximum_identifier_cases = std::array{
        maximum_identifier_case{
            "section",
            R"(<w:sectPr><w:sectPrChange w:id="9223372036854775807"><w:sectPr/></w:sectPrChange></w:sectPr>)"},
        maximum_identifier_case{
            "custom_xml_conflict_ins_start",
            R"(<w:p><w14:customXmlConflictInsRangeStart w:id="9223372036854775807"/></w:p>)"},
        maximum_identifier_case{
            "custom_xml_conflict_ins_end",
            R"(<w:p><w14:customXmlConflictInsRangeEnd w:id="9223372036854775807"/></w:p>)"},
        maximum_identifier_case{
            "custom_xml_conflict_del_start",
            R"(<w:p><w14:customXmlConflictDelRangeStart w:id="9223372036854775807"/></w:p>)"},
        maximum_identifier_case{
            "custom_xml_conflict_del_end",
            R"(<w:p><w14:customXmlConflictDelRangeEnd w:id="9223372036854775807"/></w:p>)"},
    };

    for (const auto &identifier_case : maximum_identifier_cases) {
        const auto stem = "review_revision_" +
                          std::string{identifier_case.name} + "_exhaustion";
        const auto source = fs::current_path() / (stem + ".docx");
        const auto before = fs::current_path() / (stem + ".before.docx");
        const auto after = fs::current_path() / (stem + ".after.docx");
        fs::remove(source);
        fs::remove(before);
        fs::remove(after);
        write_test_docx(
            source,
            std::string{
                R"(<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main" xmlns:w14="http://schemas.microsoft.com/office/word/2010/wordml"><w:body><w:p><w:r><w:t>Keep unchanged</w:t></w:r></w:p>)"} +
                std::string{identifier_case.body_xml} +
                R"(</w:body></w:document>)");

        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save_as(before));

        CHECK_EQ(document.append_insertion_revision("Rejected"), 0U);
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::identifier_space_exhausted);
        CHECK_EQ(document.last_error().entry_name, test_document_xml_entry);
        REQUIRE_FALSE(document.save_as(after));
        CHECK_EQ(read_test_docx_entry(after, test_document_xml_entry),
                 read_test_docx_entry(before, test_document_xml_entry));

        fs::remove(source);
        fs::remove(before);
        fs::remove(after);
    }
}

TEST_CASE("tracked change identifiers are reserved across all document stories") {
    namespace fs = std::filesystem;

    const auto base = fs::current_path() / "review_revision_story_base.docx";
    fs::remove(base);

    featherdoc::Document seed(base);
    REQUIRE_FALSE(seed.create_empty());
    REQUIRE(seed.paragraphs().set_text("Body"));
    auto header = seed.ensure_header_paragraphs();
    auto footer = seed.ensure_footer_paragraphs();
    REQUIRE(header.add_run("Header").valid());
    REQUIRE(footer.add_run("Footer").valid());
    REQUIRE_EQ(seed.append_footnote("Footnote anchor", "Footnote"), 1U);
    REQUIRE_EQ(seed.append_endnote("Endnote anchor", "Endnote"), 1U);
    REQUIRE_EQ(seed.append_comment("Comment anchor", "Comment"), 1U);
    REQUIRE_FALSE(seed.save());

    const auto base_entries = read_test_archive_entries(base);
    constexpr auto story_entries = std::array{
        std::string_view{test_document_xml_entry},
        std::string_view{"word/header1.xml"},
        std::string_view{"word/footer1.xml"},
        std::string_view{"word/footnotes.xml"},
        std::string_view{"word/endnotes.xml"},
        std::string_view{"word/comments.xml"},
    };
    const auto inject_revision = [](std::string xml,
                                    std::string_view identifier) {
        const auto paragraph_end = xml.find("</w:p>");
        if (paragraph_end == std::string::npos) {
            return std::string{};
        }
        xml.insert(paragraph_end,
                   "<w:ins w:id=\"" + std::string{identifier} +
                       "\"><w:r><w:t>reserved</w:t></w:r></w:ins>");
        return xml;
    };

    for (const auto story_entry : story_entries) {
        auto entries = base_entries;
        const auto entry = std::find_if(
            entries.begin(), entries.end(), [story_entry](const auto &item) {
                return item.first == story_entry;
            });
        REQUIRE(entry != entries.end());
        entry->second = inject_revision(std::move(entry->second), "7");
        REQUIRE_FALSE(entry->second.empty());

        auto stem = std::string{"review_revision_story_"};
        std::replace_copy(story_entry.begin(), story_entry.end(),
                          std::back_inserter(stem), '/', '_');
        const auto source = fs::current_path() / (stem + ".docx");
        const auto output = fs::current_path() / (stem + ".out.docx");
        fs::remove(source);
        fs::remove(output);
        write_test_archive_entries(source, entries);

        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        REQUIRE_EQ(document.append_insertion_revision("Inserted"), 1U);
        REQUIRE_FALSE(document.save_as(output));
        CHECK_NE(read_test_docx_entry(output, test_document_xml_entry)
                     .find(R"(w:id="8")"),
                 std::string::npos);

        fs::remove(source);
        fs::remove(output);
    }

    constexpr auto lazy_story_entries = std::array{
        std::string_view{"word/footnotes.xml"},
        std::string_view{"word/endnotes.xml"},
        std::string_view{"word/comments.xml"},
    };
    for (const auto story_entry : lazy_story_entries) {
        auto entries = base_entries;
        const auto entry = std::find_if(
            entries.begin(), entries.end(), [story_entry](const auto &item) {
                return item.first == story_entry;
            });
        REQUIRE(entry != entries.end());
        entry->second = inject_revision(std::move(entry->second),
                                        "9223372036854775807");
        REQUIRE_FALSE(entry->second.empty());

        auto stem = std::string{"review_revision_story_max_"};
        std::replace_copy(story_entry.begin(), story_entry.end(),
                          std::back_inserter(stem), '/', '_');
        const auto source = fs::current_path() / (stem + ".docx");
        const auto before = fs::current_path() / (stem + ".before.docx");
        const auto after = fs::current_path() / (stem + ".after.docx");
        fs::remove(source);
        fs::remove(before);
        fs::remove(after);
        write_test_archive_entries(source, entries);

        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save_as(before));
        CHECK_FALSE(document.replace_run_revision(0U, 0U, "Replacement"));
        CHECK_EQ(document.last_error().code,
                 featherdoc::document_errc::identifier_space_exhausted);
        REQUIRE_FALSE(document.save_as(after));
        CHECK_EQ(read_test_docx_entry(after, test_document_xml_entry),
                 read_test_docx_entry(before, test_document_xml_entry));

        fs::remove(source);
        fs::remove(before);
        fs::remove(after);
    }

    fs::remove(base);
}

TEST_CASE("orphan comments fail resolved and reply edits without package changes") {
    namespace fs = std::filesystem;

    const auto source = fs::current_path() / "review_orphan_comments.docx";
    fs::remove(source);
    featherdoc::Document seed(source);
    REQUIRE_FALSE(seed.create_empty());
    REQUIRE(seed.paragraphs().set_text("Keep package unchanged"));
    REQUIRE_FALSE(seed.save());

    auto source_entries = read_test_archive_entries(source);
    source_entries.emplace_back(
        "word/comments.xml",
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:comment w:id="0"/></w:comments>)");
    write_test_archive_entries(source, source_entries);

    const auto sorted_entries = [](const fs::path &path) {
        auto entries = read_test_archive_entries(path);
        std::sort(entries.begin(), entries.end(),
                  [](const auto &left, const auto &right) {
                      return left.first < right.first;
                  });
        return entries;
    };
    const auto verify_failure_is_atomic = [&](std::string_view operation_name,
                                              auto &&operation) {
        const auto stem =
            "review_orphan_comments_" + std::string{operation_name};
        const auto before = fs::current_path() / (stem + ".before.docx");
        const auto after = fs::current_path() / (stem + ".after.docx");
        fs::remove(before);
        fs::remove(after);

        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        REQUIRE_FALSE(document.save_as(before));
        CHECK_FALSE(operation(document));
        CHECK_EQ(document.last_error().code,
                 std::make_error_code(std::errc::invalid_argument));
        REQUIRE_FALSE(document.save_as(after));

        const auto before_entries = sorted_entries(before);
        const auto after_entries = sorted_entries(after);
        REQUIRE_EQ(after_entries.size(), before_entries.size());
        for (std::size_t index = 0U; index < before_entries.size(); ++index) {
            CHECK_EQ(after_entries[index].first, before_entries[index].first);
            CHECK_EQ(after_entries[index].second, before_entries[index].second);
        }

        fs::remove(before);
        fs::remove(after);
    };

    verify_failure_is_atomic("resolved", [](auto &document) {
        return document.set_comment_resolved(0U, true);
    });
    verify_failure_is_atomic("reply", [](auto &document) {
        return document.append_comment_reply(0U, "Reply") != 0U;
    });

    fs::remove(source);
}

TEST_CASE("editing a missing review part does not create package state") {
    namespace fs = std::filesystem;

    const auto verify_missing_part_edit =
        [](std::string_view operation_name, auto &&operation) {
            const auto before = fs::current_path() /
                                ("review_missing_" +
                                 std::string{operation_name} + ".before.docx");
            const auto after = fs::current_path() /
                               ("review_missing_" +
                                std::string{operation_name} + ".after.docx");
            fs::remove(before);
            fs::remove(after);

            featherdoc::Document document(before);
            REQUIRE_FALSE(document.create_empty());
            REQUIRE(document.paragraphs().set_text("Keep package unchanged"));
            REQUIRE_FALSE(document.save());

            constexpr auto package_entries = std::array{
                test_document_xml_entry, "word/_rels/document.xml.rels",
                test_content_types_xml_entry};
            std::array<std::optional<std::string>, package_entries.size()>
                before_entries;
            for (std::size_t index = 0U; index < package_entries.size(); ++index) {
                if (test_docx_entry_exists(before, package_entries[index])) {
                    before_entries[index] =
                        read_test_docx_entry(before, package_entries[index]);
                }
            }

            CHECK_FALSE(operation(document));
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::invalid_argument));
            REQUIRE_FALSE(document.save_as(after));

            for (std::size_t index = 0U; index < package_entries.size(); ++index) {
                CHECK_EQ(test_docx_entry_exists(after, package_entries[index]),
                         before_entries[index].has_value());
                if (before_entries[index].has_value()) {
                    CHECK_EQ(read_test_docx_entry(after, package_entries[index]),
                             *before_entries[index]);
                }
            }
            CHECK_FALSE(test_docx_entry_exists(after, "word/footnotes.xml"));
            CHECK_FALSE(test_docx_entry_exists(after, "word/endnotes.xml"));
            CHECK_FALSE(test_docx_entry_exists(after, "word/comments.xml"));
            CHECK_FALSE(
                test_docx_entry_exists(after, "word/commentsExtended.xml"));

            fs::remove(before);
            fs::remove(after);
        };

    verify_missing_part_edit("replace_footnote", [](auto &document) {
        return document.replace_footnote(0U, "Replacement");
    });
    verify_missing_part_edit("remove_footnote", [](auto &document) {
        return document.remove_footnote(0U);
    });
    verify_missing_part_edit("replace_endnote", [](auto &document) {
        return document.replace_endnote(0U, "Replacement");
    });
    verify_missing_part_edit("remove_endnote", [](auto &document) {
        return document.remove_endnote(0U);
    });
    verify_missing_part_edit("resolve_comment", [](auto &document) {
        return document.set_comment_resolved(0U, true);
    });
    verify_missing_part_edit("replace_comment", [](auto &document) {
        return document.replace_comment(0U, "Replacement");
    });
    verify_missing_part_edit("remove_comment", [](auto &document) {
        return document.remove_comment(0U);
    });
    verify_missing_part_edit("comment_metadata", [](auto &document) {
        featherdoc::comment_metadata_update metadata;
        metadata.author = "Reviewer";
        return document.set_comment_metadata(0U, metadata);
    });
    verify_missing_part_edit("paragraph_comment_range", [](auto &document) {
        return document.set_paragraph_text_comment_range(0U, 0U, 0U, 1U);
    });
    verify_missing_part_edit("comment_range", [](auto &document) {
        return document.set_text_range_comment_range(0U, 0U, 0U, 0U, 1U);
    });
    verify_missing_part_edit("comment_reply", [](auto &document) {
        return document.append_comment_reply(0U, "Reply") != 0U;
    });
}

TEST_CASE("out of range review edits preserve existing package parts") {
    namespace fs = std::filesystem;

    const auto before =
        fs::current_path() / "review_out_of_range_edits.before.docx";
    const auto after =
        fs::current_path() / "review_out_of_range_edits.after.docx";
    fs::remove(before);
    fs::remove(after);

    featherdoc::Document seed(before);
    REQUIRE_FALSE(seed.create_empty());
    REQUIRE_EQ(seed.append_footnote("Footnote anchor", "Footnote"), 1U);
    REQUIRE_EQ(seed.append_endnote("Endnote anchor", "Endnote"), 1U);
    REQUIRE_EQ(seed.append_comment("Comment anchor", "Comment"), 1U);
    REQUIRE_FALSE(seed.save());

    constexpr auto package_entries = std::array{
        test_document_xml_entry, "word/_rels/document.xml.rels",
        test_content_types_xml_entry, "word/footnotes.xml",
        "word/endnotes.xml", "word/comments.xml"};
    std::array<std::string, package_entries.size()> before_entries;
    for (std::size_t index = 0U; index < package_entries.size(); ++index) {
        REQUIRE(test_docx_entry_exists(before, package_entries[index]));
        before_entries[index] =
            read_test_docx_entry(before, package_entries[index]);
    }

    featherdoc::Document document(before);
    REQUIRE_FALSE(document.open());
    CHECK_FALSE(document.replace_footnote(99U, "Replacement"));
    CHECK_FALSE(document.remove_footnote(99U));
    CHECK_FALSE(document.replace_endnote(99U, "Replacement"));
    CHECK_FALSE(document.remove_endnote(99U));
    CHECK_FALSE(document.set_comment_resolved(99U, true));
    CHECK_FALSE(document.replace_comment(99U, "Replacement"));
    CHECK_FALSE(document.remove_comment(99U));
    featherdoc::comment_metadata_update metadata;
    metadata.author = "Reviewer";
    CHECK_FALSE(document.set_comment_metadata(99U, metadata));
    CHECK_FALSE(document.set_paragraph_text_comment_range(99U, 0U, 0U, 1U));
    CHECK_FALSE(document.set_text_range_comment_range(99U, 0U, 0U, 0U, 1U));
    CHECK_EQ(document.append_comment_reply(99U, "Reply"), 0U);

    REQUIRE_FALSE(document.save_as(after));
    for (std::size_t index = 0U; index < package_entries.size(); ++index) {
        REQUIRE(test_docx_entry_exists(after, package_entries[index]));
        CHECK_EQ(read_test_docx_entry(after, package_entries[index]),
                 before_entries[index]);
    }
    CHECK_FALSE(test_docx_entry_exists(after, "word/commentsExtended.xml"));

    fs::remove(before);
    fs::remove(after);
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

TEST_CASE("run revision insertion preserves intervening marker boundaries") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "review_revision_marker_boundary.docx";
    fs::remove(target);
    write_test_docx(target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:bookmarkStart w:id="7" w:name="范围"/>
      <w:r><w:t>锚点😀</w:t></w:r>
      <w:bookmarkEnd w:id="7"/>
      <w:proofErr w:type="spellStart"/>
      <w:proofErr w:type="spellEnd"/>
      <w:r><w:t>尾部</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.insert_run_revision_after(0U, 0U, "插入修订🚀", "作者甲",
                                        "2026-07-17T08:00:00Z"));
    CHECK_FALSE(doc.save());

    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    const auto anchor_position = saved_xml.find("<w:t>锚点😀</w:t>");
    const auto revision_position = saved_xml.find("<w:ins ", anchor_position);
    const auto bookmark_end_position =
        saved_xml.find(R"(<w:bookmarkEnd w:id="7"/>)", anchor_position);
    const auto proofing_position =
        saved_xml.find(R"(<w:proofErr w:type="spellStart"/>)", anchor_position);
    const auto tail_position = saved_xml.find("<w:t>尾部</w:t>", anchor_position);
    REQUIRE_NE(anchor_position, std::string::npos);
    REQUIRE_NE(revision_position, std::string::npos);
    REQUIRE_NE(bookmark_end_position, std::string::npos);
    REQUIRE_NE(proofing_position, std::string::npos);
    REQUIRE_NE(tail_position, std::string::npos);
    CHECK_LT(anchor_position, revision_position);
    CHECK_LT(revision_position, bookmark_end_position);
    CHECK_LT(bookmark_end_position, proofing_position);
    CHECK_LT(proofing_position, tail_position);
    CHECK_NE(saved_xml.find("<w:t>插入修订🚀</w:t>", revision_position),
             std::string::npos);

    fs::remove(target);
}

TEST_CASE("run revision authoring invalidates retained direct run handles") {
    namespace fs = std::filesystem;

    const fs::path delete_target =
        fs::current_path() / "review_revisions_run_handle_delete.docx";
    fs::remove(delete_target);
    write_test_docx(delete_target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>Delete me</w:t></w:r><w:r><w:t>Keep</w:t></w:r></w:p></w:body>
</w:document>
)");

    featherdoc::Document delete_doc(delete_target);
    CHECK_FALSE(delete_doc.open());
    auto delete_paragraph = delete_doc.paragraphs();
    auto deleted_run = delete_paragraph.runs();
    REQUIRE(deleted_run.valid());
    CHECK_EQ(deleted_run.get_text(), "Delete me");
    CHECK(delete_doc.delete_run_revision(0U, 0U));
    CHECK_FALSE(deleted_run.valid());

    auto refreshed_delete_paragraph = delete_doc.paragraphs();
    auto refreshed_delete_run = refreshed_delete_paragraph.runs();
    REQUIRE(refreshed_delete_run.valid());
    CHECK_EQ(refreshed_delete_run.get_text(), "Keep");

    const fs::path replace_target =
        fs::current_path() / "review_revisions_run_handle_replace.docx";
    fs::remove(replace_target);
    write_test_docx(replace_target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>Replace me</w:t></w:r><w:r><w:t>Keep</w:t></w:r></w:p></w:body>
</w:document>
)");

    featherdoc::Document replace_doc(replace_target);
    CHECK_FALSE(replace_doc.open());
    auto replace_paragraph = replace_doc.paragraphs();
    auto replaced_run = replace_paragraph.runs();
    REQUIRE(replaced_run.valid());
    CHECK_EQ(replaced_run.get_text(), "Replace me");
    CHECK(replace_doc.replace_run_revision(0U, 0U, "Replacement"));
    CHECK_FALSE(replaced_run.valid());

    auto refreshed_replace_paragraph = replace_doc.paragraphs();
    auto refreshed_replace_run = refreshed_replace_paragraph.runs();
    REQUIRE(refreshed_replace_run.valid());
    CHECK_EQ(refreshed_replace_run.get_text(), "Keep");

    fs::remove(delete_target);
    fs::remove(replace_target);
}

TEST_CASE("paragraph text revisions invalidate retained body handles") {
    namespace fs = std::filesystem;

    const auto write_source = [](const fs::path &path) {
        write_test_docx(
            path,
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>Alpha</w:t></w:r><w:r><w:t>Keep</w:t></w:r></w:p></w:body>
</w:document>
)");
    };

    const auto delete_target =
        fs::current_path() / "review_revisions_paragraph_handle_delete.docx";
    fs::remove(delete_target);
    write_source(delete_target);

    featherdoc::Document delete_document(delete_target);
    REQUIRE_FALSE(delete_document.open());
    auto retained_delete_paragraph = delete_document.paragraphs();
    auto retained_delete_run = retained_delete_paragraph.runs();
    REQUIRE(retained_delete_paragraph.valid());
    REQUIRE(retained_delete_run.valid());
    CHECK(delete_document.delete_paragraph_text_revision(0U, 1U, 3U));
    CHECK_FALSE(retained_delete_paragraph.valid());
    CHECK_FALSE(retained_delete_run.valid());

    auto refreshed_delete_paragraph = delete_document.paragraphs();
    REQUIRE(refreshed_delete_paragraph.valid());
    CHECK_EQ(delete_document.list_revisions().size(), 1U);

    const auto replace_target =
        fs::current_path() / "review_revisions_paragraph_handle_replace.docx";
    fs::remove(replace_target);
    write_source(replace_target);

    featherdoc::Document replace_document(replace_target);
    REQUIRE_FALSE(replace_document.open());
    auto retained_replace_paragraph = replace_document.paragraphs();
    auto retained_replace_run = retained_replace_paragraph.runs();
    REQUIRE(retained_replace_paragraph.valid());
    REQUIRE(retained_replace_run.valid());
    CHECK(replace_document.replace_paragraph_text_revision(0U, 1U, 3U,
                                                           "Replacement"));
    CHECK_FALSE(retained_replace_paragraph.valid());
    CHECK_FALSE(retained_replace_run.valid());

    auto refreshed_replace_paragraph = replace_document.paragraphs();
    REQUIRE(refreshed_replace_paragraph.valid());
    CHECK_EQ(replace_document.list_revisions().size(), 2U);

    fs::remove(delete_target);
    fs::remove(replace_target);
}

TEST_CASE("run revision text preserves leading and trailing whitespace") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "review_revisions_run_text_space.docx";
    fs::remove(target);
    write_test_docx(target,
                    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body><w:p><w:r><w:t>Anchor</w:t></w:r></w:p></w:body>
</w:document>
)");

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK(doc.insert_run_revision_after(0U, 0U, " Leading and trailing "));
    CHECK_FALSE(doc.save());

    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find(
                 R"(<w:t xml:space="preserve"> Leading and trailing </w:t>)"),
             std::string::npos);

    fs::remove(target);
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
