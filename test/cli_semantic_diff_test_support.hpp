#pragma once

#include "cli_test_support.hpp"

#include <zip.h>

namespace {
void create_cli_semantic_diff_fixture(const fs::path &path,
                                      std::string_view status,
                                      std::string_view customer,
                                      std::string_view amount) {
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
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";
    const auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Semantic diff report</w:t></w:r></w:p>
    <w:p><w:r><w:t>Status: )"} + std::string{status} + R"(</w:t></w:r></w:p>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Customer Name"/>
        <w:tag w:val="customer_name"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>)" + std::string{customer} + R"(</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
    <w:tbl>
      <w:tr><w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc><w:tc><w:p><w:r><w:t>)" + std::string{amount} + R"(</w:t></w:r></w:p></w:tc></w:tr>
    </w:tbl>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}


void create_cli_semantic_diff_field_fixture(
    const fs::path &path, std::uint32_t toc_max_level,
    std::string_view reference_result, std::uint32_t sequence_restart,
    std::string_view sequence_result) {
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
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";
    const auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Semantic field diff report</w:t></w:r></w:p>
    <w:p><w:fldSimple w:instr=" TOC \o &quot;1-)"} +
                              std::to_string(toc_max_level) +
                              R"(&quot; \h \z \u "><w:r><w:t>TOC placeholder</w:t></w:r></w:fldSimple></w:p>
    <w:p><w:fldSimple w:instr=" REF target_bookmark \h \* MERGEFORMAT "><w:r><w:t>)" +
                              std::string{reference_result} +
                              R"(</w:t></w:r></w:fldSimple></w:p>
    <w:p><w:fldSimple w:instr=" SEQ Figure \* ARABIC \r )" +
                              std::to_string(sequence_restart) +
                              R"( \* MERGEFORMAT "><w:r><w:t>)" +
                              std::string{sequence_result} +
                              R"(</w:t></w:r></w:fldSimple></w:p>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}



void create_cli_semantic_diff_style_numbering_fixture(
    const fs::path &path, std::string_view style_name,
    std::string_view based_on, std::string_view numbering_name,
    featherdoc::list_kind numbering_kind, std::uint32_t numbering_start,
    std::string_view numbering_pattern) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("CLI semantic style and numbering diff fixture"));

    auto style = featherdoc::paragraph_style_definition{};
    style.name = std::string{style_name};
    style.based_on = std::string{based_on};
    style.is_quick_format = true;
    REQUIRE(document.ensure_paragraph_style("CliSemanticDiffHeading", style));

    auto numbering = featherdoc::numbering_definition{};
    numbering.name = std::string{numbering_name};
    numbering.levels = {featherdoc::numbering_level_definition{
        numbering_kind, numbering_start, 0U, std::string{numbering_pattern}}};
    const auto numbering_id = document.ensure_numbering_definition(numbering);
    REQUIRE(numbering_id.has_value());
    REQUIRE(document.set_paragraph_style_numbering("CliSemanticDiffHeading",
                                                   *numbering_id, 0U));
    REQUIRE_FALSE(document.save());
}


void create_cli_semantic_diff_part_fixture(
    const fs::path &path, std::string_view body_text,
    std::string_view header_text, std::string_view footer_text) {
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
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
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

    auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>)"};
    document_xml += body_text;
    document_xml += R"(</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    auto header_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>)"};
    header_xml += header_text;
    header_xml += R"(</w:t></w:r></w:p>
</w:hdr>
)";

    auto footer_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>)"};
    footer_xml += footer_text;
    footer_xml += R"(</w:t></w:r></w:p>
</w:ftr>
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
    REQUIRE(write_archive_entry(archive, "word/header1.xml", header_xml));
    REQUIRE(write_archive_entry(archive, "word/footer1.xml", footer_xml));
    zip_close(archive);
}


void create_cli_semantic_diff_paragraph_fixture(
    const fs::path &path, const std::vector<std::string_view> &paragraphs) {
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
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
)";

    auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
)"};
    for (const auto paragraph : paragraphs) {
        document_xml += "    <w:p><w:r><w:t>";
        document_xml += paragraph;
        document_xml += "</w:t></w:r></w:p>\n";
    }
    document_xml += R"(  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}
void create_cli_semantic_diff_review_fixture(
    const fs::path &path, std::string_view footnote_text,
    std::string_view endnote_text, std::string_view comment_text,
    std::string_view revision_text, std::string_view revision_author) {
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
    constexpr auto document_relationships_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
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

    auto document_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
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
      <w:ins w:id="5" w:author=")"};
    document_xml += revision_author;
    document_xml += R"(" w:date="2026-05-01T10:00:00Z"><w:r><w:t>)";
    document_xml += revision_text;
    document_xml += R"(</w:t></w:r></w:ins>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
)";

    auto footnotes_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="2"><w:p><w:r><w:t>)"};
    footnotes_xml += footnote_text;
    footnotes_xml += R"(</w:t></w:r></w:p></w:footnote>
</w:footnotes>
)";

    auto endnotes_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:endnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:endnote w:id="3"><w:p><w:r><w:t>)"};
    endnotes_xml += endnote_text;
    endnotes_xml += R"(</w:t></w:r></w:p></w:endnote>
</w:endnotes>
)";

    auto comments_xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-05-01T12:00:00Z">
    <w:p><w:r><w:t>)"};
    comments_xml += comment_text;
    comments_xml += R"(</w:t></w:r></w:p>
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
void create_cli_page_setup_fixture(const fs::path &path) {
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
</Types>
)";
    constexpr auto document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr>
        <w:sectPr>
          <w:pgSz w:w="12240" w:h="15840"/>
          <w:pgMar w:top="1440" w:right="1800" w:bottom="1440" w:left="1800" w:header="720" w:footer="720"/>
          <w:pgNumType w:start="5"/>
        </w:sectPr>
      </w:pPr>
      <w:r><w:t>portrait section</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>landscape section</w:t></w:r></w:p>
    <w:sectPr>
      <w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/>
      <w:pgMar w:top="720" w:right="1440" w:bottom="1080" w:left="1440" w:header="360" w:footer="540"/>
    </w:sectPr>
  </w:body>
</w:document>
)";

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE(write_archive_entry(archive, "_rels/.rels", relationships_xml));
    REQUIRE(write_archive_entry(archive, "[Content_Types].xml", content_types_xml));
    REQUIRE(write_archive_entry(archive, "word/document.xml", document_xml));
    zip_close(archive);
}
} // namespace
