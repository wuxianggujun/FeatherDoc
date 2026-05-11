#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string>
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
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/numbering.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
  <Override PartName="/word/header1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footnotes.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footnotes+xml"/>
  <Override PartName="/word/endnotes.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.endnotes+xml"/>
  <Override PartName="/word/comments.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"/>
</Types>
)"};

constexpr auto document_relationships_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header" Target="header1.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer" Target="footer1.xml"/>
  <Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
  <Relationship Id="rId5" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering" Target="numbering.xml"/>
  <Relationship Id="rId6" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes" Target="footnotes.xml"/>
  <Relationship Id="rId7" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes" Target="endnotes.xml"/>
  <Relationship Id="rId8" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments" Target="comments.xml"/>
</Relationships>
)"};

bool write_entry(zip_t *zip, const char *entry_name, std::string_view content) {
    if (zip_entry_open(zip, entry_name) != 0) {
        std::cerr << "failed to open " << entry_name << '\n';
        return false;
    }
    if (zip_entry_write(zip, content.data(), content.size()) < 0) {
        zip_entry_close(zip);
        return false;
    }
    return zip_entry_close(zip) == 0;
}

std::string make_document_xml(std::string_view title, std::string_view status,
                              std::string_view customer,
                              std::string_view amount,
                              std::string_view inserted_note,
                              std::string_view footer_note,
                              std::string_view revision_author,
                              bool landscape_section) {
    auto xml = std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:pPr><w:pStyle w:val="SemanticDiffHeading"/></w:pPr><w:r><w:t>)"};
    xml += title;
    xml += R"(</w:t></w:r></w:p>
    <w:p><w:r><w:t>Status: )";
    xml += status;
    xml += R"(</w:t></w:r></w:p>
    <w:p><w:r><w:t>Stable semantic anchor: invoice 2026</w:t></w:r></w:p>
    <w:p><w:r><w:t>Review footnote marker</w:t></w:r><w:r><w:footnoteReference w:id="2"/></w:r></w:p>
    <w:p><w:r><w:t>Review endnote marker</w:t></w:r><w:r><w:endnoteReference w:id="3"/></w:r></w:p>
    <w:p><w:commentRangeStart w:id="4"/><w:r><w:t>Review comment anchor</w:t></w:r><w:commentRangeEnd w:id="4"/><w:r><w:commentReference w:id="4"/></w:r></w:p>
    <w:p><w:ins w:id="5" w:author=")";
    xml += revision_author;
    xml += R"(" w:date="2026-05-01T10:00:00Z"><w:r><w:t>Review revision anchor text</w:t></w:r></w:ins></w:p>
)";
    if (!inserted_note.empty()) {
        xml += R"(    <w:p><w:r><w:t>)";
        xml += inserted_note;
        xml += R"(</w:t></w:r></w:p>
)";
    }
    xml += R"(    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Customer Name"/>
        <w:tag w:val="customer_name"/>
        <w:id w:val="2026"/>
        <w:dataBinding w:storeItemID="{77777777-7777-7777-7777-777777777777}" w:xpath="/invoice/customer"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>)";
    xml += customer;
    xml += R"(</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="TableGrid"/>
        <w:tblW w:type="pct" w:w="5000"/>
      </w:tblPr>
      <w:tblGrid><w:gridCol w:w="3200"/><w:gridCol w:w="3200"/></w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Field</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Value</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Total</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>)";
    xml += amount;
    xml += R"(</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>)";
    xml += footer_note;
    xml += R"(</w:t></w:r></w:p>
)";
    if (landscape_section) {
        xml += R"(    <w:sectPr><w:headerReference w:type="default" r:id="rId2"/><w:footerReference w:type="default" r:id="rId3"/><w:pgSz w:w="15840" w:h="12240" w:orient="landscape"/><w:pgMar w:top="720" w:right="1800" w:bottom="1080" w:left="1800" w:header="360" w:footer="540" w:gutter="0"/><w:pgNumType w:start="5"/></w:sectPr>
)";
    } else {
        xml += R"(    <w:sectPr><w:headerReference w:type="default" r:id="rId2"/><w:footerReference w:type="default" r:id="rId3"/><w:pgSz w:w="12240" w:h="15840"/><w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440" w:header="720" w:footer="720" w:gutter="0"/><w:pgNumType w:start="1"/></w:sectPr>
)";
    }
    xml += R"(  </w:body>
</w:document>
)";
    return xml;
}

std::string make_header_xml(std::string_view header_text) {
    return std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>)"} + std::string{header_text} + R"(</w:t></w:r></w:p>
</w:hdr>
)";
}

std::string make_footer_xml(std::string_view footer_text) {
    return std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>)"} + std::string{footer_text} + R"(</w:t></w:r></w:p>
</w:ftr>
)";
}

std::string make_footnotes_xml(std::string_view footnote_text) {
    return std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:footnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:footnote w:id="2"><w:p><w:r><w:t>)"} + std::string{footnote_text} + R"(</w:t></w:r></w:p></w:footnote>
</w:footnotes>
)";
}

std::string make_endnotes_xml(std::string_view endnote_text) {
    return std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:endnotes xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:endnote w:id="3"><w:p><w:r><w:t>)"} + std::string{endnote_text} + R"(</w:t></w:r></w:p></w:endnote>
</w:endnotes>
)";
}

std::string make_comments_xml(std::string_view comment_text) {
    return std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-05-01T12:00:00Z"><w:p><w:r><w:t>)"} + std::string{comment_text} + R"(</w:t></w:r></w:p></w:comment>
</w:comments>
)";
}

std::string make_styles_xml(std::string_view semantic_style_name,
                            std::string_view based_on_style) {
    return std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults><w:rPrDefault><w:rPr/></w:rPrDefault><w:pPrDefault><w:pPr/></w:pPrDefault></w:docDefaults>
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/><w:qFormat/></w:style>
  <w:style w:type="paragraph" w:styleId="Heading1"><w:name w:val="heading 1"/><w:basedOn w:val="Normal"/><w:qFormat/></w:style>
  <w:style w:type="paragraph" w:styleId="Title"><w:name w:val="Title"/><w:basedOn w:val="Normal"/><w:qFormat/></w:style>
  <w:style w:type="paragraph" w:customStyle="1" w:styleId="SemanticDiffHeading">
    <w:name w:val=")"} + std::string{semantic_style_name} + R"("/>
    <w:basedOn w:val=")" + std::string{based_on_style} + R"("/>
    <w:qFormat/>
    <w:pPr><w:numPr><w:ilvl w:val="0"/><w:numId w:val="42"/></w:numPr></w:pPr>
    <w:rPr><w:b/><w:sz w:val="32"/></w:rPr>
  </w:style>
</w:styles>
)";
}

std::string make_numbering_xml(std::string_view numbering_name,
                               std::string_view number_format,
                               std::uint32_t start_value,
                               std::string_view level_text) {
    return std::string{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="7">
    <w:name w:val=")"} + std::string{numbering_name} + R"("/>
    <w:lvl w:ilvl="0">
      <w:start w:val=")" + std::to_string(start_value) + R"("/>
      <w:numFmt w:val=")" + std::string{number_format} + R"("/>
      <w:lvlText w:val=")" + std::string{level_text} + R"("/>
    </w:lvl>
  </w:abstractNum>
  <w:num w:numId="42"><w:abstractNumId w:val="7"/></w:num>
</w:numbering>
)";
}

bool write_docx(const fs::path &path, std::string_view title,
                std::string_view status, std::string_view customer,
                std::string_view amount, std::string_view inserted_note,
                std::string_view footer_note, std::string_view header_text,
                std::string_view part_footer_text,
                std::string_view semantic_style_name,
                std::string_view based_on_style,
                std::string_view numbering_name,
                std::string_view number_format,
                std::uint32_t numbering_start,
                std::string_view numbering_level_text,
                std::string_view footnote_text,
                std::string_view endnote_text,
                std::string_view comment_text,
                std::string_view revision_author,
                bool landscape_section) {
    if (path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << path.parent_path().string() << '\n';
            return false;
        }
    }

    std::error_code remove_error;
    fs::remove(path, remove_error);
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(), zip_compression_level, 'w',
                                   &zip_error);
    if (zip == nullptr) {
        return false;
    }

    const auto document_xml = make_document_xml(title, status, customer, amount,
                                                inserted_note, footer_note,
                                                revision_author,
                                                landscape_section);
    const auto header_xml = make_header_xml(header_text);
    const auto footer_xml = make_footer_xml(part_footer_text);
    const auto styles_xml = make_styles_xml(semantic_style_name, based_on_style);
    const auto numbering_xml = make_numbering_xml(
        numbering_name, number_format, numbering_start, numbering_level_text);
    const auto footnotes_xml = make_footnotes_xml(footnote_text);
    const auto endnotes_xml = make_endnotes_xml(endnote_text);
    const auto comments_xml = make_comments_xml(comment_text);
    const bool ok = write_entry(zip, "_rels/.rels", root_relationships_xml) &&
                    write_entry(zip, "[Content_Types].xml", content_types_xml) &&
                    write_entry(zip, "word/_rels/document.xml.rels",
                                document_relationships_xml) &&
                    write_entry(zip, "word/document.xml", document_xml) &&
                    write_entry(zip, "word/styles.xml", styles_xml) &&
                    write_entry(zip, "word/numbering.xml", numbering_xml) &&
                    write_entry(zip, "word/header1.xml", header_xml) &&
                    write_entry(zip, "word/footer1.xml", footer_xml) &&
                    write_entry(zip, "word/footnotes.xml", footnotes_xml) &&
                    write_entry(zip, "word/endnotes.xml", endnotes_xml) &&
                    write_entry(zip, "word/comments.xml", comments_xml);
    zip_close(zip);
    return ok;
}

bool verify_diff(const fs::path &left_path, const fs::path &right_path) {
    featherdoc::Document left(left_path);
    if (left.open()) {
        return false;
    }
    featherdoc::Document right(right_path);
    if (right.open()) {
        return false;
    }

    const auto result = left.compare_semantic(right);
    if (!result.has_value() || !result->different() ||
        result->paragraphs.changed_count != 3U ||
        result->paragraphs.added_count != 1U ||
        result->paragraphs.unchanged_count != 5U ||
        result->tables.changed_count != 1U ||
        result->content_controls.changed_count != 1U ||
        result->styles.changed_count != 1U ||
        result->numbering.changed_count != 1U ||
        result->footnotes.changed_count != 1U ||
        result->endnotes.changed_count != 1U ||
        result->comments.changed_count != 1U ||
        result->revisions.changed_count != 1U ||
        result->sections.changed_count != 1U) {
        return false;
    }

    const auto has_resolved_section_part = [&result](
                                               featherdoc::template_schema_part_kind kind) {
        for (const auto &part_result : result->template_part_results) {
            if (part_result.target.part == kind &&
                part_result.target.section_index == std::optional<std::size_t>{0U} &&
                part_result.target.reference_kind ==
                    featherdoc::section_reference_kind::default_reference &&
                part_result.different()) {
                return true;
            }
        }
        return false;
    };
    return has_resolved_section_part(
               featherdoc::template_schema_part_kind::section_header) &&
           has_resolved_section_part(
               featherdoc::template_schema_part_kind::section_footer);
}
} // namespace

int main(int argc, char **argv) {
    const fs::path output_dir = argc > 1 ? fs::path(argv[1])
                                        : fs::current_path() / "semantic-diff-visual";
    const auto left_path = output_dir / "semantic_diff_left.docx";
    const auto right_path = output_dir / "semantic_diff_right.docx";

    if (!write_docx(left_path, "Semantic diff visual - BEFORE", "draft",
                    "Ada Lovelace", "$120", "",
                    "Before document: outdated wording.", "Header BEFORE",
                    "Footer BEFORE", "Semantic Diff Heading Draft", "Heading1",
                    "SemanticDiffDraftOutline", "decimal", 1U, "%1.",
                    "Semantic original footnote", "Semantic original endnote",
                    "Semantic original comment", "Ada Lovelace", false)) {
        std::cerr << "failed to write left DOCX\n";
        return 1;
    }
    if (!write_docx(right_path, "Semantic diff visual - AFTER", "approved",
                    "Grace Hopper", "$150",
                    "Inserted approval note: finance review completed.",
                    "After document: approved wording.", "Header AFTER",
                    "Footer AFTER", "Semantic Diff Heading Approved", "Title",
                    "SemanticDiffApprovedOutline", "bullet", 3U, "o",
                    "Semantic approved footnote", "Semantic approved endnote",
                    "Semantic approved comment", "Grace Hopper", true)) {
        std::cerr << "failed to write right DOCX\n";
        return 1;
    }
    if (!verify_diff(left_path, right_path)) {
        std::cerr << "semantic diff API verification failed\n";
        return 1;
    }

    std::cout << "left: " << left_path.string() << '\n';
    std::cout << "right: " << right_path.string() << '\n';
    return 0;
}
