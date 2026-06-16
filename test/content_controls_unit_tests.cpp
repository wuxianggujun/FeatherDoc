#include <filesystem>
#include <string>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"
#include "basic_image_fixture_test_support.hpp"

#include <featherdoc.hpp>
TEST_CASE("content controls can be listed and filtered by tag or alias") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "content_controls_inspect.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
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
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto content_controls = doc.list_content_controls();
    CHECK_FALSE(doc.last_error());
    REQUIRE(content_controls.size() == 3U);
    CHECK_EQ(content_controls[0].index, 0U);
    CHECK_EQ(content_controls[0].kind, featherdoc::content_control_kind::block);
    REQUIRE(content_controls[0].tag.has_value());
    CHECK_EQ(*content_controls[0].tag, "customer_name");
    REQUIRE(content_controls[0].alias.has_value());
    CHECK_EQ(*content_controls[0].alias, "Customer Name");
    REQUIRE(content_controls[0].id.has_value());
    CHECK_EQ(*content_controls[0].id, "42");
    CHECK_FALSE(content_controls[0].showing_placeholder);
    CHECK_EQ(content_controls[0].text, "Ada Lovelace");
    CHECK(content_controls[0].has_tag());
    CHECK(content_controls[0].has_alias());

    CHECK_EQ(content_controls[1].kind, featherdoc::content_control_kind::run);
    CHECK(content_controls[1].showing_placeholder);
    CHECK_EQ(content_controls[1].text, "INV-001");
    CHECK_EQ(content_controls[2].kind, featherdoc::content_control_kind::table_row);
    CHECK_EQ(content_controls[2].text, "SKU-1");

    const auto order_controls = doc.find_content_controls_by_tag("order_no");
    CHECK_FALSE(doc.last_error());
    REQUIRE(order_controls.size() == 1U);
    CHECK_EQ(order_controls.front().index, 1U);
    REQUIRE(order_controls.front().alias.has_value());
    CHECK_EQ(*order_controls.front().alias, "Order Number");

    const auto line_item_controls =
        doc.body_template().find_content_controls_by_alias("Line Items");
    CHECK_FALSE(doc.last_error());
    REQUIRE(line_item_controls.size() == 1U);
    CHECK_EQ(line_item_controls.front().kind,
             featherdoc::content_control_kind::table_row);

    const auto missing_controls = doc.find_content_controls_by_tag("missing");
    CHECK_FALSE(doc.last_error());
    CHECK(missing_controls.empty());

    const auto invalid_controls = doc.find_content_controls_by_tag("");
    CHECK(invalid_controls.empty());
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "content control tag must not be empty");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}

TEST_CASE("content control inspection reports form state metadata") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_form_state.docx";
    fs::remove(target);

    const std::string document_xml =
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
          <w14:checkbox><w14:checked w14:val="1"/></w14:checkbox>
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
        <w:dataBinding w:storeItemID="{11111111-1111-1111-1111-111111111111}" w:xpath="/invoice/dueDate" w:prefixMappings="xmlns:fd=&quot;urn:featherdoc&quot;"/>
        <w:date><w:dateFormat w:val="yyyy-MM-dd"/><w:lid w:val="en-US"/></w:date>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>2026-05-01</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 3U);
    CHECK_EQ(controls[0].form_kind,
             featherdoc::content_control_form_kind::checkbox);
    REQUIRE(controls[0].lock.has_value());
    CHECK_EQ(*controls[0].lock, "sdtContentLocked");
    REQUIRE(controls[0].checked.has_value());
    CHECK(*controls[0].checked);

    CHECK_EQ(controls[1].form_kind,
             featherdoc::content_control_form_kind::drop_down_list);
    REQUIRE(controls[1].selected_list_item.has_value());
    CHECK_EQ(*controls[1].selected_list_item, 1U);
    REQUIRE(controls[1].list_items.size() == 2U);
    CHECK_EQ(controls[1].list_items[0].display_text, "Draft");
    CHECK_EQ(controls[1].list_items[0].value, "draft");
    CHECK_EQ(controls[1].list_items[1].display_text, "Approved");
    CHECK_EQ(controls[1].list_items[1].value, "approved");

    CHECK_EQ(controls[2].form_kind,
             featherdoc::content_control_form_kind::date);
    REQUIRE(controls[2].date_format.has_value());
    CHECK_EQ(*controls[2].date_format, "yyyy-MM-dd");
    REQUIRE(controls[2].date_locale.has_value());
    CHECK_EQ(*controls[2].date_locale, "en-US");
    REQUIRE(controls[2].data_binding_store_item_id.has_value());
    CHECK_EQ(*controls[2].data_binding_store_item_id,
             "{11111111-1111-1111-1111-111111111111}");
    REQUIRE(controls[2].data_binding_xpath.has_value());
    CHECK_EQ(*controls[2].data_binding_xpath, "/invoice/dueDate");
    REQUIRE(controls[2].data_binding_prefix_mappings.has_value());
    CHECK_EQ(*controls[2].data_binding_prefix_mappings,
             "xmlns:fd=\"urn:featherdoc\"");

    fs::remove(target);
}

TEST_CASE("content control form state can be mutated by tag or alias") {
    namespace fs = std::filesystem;

    const fs::path target =
        fs::current_path() / "content_controls_form_state_mutation.docx";
    fs::remove(target);

    const std::string document_xml =
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
          <w14:checkbox><w14:checked w14:val="1"/></w14:checkbox>
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
        <w:date><w:dateFormat w:val="yyyy-MM-dd"/><w:lid w:val="en-US"/></w:date>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>2026-05-01</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    featherdoc::content_control_form_state_options checkbox_options;
    checkbox_options.clear_lock = true;
    checkbox_options.checked = false;
    CHECK_EQ(doc.set_content_control_form_state_by_tag("approved",
                                                        checkbox_options),
             1U);
    CHECK_FALSE(doc.last_error());

    featherdoc::content_control_form_state_options list_options;
    list_options.lock = "sdtLocked";
    list_options.selected_list_item = "draft";
    CHECK_EQ(doc.set_content_control_form_state_by_alias("Status", list_options),
             1U);
    CHECK_FALSE(doc.last_error());

    featherdoc::content_control_form_state_options date_options;
    date_options.date_text = "2026-06-01";
    date_options.date_format = "yyyy/MM/dd";
    date_options.date_locale = "zh-CN";
    date_options.data_binding_store_item_id =
        "{22222222-2222-2222-2222-222222222222}";
    date_options.data_binding_xpath = "/invoice/dueDate";
    date_options.data_binding_prefix_mappings =
        "xmlns:fd=\"urn:featherdoc\"";
    CHECK_EQ(doc.body_template().set_content_control_form_state_by_tag(
                 "due_date", date_options),
             1U);
    CHECK_FALSE(doc.last_error());

    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 3U);
    CHECK_EQ(controls[0].form_kind,
             featherdoc::content_control_form_kind::checkbox);
    REQUIRE(controls[0].checked.has_value());
    CHECK_FALSE(*controls[0].checked);
    CHECK_FALSE(controls[0].lock.has_value());
    CHECK_EQ(controls[0].text, "☐");

    CHECK_EQ(controls[1].form_kind,
             featherdoc::content_control_form_kind::drop_down_list);
    REQUIRE(controls[1].lock.has_value());
    CHECK_EQ(*controls[1].lock, "sdtLocked");
    REQUIRE(controls[1].selected_list_item.has_value());
    CHECK_EQ(*controls[1].selected_list_item, 0U);
    CHECK_EQ(controls[1].text, "Draft");

    CHECK_EQ(controls[2].form_kind, featherdoc::content_control_form_kind::date);
    REQUIRE(controls[2].date_format.has_value());
    CHECK_EQ(*controls[2].date_format, "yyyy/MM/dd");
    REQUIRE(controls[2].date_locale.has_value());
    CHECK_EQ(*controls[2].date_locale, "zh-CN");
    CHECK_EQ(controls[2].text, "2026-06-01");
    REQUIRE(controls[2].data_binding_store_item_id.has_value());
    CHECK_EQ(*controls[2].data_binding_store_item_id,
             "{22222222-2222-2222-2222-222222222222}");
    REQUIRE(controls[2].data_binding_xpath.has_value());
    CHECK_EQ(*controls[2].data_binding_xpath, "/invoice/dueDate");
    REQUIRE(controls[2].data_binding_prefix_mappings.has_value());
    CHECK_EQ(*controls[2].data_binding_prefix_mappings,
             "xmlns:fd=\"urn:featherdoc\"");

    featherdoc::content_control_form_state_options clear_binding_options;
    clear_binding_options.clear_data_binding = true;
    CHECK_EQ(doc.set_content_control_form_state_by_tag("due_date",
                                                        clear_binding_options),
             1U);
    CHECK_FALSE(doc.last_error());
    const auto cleared_controls = doc.list_content_controls();
    REQUIRE(cleared_controls.size() == 3U);
    CHECK_FALSE(cleared_controls[2].data_binding_store_item_id.has_value());
    CHECK_FALSE(cleared_controls[2].data_binding_xpath.has_value());
    CHECK_FALSE(cleared_controls[2].data_binding_prefix_mappings.has_value());

    featherdoc::content_control_form_state_options empty_options;
    CHECK_EQ(doc.set_content_control_form_state_by_tag("approved", empty_options),
             0U);
    CHECK_EQ(doc.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail,
             "content control form-state options must include at least one change");

    fs::remove(target);
}

TEST_CASE("content controls can sync text from Custom XML data bindings") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "content_controls_custom_xml_sync.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Due Date"/>
        <w:tag w:val="due_date"/>
        <w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/dueDate"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>Pending date</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Total"/>
        <w:tag w:val="total"/>
        <w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/fd:invoice/fd:total" w:prefixMappings="xmlns:fd=&quot;urn:featherdoc&quot;"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>Pending total</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
    <w:sdt>
      <w:sdtPr>
        <w:tag w:val="missing"/>
        <w:dataBinding w:storeItemID="{55555555-5555-5555-5555-555555555555}" w:xpath="/invoice/missing"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>Missing value</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    const std::string custom_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<invoice xmlns="urn:featherdoc">
  <dueDate>2026-07-15</dueDate>
  <total currency="USD">123.45</total>
</invoice>
)";
    const std::string item_props =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}"
                  xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml">
  <ds:schemaRefs/>
</ds:datastoreItem>
)";
    const std::string item_relationships =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps"
                Target="itemProps1.xml"/>
</Relationships>
)";

    write_test_archive_entries(
        target,
        {{test_content_types_xml_entry, test_content_types_xml},
         {test_relationships_xml_entry, test_relationships_xml},
         {test_document_xml_entry, document_xml},
         {"customXml/item1.xml", custom_xml},
         {"customXml/itemProps1.xml", item_props},
         {"customXml/_rels/item1.xml.rels", item_relationships}});

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    const auto result = doc.sync_content_controls_from_custom_xml();
    REQUIRE(result.has_value());
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(result->scanned_content_controls, 3U);
    CHECK_EQ(result->bound_content_controls, 3U);
    CHECK_EQ(result->synced_content_controls, 2U);
    REQUIRE(result->synced_items.size() == 2U);
    CHECK_EQ(result->synced_items[0].previous_text, "Pending date");
    CHECK_EQ(result->synced_items[0].value, "2026-07-15");
    CHECK_EQ(result->synced_items[1].value, "123.45");
    REQUIRE(result->issues.size() == 1U);
    CHECK_EQ(result->issues[0].reason, "custom_xml_value_not_found");
    CHECK_EQ(result->issues[0].xpath, "/invoice/missing");

    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 3U);
    CHECK_EQ(controls[0].text, "2026-07-15");
    CHECK_EQ(controls[1].text, "123.45");
    CHECK_EQ(controls[2].text, "Missing value");

    CHECK_FALSE(doc.save());
    const auto saved_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_NE(saved_xml.find("2026-07-15"), std::string::npos);
    CHECK_NE(saved_xml.find("123.45"), std::string::npos);
    CHECK_NE(saved_xml.find("Missing value"), std::string::npos);

    fs::remove(target);
}

TEST_CASE("content control text can be replaced by tag or alias") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "content_controls_replace_text.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
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
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_content_control_text_by_tag("order_no", "INV-002\nready"),
             1U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(doc.body_template().replace_content_control_text_by_alias("Line Items",
                                                                       "SKU-2"),
             1U);
    CHECK_FALSE(doc.last_error());

    const auto content_controls = doc.list_content_controls();
    CHECK_FALSE(doc.last_error());
    REQUIRE(content_controls.size() == 3U);
    CHECK_EQ(content_controls[1].text, "INV-002\nready");
    CHECK_FALSE(content_controls[1].showing_placeholder);
    CHECK_EQ(content_controls[2].kind, featherdoc::content_control_kind::table_row);
    CHECK_EQ(content_controls[2].text, "SKU-2");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_document_xml.c_str()));

    const auto body = saved_document.child("w:document").child("w:body");
    const auto order_control = body.child("w:p").child("w:sdt");
    REQUIRE(order_control != pugi::xml_node{});
    CHECK(order_control.child("w:sdtPr").child("w:showingPlcHdr") ==
          pugi::xml_node{});
    const auto order_run = order_control.child("w:sdtContent").child("w:r");
    REQUIRE(order_run != pugi::xml_node{});
    CHECK_EQ(std::string{order_run.child("w:t").text().get()}, "INV-002");
    REQUIRE(order_run.child("w:br") != pugi::xml_node{});
    CHECK_EQ(std::string{order_run.last_child().text().get()}, "ready");

    const auto line_item_cell = body.child("w:tbl")
                                    .child("w:sdt")
                                    .child("w:sdtContent")
                                    .child("w:tr")
                                    .child("w:tc");
    REQUIRE(line_item_cell != pugi::xml_node{});
    CHECK_EQ(std::string{line_item_cell.child("w:p")
                             .child("w:r")
                             .child("w:t")
                             .text()
                             .get()},
             "SKU-2");
    CHECK_EQ(saved_document_xml.find("INV-001"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("SKU-1"), std::string::npos);

    CHECK_EQ(doc.replace_content_control_text_by_alias("missing", "noop"), 0U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(doc.replace_content_control_text_by_tag("", "noop"), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(doc.last_error().detail, "content control tag must not be empty");
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    fs::remove(target);
}

TEST_CASE("content controls can be replaced with rich paragraphs table rows and tables") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "content_controls_replace_rich.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:sdt>
      <w:sdtPr>
        <w:tag w:val="summary"/>
        <w:showingPlcHdr/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>old summary</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Line Items"/>
          <w:tag w:val="line_items"/>
        </w:sdtPr>
        <w:sdtContent>
          <w:tr>
            <w:trPr><w:cantSplit/></w:trPr>
            <w:tc>
              <w:tcPr><w:shd w:fill="AAAAAA"/></w:tcPr>
              <w:p><w:r><w:rPr><w:b/></w:rPr><w:t>template item</w:t></w:r></w:p>
            </w:tc>
            <w:tc>
              <w:tcPr><w:shd w:fill="BBBBBB"/></w:tcPr>
              <w:p><w:r><w:rPr><w:i/></w:rPr><w:t>template qty</w:t></w:r></w:p>
            </w:tc>
          </w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
    <w:sdt>
      <w:sdtPr><w:tag w:val="metrics"/></w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>old metrics</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_content_control_with_paragraphs_by_tag(
                 "summary", {"Executive summary", "Second paragraph"}),
             1U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(doc.body_template().replace_content_control_with_table_rows_by_alias(
                 "Line Items", {{"Apple", "2"}, {"Pear", "5"}}),
             1U);
    CHECK_FALSE(doc.last_error());
    CHECK_EQ(doc.replace_content_control_with_table_by_tag(
                 "metrics", {{"Metric", "Value"}, {"Quality", "Green"}}),
             1U);
    CHECK_FALSE(doc.last_error());

    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 3U);
    CHECK_EQ(controls[0].text, "Executive summarySecond paragraph");
    CHECK_FALSE(controls[0].showing_placeholder);
    CHECK_EQ(controls[1].kind, featherdoc::content_control_kind::table_row);
    CHECK_EQ(controls[1].text, "Apple2Pear5");
    CHECK_EQ(controls[2].text, "MetricValueQualityGreen");

    CHECK_FALSE(doc.save());

    const auto saved_document_xml = read_test_docx_entry(target, test_document_xml_entry);
    CHECK_EQ(saved_document_xml.find("old summary"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template item"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("template qty"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("old metrics"), std::string::npos);
    CHECK_EQ(saved_document_xml.find("w:showingPlcHdr"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Executive summary"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Second paragraph"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Apple"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Pear"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Metric"), std::string::npos);
    CHECK_NE(saved_document_xml.find("Green"), std::string::npos);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:fill=\"AAAAAA\""),
             2U);
    CHECK_EQ(count_substring_occurrences(saved_document_xml, "w:fill=\"BBBBBB\""),
             2U);
    pugi::xml_document saved_document;
    REQUIRE(saved_document.load_string(saved_document_xml.c_str()));
    const auto body = saved_document.child("w:document").child("w:body");
    REQUIRE(body != pugi::xml_node{});
    const auto summary_content = body.child("w:sdt").child("w:sdtContent");
    REQUIRE(summary_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(summary_content, "w:p"), 2U);
    const auto metrics_content = body.last_child().child("w:sdtContent");
    REQUIRE(metrics_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(metrics_content, "w:tbl"), 1U);
    const auto row_control_content = body.child("w:tbl")
                                         .child("w:sdt")
                                         .child("w:sdtContent");
    REQUIRE(row_control_content != pugi::xml_node{});
    CHECK_EQ(count_named_children(row_control_content, "w:tr"), 2U);
    CHECK_EQ(count_named_descendants(row_control_content, "w:cantSplit"), 2U);
    CHECK_EQ(count_named_descendants(row_control_content, "w:b"), 2U);
    CHECK_EQ(count_named_descendants(row_control_content, "w:i"), 2U);

    featherdoc::Document reopened(target);
    CHECK_FALSE(reopened.open());
    const auto reopened_controls = reopened.list_content_controls();
    REQUIRE(reopened_controls.size() == 3U);
    CHECK_EQ(reopened_controls[0].text, "Executive summarySecond paragraph");
    CHECK_EQ(reopened_controls[1].text, "Apple2Pear5");
    CHECK_EQ(reopened_controls[2].text, "MetricValueQualityGreen");

    fs::remove(target);
}

TEST_CASE("content control rich replacement rejects incompatible run controls") {
    namespace fs = std::filesystem;

    const fs::path target = fs::current_path() / "content_controls_replace_rich_invalid.docx";
    fs::remove(target);

    const std::string document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Before </w:t></w:r>
      <w:sdt>
        <w:sdtPr><w:tag w:val="inline"/></w:sdtPr>
        <w:sdtContent><w:r><w:t>inline value</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
  </w:body>
</w:document>
)";
    write_test_docx(target, document_xml);

    featherdoc::Document doc(target);
    CHECK_FALSE(doc.open());

    CHECK_EQ(doc.replace_content_control_with_paragraphs_by_tag("inline", {"A"}),
             0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("paragraph replacement"), std::string::npos);
    CHECK_EQ(doc.last_error().entry_name, test_document_xml_entry);

    CHECK_EQ(doc.replace_content_control_with_table_by_tag("inline", {{"A"}}), 0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("table replacement"), std::string::npos);

    CHECK_EQ(doc.replace_content_control_with_table_rows_by_tag("inline", {{"A"}}),
             0U);
    CHECK_EQ(doc.last_error().code, std::make_error_code(std::errc::invalid_argument));
    CHECK_NE(doc.last_error().detail.find("table-row content control"),
             std::string::npos);
    const auto controls = doc.list_content_controls();
    REQUIRE(controls.size() == 1U);
    CHECK_EQ(controls.front().text, "inline value");
    CHECK_EQ(collect_document_text(doc), "Before \n");

    fs::remove(target);
}
