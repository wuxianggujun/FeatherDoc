#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include <zip.h>

namespace fs = std::filesystem;

namespace {

constexpr auto relationships_xml_entry = "_rels/.rels";
constexpr auto content_types_xml_entry = "[Content_Types].xml";
constexpr auto document_xml_entry = "word/document.xml";
constexpr auto document_relationships_xml_entry = "word/_rels/document.xml.rels";
constexpr auto header_xml_entry = "word/header1.xml";
constexpr auto footer_xml_entry = "word/footer1.xml";
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
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";
constexpr auto document_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>Template schema manifest fixture</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
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
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)";
constexpr auto header_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Header title: </w:t></w:r>
    <w:bookmarkStart w:id="0" w:name="header_title"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_note"/>
    <w:r><w:t>standalone header note</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
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
          <w:bookmarkStart w:id="2" w:name="header_rows"/>
          <w:r><w:t>row placeholder</w:t></w:r>
          <w:bookmarkEnd w:id="2"/>
        </w:p>
      </w:tc>
      <w:tc><w:p><w:r><w:t>0</w:t></w:r></w:p></w:tc>
    </w:tr>
  </w:tbl>
</w:hdr>
)";
constexpr auto footer_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Footer company A: </w:t></w:r>
    <w:bookmarkStart w:id="3" w:name="footer_company"/>
    <w:r><w:t>placeholder A</w:t></w:r>
    <w:bookmarkEnd w:id="3"/>
  </w:p>
  <w:p>
    <w:r><w:t>Footer company B: </w:t></w:r>
    <w:bookmarkStart w:id="4" w:name="footer_company"/>
    <w:r><w:t>placeholder B</w:t></w:r>
    <w:bookmarkEnd w:id="4"/>
  </w:p>
  <w:p>
    <w:r><w:t>Summary: prefix </w:t></w:r>
    <w:bookmarkStart w:id="5" w:name="footer_summary"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="5"/>
    <w:r><w:t> suffix</w:t></w:r>
  </w:p>
</w:ftr>
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
        write_entry(zip, document_xml_entry, document_xml) &&
        write_entry(zip, document_relationships_xml_entry, document_relationships_xml) &&
        write_entry(zip, header_xml_entry, header_xml) &&
        write_entry(zip, footer_xml_entry, footer_xml);
    zip_close(zip);
    return ok;
}

bool append_body_paragraph(featherdoc::Document &document, std::string_view text) {
    auto paragraph = document.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    return paragraph.insert_paragraph_after(std::string{text}).has_next();
}

void print_document_error(const featherdoc::Document &doc, std::string_view operation) {
    const auto &error_info = doc.last_error();
    std::cerr << operation << " failed";
    if (error_info.code) {
        std::cerr << ": " << error_info.code.message();
    }
    if (!error_info.detail.empty()) {
        std::cerr << " - " << error_info.detail;
    }
    if (!error_info.entry_name.empty()) {
        std::cerr << " [entry=" << error_info.entry_name << ']';
    }
    if (error_info.xml_offset.has_value()) {
        std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
    }
    std::cerr << '\n';
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_docx =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "output" / "template-schema-manifest" /
                       "template_schema_resolved_fixture.docx";
    const fs::path output_dir = output_docx.parent_path();

    std::error_code directory_error;
    fs::create_directories(output_dir, directory_error);
    if (directory_error) {
        std::cerr << "failed to create output directory: "
                  << output_dir.string() << '\n';
        return 1;
    }

    fs::remove(output_docx, directory_error);
    directory_error.clear();

    if (!write_docx(output_docx)) {
        return 1;
    }

    featherdoc::Document document(output_docx);
    if (const auto error = document.open()) {
        print_document_error(document, "open");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    if (!document.append_section(false)) {
        print_document_error(document, "append_section");
        return 1;
    }

    if (!append_body_paragraph(document, "section 1 body")) {
        std::cerr << "failed to append section body paragraph\n";
        return 1;
    }

    if (document.section_count() != 2U) {
        std::cerr << "expected 2 sections but found " << document.section_count()
                  << '\n';
        return 1;
    }

    if (const auto error = document.save()) {
        print_document_error(document, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << "wrote template schema manifest fixture to "
              << output_docx.string() << '\n';
    return 0;
}
