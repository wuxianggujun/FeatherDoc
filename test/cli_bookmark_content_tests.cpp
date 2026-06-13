#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_bookmark_content_test_support.hpp"

namespace {
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

} // namespace
