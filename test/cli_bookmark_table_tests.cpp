#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_bookmark_content_test_support.hpp"

namespace {
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

} // namespace
