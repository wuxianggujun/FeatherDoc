#pragma once

#include "cli_test_support.hpp"

#include <zip.h>

namespace {
void create_cli_review_inspection_fixture(const fs::path &path) {
    remove_if_exists(path);

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
  <Override PartName="/word/footnotes.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"/>
  <Override PartName="/word/endnotes.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"/>
  <Override PartName="/word/comments.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"/>
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math">
  <w:body>
    <w:p>
      <w:hyperlink r:id="rHyperlink">
        <w:r><w:t>Open docs</w:t></w:r>
      </w:hyperlink>
    </w:p>
    <w:p>
      <w:r><w:t>Footnote mark</w:t></w:r>
      <w:r><w:footnoteReference w:id="2"/></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Endnote mark</w:t></w:r>
      <w:r><w:endnoteReference w:id="3"/></w:r>
    </w:p>
    <w:p>
      <w:commentRangeStart w:id="4"/>
      <w:r><w:t>Commented text</w:t></w:r>
      <w:commentRangeEnd w:id="4"/>
      <w:r><w:commentReference w:id="4"/></w:r>
    </w:p>
    <w:p>
      <w:ins w:id="5" w:author="Ada" w:date="2026-04-01T00:00:00Z">
        <w:r><w:t>Inserted review text</w:t></w:r>
      </w:ins>
    </w:p>
    <w:p>
      <w:del w:id="6" w:author="Ada" w:date="2026-04-02T00:00:00Z">
        <w:r><w:delText>Deleted review text</w:delText></w:r>
      </w:del>
    </w:p>
    <w:p>
      <m:oMath><m:r><m:t>x+1</m:t></m:r></m:oMath>
    </w:p>
    <m:oMathPara>
      <m:oMath><m:r><m:t>y=2</m:t></m:r></m:oMath>
    </m:oMathPara>
    <w:sectPr/>
  </w:body>
</w:document>
)";
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rHyperlink"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink"
                Target="https://example.com/docs"
                TargetMode="External"/>
  <Relationship Id="rFootnotes"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes"
                Target="footnotes.xml"/>
  <Relationship Id="rEndnotes"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes"
                Target="endnotes.xml"/>
  <Relationship Id="rComments"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments"
                Target="comments.xml"/>
</Relationships>
)";
    constexpr auto footnotes_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="2">
    <w:p><w:r><w:t>Footnote body</w:t></w:r></w:p>
  </w:footnote>
</w:footnotes>
)";
    constexpr auto endnotes_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:endnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:endnote w:id="3">
    <w:p><w:r><w:t>Endnote body</w:t></w:r></w:p>
  </w:endnote>
</w:endnotes>
)";
    constexpr auto comments_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-04-03T00:00:00Z">
    <w:p><w:r><w:t>Reviewer note</w:t></w:r></w:p>
  </w:comment>
</w:comments>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    REQUIRE(write_archive_entry(archive, "word/_rels/document.xml.rels",
                                document_relationships_xml));
    REQUIRE(write_archive_entry(archive, "word/footnotes.xml", footnotes_xml));
    REQUIRE(write_archive_entry(archive, "word/endnotes.xml", endnotes_xml));
    REQUIRE(write_archive_entry(archive, "word/comments.xml", comments_xml));
    zip_close(archive);
}

} // namespace
