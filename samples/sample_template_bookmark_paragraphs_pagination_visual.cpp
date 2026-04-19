#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

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
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)"}; 

constexpr auto document_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)"}; 

constexpr auto header_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Retained header line should stay unchanged.</w:t></w:r>
  </w:p>
</w:hdr>
)"}; 

constexpr auto footer_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Retained footer line should stay unchanged.</w:t></w:r>
  </w:p>
</w:ftr>
)"}; 

void append_plain_paragraph(std::ostringstream &xml, std::string_view text) {
    xml << "<w:p><w:r><w:t>" << text << "</w:t></w:r></w:p>\n";
}

auto build_document_xml() -> std::string {
    std::ostringstream xml;
    xml << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
)";

    append_plain_paragraph(
        xml, "Template bookmark paragraphs pagination visual regression baseline.");
    append_plain_paragraph(
        xml,
        "This sample stays long enough to expose page 1 and page 2 before and after body bookmark replacement.");
    append_plain_paragraph(xml, "Body bookmark target:");
    xml << R"(    <w:p>
      <w:bookmarkStart w:id="0" w:name="body_note"/>
      <w:r><w:t>Body placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
)";
    append_plain_paragraph(
        xml,
        "This retained body paragraph should move downward but remain readable after the inserted paragraphs.");

    for (int index = 1; index <= 70; ++index) {
        xml << "<w:p><w:r><w:t>Pagination filler line " << index
            << " keeps the document long enough to expose page 1 and page 2.</w:t></w:r></w:p>\n";
    }

    xml << R"(    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440"
               w:header="720" w:footer="720" w:gutter="0"/>
    </w:sectPr>
  </w:body>
</w:document>
)";
    return xml.str();
}

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
    zip_t *zip = zip_openwitherror(path.string().c_str(), zip_compression_level,
                                   'w', &zip_error);
    if (zip == nullptr) {
        std::cerr << "failed to create archive: " << path.string()
                  << " (zip_error=" << zip_error << ")\n";
        return false;
    }

    const auto document_xml = build_document_xml();
    const bool ok =
        write_entry(zip, "_rels/.rels", root_relationships_xml) &&
        write_entry(zip, "[Content_Types].xml", content_types_xml) &&
        write_entry(zip, "word/document.xml", document_xml) &&
        write_entry(zip, "word/_rels/document.xml.rels", document_relationships_xml) &&
        write_entry(zip, "word/header1.xml", header_xml) &&
        write_entry(zip, "word/footer1.xml", footer_xml);
    zip_close(zip);
    return ok;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() /
                       "template_bookmark_paragraphs_pagination_visual_baseline.docx";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    if (!write_docx(output_path)) {
        return 1;
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
