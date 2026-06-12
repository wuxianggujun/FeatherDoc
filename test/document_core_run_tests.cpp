#include "document_core_unit_test_support.hpp"

TEST_CASE("paragraph set_text replaces all runs in place") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "paragraph_set_text.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    auto first_run = paragraph.add_run("first");
    REQUIRE(first_run.has_next());
    REQUIRE(paragraph.add_run(" second").has_next());
    REQUIRE(paragraph.add_run(" third").has_next());

    CHECK(paragraph.set_text("replaced paragraph"));
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "replaced paragraph\n");

    auto reopened_paragraph = reopened.paragraphs();
    REQUIRE(reopened_paragraph.has_next());

    std::size_t run_count = 0U;
    for (auto run = reopened_paragraph.runs(); run.has_next(); run.next()) {
        ++run_count;
    }
    CHECK_EQ(run_count, 1U);

    fs::remove(target);
}

TEST_CASE("run remove deletes the targeted run from a paragraph") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_remove.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.set_text("seed"));

    auto first_run = paragraph.runs();
    REQUIRE(first_run.has_next());
    REQUIRE(first_run.set_text("left"));
    auto removed_run = paragraph.add_run(" middle");
    REQUIRE(removed_run.has_next());
    REQUIRE(paragraph.add_run(" right").has_next());

    CHECK(removed_run.remove());
    CHECK(removed_run.has_next());
    CHECK_EQ(removed_run.get_text(), " right");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "left right\n");

    fs::remove(target);
}

TEST_CASE("run insertions keep the anchor usable and preserve run order") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "run_insert_around_anchor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("anchor"));

    auto anchor = paragraph.runs();
    REQUIRE(anchor.has_next());

    auto inserted_before =
        anchor.insert_run_before("left ", featherdoc::formatting_flag::bold);
    REQUIRE(inserted_before.has_next());
    auto inserted_after = anchor.insert_run_after(" right");
    REQUIRE(inserted_after.has_next());

    CHECK(anchor.has_next());
    CHECK_EQ(anchor.get_text(), "anchor");
    CHECK_EQ(inserted_before.get_text(), "left ");
    CHECK_EQ(inserted_after.get_text(), " right");

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "left anchor right\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "left ");
    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), " right");

    fs::remove(target);
}

TEST_CASE("insert_run_before saves cleanly from an empty paragraph run cursor") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_before_empty_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto runs = doc.paragraphs().runs();
    CHECK_FALSE(runs.has_next());

    auto inserted = runs.insert_run_before("created from empty run cursor");
    REQUIRE(inserted.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "created from empty run cursor\n");

    fs::remove(target);
}

TEST_CASE("insert_run_after appends cleanly from an exhausted run cursor") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_after_exhausted_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("seed"));

    auto runs = paragraph.runs();
    while (runs.has_next()) {
        runs.next();
    }

    auto inserted = runs.insert_run_after(" tail");
    REQUIRE(inserted.has_next());
    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "seed tail\n");

    fs::remove(target);
}

TEST_CASE("insert_run_like_before clones run properties and keeps the anchor usable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_before_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto anchor = paragraph.add_run("anchor");
    REQUIRE(anchor.has_next());
    CHECK(doc.set_run_style(anchor, "Strong"));
    CHECK(anchor.set_font_family("Segoe UI"));
    CHECK(anchor.set_east_asia_font_family("Microsoft YaHei"));
    CHECK(anchor.set_language("en-US"));
    CHECK(anchor.set_east_asia_language("zh-CN"));
    CHECK(anchor.set_bidi_language("ar-SA"));
    CHECK(anchor.set_rtl());

    auto inserted = anchor.insert_run_like_before();
    REQUIRE(inserted.has_next());
    CHECK_EQ(inserted.get_text(), "");
    CHECK(inserted.set_text("before "));

    const auto inserted_font_family = inserted.font_family();
    REQUIRE(inserted_font_family.has_value());
    CHECK_EQ(*inserted_font_family, "Segoe UI");

    const auto inserted_east_asia_font = inserted.east_asia_font_family();
    REQUIRE(inserted_east_asia_font.has_value());
    CHECK_EQ(*inserted_east_asia_font, "Microsoft YaHei");

    const auto inserted_language = inserted.language();
    REQUIRE(inserted_language.has_value());
    CHECK_EQ(*inserted_language, "en-US");

    const auto inserted_east_asia_language = inserted.east_asia_language();
    REQUIRE(inserted_east_asia_language.has_value());
    CHECK_EQ(*inserted_east_asia_language, "zh-CN");

    const auto inserted_bidi_language = inserted.bidi_language();
    REQUIRE(inserted_bidi_language.has_value());
    CHECK_EQ(*inserted_bidi_language, "ar-SA");

    const auto inserted_rtl = inserted.rtl();
    REQUIRE(inserted_rtl.has_value());
    CHECK(*inserted_rtl);

    CHECK(anchor.has_next());
    CHECK_EQ(anchor.get_text(), "anchor");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:rStyle w:val=\"Strong\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:ascii=\"Segoe UI\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "w:eastAsia=\"Microsoft YaHei\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:val=\"en-US\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:eastAsia=\"zh-CN\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:bidi=\"ar-SA\""), 2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:rtl w:val=\"1\""), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "before anchor\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "before ");
    const auto reopened_inserted_language = reopened_runs.language();
    REQUIRE(reopened_inserted_language.has_value());
    CHECK_EQ(*reopened_inserted_language, "en-US");
    const auto reopened_inserted_rtl = reopened_runs.rtl();
    REQUIRE(reopened_inserted_rtl.has_value());
    CHECK(*reopened_inserted_rtl);

    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    const auto reopened_anchor_language = reopened_runs.language();
    REQUIRE(reopened_anchor_language.has_value());
    CHECK_EQ(*reopened_anchor_language, "en-US");
    const auto reopened_anchor_rtl = reopened_runs.rtl();
    REQUIRE(reopened_anchor_rtl.has_value());
    CHECK(*reopened_anchor_rtl);

    fs::remove(target);
}

TEST_CASE("insert_run_like_after clones run properties without copying anchor text") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_after_body.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());

    auto anchor = paragraph.add_run("anchor");
    REQUIRE(anchor.has_next());
    CHECK(doc.set_run_style(anchor, "Emphasis"));
    CHECK(anchor.set_language("en-US"));
    CHECK(anchor.set_rtl());

    auto inserted = anchor.insert_run_like_after();
    REQUIRE(inserted.has_next());
    CHECK_EQ(inserted.get_text(), "");
    CHECK(inserted.set_text(" after"));
    CHECK_EQ(anchor.get_text(), "anchor");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(saved_document_xml,
                                         "<w:rStyle w:val=\"Emphasis\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "<w:t>anchor</w:t>"), 1U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "anchor after\n");

    auto reopened_runs = reopened.paragraphs().runs();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), "anchor");
    const auto reopened_anchor_language = reopened_runs.language();
    REQUIRE(reopened_anchor_language.has_value());
    CHECK_EQ(*reopened_anchor_language, "en-US");

    reopened_runs.next();
    REQUIRE(reopened_runs.has_next());
    CHECK_EQ(reopened_runs.get_text(), " after");
    const auto reopened_inserted_language = reopened_runs.language();
    REQUIRE(reopened_inserted_language.has_value());
    CHECK_EQ(*reopened_inserted_language, "en-US");
    const auto reopened_inserted_rtl = reopened_runs.rtl();
    REQUIRE(reopened_inserted_rtl.has_value());
    CHECK(*reopened_inserted_rtl);

    fs::remove(target);
}

TEST_CASE("insert_run_like requires a live anchor run") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "insert_run_like_invalid_cursor.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto empty_runs = doc.paragraphs().runs();
    CHECK_FALSE(empty_runs.has_next());

    auto inserted_before = empty_runs.insert_run_like_before();
    CHECK_FALSE(inserted_before.has_next());

    auto paragraph = doc.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("seed"));

    auto exhausted_runs = paragraph.runs();
    while (exhausted_runs.has_next()) {
        exhausted_runs.next();
    }

    auto inserted_after = exhausted_runs.insert_run_like_after();
    CHECK_FALSE(inserted_after.has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    CHECK_EQ(collect_document_text(reopened), "seed\n");

    fs::remove(target);
}
