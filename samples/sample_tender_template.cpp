#include <filesystem>
#include <iostream>
#include <string_view>

#include <zip.h>

namespace fs = std::filesystem;

namespace {

constexpr auto relationships_xml_entry = "_rels/.rels";
constexpr auto content_types_xml_entry = "[Content_Types].xml";
constexpr auto document_xml_entry = "word/document.xml";

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
    <w:p><w:r><w:t>Tender Template Smoke Fixture</w:t></w:r></w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Tender title: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="tender_title"/>
      <w:r><w:t>Procurement Tender</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Issuing organization: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="issuing_organization"/>
      <w:r><w:t>Procurement Office</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:p>
      <w:r><w:t xml:space="preserve">Submission deadline: </w:t></w:r>
      <w:bookmarkStart w:id="2" w:name="submission_deadline"/>
      <w:r><w:t>2026-09-30 17:00 UTC</w:t></w:r>
      <w:bookmarkEnd w:id="2"/>
    </w:p>
    <w:p><w:r><w:t>Procurement items</w:t></w:r></w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblW w:w="9000" w:type="dxa"/>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="1800"/>
        <w:gridCol w:w="4200"/>
        <w:gridCol w:w="3000"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Item</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Requirement</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Evaluation focus</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="3" w:name="tender_item_row"/>
            <w:r><w:t>DOCX template automation</w:t></w:r>
            <w:bookmarkEnd w:id="3"/>
          </w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>Provide a reproducible document generation workflow.</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Quality, delivery risk, and traceability.</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>Evaluation criteria</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="4" w:name="evaluation_criteria"/>
      <w:r><w:t>Technical fit, delivery plan, and evidence quality.</w:t></w:r>
      <w:bookmarkEnd w:id="4"/>
    </w:p>
    <w:p><w:r><w:t>Submission instructions</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="5" w:name="submission_instructions"/>
      <w:r><w:t>Submit the proposal package before the stated deadline.</w:t></w:r>
      <w:bookmarkEnd w:id="5"/>
    </w:p>
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440"
               w:header="720" w:footer="720" w:gutter="0"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

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

bool write_docx(const fs::path &path) {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    if (zip == nullptr) {
        std::cerr << "failed to create archive: " << path.string()
                  << " (zip_error=" << zip_error << ")\n";
        return false;
    }

    const bool ok =
        write_entry(zip, content_types_xml_entry, content_types_xml) &&
        write_entry(zip, relationships_xml_entry, relationships_xml) &&
        write_entry(zip, document_xml_entry, document_xml);
    zip_close(zip);
    return ok;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_docx =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "output" / "project-template-smoke-sample" /
                       "tender_template.docx";
    const fs::path output_dir = output_docx.parent_path();

    std::error_code directory_error;
    fs::create_directories(output_dir, directory_error);
    if (directory_error) {
        std::cerr << "failed to create output directory: "
                  << output_dir.string() << '\n';
        return 1;
    }

    fs::remove(output_docx, directory_error);
    if (!write_docx(output_docx)) {
        return 1;
    }

    std::cout << "wrote tender template fixture to "
              << output_docx.string() << '\n';
    return 0;
}
