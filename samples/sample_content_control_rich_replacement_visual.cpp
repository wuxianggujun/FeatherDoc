#include <featherdoc.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr auto zip_compression_level = 0;

constexpr auto root_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)"};

constexpr auto content_types_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml"
           ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)"};

constexpr auto custom_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<invoice xmlns="urn:featherdoc">
  <dueDate>Due date: 2026-07-15</dueDate>
</invoice>
)"};

constexpr auto custom_xml_item_props = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ds:datastoreItem ds:itemID="{55555555-5555-5555-5555-555555555555}"
                  xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml">
  <ds:schemaRefs/>
</ds:datastoreItem>
)"};

constexpr auto custom_xml_relationships = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps"
                Target="itemProps1.xml"/>
</Relationships>
)"};

constexpr auto document_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:w14="http://schemas.microsoft.com/office/word/2010/wordml">
  <w:body>
    <w:p>
      <w:r>
        <w:t>Content Control rich replacement visual regression.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>The generated document should show two summary paragraphs, two cloned item rows, and a generated metrics table.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Block content control below becomes multiple paragraphs:</w:t></w:r>
    </w:p>
    <w:sdt>
      <w:sdtPr>
        <w:tag w:val="summary"/>
        <w:showingPlcHdr/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Summary placeholder should disappear.</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Row content control below expands inside the existing bordered table:</w:t></w:r>
    </w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblW w:w="7200" w:type="dxa"/>
        <w:tblLayout w:type="fixed"/>
        <w:tblBorders>
          <w:top w:val="single" w:sz="8" w:space="0" w:color="666666"/>
          <w:left w:val="single" w:sz="8" w:space="0" w:color="666666"/>
          <w:bottom w:val="single" w:sz="8" w:space="0" w:color="666666"/>
          <w:right w:val="single" w:sz="8" w:space="0" w:color="666666"/>
          <w:insideH w:val="single" w:sz="8" w:space="0" w:color="666666"/>
          <w:insideV w:val="single" w:sz="8" w:space="0" w:color="666666"/>
        </w:tblBorders>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="4800"/>
        <w:gridCol w:w="2400"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="4800" w:type="dxa"/>
            <w:shd w:val="clear" w:color="auto" w:fill="D9EAF7"/>
          </w:tcPr>
          <w:p><w:r><w:t>Item</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="2400" w:type="dxa"/>
            <w:shd w:val="clear" w:color="auto" w:fill="D9EAF7"/>
          </w:tcPr>
          <w:p><w:r><w:t>Status</w:t></w:r></w:p>
        </w:tc>
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
              <w:tcPr>
                <w:tcW w:w="4800" w:type="dxa"/>
                <w:shd w:val="clear" w:color="auto" w:fill="FCE4D6"/>
              </w:tcPr>
              <w:p><w:r><w:rPr><w:b/></w:rPr><w:t>Template item</w:t></w:r></w:p>
            </w:tc>
            <w:tc>
              <w:tcPr>
                <w:tcW w:w="2400" w:type="dxa"/>
                <w:shd w:val="clear" w:color="auto" w:fill="E2F0D9"/>
              </w:tcPr>
              <w:p><w:r><w:rPr><w:i/></w:rPr><w:t>Template status</w:t></w:r></w:p>
            </w:tc>
          </w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
    <w:p>
      <w:r><w:t>Block content control below becomes a generated table:</w:t></w:r>
    </w:p>
    <w:sdt>
      <w:sdtPr>
        <w:tag w:val="metrics"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Metrics placeholder should disappear.</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Form content-control mutation state below should show final values:</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Approved checkbox: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Approved"/>
          <w:tag w:val="approved"/>
          <w:lock w:val="sdtContentLocked"/>
          <w14:checkbox><w14:checked w14:val="1"/></w14:checkbox>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>☒</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:p>
      <w:r><w:t>Status dropdown: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Status"/>
          <w:tag w:val="status"/>
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
        <w:date><w:dateFormat w:val="yyyy-MM-dd"/><w:lid w:val="en-US"/></w:date>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Due date: 2026-05-01</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r>
        <w:t>Visual cues: placeholder text must be absent; generated summary, rows, metrics, unchecked checkbox, draft dropdown, and updated date text must remain readable after Word PDF export.</w:t>
      </w:r>
    </w:p>
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440"
               w:header="720" w:footer="720" w:gutter="0"/>
    </w:sectPr>
  </w:body>
</w:document>
)"};

bool write_entry(zip_t *zip, const char *entry_name, std::string_view content) {
    if (zip_entry_open(zip, entry_name) != 0) {
        std::cerr << "failed to open archive entry: " << entry_name << '\n';
        return false;
    }
    if (zip_entry_write(zip, content.data(), content.size()) < 0) {
        std::cerr << "failed to write archive entry: " << entry_name << '\n';
        zip_entry_close(zip);
        return false;
    }
    if (zip_entry_close(zip) != 0) {
        std::cerr << "failed to close archive entry: " << entry_name << '\n';
        return false;
    }
    return true;
}

bool write_seed_docx(const fs::path &path) {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(), zip_compression_level,
                                   'w', &zip_error);
    if (zip == nullptr) {
        std::cerr << "failed to create archive: " << path.string()
                  << " (zip_error=" << zip_error << ")\n";
        return false;
    }

    const bool ok =
        write_entry(zip, "_rels/.rels", root_relationships_xml) &&
        write_entry(zip, "[Content_Types].xml", content_types_xml) &&
        write_entry(zip, "word/document.xml", document_xml) &&
        write_entry(zip, "customXml/item1.xml", custom_xml) &&
        write_entry(zip, "customXml/itemProps1.xml", custom_xml_item_props) &&
        write_entry(zip, "customXml/_rels/item1.xml.rels", custom_xml_relationships);
    zip_close(zip);
    return ok;
}

bool replace_rich_content_controls(const fs::path &path) {
    featherdoc::Document document(path);
    if (document.open()) {
        std::cerr << "failed to open seed DOCX: " << document.last_error().detail << '\n';
        return false;
    }

    if (document.replace_content_control_with_paragraphs_by_tag(
            "summary", {"Executive summary: content controls now support typed rich replacement.",
                        "Second paragraph: the old placeholder flag and text are removed."}) != 1U) {
        std::cerr << "failed to replace summary content control: "
                  << document.last_error().detail << '\n';
        return false;
    }

    if (document.body_template().replace_content_control_with_table_rows_by_alias(
            "Line Items", {{"Paragraph replacement", "PASS"},
                           {"Table-row replacement", "PASS"}}) != 1U) {
        std::cerr << "failed to replace row content control: "
                  << document.last_error().detail << '\n';
        return false;
    }

    if (document.replace_content_control_with_table_by_tag(
            "metrics", {{"Capability", "Visual cue"},
                        {"Block SDT to table", "Generated cells are visible"},
                        {"Row SDT clone", "Fills and run styles are preserved"}}) != 1U) {
        std::cerr << "failed to replace metrics content control: "
                  << document.last_error().detail << '\n';
        return false;
    }

    featherdoc::content_control_form_state_options checkbox_options;
    checkbox_options.clear_lock = true;
    checkbox_options.checked = false;
    if (document.set_content_control_form_state_by_tag("approved",
                                                       checkbox_options) != 1U) {
        std::cerr << "failed to update checkbox content control: "
                  << document.last_error().detail << '\n';
        return false;
    }

    featherdoc::content_control_form_state_options dropdown_options;
    dropdown_options.lock = "sdtLocked";
    dropdown_options.selected_list_item = "draft";
    if (document.set_content_control_form_state_by_alias("Status",
                                                         dropdown_options) != 1U) {
        std::cerr << "failed to update dropdown content control: "
                  << document.last_error().detail << '\n';
        return false;
    }

    featherdoc::content_control_form_state_options date_options;
    date_options.date_text = "Due date: 2026-06-01";
    date_options.date_format = "yyyy/MM/dd";
    date_options.date_locale = "zh-CN";
    date_options.data_binding_store_item_id =
        "{55555555-5555-5555-5555-555555555555}";
    date_options.data_binding_xpath = "/invoice/dueDate";
    date_options.data_binding_prefix_mappings =
        "xmlns:fd=\"urn:featherdoc\"";
    if (document.body_template().set_content_control_form_state_by_tag(
            "due_date", date_options) != 1U) {
        std::cerr << "failed to update date content control: "
                  << document.last_error().detail << '\n';
        return false;
    }

    const auto sync_result = document.sync_content_controls_from_custom_xml();
    if (!sync_result.has_value() || sync_result->synced_content_controls != 1U ||
        sync_result->has_issues()) {
        std::cerr << "failed to synchronize data-bound content control from Custom XML: "
                  << document.last_error().detail << '\n';
        return false;
    }

    const auto controls = document.list_content_controls();
    const auto has_checkbox = std::any_of(
        controls.begin(), controls.end(), [](const auto &control) {
            return control.form_kind ==
                       featherdoc::content_control_form_kind::checkbox &&
                   control.checked.has_value() && !*control.checked &&
                   !control.lock.has_value() && control.text == "☐";
        });
    const auto has_dropdown = std::any_of(
        controls.begin(), controls.end(), [](const auto &control) {
            return control.form_kind ==
                       featherdoc::content_control_form_kind::drop_down_list &&
                   control.selected_list_item.value_or(1U) == 0U &&
                   control.list_items.size() == 2U && control.text == "Draft" &&
                   control.lock.value_or(std::string{}) == "sdtLocked";
        });
    const auto has_date = std::any_of(
        controls.begin(), controls.end(), [](const auto &control) {
            return control.form_kind == featherdoc::content_control_form_kind::date &&
                   control.date_format.value_or(std::string{}) == "yyyy/MM/dd" &&
                   control.date_locale.value_or(std::string{}) == "zh-CN" &&
                   control.data_binding_store_item_id.value_or(std::string{}) ==
                       "{55555555-5555-5555-5555-555555555555}" &&
                   control.data_binding_xpath.value_or(std::string{}) ==
                       "/invoice/dueDate" &&
                   control.data_binding_prefix_mappings.value_or(std::string{}) ==
                       "xmlns:fd=\"urn:featherdoc\"" &&
                   control.text == "Due date: 2026-07-15";
        });
    if (!has_checkbox || !has_dropdown || !has_date) {
        std::cerr << "failed to inspect mutated content-control form state metadata\n";
        return false;
    }

    if (document.save()) {
        std::cerr << "failed to save generated DOCX: " << document.last_error().detail << '\n';
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() /
                       "content_control_rich_replacement_visual.docx";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    std::error_code remove_error;
    fs::remove(output_path, remove_error);

    if (!write_seed_docx(output_path)) {
        return 1;
    }
    if (!replace_rich_content_controls(output_path)) {
        return 1;
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
