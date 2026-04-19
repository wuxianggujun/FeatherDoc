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
        <w:t>Bookmark-based template table visual regression baseline.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>The blue table must remain unchanged. The green table below is the bookmark-selected target.</w:t>
      </w:r>
    </w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblW w:w="6400" w:type="dxa"/>
        <w:tblLayout w:type="fixed"/>
        <w:jc w:val="center"/>
        <w:tblBorders>
          <w:top w:val="single" w:sz="8" w:color="666666"/>
          <w:left w:val="single" w:sz="8" w:color="666666"/>
          <w:bottom w:val="single" w:sz="8" w:color="666666"/>
          <w:right w:val="single" w:sz="8" w:color="666666"/>
          <w:insideH w:val="single" w:sz="8" w:color="666666"/>
          <w:insideV w:val="single" w:sz="8" w:color="666666"/>
        </w:tblBorders>
        <w:tblCellMar>
          <w:left w:w="96" w:type="dxa"/>
          <w:right w:w="96" w:type="dxa"/>
        </w:tblCellMar>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="2600"/>
        <w:gridCol w:w="1600"/>
        <w:gridCol w:w="2200"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="2600" w:type="dxa"/>
            <w:shd w:val="clear" w:color="auto" w:fill="D9EAF7"/>
          </w:tcPr>
          <w:p><w:r><w:t>Retained</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="1600" w:type="dxa"/>
            <w:shd w:val="clear" w:color="auto" w:fill="D9EAF7"/>
          </w:tcPr>
          <w:p><w:r><w:t>Qty</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="2200" w:type="dxa"/>
            <w:shd w:val="clear" w:color="auto" w:fill="D9EAF7"/>
          </w:tcPr>
          <w:p><w:r><w:t>Amount</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:tcPr><w:tcW w:w="2600" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>keep-left</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="1600" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>4</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="2200" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>40</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="target_before_table"/>
      <w:r><w:t>Bookmark paragraph immediately before the target table.</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblW w:w="6400" w:type="dxa"/>
        <w:tblLayout w:type="fixed"/>
        <w:jc w:val="center"/>
        <w:tblBorders>
          <w:top w:val="single" w:sz="8" w:color="666666"/>
          <w:left w:val="single" w:sz="8" w:color="666666"/>
          <w:bottom w:val="single" w:sz="8" w:color="666666"/>
          <w:right w:val="single" w:sz="8" w:color="666666"/>
          <w:insideH w:val="single" w:sz="8" w:color="666666"/>
          <w:insideV w:val="single" w:sz="8" w:color="666666"/>
        </w:tblBorders>
        <w:tblCellMar>
          <w:left w:w="96" w:type="dxa"/>
          <w:right w:w="96" w:type="dxa"/>
        </w:tblCellMar>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="2600"/>
        <w:gridCol w:w="1600"/>
        <w:gridCol w:w="2200"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="2600" w:type="dxa"/>
            <w:shd w:val="clear" w:color="auto" w:fill="E2F0D9"/>
          </w:tcPr>
          <w:p><w:r><w:t>Region</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="1600" w:type="dxa"/>
            <w:shd w:val="clear" w:color="auto" w:fill="E2F0D9"/>
          </w:tcPr>
          <w:p><w:r><w:t>Qty</w:t></w:r></w:p>
        </w:tc>
        <w:tc>
          <w:tcPr>
            <w:tcW w:w="2200" w:type="dxa"/>
            <w:shd w:val="clear" w:color="auto" w:fill="E2F0D9"/>
          </w:tcPr>
          <w:p><w:r><w:t>Amount</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
      <w:tr>
        <w:tc>
          <w:tcPr><w:tcW w:w="2600" w:type="dxa"/></w:tcPr>
          <w:p>
            <w:bookmarkStart w:id="1" w:name="target_inside_table"/>
            <w:r><w:t>North seed</w:t></w:r>
            <w:bookmarkEnd w:id="1"/>
          </w:p>
        </w:tc>
        <w:tc><w:tcPr><w:tcW w:w="1600" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>10</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="2200" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>100</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:tcPr><w:tcW w:w="2600" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>South seed</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="1600" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>20</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="2200" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>200</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:r>
        <w:t>The target table should keep visible borders and remain separated from this paragraph after mutation.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>This baseline is intentionally simple so before/after differences stay obvious in rendered screenshots.</w:t>
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

bool write_docx(const fs::path &path) {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(), zip_compression_level,
                                   'w', &zip_error);
    if (zip == nullptr) {
        std::cerr << "failed to create archive: " << path.string()
                  << " (zip_error=" << zip_error << ")\n";
        return false;
    }

    const bool ok = write_entry(zip, "_rels/.rels", root_relationships_xml) &&
                    write_entry(zip, "[Content_Types].xml", content_types_xml) &&
                    write_entry(zip, "word/document.xml", document_xml);
    zip_close(zip);
    return ok;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() /
                       "template_table_cli_bookmark_visual_baseline.docx";

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
