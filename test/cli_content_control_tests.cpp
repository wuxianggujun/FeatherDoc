#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_bookmark_content_test_support.hpp"

namespace {

TEST_CASE("cli replace-content-control-text rewrites selected content controls") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_content_control_text_source.docx";
    const fs::path updated =
        working_directory / "cli_content_control_text_updated.docx";
    const fs::path replace_output =
        working_directory / "cli_content_control_text_replace.json";
    const fs::path inspect_output =
        working_directory / "cli_content_control_text_inspect.json";
    const fs::path header_updated =
        working_directory / "cli_content_control_text_header.docx";
    const fs::path header_output =
        working_directory / "cli_content_control_text_header.json";
    const fs::path header_inspect_output =
        working_directory / "cli_content_control_text_header_inspect.json";
    const fs::path parse_output =
        working_directory / "cli_content_control_text_parse.json";
    const fs::path missing_output =
        working_directory / "cli_content_control_text_missing.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(replace_output);
    remove_if_exists(inspect_output);
    remove_if_exists(header_updated);
    remove_if_exists(header_output);
    remove_if_exists(header_inspect_output);
    remove_if_exists(parse_output);
    remove_if_exists(missing_output);

    create_cli_content_controls_fixture(source);

    CHECK_EQ(run_cli({"replace-content-control-text",
                      source.string(),
                      "--tag",
                      "order_no",
                      "--text",
                      "INV-002",
                      "--output",
                      updated.string(),
                      "--json"},
                     replace_output),
             0);
    const auto replace_json = read_text_file(replace_output);
    CHECK_NE(replace_json.find(R"("command":"replace-content-control-text")"),
             std::string::npos);
    CHECK_NE(replace_json.find(R"("selector":{"kind":"tag","value":"order_no"})"),
             std::string::npos);
    CHECK_NE(replace_json.find(R"("replaced":1)"), std::string::npos);
    CHECK_NE(replace_json.find(R"("text":"INV-002")"), std::string::npos);

    const auto document_xml = read_docx_entry(updated, "word/document.xml");
    CHECK_NE(document_xml.find("INV-002"), std::string::npos);
    CHECK_EQ(document_xml.find("INV-001"), std::string::npos);
    CHECK_EQ(document_xml.find("w:showingPlcHdr"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-content-controls",
                      updated.string(),
                      "--tag",
                      "order_no",
                      "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("count":1)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"INV-002")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("showing_placeholder":false)"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-content-control-text",
                      updated.string(),
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--alias",
                      "Header Review",
                      "--text",
                      "Approved",
                      "--output",
                      header_updated.string(),
                      "--json"},
                     header_output),
             0);
    const auto header_json = read_text_file(header_output);
    CHECK_NE(header_json.find(R"("part":"header")"), std::string::npos);
    CHECK_NE(header_json.find(R"("part_index":0)"), std::string::npos);
    CHECK_NE(header_json.find(
                 R"("selector":{"kind":"alias","value":"Header Review"})"),
             std::string::npos);
    CHECK_NE(header_json.find(R"("replaced":1)"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-content-controls",
                      header_updated.string(),
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--alias",
                      "Header Review",
                      "--json"},
                     header_inspect_output),
             0);
    const auto header_inspect_json = read_text_file(header_inspect_output);
    CHECK_NE(header_inspect_json.find(R"("count":1)"), std::string::npos);
    CHECK_NE(header_inspect_json.find(R"("text":"Approved")"),
             std::string::npos);
    CHECK_NE(read_docx_entry(header_updated, "word/header1.xml").find("Approved"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-content-control-text",
                      source.string(),
                      "--tag",
                      "order_no",
                      "--alias",
                      "Order Number",
                      "--text",
                      "INV-003",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find("--tag cannot be combined with --alias"),
             std::string::npos);

    CHECK_EQ(run_cli({"replace-content-control-text",
                      source.string(),
                      "--tag",
                      "missing",
                      "--text",
                      "noop",
                      "--json"},
                     missing_output),
             1);
    const auto missing_json = read_text_file(missing_output);
    CHECK_NE(missing_json.find(R"("stage":"mutate")"), std::string::npos);
    CHECK_NE(missing_json.find("matching content control not found"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(replace_output);
    remove_if_exists(inspect_output);
    remove_if_exists(header_updated);
    remove_if_exists(header_output);
    remove_if_exists(header_inspect_output);
    remove_if_exists(parse_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli replace-content-control-paragraphs replaces tagged controls") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_content_control_paragraphs_source.docx";
    const fs::path updated =
        working_directory / "cli_content_control_paragraphs_updated.docx";
    const fs::path output =
        working_directory / "cli_content_control_paragraphs.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_content_controls_fixture(source);

    CHECK_EQ(run_cli({"replace-content-control-paragraphs",
                      source.string(),
                      "--tag",
                      "customer_name",
                      "--paragraph",
                      "First replacement paragraph",
                      "--paragraph",
                      "Second replacement paragraph",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find(R"("command":"replace-content-control-paragraphs")"),
             std::string::npos);
    CHECK_NE(json.find(R"("selector":{"kind":"tag","value":"customer_name"})"),
             std::string::npos);
    CHECK_NE(json.find(R"("replaced":1)"), std::string::npos);
    CHECK_NE(json.find(R"("paragraph_count":2)"), std::string::npos);

    const auto document_xml = read_docx_entry(updated, "word/document.xml");
    CHECK_NE(document_xml.find("First replacement paragraph"),
             std::string::npos);
    CHECK_NE(document_xml.find("Second replacement paragraph"),
             std::string::npos);
    CHECK_EQ(document_xml.find("Ada Lovelace"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-content-control-table-rows expands tagged row controls") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_content_control_rows_source.docx";
    const fs::path updated =
        working_directory / "cli_content_control_rows_updated.docx";
    const fs::path output = working_directory / "cli_content_control_rows.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_content_controls_fixture(source);

    CHECK_EQ(run_cli({"replace-content-control-table-rows",
                      source.string(),
                      "--tag",
                      "line_items",
                      "--row",
                      "SKU-A",
                      "--row",
                      "SKU-B",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find(R"("command":"replace-content-control-table-rows")"),
             std::string::npos);
    CHECK_NE(json.find(R"("row_count":2)"), std::string::npos);
    CHECK_NE(json.find(R"("rows":[["SKU-A"],["SKU-B"]])"),
             std::string::npos);

    const auto document_xml = read_docx_entry(updated, "word/document.xml");
    CHECK_NE(document_xml.find("SKU-A"), std::string::npos);
    CHECK_NE(document_xml.find("SKU-B"), std::string::npos);
    CHECK_EQ(document_xml.find("SKU-1"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-content-control-image replaces tagged controls") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_content_control_image_source.docx";
    const fs::path updated =
        working_directory / "cli_content_control_image_updated.docx";
    const fs::path image_path = working_directory / "cli_content_control_image.png";
    const fs::path output = working_directory / "cli_content_control_image.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(image_path);
    remove_if_exists(output);

    create_cli_content_controls_fixture(source);
    write_binary_file(image_path, tiny_png_data());

    CHECK_EQ(run_cli({"replace-content-control-image",
                      source.string(),
                      image_path.string(),
                      "--tag",
                      "customer_name",
                      "--width",
                      "24",
                      "--height",
                      "24",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find(R"("command":"replace-content-control-image")"),
             std::string::npos);
    CHECK_NE(json.find(R"("selector":{"kind":"tag","value":"customer_name"})"),
             std::string::npos);
    CHECK_NE(json.find(R"("replaced":1)"), std::string::npos);
    CHECK_NE(json.find(R"("content_type":"image/png")"), std::string::npos);

    const auto document_xml = read_docx_entry(updated, "word/document.xml");
    CHECK_NE(document_xml.find("w:drawing"), std::string::npos);
    CHECK_EQ(document_xml.find("Ada Lovelace"), std::string::npos);
    CHECK_FALSE(read_docx_entry(updated, "word/media/image1.png").empty());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(image_path);
    remove_if_exists(output);
}

} // namespace
