#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("body template part can append and edit tables") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "body_template_tables.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body intro"));

    auto first_table = body_template.append_table(1, 2);
    REQUIRE(first_table.has_next());
    auto first_cell = first_table.rows().cells();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_text("body-a"));
    first_cell.next();
    REQUIRE(first_cell.has_next());
    CHECK(first_cell.set_text("body-b"));

    auto removable_table = body_template.append_table(1, 1);
    REQUIRE(removable_table.has_next());
    CHECK(removable_table.rows().cells().set_text("temporary table"));

    CHECK(removable_table.remove());
    CHECK(removable_table.has_next());
    CHECK_EQ(removable_table.rows().cells().get_text(), "body-a");
    CHECK(removable_table.rows().cells().set_text("body-a-updated"));

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    CHECK_EQ(collect_template_part_text(reopened_body), "body intro\n");
    CHECK_EQ(collect_template_part_table_text(reopened_body), "body-a-updated\nbody-b\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:tbl"), 1U);

    fs::remove(target);
}

TEST_CASE("body template part can append paragraphs before section properties") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "body_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("header"));

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto paragraph = body_template.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("body intro"));

    auto appended = body_template.append_paragraph("body appended");
    REQUIRE(appended.has_next());
    CHECK(appended.add_run(" tail").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    auto reopened_body = reopened.body_template();
    REQUIRE(static_cast<bool>(reopened_body));
    CHECK_EQ(collect_template_part_text(reopened_body), "body intro\nbody appended tail\n");

    const auto xml_text = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(xml_text.c_str()));
    const auto body_node = xml_document.child("w:document").child("w:body");
    REQUIRE(body_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(body_node, "w:p"), 2U);
    CHECK_EQ(std::string_view{body_node.last_child().name()}, "w:sectPr");

    fs::remove(target);
}

TEST_CASE("header template part can append paragraphs and keep the returned paragraph editable") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "header_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &header = doc.ensure_section_header_paragraphs(0);
    REQUIRE(header.has_next());
    CHECK(header.set_text("Header intro"));

    auto header_template = doc.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));

    auto appended = header_template.append_paragraph("Header appended");
    REQUIRE(appended.has_next());
    CHECK(appended.add_run(" tail").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_template_part_text(header_template), "Header intro\nHeader appended tail\n");

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(header_xml.c_str()));
    const auto header_node = xml_document.child("w:hdr");
    REQUIRE(header_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:p"), 2U);

    fs::remove(target);
}

TEST_CASE("footer template part can append an empty paragraph and populate it later") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "footer_template_append_paragraph.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &footer = doc.ensure_section_footer_paragraphs(0);
    REQUIRE(footer.has_next());
    CHECK(footer.set_text("Footer intro"));

    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));

    auto appended = footer_template.append_paragraph();
    REQUIRE(appended.has_next());
    CHECK(appended.add_run("Footer appended").has_next());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    footer_template = reopened.section_footer_template(0);
    REQUIRE(static_cast<bool>(footer_template));
    CHECK_EQ(collect_template_part_text(footer_template), "Footer intro\nFooter appended\n");

    const auto footer_xml = read_test_docx_entry(target, "word/footer1.xml");
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(footer_xml.c_str()));
    const auto footer_node = xml_document.child("w:ftr");
    REQUIRE(footer_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(footer_node, "w:p"), 2U);

    fs::remove(target);
}

TEST_CASE("header and footer template parts can append page number fields") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_page_number_fields.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto &header = doc.ensure_section_header_paragraphs(0);
    auto &footer = doc.ensure_section_footer_paragraphs(0);
    REQUIRE(header.has_next());
    REQUIRE(footer.has_next());

    auto header_template = doc.section_header_template(0);
    auto footer_template = doc.section_footer_template(0);
    REQUIRE(static_cast<bool>(header_template));
    REQUIRE(static_cast<bool>(footer_template));

    CHECK(header_template.append_page_number_field());
    CHECK(footer_template.append_total_pages_field());

    CHECK_FALSE(doc.save());

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    REQUIRE(static_cast<bool>(reopened.section_header_template(0)));
    REQUIRE(static_cast<bool>(reopened.section_footer_template(0)));

    const auto header_xml = read_test_docx_entry(target, "word/header1.xml");
    const auto footer_xml = read_test_docx_entry(target, "word/footer1.xml");

    pugi::xml_document header_document;
    pugi::xml_document footer_document;
    REQUIRE(header_document.load_string(header_xml.c_str()));
    REQUIRE(footer_document.load_string(footer_xml.c_str()));

    const auto header_node = header_document.child("w:hdr");
    const auto footer_node = footer_document.child("w:ftr");
    REQUIRE(header_node != pugi::xml_node{});
    REQUIRE(footer_node != pugi::xml_node{});
    CHECK_EQ(count_named_children(header_node, "w:p"), 1U);
    CHECK_EQ(count_named_children(footer_node, "w:p"), 1U);

    const auto header_field = header_node.child("w:p").child("w:fldSimple");
    const auto footer_field = footer_node.child("w:p").child("w:fldSimple");
    REQUIRE(header_field != pugi::xml_node{});
    REQUIRE(footer_field != pugi::xml_node{});
    CHECK_EQ(std::string_view{header_field.attribute("w:instr").value()}, " PAGE ");
    CHECK_EQ(std::string_view{footer_field.attribute("w:instr").value()},
             " NUMPAGES ");
    CHECK_EQ(std::string_view{header_field.child("w:r").child("w:t").text().get()},
             "1");
    CHECK_EQ(std::string_view{footer_field.child("w:r").child("w:t").text().get()},
             "1");

    fs::remove(target);
}

TEST_CASE("template parts can append inspect and replace TOC REF and SEQ fields") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_generic_fields.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));
    REQUIRE(body_template.paragraphs().has_next());
    CHECK(body_template.paragraphs().set_text("Field API fixture"));

    auto toc_options = featherdoc::table_of_contents_field_options{};
    toc_options.min_outline_level = 1U;
    toc_options.max_outline_level = 2U;
    CHECK(body_template.append_table_of_contents_field(toc_options, "TOC placeholder"));
    CHECK(body_template.append_reference_field("target_bookmark", {}, "Referenced heading"));

    auto page_reference_options = featherdoc::page_reference_field_options{};
    page_reference_options.relative_position = true;
    CHECK(body_template.append_page_reference_field("target_bookmark",
                                                   page_reference_options, "3"));

    auto style_reference_options = featherdoc::style_reference_field_options{};
    style_reference_options.paragraph_number = true;
    style_reference_options.relative_position = true;
    CHECK(body_template.append_style_reference_field(
        "Heading 1", style_reference_options, "Section heading"));

    CHECK(body_template.append_document_property_field("Title", {}, "FeatherDoc"));

    auto date_options = featherdoc::date_field_options{};
    date_options.format = "yyyy-MM-dd";
    date_options.state.dirty = true;
    CHECK(body_template.append_date_field(date_options, "2026-05-01"));

    auto hyperlink_options = featherdoc::hyperlink_field_options{};
    hyperlink_options.anchor = "target_heading";
    hyperlink_options.tooltip = "Open target heading";
    hyperlink_options.state.locked = true;
    CHECK(body_template.append_hyperlink_field(
        "https://example.com/report", hyperlink_options, "Open report"));

    auto sequence_options = featherdoc::sequence_field_options{};
    sequence_options.restart_value = 1U;
    CHECK(body_template.append_sequence_field("Figure", sequence_options, "1"));

    auto fields = body_template.list_fields();
    REQUIRE(fields.size() == 8U);
    CHECK_EQ(fields[0].kind, featherdoc::field_kind::table_of_contents);
    CHECK_EQ(fields[0].instruction, " TOC \\o \"1-2\" \\h \\z \\u ");
    CHECK_EQ(fields[0].result_text, "TOC placeholder");
    CHECK_EQ(fields[1].kind, featherdoc::field_kind::reference);
    CHECK_EQ(fields[1].instruction, " REF target_bookmark \\h \\* MERGEFORMAT ");
    CHECK_EQ(fields[1].result_text, "Referenced heading");
    CHECK_EQ(fields[2].kind, featherdoc::field_kind::page_reference);
    CHECK_EQ(fields[2].instruction,
             " PAGEREF target_bookmark \\h \\p \\* MERGEFORMAT ");
    CHECK_EQ(fields[2].result_text, "3");
    CHECK_EQ(fields[3].kind, featherdoc::field_kind::style_reference);
    CHECK_EQ(fields[3].instruction,
             " STYLEREF \"Heading 1\" \\n \\p \\* MERGEFORMAT ");
    CHECK_EQ(fields[3].result_text, "Section heading");
    CHECK_EQ(fields[4].kind, featherdoc::field_kind::document_property);
    CHECK_EQ(fields[4].instruction, " DOCPROPERTY Title \\* MERGEFORMAT ");
    CHECK_EQ(fields[4].result_text, "FeatherDoc");
    CHECK_EQ(fields[5].kind, featherdoc::field_kind::date);
    CHECK_EQ(fields[5].instruction, " DATE \\@ \"yyyy-MM-dd\" \\* MERGEFORMAT ");
    CHECK_EQ(fields[5].result_text, "2026-05-01");
    CHECK(fields[5].dirty);
    CHECK_FALSE(fields[5].locked);
    CHECK_EQ(fields[6].kind, featherdoc::field_kind::hyperlink);
    CHECK_EQ(fields[6].instruction,
             " HYPERLINK \"https://example.com/report\" \\l \"target_heading\" "
             "\\o \"Open target heading\" \\* MERGEFORMAT ");
    CHECK_EQ(fields[6].result_text, "Open report");
    CHECK_FALSE(fields[6].dirty);
    CHECK(fields[6].locked);
    CHECK_EQ(fields[7].kind, featherdoc::field_kind::sequence);
    CHECK_EQ(fields[7].instruction, " SEQ Figure \\* ARABIC \\r 1 \\* MERGEFORMAT ");

    CHECK(body_template.replace_field(7U, " SEQ Table \\* ARABIC \\r 1 ", "1"));
    fields = body_template.list_fields();
    REQUIRE(fields.size() == 8U);
    CHECK_EQ(fields[7].kind, featherdoc::field_kind::sequence);
    CHECK_EQ(fields[7].instruction, " SEQ Table \\* ARABIC \\r 1 ");

    CHECK_FALSE(doc.save());

    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document xml_document;
    REQUIRE(xml_document.load_string(document_xml.c_str()));
    const auto body = xml_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});
    CHECK_EQ(count_named_descendants(body, "w:fldSimple"), 8U);
    CHECK_NE(document_xml.find("TOC \\o &quot;1-2&quot; \\h \\z \\u"),
             std::string::npos);
    CHECK_NE(document_xml.find("REF target_bookmark \\h \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("PAGEREF target_bookmark \\h \\p \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("STYLEREF &quot;Heading 1&quot; \\n \\p \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("DOCPROPERTY Title \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("DATE \\@ &quot;yyyy-MM-dd&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("HYPERLINK &quot;https://example.com/report&quot; "
                               "\\l &quot;target_heading&quot; "
                               "\\o &quot;Open target heading&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("SEQ Table \\* ARABIC \\r 1"),
             std::string::npos);
    CHECK_EQ(document_xml.find("SEQ Figure"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("template part generic fields validate options and indexes") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_generic_fields_errors.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    CHECK_FALSE(body_template.append_field(""));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "field instruction must not be empty");

    auto toc_options = featherdoc::table_of_contents_field_options{};
    toc_options.min_outline_level = 3U;
    toc_options.max_outline_level = 2U;
    CHECK_FALSE(body_template.append_table_of_contents_field(toc_options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("TOC outline levels"), std::string::npos);

    CHECK_FALSE(body_template.append_reference_field("bad name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("REF bookmark name"), std::string::npos);

    CHECK_FALSE(body_template.append_sequence_field("bad name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("SEQ identifier"), std::string::npos);

    CHECK_FALSE(body_template.append_page_reference_field("bad name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("PAGEREF bookmark name"),
             std::string::npos);

    CHECK_FALSE(body_template.append_style_reference_field("bad\"name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("STYLEREF style name"),
             std::string::npos);

    CHECK_FALSE(body_template.append_document_property_field("bad\\name"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("DOCPROPERTY property name"),
             std::string::npos);

    auto bad_date_options = featherdoc::date_field_options{};
    bad_date_options.format = "bad\\format";
    CHECK_FALSE(body_template.append_date_field(bad_date_options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("DATE format"), std::string::npos);

    CHECK_FALSE(body_template.append_hyperlink_field("bad\\target"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("HYPERLINK target"),
             std::string::npos);

    auto bad_hyperlink_options = featherdoc::hyperlink_field_options{};
    bad_hyperlink_options.anchor = "bad\\anchor";
    CHECK_FALSE(body_template.append_hyperlink_field("https://example.com",
                                                     bad_hyperlink_options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("HYPERLINK anchor"),
             std::string::npos);

    auto author_state = featherdoc::field_state_options{};
    author_state.dirty = true;
    author_state.locked = true;
    CHECK(body_template.append_field(" AUTHOR ", "Author Name", author_state));
    auto fields = body_template.list_fields();
    REQUIRE(fields.size() == 1U);
    CHECK_EQ(fields.front().kind, featherdoc::field_kind::custom);
    CHECK(fields.front().dirty);
    CHECK(fields.front().locked);
    CHECK_FALSE(body_template.replace_field(9U, " PAGE ", "1"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "field index is out of range");

    fs::remove(target);
}


TEST_CASE("template parts can append and inspect complex nested fields") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "template_part_complex_nested_fields.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto outer_options = featherdoc::complex_field_options{};
    outer_options.state.dirty = true;
    outer_options.state.locked = true;
    CHECK(body_template.append_complex_field(
        " IF 1 = 1 \"Yes\" \"No\" ", "Yes", outer_options));

    CHECK(body_template.append_complex_field(
        {featherdoc::complex_field_text_fragment(" IF "),
         featherdoc::complex_field_nested_fragment(
             " MERGEFIELD CustomerName ", "Ada"),
         featherdoc::complex_field_text_fragment(
             " = \"Ada\" \"Matched\" \"Other\" ")},
        "Matched"));

    auto fields = body_template.list_fields();
    REQUIRE(fields.size() == 3U);
    CHECK(fields[0].complex);
    CHECK_EQ(fields[0].depth, 0U);
    CHECK_EQ(fields[0].instruction, " IF 1 = 1 \"Yes\" \"No\" ");
    CHECK_EQ(fields[0].result_text, "Yes");
    CHECK(fields[0].dirty);
    CHECK(fields[0].locked);

    CHECK(fields[1].complex);
    CHECK_EQ(fields[1].depth, 0U);
    CHECK_EQ(fields[1].instruction, " IF  = \"Ada\" \"Matched\" \"Other\" ");
    CHECK_EQ(fields[1].result_text, "Matched");

    CHECK(fields[2].complex);
    CHECK_EQ(fields[2].depth, 1U);
    CHECK_EQ(fields[2].instruction, " MERGEFIELD CustomerName ");
    CHECK_EQ(fields[2].result_text, "Ada");

    CHECK_FALSE(body_template.replace_field(0U, " AUTHOR ", "Ada"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "field index refers to a complex field");

    CHECK_FALSE(doc.save());

    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:fldCharType=\"begin\""), 3U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:fldCharType=\"separate\""), 3U);
    CHECK_EQ(count_substring_occurrences(document_xml, "w:fldCharType=\"end\""), 3U);
    CHECK_NE(document_xml.find("<w:instrText xml:space=\"preserve\"> IF "),
             std::string::npos);
    CHECK_NE(document_xml.find("MERGEFIELD CustomerName"), std::string::npos);
    CHECK_NE(document_xml.find("Matched"), std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    auto reopened_fields = reopened.body_template().list_fields();
    REQUIRE(reopened_fields.size() == 3U);
    CHECK(reopened_fields[1].complex);
    CHECK_EQ(reopened_fields[2].depth, 1U);

    fs::remove(target);
}

TEST_CASE("template parts can append captions and index fields") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "template_part_caption_index_fields.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    auto body_template = doc.body_template();
    REQUIRE(static_cast<bool>(body_template));

    auto caption_options = featherdoc::caption_field_options{};
    caption_options.restart_value = 1U;
    caption_options.state.dirty = true;
    CHECK(body_template.append_caption("Figure", "Architecture overview",
                                       caption_options, "1"));

    auto index_entry_options = featherdoc::index_entry_field_options{};
    index_entry_options.subentry = "API";
    index_entry_options.bookmark_name = "target_bookmark";
    index_entry_options.cross_reference = "See API";
    index_entry_options.bold_page_number = true;
    index_entry_options.italic_page_number = true;
    index_entry_options.state.locked = true;
    CHECK(body_template.append_index_entry_field("FeatherDoc",
                                                index_entry_options));

    auto index_options = featherdoc::index_field_options{};
    index_options.columns = 2U;
    CHECK(body_template.append_index_field(index_options, "Index placeholder"));

    auto fields = body_template.list_fields();
    REQUIRE(fields.size() == 3U);
    CHECK_EQ(fields[0].kind, featherdoc::field_kind::sequence);
    CHECK_EQ(fields[0].instruction, " SEQ Figure \\* ARABIC \\r 1 \\* MERGEFORMAT ");
    CHECK_EQ(fields[0].result_text, "1");
    CHECK(fields[0].dirty);
    CHECK_FALSE(fields[0].locked);
    CHECK_EQ(fields[1].kind, featherdoc::field_kind::index_entry);
    CHECK_EQ(fields[1].instruction,
             " XE \"FeatherDoc:API\" \\r target_bookmark \\t \"See API\" \\b \\i ");
    CHECK(fields[1].result_text.empty());
    CHECK_FALSE(fields[1].dirty);
    CHECK(fields[1].locked);
    CHECK_EQ(fields[2].kind, featherdoc::field_kind::index);
    CHECK_EQ(fields[2].instruction, " INDEX \\c \"2\" \\* MERGEFORMAT ");
    CHECK_EQ(fields[2].result_text, "Index placeholder");

    CHECK_FALSE(body_template.append_caption("bad label", "Invalid"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("SEQ identifier"), std::string::npos);

    auto bad_index_options = featherdoc::index_field_options{};
    bad_index_options.columns = 0U;
    CHECK_FALSE(body_template.append_index_field(bad_index_options));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("INDEX columns"), std::string::npos);

    CHECK_FALSE(body_template.append_index_entry_field("bad\\entry"));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("XE entry text"), std::string::npos);

    CHECK_FALSE(doc.save());

    const auto document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(document_xml.find("Figure"), std::string::npos);
    CHECK_NE(document_xml.find("Architecture overview"), std::string::npos);
    CHECK_NE(document_xml.find("SEQ Figure \\* ARABIC \\r 1 \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("XE &quot;FeatherDoc:API&quot; \\r target_bookmark "
                               "\\t &quot;See API&quot; \\b \\i"),
             std::string::npos);
    CHECK_NE(document_xml.find("INDEX \\c &quot;2&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    fs::remove(target);
}
