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
      <w:r><w:t>Generic field API visual regression.</w:t></w:r>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="target_heading"/>
      <w:r><w:t>Target heading for REF field</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:p>
      <w:r><w:t>The generated fields below should remain readable after Word PDF export.</w:t></w:r>
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

bool append_fields(const fs::path &path) {
    featherdoc::Document document(path);
    if (document.open()) {
        std::cerr << "failed to open seed DOCX: " << document.last_error().detail << '\n';
        return false;
    }

    if (!document.enable_update_fields_on_open()) {
        std::cerr << "failed to enable updateFields: " << document.last_error().detail << '\n';
        return false;
    }

    auto body = document.body_template();
    if (!body) {
        std::cerr << "failed to open body template: " << document.last_error().detail << '\n';
        return false;
    }

    auto toc_options = featherdoc::table_of_contents_field_options{};
    toc_options.min_outline_level = 1U;
    toc_options.max_outline_level = 2U;
    if (!body.append_table_of_contents_field(
            toc_options, "TOC field placeholder - update fields in Word")) {
        std::cerr << "failed to append TOC field: " << document.last_error().detail << '\n';
        return false;
    }

    if (!body.append_reference_field("target_heading", {},
                                     "Referenced target heading")) {
        std::cerr << "failed to append REF field: " << document.last_error().detail << '\n';
        return false;
    }

    auto page_reference_options = featherdoc::page_reference_field_options{};
    page_reference_options.relative_position = true;
    if (!body.append_page_reference_field("target_heading", page_reference_options,
                                          "Page reference placeholder")) {
        std::cerr << "failed to append PAGEREF field: " << document.last_error().detail << '\n';
        return false;
    }

    auto style_reference_options = featherdoc::style_reference_field_options{};
    style_reference_options.paragraph_number = true;
    style_reference_options.relative_position = true;
    if (!body.append_style_reference_field("Heading 1", style_reference_options,
                                           "Style reference placeholder")) {
        std::cerr << "failed to append STYLEREF field: " << document.last_error().detail << '\n';
        return false;
    }

    if (!body.append_document_property_field("Title", {},
                                             "Document title placeholder")) {
        std::cerr << "failed to append DOCPROPERTY field: " << document.last_error().detail << '\n';
        return false;
    }

    auto date_options = featherdoc::date_field_options{};
    date_options.format = "yyyy-MM-dd";
    date_options.state.dirty = true;
    if (!body.append_date_field(date_options, "2026-05-01")) {
        std::cerr << "failed to append DATE field: " << document.last_error().detail << '\n';
        return false;
    }

    auto hyperlink_options = featherdoc::hyperlink_field_options{};
    hyperlink_options.anchor = "target_heading";
    hyperlink_options.tooltip = "Open target heading";
    hyperlink_options.state.locked = true;
    if (!body.append_hyperlink_field("https://example.com/report",
                                     hyperlink_options, "Open report")) {
        std::cerr << "failed to append HYPERLINK field: " << document.last_error().detail << '\n';
        return false;
    }

    auto figure_sequence = featherdoc::sequence_field_options{};
    figure_sequence.restart_value = 1U;
    if (!body.append_sequence_field("Figure", figure_sequence, "Figure 1")) {
        std::cerr << "failed to append first SEQ field: " << document.last_error().detail << '\n';
        return false;
    }
    if (!body.append_sequence_field("Figure", {}, "Figure 2")) {
        std::cerr << "failed to append second SEQ field: " << document.last_error().detail << '\n';
        return false;
    }

    auto caption_options = featherdoc::caption_field_options{};
    caption_options.restart_value = 3U;
    if (!body.append_caption("Figure", "Generated caption placeholder",
                             caption_options, "3")) {
        std::cerr << "failed to append caption: " << document.last_error().detail << '\n';
        return false;
    }

    auto index_entry_options = featherdoc::index_entry_field_options{};
    index_entry_options.subentry = "Fields";
    index_entry_options.bookmark_name = "target_heading";
    index_entry_options.cross_reference = "See typed fields";
    index_entry_options.bold_page_number = true;
    if (!body.append_index_entry_field("FeatherDoc", index_entry_options)) {
        std::cerr << "failed to append XE field: " << document.last_error().detail << '\n';
        return false;
    }

    auto index_options = featherdoc::index_field_options{};
    index_options.columns = 2U;
    if (!body.append_index_field(index_options, "Index field placeholder")) {
        std::cerr << "failed to append INDEX field: " << document.last_error().detail << '\n';
        return false;
    }

    auto complex_options = featherdoc::complex_field_options{};
    complex_options.state.dirty = true;
    if (!body.append_complex_field(
            {featherdoc::complex_field_text_fragment(" IF "),
             featherdoc::complex_field_nested_fragment(
                 " MERGEFIELD CustomerName ", "Ada"),
             featherdoc::complex_field_text_fragment(
                 " = \"Ada\" \"Nested field matched\" \"Other customer\" ")},
            "Nested field matched", complex_options)) {
        std::cerr << "failed to append complex nested field: "
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
                 : fs::current_path() / "generic_fields_visual.docx";

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
    if (!append_fields(output_path)) {
        return 1;
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
