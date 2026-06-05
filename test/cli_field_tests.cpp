#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_style_test_support.hpp"

namespace {
TEST_CASE("cli append page number field updates an existing section header") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_page_number_field_source.docx";
    const fs::path updated =
        working_directory / "cli_append_page_number_field_updated.docx";
    const fs::path output =
        working_directory / "cli_append_page_number_field_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-page-number-field", source.string(), "--part",
                      "section-header", "--section", "1", "--output",
                      updated.string(), "--json"},
                     output),
             0);
    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-page-number-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":1"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"field\":\"page_number\""), std::string::npos);

    featherdoc::Document updated_doc(updated);
    REQUIRE_FALSE(updated_doc.open());
    const auto header_entry_name =
        std::string(updated_doc.section_header_template(1).entry_name());
    const auto header_xml = read_docx_entry(updated, header_entry_name.c_str());
    CHECK_NE(header_xml.find("w:fldSimple"), std::string::npos);
    CHECK_NE(header_xml.find("w:instr=\" PAGE \""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append total pages field materializes a missing section footer") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_total_pages_field_source.docx";
    const fs::path updated =
        working_directory / "cli_append_total_pages_field_updated.docx";
    const fs::path output =
        working_directory / "cli_append_total_pages_field_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"append-total-pages-field", source.string(), "--part",
                      "section-footer", "--section", "0", "--output",
                      updated.string(), "--json"},
                     output),
             0);
    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-total-pages-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-footer\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"field\":\"total_pages\""), std::string::npos);
    CHECK_NE(json.find("\"footers\":1"), std::string::npos);

    featherdoc::Document updated_doc(updated);
    REQUIRE_FALSE(updated_doc.open());
    CHECK_EQ(updated_doc.footer_count(), 1U);
    const auto footer_entry_name =
        std::string(updated_doc.section_footer_template(0).entry_name());
    const auto footer_xml = read_docx_entry(updated, footer_entry_name.c_str());
    CHECK_NE(footer_xml.find("w:fldSimple"), std::string::npos);
    CHECK_NE(footer_xml.find("w:instr=\" NUMPAGES \""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append table of contents field updates body") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_toc_field_source.docx";
    const fs::path updated =
        working_directory / "cli_append_toc_field_updated.docx";
    const fs::path output =
        working_directory / "cli_append_toc_field_output.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-table-of-contents-field",
                      source.string(),
                      "--part",
                      "body",
                      "--min-outline-level",
                      "1",
                      "--max-outline-level",
                      "2",
                      "--no-hyperlinks",
                      "--show-page-numbers-in-web-layout",
                      "--no-outline-levels",
                      "--result-text",
                      "TOC placeholder",
                      "--dirty",
                      "--locked",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-table-of-contents-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"table_of_contents\""),
             std::string::npos);
    CHECK_NE(json.find("\"min_outline_level\":1"), std::string::npos);
    CHECK_NE(json.find("\"max_outline_level\":2"), std::string::npos);
    CHECK_NE(json.find("\"hyperlinks\":false"), std::string::npos);
    CHECK_NE(json.find("\"hide_page_numbers_in_web_layout\":false"),
             std::string::npos);
    CHECK_NE(json.find("\"use_outline_levels\":false"), std::string::npos);
    CHECK_NE(json.find("\"result_text\":\"TOC placeholder\""),
             std::string::npos);

    const auto document_xml = read_docx_entry(updated, "word/document.xml");
    CHECK_NE(document_xml.find("TOC \\o &quot;1-2&quot;"), std::string::npos);
    CHECK_EQ(document_xml.find("TOC \\o &quot;1-2&quot; \\h"),
             std::string::npos);
    CHECK_EQ(document_xml.find("TOC \\o &quot;1-2&quot; \\z"),
             std::string::npos);
    CHECK_EQ(document_xml.find("TOC \\o &quot;1-2&quot; \\u"),
             std::string::npos);
    CHECK_NE(document_xml.find("TOC placeholder"), std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli append typed reference fields updates body") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_typed_reference_fields_source.docx";
    const fs::path page_ref_updated =
        working_directory / "cli_append_typed_reference_fields_pageref.docx";
    const fs::path style_ref_updated =
        working_directory / "cli_append_typed_reference_fields_styleref.docx";
    const fs::path property_updated =
        working_directory / "cli_append_typed_reference_fields_property.docx";
    const fs::path date_updated =
        working_directory / "cli_append_typed_reference_fields_date.docx";
    const fs::path hyperlink_updated =
        working_directory / "cli_append_typed_reference_fields_hyperlink.docx";
    const fs::path output =
        working_directory / "cli_append_typed_reference_fields_output.json";

    remove_if_exists(source);
    remove_if_exists(page_ref_updated);
    remove_if_exists(style_ref_updated);
    remove_if_exists(property_updated);
    remove_if_exists(date_updated);
    remove_if_exists(hyperlink_updated);
    remove_if_exists(output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-page-reference-field", source.string(),
                      "target_bookmark", "--part", "body", "--relative-position",
                      "--result-text", "3", "--output", page_ref_updated.string(),
                      "--json"},
                     output),
             0);
    auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-page-reference-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"page_reference\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\"target_bookmark\""),
             std::string::npos);
    CHECK_NE(json.find("\"result_text\":\"3\""), std::string::npos);

    CHECK_EQ(run_cli({"append-style-reference-field", page_ref_updated.string(),
                      "Heading 1", "--part", "body", "--paragraph-number",
                      "--relative-position", "--result-text", "Section heading",
                      "--output", style_ref_updated.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-style-reference-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"style_reference\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\"Heading 1\""), std::string::npos);

    CHECK_EQ(run_cli({"append-document-property-field", style_ref_updated.string(),
                      "Title", "--part", "body", "--result-text", "FeatherDoc",
                      "--output", property_updated.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-document-property-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"document_property\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\"Title\""), std::string::npos);

    CHECK_EQ(run_cli({"append-date-field", property_updated.string(), "--part",
                      "body", "--format", "yyyy-MM-dd", "--result-text",
                      "2026-05-01", "--dirty", "--output",
                      date_updated.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-date-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"date\""), std::string::npos);
    CHECK_NE(json.find("\"result_text\":\"2026-05-01\""),
             std::string::npos);

    CHECK_EQ(run_cli({"append-hyperlink-field", date_updated.string(),
                      "https://example.com/report", "--part", "body",
                      "--anchor", "target_bookmark", "--tooltip",
                      "Open target heading", "--result-text", "Open report",
                      "--locked", "--output", hyperlink_updated.string(),
                      "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-hyperlink-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"hyperlink\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\"https://example.com/report\""),
             std::string::npos);

    const auto document_xml = read_docx_entry(hyperlink_updated, "word/document.xml");
    CHECK_NE(document_xml.find("PAGEREF target_bookmark \\h \\p \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("STYLEREF &quot;Heading 1&quot; \\n \\p \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("DOCPROPERTY Title \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("DATE \\@ &quot;yyyy-MM-dd&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("HYPERLINK &quot;https://example.com/report&quot; "
                               "\\l &quot;target_bookmark&quot; "
                               "\\o &quot;Open target heading&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(page_ref_updated);
    remove_if_exists(style_ref_updated);
    remove_if_exists(property_updated);
    remove_if_exists(date_updated);
    remove_if_exists(hyperlink_updated);
    remove_if_exists(output);
}

TEST_CASE("cli append generic reference sequence and replace fields updates body") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_generic_reference_sequence_fields_source.docx";
    const fs::path generic_updated =
        working_directory / "cli_append_generic_reference_sequence_fields_generic.docx";
    const fs::path reference_updated =
        working_directory / "cli_append_generic_reference_sequence_fields_ref.docx";
    const fs::path sequence_updated =
        working_directory / "cli_append_generic_reference_sequence_fields_seq.docx";
    const fs::path replaced =
        working_directory / "cli_append_generic_reference_sequence_fields_replaced.docx";
    const fs::path output =
        working_directory / "cli_append_generic_reference_sequence_fields_output.json";

    remove_if_exists(source);
    remove_if_exists(generic_updated);
    remove_if_exists(reference_updated);
    remove_if_exists(sequence_updated);
    remove_if_exists(replaced);
    remove_if_exists(output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-field", source.string(), " AUTHOR ", "--part",
                      "body", "--result-text", "Ada Lovelace", "--dirty",
                      "--locked", "--output", generic_updated.string(), "--json"},
                     output),
             0);
    auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-field\""), std::string::npos);
    CHECK_NE(json.find("\"field\":\"field\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\" AUTHOR \""),
             std::string::npos);
    CHECK_NE(json.find("\"result_text\":\"Ada Lovelace\""),
             std::string::npos);

    CHECK_EQ(run_cli({"append-reference-field", generic_updated.string(),
                      "target_bookmark", "--part", "body", "--no-hyperlink",
                      "--no-preserve-formatting", "--result-text",
                      "Referenced heading", "--output", reference_updated.string(),
                      "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-reference-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"reference\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\"target_bookmark\""),
             std::string::npos);

    CHECK_EQ(run_cli({"append-sequence-field", reference_updated.string(),
                      "Figure", "--part", "body", "--number-format", "ROMAN",
                      "--restart", "4", "--result-text", "IV",
                      "--no-preserve-formatting", "--output",
                      sequence_updated.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-sequence-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"sequence\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\"Figure\""), std::string::npos);
    CHECK_NE(json.find("\"result_text\":\"IV\""), std::string::npos);
    CHECK_NE(json.find("\"restart\":4"), std::string::npos);

    CHECK_EQ(run_cli({"replace-field", sequence_updated.string(), "2",
                      " SEQ Table \\* ARABIC \\r 1 ", "--part", "body",
                      "--result-text", "1", "--output", replaced.string(),
                      "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-field\""), std::string::npos);
    CHECK_NE(json.find("\"field_index\":2"), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\" SEQ Table \\\\* ARABIC \\\\r 1 \""),
             std::string::npos);

    const auto document_xml = read_docx_entry(replaced, "word/document.xml");
    CHECK_NE(document_xml.find("AUTHOR"), std::string::npos);
    CHECK_NE(document_xml.find("Ada Lovelace"), std::string::npos);
    CHECK_NE(document_xml.find("REF target_bookmark"), std::string::npos);
    CHECK_EQ(document_xml.find("REF target_bookmark \\h"), std::string::npos);
    CHECK_EQ(document_xml.find("REF target_bookmark \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("SEQ Table \\* ARABIC \\r 1"),
             std::string::npos);
    CHECK_EQ(document_xml.find("SEQ Figure"), std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(generic_updated);
    remove_if_exists(reference_updated);
    remove_if_exists(sequence_updated);
    remove_if_exists(replaced);
    remove_if_exists(output);
}


TEST_CASE("cli append caption and index fields updates body") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_caption_index_fields_source.docx";
    const fs::path caption_updated =
        working_directory / "cli_append_caption_index_fields_caption.docx";
    const fs::path entry_updated =
        working_directory / "cli_append_caption_index_fields_entry.docx";
    const fs::path index_updated =
        working_directory / "cli_append_caption_index_fields_index.docx";
    const fs::path output =
        working_directory / "cli_append_caption_index_fields_output.json";

    remove_if_exists(source);
    remove_if_exists(caption_updated);
    remove_if_exists(entry_updated);
    remove_if_exists(index_updated);
    remove_if_exists(output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-caption", source.string(), "Figure", "--part",
                      "body", "--text", "Architecture overview",
                      "--number-result", "7", "--restart", "7",
                      "--output", caption_updated.string(), "--json"},
                     output),
             0);
    auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-caption\""), std::string::npos);
    CHECK_NE(json.find("\"field\":\"caption\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\"Figure\""), std::string::npos);
    CHECK_NE(json.find("\"caption_text\":\"Architecture overview\""),
             std::string::npos);
    CHECK_NE(json.find("\"number_result_text\":\"7\""), std::string::npos);
    CHECK_NE(json.find("\"restart\":7"), std::string::npos);

    CHECK_EQ(run_cli({"append-index-entry-field", caption_updated.string(),
                      "FeatherDoc", "--part", "body", "--subentry", "API",
                      "--bookmark", "target_bookmark", "--cross-reference",
                      "See API", "--bold-page-number", "--italic-page-number",
                      "--locked", "--output", entry_updated.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-index-entry-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"index_entry\""), std::string::npos);
    CHECK_NE(json.find("\"field_argument\":\"FeatherDoc\""),
             std::string::npos);
    CHECK_NE(json.find("\"subentry\":\"API\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":\"target_bookmark\""), std::string::npos);
    CHECK_NE(json.find("\"cross_reference\":\"See API\""), std::string::npos);

    CHECK_EQ(run_cli({"append-index-field", entry_updated.string(), "--part",
                      "body", "--columns", "2", "--result-text",
                      "Index placeholder", "--dirty", "--output",
                      index_updated.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-index-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"index\""), std::string::npos);
    CHECK_NE(json.find("\"columns\":2"), std::string::npos);
    CHECK_NE(json.find("\"result_text\":\"Index placeholder\""),
             std::string::npos);

    const auto document_xml = read_docx_entry(index_updated, "word/document.xml");
    CHECK_NE(document_xml.find("Figure"), std::string::npos);
    CHECK_NE(document_xml.find("Architecture overview"), std::string::npos);
    CHECK_NE(document_xml.find("SEQ Figure \\* ARABIC \\r 7 \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("XE &quot;FeatherDoc:API&quot; \\r target_bookmark "
                               "\\t &quot;See API&quot; \\b \\i"),
             std::string::npos);
    CHECK_NE(document_xml.find("INDEX \\c &quot;2&quot; \\* MERGEFORMAT"),
             std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(caption_updated);
    remove_if_exists(entry_updated);
    remove_if_exists(index_updated);
    remove_if_exists(output);
}

TEST_CASE("cli append complex nested field updates body") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_complex_field_source.docx";
    const fs::path simple_updated =
        working_directory / "cli_append_complex_field_simple.docx";
    const fs::path nested_updated =
        working_directory / "cli_append_complex_field_nested.docx";
    const fs::path output =
        working_directory / "cli_append_complex_field_output.json";

    remove_if_exists(source);
    remove_if_exists(simple_updated);
    remove_if_exists(nested_updated);
    remove_if_exists(output);
    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-complex-field", source.string(), "--part",
                      "body", "--instruction", " IF 1 = 1 \\\"Yes\\\" \\\"No\\\" ",
                      "--result-text", "Yes", "--dirty", "--locked",
                      "--output", simple_updated.string(), "--json"},
                     output),
             0);
    auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"append-complex-field\""),
             std::string::npos);
    CHECK_NE(json.find("\"field\":\"complex\""), std::string::npos);
    CHECK_NE(json.find("\"instruction\":"), std::string::npos);
    CHECK_NE(json.find("\"result_text\":\"Yes\""), std::string::npos);

    CHECK_EQ(run_cli({"append-complex-field", simple_updated.string(), "--part",
                      "body", "--instruction-before", " IF ",
                      "--nested-instruction", " MERGEFIELD CustomerName ",
                      "--nested-result-text", "Ada", "--instruction-after",
                      " = \\\"Ada\\\" \\\"Matched\\\" \\\"Other\\\" ",
                      "--result-text", "Matched", "--output",
                      nested_updated.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"nested_instruction\":\" MERGEFIELD CustomerName \""),
             std::string::npos);
    CHECK_NE(json.find("\"nested_result_text\":\"Ada\""), std::string::npos);

    const auto document_xml = read_docx_entry(nested_updated, "word/document.xml");
    const auto count_occurrences = [](const std::string &text,
                                      std::string_view needle) {
        std::size_t count = 0U;
        std::size_t position = 0U;
        while ((position = text.find(needle, position)) != std::string::npos) {
            ++count;
            position += needle.size();
        }
        return count;
    };
    CHECK_EQ(count_occurrences(document_xml, "w:fldCharType=\"begin\""), 3U);
    CHECK_EQ(count_occurrences(document_xml, "w:fldCharType=\"separate\""), 3U);
    CHECK_EQ(count_occurrences(document_xml, "w:fldCharType=\"end\""), 3U);
    CHECK_NE(document_xml.find("IF 1 = 1"), std::string::npos);
    CHECK_NE(document_xml.find("MERGEFIELD CustomerName"), std::string::npos);
    CHECK_NE(document_xml.find("Matched"), std::string::npos);
    CHECK_NE(document_xml.find("w:dirty=\"true\""), std::string::npos);
    CHECK_NE(document_xml.find("w:fldLock=\"true\""), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(simple_updated);
    remove_if_exists(nested_updated);
    remove_if_exists(output);
}

TEST_CASE("cli can inspect and toggle update fields on open") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_update_fields_on_open_source.docx";
    const fs::path enabled_docx =
        working_directory / "cli_update_fields_on_open_enabled.docx";
    const fs::path disabled_docx =
        working_directory / "cli_update_fields_on_open_disabled.docx";
    const fs::path output =
        working_directory / "cli_update_fields_on_open_output.json";

    remove_if_exists(source);
    remove_if_exists(enabled_docx);
    remove_if_exists(disabled_docx);
    remove_if_exists(output);
    create_cli_fixture(source);

    CHECK_EQ(run_cli({"inspect-update-fields-on-open", source.string(), "--json"},
                     output),
             0);
    auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"inspect-update-fields-on-open\""),
             std::string::npos);
    CHECK_NE(json.find("\"update_fields_on_open\":false"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-update-fields-on-open", source.string(), "--enable",
                      "--output", enabled_docx.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"set-update-fields-on-open\""),
             std::string::npos);
    CHECK_NE(json.find("\"update_fields_on_open\":true"), std::string::npos);

    auto settings_xml = read_docx_entry(enabled_docx, "word/settings.xml");
    CHECK_NE(settings_xml.find("<w:updateFields"), std::string::npos);
    CHECK_NE(settings_xml.find("w:val=\"1\""), std::string::npos);

    CHECK_EQ(run_cli({"inspect-update-fields-on-open", enabled_docx.string(),
                      "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"update_fields_on_open\":true"), std::string::npos);

    CHECK_EQ(run_cli({"set-update-fields-on-open", enabled_docx.string(),
                      "--disable", "--output", disabled_docx.string(), "--json"},
                     output),
             0);
    json = read_text_file(output);
    CHECK_NE(json.find("\"update_fields_on_open\":false"),
             std::string::npos);

    settings_xml = read_docx_entry(disabled_docx, "word/settings.xml");
    CHECK_EQ(settings_xml.find("<w:updateFields"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(enabled_docx);
    remove_if_exists(disabled_docx);
    remove_if_exists(output);
}

TEST_CASE("cli append page number field reports json parse errors") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_append_page_number_field_parse_source.docx";
    const fs::path output =
        working_directory / "cli_append_page_number_field_parse_output.json";

    remove_if_exists(source);
    remove_if_exists(output);

    CHECK_EQ(run_cli({"append-page-reference-field", source.string(), "--part",
                      "body", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-page-reference-field\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"append-page-reference-field "
            "expects <bookmark-name>\"}\n"});

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"append-hyperlink-field", source.string(), "--part",
                      "body", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-hyperlink-field\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"append-hyperlink-field "
            "expects <target>\"}\n"});


    CHECK_EQ(run_cli({"append-caption", source.string(), "--part", "body",
                      "--text", "Caption", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-caption\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"append-caption expects "
            "<label>\"}\n"});

    CHECK_EQ(run_cli({"append-caption", source.string(), "Figure", "--part",
                      "body", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-caption\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"append-caption requires "
            "--text <caption-text>\"}\n"});

    CHECK_EQ(run_cli({"append-index-entry-field", source.string(), "--part",
                      "body", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-index-entry-field\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"append-index-entry-field "
            "expects <entry-text>\"}\n"});

    CHECK_EQ(run_cli({"append-page-number-field", source.string(), "--part",
                      "section-header", "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"append-page-number-field\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"section-header/section-footer "
            "mutation requires --section <index>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(output);
}

} // namespace
