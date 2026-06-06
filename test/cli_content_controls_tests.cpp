#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

#include <zip.h>

namespace {
void create_cli_content_controls_fixture(const fs::path &path) {
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
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Customer Name"/>
        <w:tag w:val="customer_name"/>
        <w:id w:val="42"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Ada Lovelace</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Order: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Order Number"/>
          <w:tag w:val="order_no"/>
          <w:id w:val="43"/>
          <w:showingPlcHdr/>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:tbl>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Line Items"/>
          <w:tag w:val="line_items"/>
          <w:id w:val="44"/>
        </w:sdtPr>
        <w:sdtContent>
          <w:tr>
            <w:tc><w:p><w:r><w:t>SKU-1</w:t></w:r></w:p></w:tc>
          </w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
    <w:sectPr><w:headerReference w:type="default" r:id="rId2"/></w:sectPr>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
</Relationships>
)";
    constexpr auto header_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:sdt>
    <w:sdtPr>
      <w:alias w:val="Header Review"/>
      <w:tag w:val="header_review"/>
      <w:id w:val="45"/>
    </w:sdtPr>
    <w:sdtContent>
      <w:p><w:r><w:t>Reviewed by QA</w:t></w:r></w:p>
    </w:sdtContent>
  </w:sdt>
</w:hdr>
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
    zip_close(archive);
}

void create_cli_content_control_forms_fixture(const fs::path &path) {
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
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:w14="http://schemas.microsoft.com/office/word/2010/wordml">
  <w:body>
    <w:p>
      <w:r><w:t>Approved: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Approved"/>
          <w:tag w:val="approved"/>
          <w:id w:val="101"/>
          <w:lock w:val="sdtContentLocked"/>
          <w14:checkbox>
            <w14:checked w14:val="1"/>
          </w14:checkbox>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>☒</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:p>
      <w:r><w:t>Status: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Status"/>
          <w:tag w:val="status"/>
          <w:id w:val="102"/>
          <w:dropDownList>
            <w:listItem w:displayText="Draft" w:value="draft"/>
            <w:listItem w:displayText="Approved" w:value="approved"/>
          </w:dropDownList>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>Approved</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Due Date"/>
        <w:tag w:val="due_date"/>
        <w:id w:val="103"/>
        <w:dataBinding w:storeItemID="{33333333-3333-3333-3333-333333333333}" w:xpath="/invoice/dueDate" w:prefixMappings="xmlns:fd=&quot;urn:featherdoc&quot;"/>
        <w:date>
          <w:dateFormat w:val="yyyy-MM-dd"/>
          <w:lid w:val="en-US"/>
        </w:date>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>2026-05-01</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
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

TEST_CASE("cli inspect-content-controls lists and filters content controls") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_content_controls_source.docx";
    const fs::path body_output = working_directory / "cli_content_controls_body.json";
    const fs::path tag_output = working_directory / "cli_content_controls_tag.json";
    const fs::path header_output =
        working_directory / "cli_content_controls_header.json";
    const fs::path missing_output =
        working_directory / "cli_content_controls_missing.json";
    const fs::path parse_output =
        working_directory / "cli_content_controls_parse.json";

    remove_if_exists(source);
    remove_if_exists(body_output);
    remove_if_exists(tag_output);
    remove_if_exists(header_output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);

    create_cli_content_controls_fixture(source);

    CHECK_EQ(run_cli({"inspect-content-controls", source.string(), "--json"},
                     body_output),
             0);
    const auto body_json = read_text_file(body_output);
    CHECK_NE(body_json.find(R"("part":"body")"), std::string::npos);
    CHECK_NE(body_json.find(R"("count":3)"), std::string::npos);
    CHECK_NE(body_json.find(R"("tag":"customer_name")"), std::string::npos);
    CHECK_NE(body_json.find(R"("alias":"Customer Name")"), std::string::npos);
    CHECK_NE(body_json.find(R"("kind":"block")"), std::string::npos);
    CHECK_NE(body_json.find(R"("tag":"order_no")"), std::string::npos);
    CHECK_NE(body_json.find(R"("showing_placeholder":true)"),
             std::string::npos);
    CHECK_NE(body_json.find(R"("kind":"table_row")"), std::string::npos);
    CHECK_NE(body_json.find(R"("text":"SKU-1")"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-content-controls",
                      source.string(),
                      "--tag",
                      "order_no",
                      "--json"},
                     tag_output),
             0);
    const auto tag_json = read_text_file(tag_output);
    CHECK_NE(tag_json.find(R"("filters":{"tag":"order_no","alias":null})"),
             std::string::npos);
    CHECK_NE(tag_json.find(R"("count":1)"), std::string::npos);
    CHECK_NE(tag_json.find(R"("tag":"order_no")"), std::string::npos);
    CHECK_EQ(tag_json.find(R"("tag":"customer_name")"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-content-controls",
                      source.string(),
                      "--part",
                      "header",
                      "--index",
                      "0",
                      "--alias",
                      "Header Review",
                      "--json"},
                     header_output),
             0);
    const auto header_json = read_text_file(header_output);
    CHECK_NE(header_json.find(R"("part":"header")"), std::string::npos);
    CHECK_NE(header_json.find(R"("part_index":0)"), std::string::npos);
    CHECK_NE(header_json.find(R"("entry_name":"word/header1.xml")"),
             std::string::npos);
    CHECK_NE(header_json.find(R"("tag":"header_review")"),
             std::string::npos);
    CHECK_NE(header_json.find(R"("text":"Reviewed by QA")"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-content-controls",
                      source.string(),
                      "--tag",
                      "missing",
                      "--json"},
                     missing_output),
             0);
    CHECK_NE(read_text_file(missing_output).find(R"("count":0)"),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-content-controls",
                      source.string(),
                      "--tag",
                      "customer_name",
                      "--tag",
                      "order_no",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find("duplicate --tag option"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(body_output);
    remove_if_exists(tag_output);
    remove_if_exists(header_output);
    remove_if_exists(missing_output);
    remove_if_exists(parse_output);
}

TEST_CASE("cli inspect-content-controls reports form state") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_content_control_forms_source.docx";
    const fs::path json_output =
        working_directory / "cli_content_control_forms.json";
    const fs::path text_output =
        working_directory / "cli_content_control_forms.txt";

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);

    create_cli_content_control_forms_fixture(source);

    CHECK_EQ(run_cli({"inspect-content-controls", source.string(), "--json"},
                     json_output),
             0);
    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("count":3)"), std::string::npos);
    CHECK_NE(json.find(R"("form_kind":"checkbox")"), std::string::npos);
    CHECK_NE(json.find(R"("lock":"sdtContentLocked")"), std::string::npos);
    CHECK_NE(json.find(R"("checked":true)"), std::string::npos);
    CHECK_NE(json.find(R"("form_kind":"drop_down_list")"), std::string::npos);
    CHECK_NE(json.find(R"("selected_list_item":1)"), std::string::npos);
    CHECK_NE(json.find(R"("display_text":"Approved","value":"approved")"),
             std::string::npos);
    CHECK_NE(json.find(R"("form_kind":"date")"), std::string::npos);
    CHECK_NE(json.find(R"("date_format":"yyyy-MM-dd")"), std::string::npos);
    CHECK_NE(json.find(R"("date_locale":"en-US")"), std::string::npos);
    CHECK_NE(json.find(R"("data_binding_store_item_id":"{33333333-3333-3333-3333-333333333333}")"),
             std::string::npos);
    CHECK_NE(json.find(R"("data_binding_xpath":"/invoice/dueDate")"),
             std::string::npos);
    CHECK_NE(json.find("\"data_binding_prefix_mappings\":\"xmlns:fd=\\\"urn:featherdoc\\\"\""),
             std::string::npos);

    CHECK_EQ(run_cli({"inspect-content-controls", source.string()}, text_output),
             0);
    const auto text = read_text_file(text_output);
    CHECK_NE(text.find("form_kind=checkbox"), std::string::npos);
    CHECK_NE(text.find("checked=yes"), std::string::npos);
    CHECK_NE(text.find("list_items=2"), std::string::npos);
    CHECK_NE(text.find("date_format=yyyy-MM-dd"), std::string::npos);
    CHECK_NE(text.find("data_binding_xpath=/invoice/dueDate"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(json_output);
    remove_if_exists(text_output);
}

TEST_CASE("cli set-content-control-form-state mutates form controls") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_content_control_form_state_source.docx";
    const fs::path checkbox_updated =
        working_directory / "cli_content_control_form_state_checkbox.docx";
    const fs::path status_updated =
        working_directory / "cli_content_control_form_state_status.docx";
    const fs::path date_updated =
        working_directory / "cli_content_control_form_state_date.docx";
    const fs::path binding_updated =
        working_directory / "cli_content_control_form_state_binding.docx";
    const fs::path binding_cleared =
        working_directory / "cli_content_control_form_state_binding_cleared.docx";
    const fs::path checkbox_output =
        working_directory / "cli_content_control_form_state_checkbox.json";
    const fs::path status_output =
        working_directory / "cli_content_control_form_state_status.json";
    const fs::path date_output =
        working_directory / "cli_content_control_form_state_date.json";
    const fs::path binding_output =
        working_directory / "cli_content_control_form_state_binding.json";
    const fs::path binding_clear_output =
        working_directory / "cli_content_control_form_state_binding_clear.json";
    const fs::path inspect_output =
        working_directory / "cli_content_control_form_state_inspect.json";
    const fs::path parse_output =
        working_directory / "cli_content_control_form_state_parse.json";
    const fs::path missing_output =
        working_directory / "cli_content_control_form_state_missing.json";

    remove_if_exists(source);
    remove_if_exists(checkbox_updated);
    remove_if_exists(status_updated);
    remove_if_exists(date_updated);
    remove_if_exists(binding_updated);
    remove_if_exists(binding_cleared);
    remove_if_exists(checkbox_output);
    remove_if_exists(status_output);
    remove_if_exists(date_output);
    remove_if_exists(binding_output);
    remove_if_exists(binding_clear_output);
    remove_if_exists(inspect_output);
    remove_if_exists(parse_output);
    remove_if_exists(missing_output);

    create_cli_content_control_forms_fixture(source);

    CHECK_EQ(run_cli({"set-content-control-form-state",
                      source.string(),
                      "--tag",
                      "approved",
                      "--checked",
                      "false",
                      "--clear-lock",
                      "--output",
                      checkbox_updated.string(),
                      "--json"},
                     checkbox_output),
             0);
    const auto checkbox_json = read_text_file(checkbox_output);
    CHECK_NE(checkbox_json.find(R"("command":"set-content-control-form-state")"),
             std::string::npos);
    CHECK_NE(checkbox_json.find(R"("selector":{"kind":"tag","value":"approved"})"),
             std::string::npos);
    CHECK_NE(checkbox_json.find(R"("updated":1)"), std::string::npos);
    CHECK_NE(checkbox_json.find(R"("checked":false)"), std::string::npos);
    CHECK_NE(checkbox_json.find(R"("clear_lock":true)"), std::string::npos);
    auto document_xml = read_docx_entry(checkbox_updated, "word/document.xml");
    CHECK_NE(document_xml.find(R"(w14:checked w14:val="0")"),
             std::string::npos);
    CHECK_EQ(document_xml.find(R"(<w:lock w:val="sdtContentLocked"/>)"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-content-control-form-state",
                      checkbox_updated.string(),
                      "--alias",
                      "Status",
                      "--selected-item",
                      "draft",
                      "--lock",
                      "sdtLocked",
                      "--output",
                      status_updated.string(),
                      "--json"},
                     status_output),
             0);
    const auto status_json = read_text_file(status_output);
    CHECK_NE(status_json.find(R"("selector":{"kind":"alias","value":"Status"})"),
             std::string::npos);
    CHECK_NE(status_json.find(R"("selected_item":"draft")"),
             std::string::npos);
    CHECK_NE(status_json.find(R"("lock":"sdtLocked")"), std::string::npos);
    document_xml = read_docx_entry(status_updated, "word/document.xml");
    CHECK_NE(document_xml.find(R"(<w:t>Draft</w:t>)"), std::string::npos);
    CHECK_NE(document_xml.find("w:lock w:val=\"sdtLocked\""),
             std::string::npos);

    CHECK_EQ(run_cli({"set-content-control-form-state",
                      status_updated.string(),
                      "--tag",
                      "due_date",
                      "--date-text",
                      "2026-06-01",
                      "--date-format",
                      "yyyy/MM/dd",
                      "--date-locale",
                      "zh-CN",
                      "--output",
                      date_updated.string(),
                      "--json"},
                     date_output),
             0);
    const auto date_json = read_text_file(date_output);
    CHECK_NE(date_json.find(R"("date_text":"2026-06-01")"),
             std::string::npos);
    CHECK_NE(date_json.find(R"("date_format":"yyyy/MM/dd")"),
             std::string::npos);
    CHECK_NE(date_json.find(R"("date_locale":"zh-CN")"), std::string::npos);

    CHECK_EQ(run_cli({"set-content-control-form-state",
                      date_updated.string(),
                      "--tag",
                      "due_date",
                      "--data-binding-store-item-id",
                      "{44444444-4444-4444-4444-444444444444}",
                      "--data-binding-xpath",
                      "/invoice/finalDueDate",
                      "--data-binding-prefix-mappings",
                      "xmlns:fd=\"urn:featherdoc\"",
                      "--output",
                      binding_updated.string(),
                      "--json"},
                     binding_output),
             0);
    const auto binding_json = read_text_file(binding_output);
    CHECK_NE(binding_json.find(
                 R"("data_binding_store_item_id":"{44444444-4444-4444-4444-444444444444}")"),
             std::string::npos);
    CHECK_NE(binding_json.find(R"("data_binding_xpath":"/invoice/finalDueDate")"),
             std::string::npos);
    document_xml = read_docx_entry(binding_updated, "word/document.xml");
    CHECK_NE(document_xml.find("w:dataBinding"), std::string::npos);
    CHECK_NE(document_xml.find("/invoice/finalDueDate"), std::string::npos);

    CHECK_EQ(run_cli({"inspect-content-controls", binding_updated.string(), "--json"},
                     inspect_output),
             0);
    const auto inspect_json = read_text_file(inspect_output);
    CHECK_NE(inspect_json.find(R"("checked":false)"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"☐")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("lock":"sdtLocked")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("selected_list_item":0)"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"Draft")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("date_format":"yyyy/MM/dd")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("date_locale":"zh-CN")"), std::string::npos);
    CHECK_NE(inspect_json.find(R"("text":"2026-06-01")"),
             std::string::npos);
    CHECK_NE(inspect_json.find(R"("data_binding_xpath":"/invoice/finalDueDate")"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-content-control-form-state",
                      binding_updated.string(),
                      "--tag",
                      "due_date",
                      "--clear-data-binding",
                      "--output",
                      binding_cleared.string(),
                      "--json"},
                     binding_clear_output),
             0);
    const auto binding_clear_json = read_text_file(binding_clear_output);
    CHECK_NE(binding_clear_json.find(R"("clear_data_binding":true)"),
             std::string::npos);
    document_xml = read_docx_entry(binding_cleared, "word/document.xml");
    CHECK_EQ(document_xml.find("w:dataBinding"), std::string::npos);

    CHECK_EQ(run_cli({"set-content-control-form-state",
                      source.string(),
                      "--tag",
                      "approved",
                      "--json"},
                     parse_output),
             2);
    CHECK_NE(read_text_file(parse_output).find(
                 "set-content-control-form-state expects at least one form-state option"),
             std::string::npos);

    CHECK_EQ(run_cli({"set-content-control-form-state",
                      source.string(),
                      "--tag",
                      "missing",
                      "--checked",
                      "true",
                      "--json"},
                     missing_output),
             1);
    CHECK_NE(read_text_file(missing_output).find(
                 "matching content control not found"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(checkbox_updated);
    remove_if_exists(status_updated);
    remove_if_exists(date_updated);
    remove_if_exists(binding_updated);
    remove_if_exists(binding_cleared);
    remove_if_exists(checkbox_output);
    remove_if_exists(status_output);
    remove_if_exists(date_output);
    remove_if_exists(binding_output);
    remove_if_exists(binding_clear_output);
    remove_if_exists(inspect_output);
    remove_if_exists(parse_output);
    remove_if_exists(missing_output);
}

TEST_CASE("cli sync-content-controls-from-custom-xml updates bound text") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_content_control_custom_xml_source.docx";
    const fs::path updated =
        working_directory / "cli_content_control_custom_xml_synced.docx";
    const fs::path output =
        working_directory / "cli_content_control_custom_xml_sync.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

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
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Due Date"/>
        <w:tag w:val="due_date"/>
        <w:dataBinding w:storeItemID="{66666666-6666-6666-6666-666666666666}" w:xpath="/invoice/dueDate"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>Pending date</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    constexpr auto custom_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<invoice><dueDate>2026-08-20</dueDate></invoice>
)";
    constexpr auto item_props =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ds:datastoreItem ds:itemID="{66666666-6666-6666-6666-666666666666}"
                  xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml">
  <ds:schemaRefs/>
</ds:datastoreItem>
)";
    constexpr auto item_relationships =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps"
                Target="itemProps1.xml"/>
</Relationships>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(source.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "customXml/item1.xml", custom_xml));
    REQUIRE(write_archive_entry(archive, "customXml/itemProps1.xml", item_props));
    REQUIRE(write_archive_entry(archive, "customXml/_rels/item1.xml.rels",
                                item_relationships));
    zip_close(archive);

    CHECK_EQ(run_cli({"sync-content-controls-from-custom-xml",
                      source.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    const auto json = read_text_file(output);
    CHECK_NE(json.find(R"("command":"sync-content-controls-from-custom-xml")"),
             std::string::npos);
    CHECK_NE(json.find(R"("synced_content_controls":1)"), std::string::npos);
    CHECK_NE(json.find(R"("value":"2026-08-20")"), std::string::npos);
    CHECK_NE(json.find(R"("issue_count":0)"), std::string::npos);

    const auto document_xml_after = read_docx_entry(updated, "word/document.xml");
    CHECK_NE(document_xml_after.find(R"(<w:t>2026-08-20</w:t>)"),
             std::string::npos);
    CHECK_EQ(document_xml_after.find("Pending date"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}
