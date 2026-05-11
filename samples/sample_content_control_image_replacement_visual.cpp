#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
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
</Types>
)"};

constexpr auto document_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r>
        <w:t>Content Control image replacement visual regression.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>The generated document should show a large block image, a small inline badge, and an image inside a table cell.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Block image content control below:</w:t></w:r>
    </w:p>
    <w:sdt>
      <w:sdtPr>
        <w:tag w:val="hero_image"/>
        <w:showingPlcHdr/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Hero image placeholder should disappear.</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Inline badge replacement: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Inline Badge"/>
          <w:showingPlcHdr/>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>Inline badge placeholder</w:t></w:r></w:sdtContent>
      </w:sdt>
      <w:r><w:t> should stay on this line.</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Table-cell image content control below:</w:t></w:r>
    </w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblW w:w="7200" w:type="dxa"/>
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
        <w:gridCol w:w="3600"/>
        <w:gridCol w:w="3600"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr><w:tcW w:w="3600" w:type="dxa"/></w:tcPr>
          <w:p><w:r><w:t>Image cell</w:t></w:r></w:p>
        </w:tc>
        <w:sdt>
          <w:sdtPr>
            <w:tag w:val="cell_image"/>
            <w:showingPlcHdr/>
          </w:sdtPr>
          <w:sdtContent>
            <w:tc>
              <w:tcPr><w:tcW w:w="3600" w:type="dxa"/></w:tcPr>
              <w:p><w:r><w:t>Cell image placeholder should disappear.</w:t></w:r></w:p>
            </w:tc>
          </w:sdtContent>
        </w:sdt>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:r>
        <w:t>Visual cues: all placeholder text must be absent; each replacement image should remain readable after Word PDF export.</w:t>
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
        std::cerr << "failed to create DOCX archive: " << path.string() << '\n';
        return false;
    }

    const bool ok =
        write_entry(zip, "_rels/.rels", root_relationships_xml) &&
        write_entry(zip, "[Content_Types].xml", content_types_xml) &&
        write_entry(zip, "word/document.xml", document_xml);
    zip_close(zip);
    return ok;
}

bool replace_image_content_controls(const fs::path &path, const fs::path &image_path) {
    featherdoc::Document document(path);
    if (document.open()) {
        std::cerr << "failed to open seed DOCX: " << document.last_error().detail << '\n';
        return false;
    }

    if (document.replace_content_control_with_image_by_tag("hero_image", image_path,
                                                           240U, 120U) != 1U) {
        std::cerr << "failed to replace block image content control: "
                  << document.last_error().detail << '\n';
        return false;
    }

    if (document.replace_content_control_with_image_by_alias("Inline Badge", image_path,
                                                             36U, 36U) != 1U) {
        std::cerr << "failed to replace inline image content control: "
                  << document.last_error().detail << '\n';
        return false;
    }

    auto body_template = document.body_template();
    if (!body_template ||
        body_template.replace_content_control_with_image_by_tag("cell_image", image_path,
                                                                144U, 72U) != 1U) {
        std::cerr << "failed to replace table-cell image content control: "
                  << document.last_error().detail << '\n';
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
                       "content_control_image_replacement_visual.docx";
    const fs::path image_path =
        argc > 2 ? fs::path(argv[2]) : fs::current_path() / "content_control_image.png";

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
    if (!replace_image_content_controls(output_path, image_path)) {
        return 1;
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
