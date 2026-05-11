#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>

namespace fs = std::filesystem;

namespace {
constexpr auto zip_compression_level = 0;
constexpr auto root_relationships_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>
)"};
constexpr auto content_types_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/footnotes.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"/>
  <Override PartName="/word/endnotes.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"/>
  <Override PartName="/word/comments.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"/>
</Types>
)"};
constexpr auto document_relationships_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rFootnotes" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes" Target="footnotes.xml"/>
  <Relationship Id="rEndnotes" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes" Target="endnotes.xml"/>
  <Relationship Id="rComments" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments" Target="comments.xml"/>
</Relationships>
)"};
constexpr auto document_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Review inspection visual regression.</w:t></w:r></w:p>
    <w:p><w:r><w:t>Footnote marker appears here</w:t></w:r><w:r><w:footnoteReference w:id="2"/></w:r></w:p>
    <w:p><w:r><w:t>Endnote marker appears here</w:t></w:r><w:r><w:endnoteReference w:id="3"/></w:r></w:p>
    <w:p><w:commentRangeStart w:id="4"/><w:r><w:t>Commented text with review metadata</w:t></w:r><w:commentRangeEnd w:id="4"/><w:r><w:commentReference w:id="4"/></w:r></w:p>
    <w:p><w:ins w:id="5" w:author="Ada" w:date="2026-05-01T10:00:00Z"><w:r><w:t>Inserted revision text</w:t></w:r></w:ins></w:p>
    <w:p><w:del w:id="6" w:author="Grace" w:date="2026-05-01T11:00:00Z"><w:r><w:delText>Deleted revision text</w:delText></w:r></w:del></w:p>
    <w:sectPr><w:pgSz w:w="12240" w:h="15840"/><w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440" w:header="720" w:footer="720" w:gutter="0"/></w:sectPr>
  </w:body>
</w:document>
)"};
constexpr auto footnotes_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="2"><w:p><w:r><w:t>Visual footnote body text</w:t></w:r></w:p></w:footnote>
</w:footnotes>
)"};
constexpr auto endnotes_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:endnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:endnote w:id="3"><w:p><w:r><w:t>Visual endnote body text</w:t></w:r></w:p></w:endnote>
</w:endnotes>
)"};
constexpr auto comments_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-05-01T12:00:00Z"><w:p><w:r><w:t>Visual comment body text</w:t></w:r></w:p></w:comment>
</w:comments>
)"};
bool write_entry(zip_t *zip, const char *entry_name, std::string_view content) {
    if (zip_entry_open(zip, entry_name) != 0) { std::cerr << "failed to open " << entry_name << '\n'; return false; }
    if (zip_entry_write(zip, content.data(), content.size()) < 0) { zip_entry_close(zip); return false; }
    return zip_entry_close(zip) == 0;
}
bool write_docx(const fs::path &path) {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(), zip_compression_level, 'w', &zip_error);
    if (zip == nullptr) { return false; }
    const bool ok = write_entry(zip, "_rels/.rels", root_relationships_xml) &&
                    write_entry(zip, "[Content_Types].xml", content_types_xml) &&
                    write_entry(zip, "word/document.xml", document_xml) &&
                    write_entry(zip, "word/_rels/document.xml.rels", document_relationships_xml) &&
                    write_entry(zip, "word/footnotes.xml", footnotes_xml) &&
                    write_entry(zip, "word/endnotes.xml", endnotes_xml) &&
                    write_entry(zip, "word/comments.xml", comments_xml);
    zip_close(zip);
    return ok;
}
bool verify_api(const fs::path &path) {
    featherdoc::Document document(path);
    if (document.open()) { return false; }
    const auto comments = document.list_comments();
    return document.list_footnotes().size() == 1U &&
           document.list_endnotes().size() == 1U && comments.size() == 1U &&
           comments.front().anchor_text == std::optional<std::string>{"Commented text with review metadata"} &&
           document.list_revisions().size() == 2U;
}
} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1 ? fs::path(argv[1]) : fs::current_path() / "review_inspection_visual.docx";
    if (output_path.has_parent_path()) { std::error_code ec; fs::create_directories(output_path.parent_path(), ec); }
    std::error_code remove_error; fs::remove(output_path, remove_error);
    if (!write_docx(output_path)) { std::cerr << "failed to write DOCX\n"; return 1; }
    if (!verify_api(output_path)) { std::cerr << "review inspection API verification failed\n"; return 1; }
    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
