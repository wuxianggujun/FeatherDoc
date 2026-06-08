#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("get_section_page_setup reads explicit section settings") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "section_page_setup_read.docx";
    fs::remove(target);

    const std::string document_xml =
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
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/>
      <w:pgMar w:top="720" w:right="1440" w:bottom="1080" w:left="1440" w:header="360" w:footer="540"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 2);

    const auto section0 = doc.get_section_page_setup(0);
    REQUIRE(section0.has_value());
    CHECK_EQ(section0->orientation, featherdoc::page_orientation::portrait);
    CHECK_EQ(section0->width_twips, 12240U);
    CHECK_EQ(section0->height_twips, 15840U);
    CHECK_EQ(section0->margins.top_twips, 1440U);
    CHECK_EQ(section0->margins.bottom_twips, 1440U);
    CHECK_EQ(section0->margins.left_twips, 1800U);
    CHECK_EQ(section0->margins.right_twips, 1800U);
    CHECK_EQ(section0->margins.header_twips, 720U);
    CHECK_EQ(section0->margins.footer_twips, 720U);
    REQUIRE(section0->page_number_start.has_value());
    CHECK_EQ(*section0->page_number_start, 5U);

    const auto section1 = doc.get_section_page_setup(1);
    REQUIRE(section1.has_value());
    CHECK_EQ(section1->orientation, featherdoc::page_orientation::landscape);
    CHECK_EQ(section1->width_twips, 15840U);
    CHECK_EQ(section1->height_twips, 12240U);
    CHECK_EQ(section1->margins.top_twips, 720U);
    CHECK_EQ(section1->margins.bottom_twips, 1080U);
    CHECK_EQ(section1->margins.left_twips, 1440U);
    CHECK_EQ(section1->margins.right_twips, 1440U);
    CHECK_EQ(section1->margins.header_twips, 360U);
    CHECK_EQ(section1->margins.footer_twips, 540U);
    CHECK_FALSE(section1->page_number_start.has_value());

    fs::remove(target);
}

TEST_CASE("get_section_page_setup returns nullopt when setup is implicit") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "section_page_setup_implicit.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    const auto page_setup = doc.get_section_page_setup(0);
    CHECK_FALSE(page_setup.has_value());
    CHECK_FALSE(static_cast<bool>(doc.last_error().code));

    fs::remove(target);
}

TEST_CASE("get_section_page_setup reports document state errors") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "section_page_setup_errors.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.get_section_page_setup(0).has_value());
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::document_not_open);

    CHECK_FALSE(doc.create_empty());
    CHECK_FALSE(doc.get_section_page_setup(1).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("set_section_page_setup creates a final section setup and round-trips") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "section_page_setup_write_final.docx";
    fs::remove(target);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.create_empty());

    featherdoc::section_page_setup setup{};
    setup.orientation = featherdoc::page_orientation::landscape;
    setup.width_twips = 15840U;
    setup.height_twips = 12240U;
    setup.margins.top_twips = 720U;
    setup.margins.bottom_twips = 1080U;
    setup.margins.left_twips = 1440U;
    setup.margins.right_twips = 1440U;
    setup.margins.header_twips = 360U;
    setup.margins.footer_twips = 540U;
    setup.page_number_start = 7U;

    CHECK(doc.set_section_page_setup(0, setup));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_document_xml.find("<w:sectPr"), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:orient=\"landscape\""), std::string::npos);
    CHECK_NE(saved_document_xml.find("w:start=\"7\""), std::string::npos);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_setup = reopened.get_section_page_setup(0);
    REQUIRE(reopened_setup.has_value());
    CHECK_EQ(reopened_setup->orientation, featherdoc::page_orientation::landscape);
    CHECK_EQ(reopened_setup->width_twips, 15840U);
    CHECK_EQ(reopened_setup->height_twips, 12240U);
    CHECK_EQ(reopened_setup->margins.top_twips, 720U);
    CHECK_EQ(reopened_setup->margins.bottom_twips, 1080U);
    CHECK_EQ(reopened_setup->margins.left_twips, 1440U);
    CHECK_EQ(reopened_setup->margins.right_twips, 1440U);
    CHECK_EQ(reopened_setup->margins.header_twips, 360U);
    CHECK_EQ(reopened_setup->margins.footer_twips, 540U);
    REQUIRE(reopened_setup->page_number_start.has_value());
    CHECK_EQ(*reopened_setup->page_number_start, 7U);

    fs::remove(target);
}

TEST_CASE("set_section_page_setup updates paragraph and body section properties") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "section_page_setup_write_multi.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:type w:val="nextPage"/>
          <w:pgSz w:w="11906" w:h="16838" w:code="9"/>
          <w:pgMar w:top="1500" w:right="1700" w:bottom="1500" w:left="1700" w:header="700" w:footer="700" w:gutter="200"/>
          <w:pgNumType w:fmt="decimal" w:start="3"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>section one</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>section two</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840" w:code="7" w:orient="landscape"/>
      <w:pgMar w:top="1000" w:right="1100" w:bottom="1200" w:left="1300" w:header="400" w:footer="500" w:gutter="100"/>
      <w:pgNumType w:fmt="decimal" w:start="12"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());
    CHECK_EQ(doc.section_count(), 2);

    featherdoc::section_page_setup first_setup{};
    first_setup.orientation = featherdoc::page_orientation::portrait;
    first_setup.width_twips = 12240U;
    first_setup.height_twips = 15840U;
    first_setup.margins.top_twips = 1440U;
    first_setup.margins.bottom_twips = 1440U;
    first_setup.margins.left_twips = 1800U;
    first_setup.margins.right_twips = 1800U;
    first_setup.margins.header_twips = 720U;
    first_setup.margins.footer_twips = 720U;
    first_setup.page_number_start = std::nullopt;

    featherdoc::section_page_setup second_setup{};
    second_setup.orientation = featherdoc::page_orientation::landscape;
    second_setup.width_twips = 15840U;
    second_setup.height_twips = 12240U;
    second_setup.margins.top_twips = 720U;
    second_setup.margins.bottom_twips = 1080U;
    second_setup.margins.left_twips = 1440U;
    second_setup.margins.right_twips = 1440U;
    second_setup.margins.header_twips = 360U;
    second_setup.margins.footer_twips = 540U;
    second_setup.page_number_start = 1U;

    CHECK(doc.set_section_page_setup(0, first_setup));
    CHECK(doc.set_section_page_setup(1, second_setup));
    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_buffer(saved_document_xml.data(), saved_document_xml.size()));
    const auto body = saved_document.child("w:document").child("w:body");
    const auto section0_properties = body.child("w:p").child("w:pPr").child("w:sectPr");
    const auto section1_properties = body.child("w:sectPr");
    REQUIRE(section0_properties != pugi::xml_node{});
    REQUIRE(section1_properties != pugi::xml_node{});

    CHECK_EQ(std::string_view{section0_properties.child("w:type").attribute("w:val").value()},
             "nextPage");
    CHECK_EQ(std::string_view{section0_properties.child("w:pgSz").attribute("w:code").value()},
             "9");
    CHECK_EQ(section0_properties.child("w:pgSz").attribute("w:orient"), pugi::xml_attribute{});
    CHECK_EQ(std::string_view{section0_properties.child("w:pgMar").attribute("w:gutter").value()},
             "200");
    CHECK_EQ(std::string_view{section0_properties.child("w:pgNumType").attribute("w:fmt").value()},
             "decimal");
    CHECK_EQ(section0_properties.child("w:pgNumType").attribute("w:start"),
             pugi::xml_attribute{});

    CHECK_EQ(std::string_view{section1_properties.child("w:pgSz").attribute("w:code").value()},
             "7");
    CHECK_EQ(std::string_view{section1_properties.child("w:pgSz").attribute("w:orient").value()},
             "landscape");
    CHECK_EQ(std::string_view{section1_properties.child("w:pgMar").attribute("w:gutter").value()},
             "100");
    CHECK_EQ(std::string_view{section1_properties.child("w:pgNumType").attribute("w:fmt").value()},
             "decimal");
    CHECK_EQ(std::string_view{section1_properties.child("w:pgNumType").attribute("w:start").value()},
             "1");

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());

    const auto reopened_first = reopened.get_section_page_setup(0);
    REQUIRE(reopened_first.has_value());
    CHECK_EQ(reopened_first->orientation, featherdoc::page_orientation::portrait);
    CHECK_EQ(reopened_first->width_twips, 12240U);
    CHECK_EQ(reopened_first->height_twips, 15840U);
    CHECK_EQ(reopened_first->margins.left_twips, 1800U);
    CHECK_FALSE(reopened_first->page_number_start.has_value());

    const auto reopened_second = reopened.get_section_page_setup(1);
    REQUIRE(reopened_second.has_value());
    CHECK_EQ(reopened_second->orientation, featherdoc::page_orientation::landscape);
    CHECK_EQ(reopened_second->width_twips, 15840U);
    CHECK_EQ(reopened_second->height_twips, 12240U);
    CHECK_EQ(reopened_second->margins.top_twips, 720U);
    REQUIRE(reopened_second->page_number_start.has_value());
    CHECK_EQ(*reopened_second->page_number_start, 1U);

    fs::remove(target);
}

TEST_CASE("set_section_page_setup validates document state and dimensions") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "section_page_setup_write_errors.docx";
    fs::remove(target);

    featherdoc::Document doc(target);

    featherdoc::section_page_setup setup{};
    setup.width_twips = 12240U;
    setup.height_twips = 15840U;
    setup.margins.top_twips = 1440U;
    setup.margins.bottom_twips = 1440U;
    setup.margins.left_twips = 1800U;
    setup.margins.right_twips = 1800U;
    setup.margins.header_twips = 720U;
    setup.margins.footer_twips = 720U;

    CHECK_FALSE(doc.set_section_page_setup(0, setup));
    CHECK_EQ(doc.last_error().code, featherdoc::document_errc::document_not_open);

    CHECK_FALSE(doc.create_empty());
    setup.width_twips = 0U;
    CHECK_FALSE(doc.set_section_page_setup(0, setup));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    setup.width_twips = 12240U;
    CHECK_FALSE(doc.set_section_page_setup(1, setup));
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));

    fs::remove(target);
}

TEST_CASE("get_section_page_setup reports unsupported explicit page orientation") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "section_page_setup_invalid_orientation.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>section one</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840" w:orient="sideways"/>
      <w:pgMar w:top="1440" w:right="1800" w:bottom="1440" w:left="1800" w:header="720" w:footer="720"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_FALSE(doc.get_section_page_setup(0).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "unsupported section page orientation: sideways");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}

TEST_CASE("get_section_page_setup reports malformed page number start attributes") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "section_page_setup_invalid_page_number_start.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>section one</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1800" w:bottom="1440" w:left="1800" w:header="720" w:footer="720"/>
      <w:pgNumType w:start="abc"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_FALSE(doc.get_section_page_setup(0).has_value());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail,
             "invalid section page setup attribute w:pgNumType/w:start "
             "(expected unsigned integer)");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}
