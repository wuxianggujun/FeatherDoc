#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

namespace {
auto tiny_png_data() -> std::string {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes), sizeof(tiny_png_bytes)};
}

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

void create_cli_bookmark_image_fixture(const fs::path &path) {
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
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="body_logo"/>
      <w:r><w:t>body placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
    </w:sectPr>
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
  <w:p><w:r><w:t>header before</w:t></w:r></w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo"/>
    <w:r><w:t>header placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
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

void create_cli_bookmark_table_rows_fixture(const fs::path &path) {
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
    <w:tbl>
      <w:tblGrid>
        <w:gridCol w:w="2400"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="0" w:name="item_row"/>
            <w:r><w:t>template name</w:t></w:r>
            <w:bookmarkEnd w:id="0"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>template qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>7</w:t></w:r></w:p></w:tc>
      </w:tr>
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

void create_cli_bookmark_block_visibility_fixture(const fs::path &path) {
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
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="0" w:name="keep_block"/></w:p>
    <w:p><w:r><w:t>Keep me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="0"/></w:p>
    <w:p><w:r><w:t>middle</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="1" w:name="hide_block"/></w:p>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Secret Cell</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>Hide me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="1"/></w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
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

void create_cli_fill_bookmarks_fixture(const fs::path &path) {
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
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>old customer</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Invoice: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="invoice"/>
      <w:r><w:t>old invoice</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
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

TEST_CASE("cli replace-bookmark-paragraphs replaces a body bookmark with paragraphs") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_paragraphs_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_paragraphs_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_paragraphs.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      source.string(),
                      "body_logo",
                      "--paragraph",
                      "Alpha",
                      "--paragraph",
                      "Beta",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-paragraphs\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"paragraph_count\":2"), std::string::npos);
    CHECK_NE(json.find("\"paragraphs\":[\"Alpha\",\"Beta\"]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\nAlpha\nBeta\nafter\n"});
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("body_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-paragraphs requires at least one paragraph") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_paragraphs_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      "missing.docx",
                      "body_logo",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-bookmark-paragraphs\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected at least one "
            "--paragraph <text> or --paragraph-file <path>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-paragraphs reads UTF-8 paragraphs from files") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_paragraphs_file_source.docx";
    const fs::path paragraph_one =
        working_directory / "cli_replace_bookmark_paragraphs_file_one.txt";
    const fs::path paragraph_two =
        working_directory / "cli_replace_bookmark_paragraphs_file_two.txt";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_paragraphs_file_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_paragraphs_file.json";

    remove_if_exists(source);
    remove_if_exists(paragraph_one);
    remove_if_exists(paragraph_two);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);
    write_binary_file(paragraph_one, std::string{"第一行：项目范围确认"});
    write_binary_file(paragraph_two, std::string{"第二行：交付物复核"});

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      source.string(),
                      "body_logo",
                      "--paragraph-file",
                      paragraph_one.string(),
                      "--paragraph-file",
                      paragraph_two.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-paragraphs\""),
             std::string::npos);
    CHECK_NE(json.find("\"paragraph_count\":2"), std::string::npos);
    CHECK_NE(
        json.find("\"paragraphs\":[\"第一行：项目范围确认\",\"第二行：交付物复核\"]"),
        std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\n第一行：项目范围确认\n第二行：交付物复核\nafter\n"});

    remove_if_exists(source);
    remove_if_exists(paragraph_one);
    remove_if_exists(paragraph_two);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table replaces a body bookmark with a generated table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_table_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_table_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_table.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-table",
                      source.string(),
                      "body_logo",
                      "--row",
                      "Name",
                      "--cell",
                      "Qty",
                      "--row",
                      "Apple",
                      "--cell",
                      "2",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-table\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(json.find("\"rows\":[[\"Name\",\"Qty\"],[\"Apple\",\"2\"]]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\nafter\n"});
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].text, "Name\tQty\nApple\t2");
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("body_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table rejects cell data before a row") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_table_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-table",
                      "missing.docx",
                      "body_logo",
                      "--cell",
                      "Qty",
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{"{\"command\":\"replace-bookmark-table\",\"ok\":false,"
                         "\"stage\":\"parse\",\"message\":\"--cell requires "
                         "--row\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table-rows expands a template row inside a body table") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_table_rows_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_table_rows_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_table_rows.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_table_rows_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-table-rows",
                      source.string(),
                      "item_row",
                      "--row",
                      "Apple",
                      "--cell",
                      "2",
                      "--row",
                      "Pear",
                      "--cell",
                      "5",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-table-rows\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"item_row\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(json.find("\"rows\":[[\"Apple\",\"2\"],[\"Pear\",\"5\"]]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].text, "Name\tQty\nApple\t2\nPear\t5\nTotal\t7");
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("item_row").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table-rows reads UTF-8 rows from files") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_table_rows_file_source.docx";
    const fs::path row_one_name =
        working_directory / "cli_replace_bookmark_table_rows_file_one_name.txt";
    const fs::path row_one_qty =
        working_directory / "cli_replace_bookmark_table_rows_file_one_qty.txt";
    const fs::path row_two_name =
        working_directory / "cli_replace_bookmark_table_rows_file_two_name.txt";
    const fs::path row_two_qty =
        working_directory / "cli_replace_bookmark_table_rows_file_two_qty.txt";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_table_rows_file_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_table_rows_file.json";

    remove_if_exists(source);
    remove_if_exists(row_one_name);
    remove_if_exists(row_one_qty);
    remove_if_exists(row_two_name);
    remove_if_exists(row_two_qty);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_table_rows_fixture(source);
    write_binary_file(row_one_name, std::string{"苹果"});
    write_binary_file(row_one_qty, std::string{"2"});
    write_binary_file(row_two_name, std::string{"梨"});
    write_binary_file(row_two_qty, std::string{"5"});

    CHECK_EQ(run_cli({"replace-bookmark-table-rows",
                      source.string(),
                      "item_row",
                      "--row-file",
                      row_one_name.string(),
                      "--cell-file",
                      row_one_qty.string(),
                      "--row-file",
                      row_two_name.string(),
                      "--cell-file",
                      row_two_qty.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-table-rows\""),
             std::string::npos);
    CHECK_NE(json.find("\"row_count\":2"), std::string::npos);
    CHECK_NE(json.find("\"rows\":[[\"苹果\",\"2\"],[\"梨\",\"5\"]]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].text, "Name\tQty\n苹果\t2\n梨\t5\nTotal\t7");

    remove_if_exists(source);
    remove_if_exists(row_one_name);
    remove_if_exists(row_one_qty);
    remove_if_exists(row_two_name);
    remove_if_exists(row_two_qty);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-table-rows supports empty replacement lists") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_table_rows_empty_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_table_rows_empty_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_table_rows_empty.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_table_rows_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-table-rows",
                      source.string(),
                      "item_row",
                      "--empty",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-table-rows\""),
             std::string::npos);
    CHECK_NE(json.find("\"row_count\":0"), std::string::npos);
    CHECK_NE(json.find("\"rows\":[]"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    const auto tables = reopened.inspect_tables();
    REQUIRE_EQ(tables.size(), 1U);
    CHECK_EQ(tables[0].text, "Name\tQty\nTotal\t7");
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("item_row").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli remove-bookmark-block removes a section header placeholder paragraph") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_remove_bookmark_block_source.docx";
    const fs::path updated =
        working_directory / "cli_remove_bookmark_block_updated.docx";
    const fs::path output =
        working_directory / "cli_remove_bookmark_block.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"remove-bookmark-block",
                      source.string(),
                      "header_logo",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"remove-bookmark-block\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"removed\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_part_lines(header_template.paragraphs()),
             std::vector<std::string>{"header before"});
    CHECK_FALSE(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE(
    "cli replace-bookmark-paragraphs expands standalone bookmarks in body and "
    "section header") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_paragraphs_source.docx";
    const fs::path body_updated =
        working_directory / "cli_replace_bookmark_paragraphs_body_updated.docx";
    const fs::path header_updated =
        working_directory / "cli_replace_bookmark_paragraphs_header_updated.docx";
    const fs::path body_output =
        working_directory / "cli_replace_bookmark_paragraphs_body.json";
    const fs::path header_output =
        working_directory / "cli_replace_bookmark_paragraphs_header.json";

    remove_if_exists(source);
    remove_if_exists(body_updated);
    remove_if_exists(header_updated);
    remove_if_exists(body_output);
    remove_if_exists(header_output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      source.string(),
                      "body_logo",
                      "--part",
                      "body",
                      "--paragraph",
                      "body line 1",
                      "--paragraph",
                      "body line 2",
                      "--output",
                      body_updated.string(),
                      "--json"},
                     body_output),
             0);

    const auto body_json = read_text_file(body_output);
    CHECK_NE(body_json.find("\"command\":\"replace-bookmark-paragraphs\""),
             std::string::npos);
    CHECK_NE(body_json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(body_json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(body_json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(body_json.find("\"paragraph_count\":2"), std::string::npos);
    CHECK_NE(body_json.find("\"paragraphs\":[\"body line 1\",\"body line 2\"]"),
             std::string::npos);

    featherdoc::Document reopened_body(body_updated);
    REQUIRE_FALSE(reopened_body.open());
    auto updated_body_template = reopened_body.body_template();
    REQUIRE(static_cast<bool>(updated_body_template));
    CHECK_EQ(collect_part_lines(updated_body_template.paragraphs()),
             std::vector<std::string>{"before", "body line 1", "body line 2",
                                      "after"});
    CHECK_FALSE(updated_body_template.find_bookmark("body_logo").has_value());

    CHECK_EQ(run_cli({"replace-bookmark-paragraphs",
                      source.string(),
                      "header_logo",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--paragraph",
                      "header line 1",
                      "--paragraph",
                      "header line 2",
                      "--output",
                      header_updated.string(),
                      "--json"},
                     header_output),
             0);

    const auto header_json = read_text_file(header_output);
    CHECK_NE(header_json.find("\"command\":\"replace-bookmark-paragraphs\""),
             std::string::npos);
    CHECK_NE(header_json.find("\"part\":\"section-header\""),
             std::string::npos);
    CHECK_NE(header_json.find("\"section\":0"), std::string::npos);
    CHECK_NE(header_json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(header_json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(header_json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(header_json.find("\"paragraph_count\":2"), std::string::npos);
    CHECK_NE(
        header_json.find("\"paragraphs\":[\"header line 1\",\"header line 2\"]"),
        std::string::npos);

    featherdoc::Document reopened_header(header_updated);
    REQUIRE_FALSE(reopened_header.open());
    auto updated_header_template = reopened_header.section_header_template(0);
    REQUIRE(static_cast<bool>(updated_header_template));
    CHECK_EQ(collect_part_lines(updated_header_template.paragraphs()),
             std::vector<std::string>{"header before", "header line 1",
                                      "header line 2"});
    CHECK_FALSE(updated_header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(body_updated);
    remove_if_exists(header_updated);
    remove_if_exists(body_output);
    remove_if_exists(header_output);
}

TEST_CASE("cli set-bookmark-block-visibility removes a body block") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_set_bookmark_block_visibility_source.docx";
    const fs::path updated =
        working_directory / "cli_set_bookmark_block_visibility_updated.docx";
    const fs::path output =
        working_directory / "cli_set_bookmark_block_visibility.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_block_visibility_fixture(source);

    CHECK_EQ(run_cli({"set-bookmark-block-visibility",
                      source.string(),
                      "hide_block",
                      "--visible",
                      "false",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"set-bookmark-block-visibility\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"hide_block\""),
             std::string::npos);
    CHECK_NE(json.find("\"visible\":false"), std::string::npos);
    CHECK_NE(json.find("\"changed\":1"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\nKeep me\nmiddle\nafter\n"});
    CHECK_EQ(reopened.inspect_tables().size(), 0U);
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.find_bookmark("keep_block").has_value());
    CHECK_FALSE(body_template.find_bookmark("hide_block").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli set-bookmark-block-visibility requires an explicit visibility value") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_set_bookmark_block_visibility_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"set-bookmark-block-visibility",
                      "missing.docx",
                      "hide_block",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-bookmark-block-visibility\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected --visible "
            "true|false\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli apply-bookmark-block-visibility keeps and removes body blocks") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_apply_bookmark_block_visibility_source.docx";
    const fs::path updated =
        working_directory / "cli_apply_bookmark_block_visibility_updated.docx";
    const fs::path output =
        working_directory / "cli_apply_bookmark_block_visibility.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_block_visibility_fixture(source);

    CHECK_EQ(run_cli({"apply-bookmark-block-visibility",
                      source.string(),
                      "--show",
                      "keep_block",
                      "--hide",
                      "hide_block",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"apply-bookmark-block-visibility\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"complete\":true"), std::string::npos);
    CHECK_NE(json.find("\"requested\":2"), std::string::npos);
    CHECK_NE(json.find("\"matched\":2"), std::string::npos);
    CHECK_NE(json.find("\"kept\":1"), std::string::npos);
    CHECK_NE(json.find("\"removed\":1"), std::string::npos);
    CHECK_NE(json.find("\"bindings\":[{\"bookmark_name\":\"keep_block\","
                       "\"visible\":true},{\"bookmark_name\":\"hide_block\","
                       "\"visible\":false}]"),
             std::string::npos);
    CHECK_NE(json.find("\"missing_bookmarks\":[]"), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"before\nKeep me\nmiddle\nafter\n"});
    CHECK_EQ(reopened.inspect_tables().size(), 0U);
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK_FALSE(body_template.find_bookmark("keep_block").has_value());
    CHECK_FALSE(body_template.find_bookmark("hide_block").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-text rewrites a section header bookmark range") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_text_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_text_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_text.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-text",
                      source.string(),
                      "header_logo",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--text",
                      "updated header",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-text\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"text\":\"updated header\""), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_part_lines(header_template.paragraphs()),
             std::vector<std::string>{"header before", "updated header"});
    CHECK(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE(
    "cli replace-bookmark-text preserves explicit line breaks in a section header "
    "bookmark range") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_text_multiline_source.docx";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_text_multiline_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_text_multiline.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);

    CHECK_EQ(run_cli({"replace-bookmark-text",
                      source.string(),
                      "header_logo",
                      "--part",
                      "section-header",
                      "--section",
                      "0",
                      "--text",
                      "updated header\nsecond line",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-text\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"text\":\"updated header\\nsecond line\""),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    CHECK_EQ(collect_part_lines(header_template.paragraphs()),
             std::vector<std::string>{"header before",
                                      "updated header\nsecond line"});
    CHECK(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-text requires replacement text") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_text_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-text",
                      "missing.docx",
                      "header_logo",
                      "--json"},
                     output),
             2);
    CHECK_EQ(read_text_file(output),
             std::string{"{\"command\":\"replace-bookmark-text\",\"ok\":false,"
                         "\"stage\":\"parse\",\"message\":\"expected --text "
                         "<text>\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli fill-bookmarks rewrites multiple body bookmarks and reports misses") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_fill_bookmarks_source.docx";
    const fs::path updated =
        working_directory / "cli_fill_bookmarks_updated.docx";
    const fs::path output =
        working_directory / "cli_fill_bookmarks.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_fill_bookmarks_fixture(source);

    CHECK_EQ(run_cli({"fill-bookmarks",
                      source.string(),
                      "--set",
                      "customer",
                      "Acme Corp",
                      "--set",
                      "invoice",
                      "INV-2026-0001",
                      "--set",
                      "missing",
                      "ignored",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"fill-bookmarks\""), std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"complete\":false"), std::string::npos);
    CHECK_NE(json.find("\"requested\":3"), std::string::npos);
    CHECK_NE(json.find("\"matched\":2"), std::string::npos);
    CHECK_NE(json.find("\"replaced\":2"), std::string::npos);
    CHECK_NE(json.find("\"bindings\":[{\"bookmark_name\":\"customer\","
                       "\"text\":\"Acme Corp\"},{\"bookmark_name\":\"invoice\","
                       "\"text\":\"INV-2026-0001\"},{\"bookmark_name\":"
                       "\"missing\",\"text\":\"ignored\"}]"),
             std::string::npos);
    CHECK_NE(json.find("\"missing_bookmarks\":[\"missing\"]"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"Customer: Acme Corp\nInvoice: INV-2026-0001\n"});
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    CHECK(body_template.find_bookmark("customer").has_value());
    CHECK(body_template.find_bookmark("invoice").has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli fill-bookmarks reads UTF-8 values from files") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_fill_bookmarks_file_source.docx";
    const fs::path customer_text =
        working_directory / "cli_fill_bookmarks_customer.txt";
    const fs::path invoice_text =
        working_directory / "cli_fill_bookmarks_invoice.txt";
    const fs::path updated =
        working_directory / "cli_fill_bookmarks_file_updated.docx";
    const fs::path output =
        working_directory / "cli_fill_bookmarks_file.json";

    remove_if_exists(source);
    remove_if_exists(customer_text);
    remove_if_exists(invoice_text);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_fill_bookmarks_fixture(source);
    write_binary_file(customer_text, std::string{"上海羽文科技"});
    write_binary_file(invoice_text, std::string{"发票-2026-甲"});

    CHECK_EQ(run_cli({"fill-bookmarks",
                      source.string(),
                      "--set-file",
                      "customer",
                      customer_text.string(),
                      "--set-file",
                      "invoice",
                      invoice_text.string(),
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"fill-bookmarks\""), std::string::npos);
    CHECK_NE(json.find("\"complete\":true"), std::string::npos);
    CHECK_NE(json.find("\"matched\":2"), std::string::npos);
    CHECK_NE(json.find("\"replaced\":2"), std::string::npos);
    CHECK_NE(json.find("\"text\":\"上海羽文科技\""), std::string::npos);
    CHECK_NE(json.find("\"text\":\"发票-2026-甲\""), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    CHECK_EQ(collect_non_empty_document_text(reopened),
             std::string{"Customer: 上海羽文科技\nInvoice: 发票-2026-甲\n"});

    remove_if_exists(source);
    remove_if_exists(customer_text);
    remove_if_exists(invoice_text);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli fill-bookmarks rejects duplicate binding names") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_fill_bookmarks_duplicate_source.docx";
    const fs::path output =
        working_directory / "cli_fill_bookmarks_duplicate.json";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_fill_bookmarks_fixture(source);

    CHECK_EQ(run_cli({"fill-bookmarks",
                      source.string(),
                      "--set",
                      "customer",
                      "Acme Corp",
                      "--set",
                      "customer",
                      "Other",
                      "--json"},
                     output),
             1);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"fill-bookmarks\""), std::string::npos);
    CHECK_NE(json.find("\"stage\":\"mutate\""), std::string::npos);
    CHECK_NE(json.find("duplicate"), std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-image replaces a body bookmark with a scaled inline image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_image_source.docx";
    const fs::path image =
        working_directory / "cli_replace_bookmark_image_fixture.png";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_image_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_image.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(run_cli({"replace-bookmark-image",
                      source.string(),
                      "body_logo",
                      image.string(),
                      "--width",
                      "32",
                      "--height",
                      "16",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-image\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"body\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"body_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"image_path\":" + json_quote(image.string())),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"inline\""), std::string::npos);
    CHECK_NE(json.find("\"width_px\":32"), std::string::npos);
    CHECK_NE(json.find("\"height_px\":16"), std::string::npos);
    CHECK_NE(json.find("\"content_type\":\"image/png\""), std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto body_template = reopened.body_template();
    REQUIRE(static_cast<bool>(body_template));
    const auto images = body_template.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].placement, featherdoc::drawing_image_placement::inline_object);
    CHECK_EQ(images[0].width_px, 32U);
    CHECK_EQ(images[0].height_px, 16U);
    CHECK_EQ(images[0].content_type, "image/png");
    CHECK_EQ(read_docx_entry(updated, images[0].entry_name.c_str()), tiny_png_data());
    CHECK_FALSE(body_template.find_bookmark("body_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-image rejects floating layout options") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-image",
                      "missing.docx",
                      "logo",
                      "logo.png",
                      "--floating",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-bookmark-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"replace-bookmark-image does not "
            "accept floating layout options; use "
            "replace-bookmark-floating-image\"}\n"});

    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-floating-image replaces a section header bookmark with an anchored image") {
    const fs::path working_directory = fs::current_path();
    const fs::path source =
        working_directory / "cli_replace_bookmark_floating_image_source.docx";
    const fs::path image =
        working_directory / "cli_replace_bookmark_floating_image_fixture.png";
    const fs::path updated =
        working_directory / "cli_replace_bookmark_floating_image_updated.docx";
    const fs::path output =
        working_directory / "cli_replace_bookmark_floating_image.json";

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_bookmark_image_fixture(source);
    write_binary_file(image, tiny_png_data());

    CHECK_EQ(
        run_cli({"replace-bookmark-floating-image",
                 source.string(),
                 "header_logo",
                 image.string(),
                 "--part",
                 "section-header",
                 "--section",
                 "0",
                 "--width",
                 "30",
                 "--height",
                 "15",
                 "--horizontal-reference",
                 "page",
                 "--horizontal-offset",
                 "40",
                 "--vertical-reference",
                 "margin",
                 "--vertical-offset",
                 "12",
                 "--behind-text",
                 "true",
                 "--allow-overlap",
                 "false",
                 "--z-order",
                 "108",
                 "--wrap-mode",
                 "square",
                 "--wrap-distance-left",
                 "5",
                 "--wrap-distance-right",
                 "7",
                 "--crop-left",
                 "10",
                 "--crop-top",
                 "20",
                 "--crop-right",
                 "30",
                 "--crop-bottom",
                 "40",
                 "--output",
                 updated.string(),
                 "--json"},
                output),
        0);

    const auto json = read_text_file(output);
    CHECK_NE(json.find("\"command\":\"replace-bookmark-floating-image\""),
             std::string::npos);
    CHECK_NE(json.find("\"part\":\"section-header\""), std::string::npos);
    CHECK_NE(json.find("\"section\":0"), std::string::npos);
    CHECK_NE(json.find("\"kind\":\"default\""), std::string::npos);
    CHECK_NE(json.find("\"bookmark\":{\"bookmark_name\":\"header_logo\""),
             std::string::npos);
    CHECK_NE(json.find("\"replaced\":1"), std::string::npos);
    CHECK_NE(json.find("\"placement\":\"anchored\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_reference\":\"page\""), std::string::npos);
    CHECK_NE(json.find("\"horizontal_offset_px\":40"), std::string::npos);
    CHECK_NE(json.find("\"vertical_reference\":\"margin\""), std::string::npos);
    CHECK_NE(json.find("\"vertical_offset_px\":12"), std::string::npos);
    CHECK_NE(json.find("\"behind_text\":true"), std::string::npos);
    CHECK_NE(json.find("\"allow_overlap\":false"), std::string::npos);
    CHECK_NE(json.find("\"z_order\":108"), std::string::npos);
    CHECK_NE(json.find("\"wrap_mode\":\"square\""), std::string::npos);
    CHECK_NE(json.find("\"crop\":{\"left_per_mille\":10,\"top_per_mille\":20,"
                       "\"right_per_mille\":30,\"bottom_per_mille\":40}"),
             std::string::npos);

    featherdoc::Document reopened(updated);
    REQUIRE_FALSE(reopened.open());
    auto header_template = reopened.section_header_template(0);
    REQUIRE(static_cast<bool>(header_template));
    const auto images = header_template.drawing_images();
    REQUIRE_EQ(images.size(), 1U);
    CHECK_EQ(images[0].placement,
             featherdoc::drawing_image_placement::anchored_object);
    CHECK_EQ(images[0].width_px, 30U);
    CHECK_EQ(images[0].height_px, 15U);
    REQUIRE(images[0].floating_options.has_value());
    CHECK_EQ(images[0].floating_options->horizontal_reference,
             featherdoc::floating_image_horizontal_reference::page);
    CHECK_EQ(images[0].floating_options->horizontal_offset_px, 40);
    CHECK_EQ(images[0].floating_options->vertical_reference,
             featherdoc::floating_image_vertical_reference::margin);
    CHECK_EQ(images[0].floating_options->vertical_offset_px, 12);
    CHECK(images[0].floating_options->behind_text);
    CHECK_FALSE(images[0].floating_options->allow_overlap);
    CHECK_EQ(images[0].floating_options->z_order, 108U);
    CHECK_EQ(images[0].floating_options->wrap_mode,
             featherdoc::floating_image_wrap_mode::square);
    CHECK_EQ(images[0].floating_options->wrap_distance_left_px, 5U);
    CHECK_EQ(images[0].floating_options->wrap_distance_right_px, 7U);
    REQUIRE(images[0].floating_options->crop.has_value());
    CHECK_EQ(images[0].floating_options->crop->left_per_mille, 10U);
    CHECK_EQ(images[0].floating_options->crop->top_per_mille, 20U);
    CHECK_EQ(images[0].floating_options->crop->right_per_mille, 30U);
    CHECK_EQ(images[0].floating_options->crop->bottom_per_mille, 40U);
    CHECK_FALSE(header_template.find_bookmark("header_logo").has_value());

    remove_if_exists(source);
    remove_if_exists(image);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli replace-bookmark-floating-image requires width and height together") {
    const fs::path working_directory = fs::current_path();
    const fs::path output =
        working_directory / "cli_replace_bookmark_floating_image_parse.json";

    remove_if_exists(output);

    CHECK_EQ(run_cli({"replace-bookmark-floating-image",
                      "missing.docx",
                      "logo",
                      "logo.png",
                      "--width",
                      "30",
                      "--json"},
                     output),
             2);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"replace-bookmark-floating-image\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"replace-bookmark-floating-image "
            "requires both --width <px> and --height <px> when scaling\"}\n"});

    remove_if_exists(output);
}

} // namespace
