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
        <w:t>Bookmark floating image visual regression baseline.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>The bookmark placeholder below should become a visible anchored image near the right margin while the surrounding body text stays readable.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>This retained line should stay above the anchored image and makes vertical placement easy to compare.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="body_logo"/>
      <w:r><w:t>body floating placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r>
        <w:t>This deliberately long paragraph should wrap beside the anchored image on the left side of the page before continuing below it. The wording is intentionally extended so the rendered evidence makes floating layout, spacing, and bookmark replacement order easy to compare.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>A second retained paragraph continues after the wrapped region and helps confirm the anchored drawing does not reorder surrounding content when the placeholder paragraph is removed.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>The final retained paragraph should remain below the wrapped text so the rendered evidence keeps a clear reading order.</w:t>
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

    const bool ok =
        write_entry(zip, "_rels/.rels", root_relationships_xml) &&
        write_entry(zip, "[Content_Types].xml", content_types_xml) &&
        write_entry(zip, "word/document.xml", document_xml);
    zip_close(zip);
    return ok;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "bookmark_floating_image_visual_baseline.docx";

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
