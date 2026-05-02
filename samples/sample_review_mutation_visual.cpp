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
</Types>
)"};
constexpr auto document_relationships_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"};
constexpr auto document_xml = std::string_view{R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Review mutation visual regression.</w:t></w:r></w:p>
    <w:p>
      <w:ins w:id="1" w:author="Ada" w:date="2026-05-01T10:00:00Z"><w:r><w:t>Accepted insertion from revision API.</w:t></w:r></w:ins>
      <w:del w:id="2" w:author="Grace" w:date="2026-05-01T11:00:00Z"><w:r><w:delText>Rejected deletion kept by API.</w:delText></w:r></w:del>
    </w:p>
    <w:sectPr><w:pgSz w:w="12240" w:h="15840"/><w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440" w:header="720" w:footer="720" w:gutter="0"/></w:sectPr>
  </w:body>
</w:document>
)"};

bool write_entry(zip_t *zip, const char *entry_name, std::string_view content) {
    if (zip_entry_open(zip, entry_name) != 0) {
        std::cerr << "failed to open " << entry_name << '\n';
        return false;
    }
    if (zip_entry_write(zip, content.data(), content.size()) < 0) {
        zip_entry_close(zip);
        std::cerr << "failed to write " << entry_name << '\n';
        return false;
    }
    return zip_entry_close(zip) == 0;
}

bool write_fixture_docx(const fs::path &path) {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(), zip_compression_level, 'w',
                                   &zip_error);
    if (zip == nullptr) {
        std::cerr << "failed to create fixture DOCX\n";
        return false;
    }
    const bool ok = write_entry(zip, "_rels/.rels", root_relationships_xml) &&
                    write_entry(zip, "[Content_Types].xml", content_types_xml) &&
                    write_entry(zip, "word/document.xml", document_xml) &&
                    write_entry(zip, "word/_rels/document.xml.rels",
                                document_relationships_xml);
    zip_close(zip);
    return ok;
}

bool mutate_with_typed_apis(const fs::path &path) {
    featherdoc::Document document(path);
    if (document.open()) {
        std::cerr << "failed to open fixture: " << document.last_error().detail << '\n';
        return false;
    }

    if (!document.accept_revision(0U) || !document.reject_revision(0U)) {
        std::cerr << "revision mutation failed: " << document.last_error().detail << '\n';
        return false;
    }
    if (document.append_insertion_revision("Authored insertion revision accepted by typed API",
                                           "Reviewer", "2026-05-02T10:00:00Z") != 1U ||
        document.append_deletion_revision("Authored deletion revision rejected by typed API",
                                          "Reviewer", "2026-05-02T11:00:00Z") != 1U ||
        !document.accept_revision(0U) || !document.reject_revision(0U)) {
        std::cerr << "revision authoring failed: " << document.last_error().detail << '\n';
        return false;
    }

    auto body = document.body_template();
    if (!body || !body.append_paragraph("Typed API evidence below").has_next() ||
        !body.append_paragraph("Run revision original visible text").has_next()) {
        std::cerr << "failed to append evidence heading\n";
        return false;
    }

    if (!document.insert_run_revision_after(5U, 0U,
                                            " accepted in-place insertion revision",
                                            "Reviewer", "2026-05-02T12:00:00Z") ||
        !document.accept_revision(0U) ||
        !document.delete_run_revision(5U, 0U, "Reviewer",
                                      "2026-05-02T13:00:00Z") ||
        !document.reject_revision(0U) ||
        !document.replace_run_revision(5U, 1U,
                                       " accepted in-place replacement revision",
                                       "Reviewer", "2026-05-02T14:00:00Z") ||
        !document.accept_revision(0U) || !document.accept_revision(0U)) {
        std::cerr << "in-place revision authoring failed: "
                  << document.last_error().detail << '\n';
        return false;
    }

    auto range_paragraph = body.append_paragraph("Paragraph range Alpha ");
    if (!range_paragraph.has_next() ||
        !range_paragraph.add_run("Beta").has_next() ||
        !range_paragraph.add_run(" Gamma").has_next() ||
        !document.replace_paragraph_text_revision(
            6U, 19U, 7U, "accepted paragraph range replacement",
            "Reviewer", "2026-05-02T15:00:00Z") ||
        !document.accept_revision(0U) || !document.accept_revision(0U)) {
        std::cerr << "paragraph text range revision authoring failed: "
                  << document.last_error().detail << '\n';
        return false;
    }

    auto cross_range_first = body.append_paragraph("Cross Alpha ");
    auto cross_range_second = body.append_paragraph("Cross Middle Text");
    auto cross_range_third = body.append_paragraph("Gamma Delta");
    if (!cross_range_first.has_next() ||
        !cross_range_first.add_run("Beta").has_next() ||
        !cross_range_second.has_next() || !cross_range_third.has_next() ||
        !document.replace_text_range_revision(
            7U, 12U, 9U, 5U, "accepted cross paragraph range replacement",
            "Reviewer", "2026-05-02T15:30:00Z") ||
        !document.accept_revision(0U) || !document.accept_revision(0U) ||
        !document.accept_revision(0U) || !document.accept_revision(0U)) {
        std::cerr << "cross paragraph text range revision authoring failed: "
                  << document.last_error().detail << '\n';
        return false;
    }

    if (!body || !body.append_paragraph("Typed API evidence details below").has_next()) {
        std::cerr << "failed to append evidence heading\n";
        return false;
    }

    if (document.append_footnote("Footnote reference from typed API ",
                                 "Original visual footnote") != 1U ||
        !document.replace_footnote(0U, "Replaced visual footnote body") ||
        document.append_footnote("Removed footnote reference ",
                                 "Removed visual footnote body") != 1U ||
        !document.remove_footnote(1U)) {
        std::cerr << "footnote mutation failed: " << document.last_error().detail << '\n';
        return false;
    }

    if (document.append_endnote("Endnote reference from typed API ",
                                "Original visual endnote") != 1U ||
        !document.replace_endnote(0U, "Replaced visual endnote body") ||
        document.append_endnote("Removed endnote reference ",
                                "Removed visual endnote body") != 1U ||
        !document.remove_endnote(1U)) {
        std::cerr << "endnote mutation failed: " << document.last_error().detail << '\n';
        return false;
    }

    if (document.append_comment("Commented typed API text",
                                "Original visual comment", "Reviewer", "RV") != 1U ||
        !document.replace_comment(0U, "Replaced visual comment body") ||
        document.append_comment("Removed comment text",
                                "Removed visual comment body") != 1U ||
        !document.remove_comment(1U)) {
        std::cerr << "comment mutation failed: " << document.last_error().detail << '\n';
        return false;
    }

    if (document.append_hyperlink("Old typed hyperlink",
                                  "https://old.example/typed") != 1U ||
        !document.replace_hyperlink(0U, "Typed hyperlink",
                                    "https://example.com/typed-api") ||
        document.append_hyperlink("Removed hyperlink text",
                                  "https://remove.example/typed") != 1U ||
        !document.remove_hyperlink(1U)) {
        std::cerr << "hyperlink mutation failed: " << document.last_error().detail << '\n';
        return false;
    }

    if (!document.append_omml(featherdoc::make_omml_fraction("a+b", "c", true)) ||
        !document.append_omml(featherdoc::make_omml_radical("x+1"))) {
        std::cerr << "OMML builder insertion failed: " << document.last_error().detail << '\n';
        return false;
    }

    if (document.save()) {
        std::cerr << "failed to save mutated DOCX: " << document.last_error().detail << '\n';
        return false;
    }
    return true;
}

bool verify_api(const fs::path &path) {
    featherdoc::Document document(path);
    if (document.open()) {
        return false;
    }
    const auto footnotes = document.list_footnotes();
    const auto endnotes = document.list_endnotes();
    const auto comments = document.list_comments();
    const auto revisions = document.list_revisions();
    const auto hyperlinks = document.list_hyperlinks();
    const auto equations = document.list_omml();
    return footnotes.size() == 1U && footnotes.front().text == "Replaced visual footnote body" &&
           endnotes.size() == 1U && endnotes.front().text == "Replaced visual endnote body" &&
           comments.size() == 1U && comments.front().text == "Replaced visual comment body" &&
           comments.front().anchor_text == std::optional<std::string>{"Commented typed API text"} &&
           revisions.empty() && hyperlinks.size() == 1U &&
           hyperlinks.front().text == "Typed hyperlink" && equations.size() == 2U;
}
} // namespace

int main(int argc, char **argv) {
    const fs::path output_path = argc > 1 ? fs::path(argv[1])
                                          : fs::current_path() /
                                                "review_mutation_visual.docx";
    if (output_path.has_parent_path()) {
        std::error_code create_error;
        fs::create_directories(output_path.parent_path(), create_error);
    }
    std::error_code remove_error;
    fs::remove(output_path, remove_error);
    if (!write_fixture_docx(output_path)) {
        return 1;
    }
    if (!mutate_with_typed_apis(output_path)) {
        return 1;
    }
    if (!verify_api(output_path)) {
        std::cerr << "review mutation API verification failed\n";
        return 1;
    }
    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
