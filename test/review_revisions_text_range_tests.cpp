#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>

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
