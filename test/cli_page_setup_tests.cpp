#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"
#include "cli_style_test_support.hpp"

#include <zip.h>

namespace {
void create_cli_page_setup_fixture(const fs::path &path) {
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
      <w:pPr>
        <w:sectPr>
          <w:pgSz w:w="12240" w:h="15840"/>
          <w:pgMar w:top="1440" w:right="1800" w:bottom="1440" w:left="1800" w:header="720" w:footer="720"/>
          <w:pgNumType w:start="5"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>portrait section</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>landscape section</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/>
      <w:pgMar w:top="720" w:right="1440" w:bottom="1080" w:left="1440" w:header="360" w:footer="540"/>
    </w:sectPr>
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

TEST_CASE("cli inspect-page-setup lists all sections in text mode") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_page_setup_source.docx";
    const fs::path output = working_directory / "cli_page_setup_output.txt";

    remove_if_exists(source);
    remove_if_exists(output);

    create_cli_page_setup_fixture(source);

    CHECK_EQ(run_cli({"inspect-page-setup", source.string()}, output), 0);
    const auto inspect_text = read_text_file(output);
    CHECK_NE(inspect_text.find("sections: 2"), std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[0]: present=yes orientation=portrait width_twips=12240 "
                 "height_twips=15840"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "margins(top=1440, bottom=1440, left=1800, right=1800, "
                 "header=720, footer=720) page_number_start=5"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[1]: present=yes orientation=landscape width_twips=15840 "
                 "height_twips=12240"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "margins(top=720, bottom=1080, left=1440, right=1440, "
                 "header=360, footer=540) page_number_start=none"),
             std::string::npos);

    remove_if_exists(source);
    remove_if_exists(output);
}

TEST_CASE("cli inspect-page-setup supports single-section json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_page_setup_single_source.docx";
    const fs::path inspect_output =
        working_directory / "cli_page_setup_single.json";
    const fs::path empty_source = working_directory / "cli_page_setup_empty.docx";
    const fs::path empty_output = working_directory / "cli_page_setup_empty.json";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(empty_source);
    remove_if_exists(empty_output);

    create_cli_page_setup_fixture(source);
    create_cli_style_defaults_fixture(empty_source);

    CHECK_EQ(run_cli({"inspect-page-setup", source.string(), "--section", "1",
                      "--json"},
                     inspect_output),
             0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"section\":1,\"present\":true,\"page_setup\":{"
            "\"orientation\":\"landscape\",\"width_twips\":15840,"
            "\"height_twips\":12240,\"margins\":{\"top_twips\":720,"
            "\"bottom_twips\":1080,\"left_twips\":1440,\"right_twips\":1440,"
            "\"header_twips\":360,\"footer_twips\":540},"
            "\"page_number_start\":null}}\n"});

    CHECK_EQ(run_cli({"inspect-page-setup", empty_source.string(), "--section",
                      "0", "--json"},
                     empty_output),
             0);
    CHECK_EQ(read_text_file(empty_output),
             std::string{"{\"section\":0,\"present\":false,\"page_setup\":null}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(empty_source);
    remove_if_exists(empty_output);
}

TEST_CASE("cli set-section-page-setup updates an existing section and clears page numbering") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_page_setup_mutate_source.docx";
    const fs::path updated = working_directory / "cli_page_setup_mutated.docx";
    const fs::path output = working_directory / "cli_page_setup_mutated.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_page_setup_fixture(source);

    CHECK_EQ(run_cli({"set-section-page-setup",
                      source.string(),
                      "0",
                      "--orientation",
                      "landscape",
                      "--width",
                      "15840",
                      "--height",
                      "12240",
                      "--margin-top",
                      "720",
                      "--clear-page-number-start",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-section-page-setup\",\"ok\":true,"
            "\"in_place\":false,\"sections\":2,\"headers\":0,\"footers\":0,"
            "\"section\":0,\"page_setup\":{"
            "\"orientation\":\"landscape\",\"width_twips\":15840,"
            "\"height_twips\":12240,\"margins\":{\"top_twips\":720,"
            "\"bottom_twips\":1440,\"left_twips\":1800,\"right_twips\":1800,"
            "\"header_twips\":720,\"footer_twips\":720},"
            "\"page_number_start\":null}}\n"});

    featherdoc::Document updated_doc(updated);
    REQUIRE_FALSE(updated_doc.open());
    const auto page_setup = updated_doc.get_section_page_setup(0);
    REQUIRE(page_setup.has_value());
    CHECK_EQ(page_setup->orientation, featherdoc::page_orientation::landscape);
    CHECK_EQ(page_setup->width_twips, 15840U);
    CHECK_EQ(page_setup->height_twips, 12240U);
    CHECK_EQ(page_setup->margins.top_twips, 720U);
    CHECK_EQ(page_setup->margins.bottom_twips, 1440U);
    CHECK_EQ(page_setup->margins.left_twips, 1800U);
    CHECK_EQ(page_setup->margins.right_twips, 1800U);
    CHECK_EQ(page_setup->margins.header_twips, 720U);
    CHECK_EQ(page_setup->margins.footer_twips, 720U);
    CHECK_FALSE(page_setup->page_number_start.has_value());

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}

TEST_CASE("cli set-section-page-setup materializes an implicit final section") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_page_setup_implicit_source.docx";
    const fs::path updated = working_directory / "cli_page_setup_implicit.docx";
    const fs::path output = working_directory / "cli_page_setup_implicit.json";

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);

    create_cli_style_defaults_fixture(source);

    CHECK_EQ(run_cli({"set-section-page-setup",
                      source.string(),
                      "0",
                      "--orientation",
                      "portrait",
                      "--width",
                      "12240",
                      "--height",
                      "15840",
                      "--margin-top",
                      "1440",
                      "--margin-bottom",
                      "1440",
                      "--margin-left",
                      "1800",
                      "--margin-right",
                      "1800",
                      "--margin-header",
                      "720",
                      "--margin-footer",
                      "720",
                      "--page-number-start",
                      "3",
                      "--output",
                      updated.string(),
                      "--json"},
                     output),
             0);
    CHECK_EQ(
        read_text_file(output),
        std::string{
            "{\"command\":\"set-section-page-setup\",\"ok\":true,"
            "\"in_place\":false,\"sections\":1,\"headers\":0,\"footers\":0,"
            "\"section\":0,\"page_setup\":{"
            "\"orientation\":\"portrait\",\"width_twips\":12240,"
            "\"height_twips\":15840,\"margins\":{\"top_twips\":1440,"
            "\"bottom_twips\":1440,\"left_twips\":1800,\"right_twips\":1800,"
            "\"header_twips\":720,\"footer_twips\":720},"
            "\"page_number_start\":3}}\n"});

    featherdoc::Document updated_doc(updated);
    REQUIRE_FALSE(updated_doc.open());
    const auto page_setup = updated_doc.get_section_page_setup(0);
    REQUIRE(page_setup.has_value());
    CHECK_EQ(page_setup->orientation, featherdoc::page_orientation::portrait);
    CHECK_EQ(page_setup->width_twips, 12240U);
    CHECK_EQ(page_setup->height_twips, 15840U);
    CHECK_EQ(page_setup->margins.top_twips, 1440U);
    CHECK_EQ(page_setup->margins.bottom_twips, 1440U);
    CHECK_EQ(page_setup->margins.left_twips, 1800U);
    CHECK_EQ(page_setup->margins.right_twips, 1800U);
    CHECK_EQ(page_setup->margins.header_twips, 720U);
    CHECK_EQ(page_setup->margins.footer_twips, 720U);
    REQUIRE(page_setup->page_number_start.has_value());
    CHECK_EQ(*page_setup->page_number_start, 3U);

    remove_if_exists(source);
    remove_if_exists(updated);
    remove_if_exists(output);
}
