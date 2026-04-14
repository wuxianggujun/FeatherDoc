#include <featherdoc.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr auto relationships_xml_entry = "_rels/.rels";
constexpr auto content_types_xml_entry = "[Content_Types].xml";
constexpr auto document_xml_entry = "word/document.xml";
constexpr auto document_relationships_xml_entry = "word/_rels/document.xml.rels";
constexpr auto header_xml_entry = "word/header1.xml";
constexpr auto footer_xml_entry = "word/footer1.xml";
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
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)";
constexpr auto document_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>Header/Footer Template Validation Visual</w:t></w:r></w:p>
    <w:p><w:r><w:t>This page keeps a valid header template and an intentionally malformed footer template.</w:t></w:r></w:p>
    <w:p><w:r><w:t>The API should accept the header schema and reject the footer schema.</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
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
constexpr auto header_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Header title: </w:t></w:r>
    <w:bookmarkStart w:id="0" w:name="header_title"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_note"/>
    <w:r><w:t>standalone header note</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
  <w:tbl>
    <w:tblGrid>
      <w:gridCol w:w="2400"/>
      <w:gridCol w:w="2400"/>
    </w:tblGrid>
    <w:tr>
      <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
      <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
    </w:tr>
    <w:tr>
      <w:tc>
        <w:p>
          <w:bookmarkStart w:id="2" w:name="header_rows"/>
          <w:r><w:t>row placeholder</w:t></w:r>
          <w:bookmarkEnd w:id="2"/>
        </w:p>
      </w:tc>
      <w:tc><w:p><w:r><w:t>0</w:t></w:r></w:p></w:tc>
    </w:tr>
  </w:tbl>
</w:hdr>
)";
constexpr auto footer_xml =
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t>Footer company A: </w:t></w:r>
    <w:bookmarkStart w:id="3" w:name="footer_company"/>
    <w:r><w:t>placeholder A</w:t></w:r>
    <w:bookmarkEnd w:id="3"/>
  </w:p>
  <w:p>
    <w:r><w:t>Footer company B: </w:t></w:r>
    <w:bookmarkStart w:id="4" w:name="footer_company"/>
    <w:r><w:t>placeholder B</w:t></w:r>
    <w:bookmarkEnd w:id="4"/>
  </w:p>
  <w:p>
    <w:r><w:t>Summary: prefix </w:t></w:r>
    <w:bookmarkStart w:id="5" w:name="footer_summary"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="5"/>
    <w:r><w:t> suffix</w:t></w:r>
  </w:p>
</w:ftr>
)";

void print_document_error(const featherdoc::Document &doc, std::string_view operation) {
    const auto &error_info = doc.last_error();
    std::cerr << operation << " failed";
    if (error_info.code) {
        std::cerr << ": " << error_info.code.message();
    }
    if (!error_info.detail.empty()) {
        std::cerr << " - " << error_info.detail;
    }
    if (!error_info.entry_name.empty()) {
        std::cerr << " [entry=" << error_info.entry_name << ']';
    }
    if (error_info.xml_offset.has_value()) {
        std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
    }
    std::cerr << '\n';
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
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    if (zip == nullptr) {
        std::cerr << "failed to create archive: " << path.string()
                  << " (zip_error=" << zip_error << ")\n";
        return false;
    }

    const bool ok =
        write_entry(zip, content_types_xml_entry, content_types_xml) &&
        write_entry(zip, relationships_xml_entry, relationships_xml) &&
        write_entry(zip, document_xml_entry, document_xml) &&
        write_entry(zip, document_relationships_xml_entry, document_relationships_xml) &&
        write_entry(zip, header_xml_entry, header_xml) &&
        write_entry(zip, footer_xml_entry, footer_xml);
    zip_close(zip);
    return ok;
}

bool same_strings(const std::vector<std::string> &lhs,
                  const std::vector<std::string> &rhs) {
    return lhs == rhs;
}

bool matches_expected(const featherdoc::template_validation_result &actual,
                      const featherdoc::template_validation_result &expected) {
    return same_strings(actual.missing_required, expected.missing_required) &&
           same_strings(actual.duplicate_bookmarks, expected.duplicate_bookmarks) &&
           same_strings(actual.malformed_placeholders, expected.malformed_placeholders);
}

std::string json_escape(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

void write_json_array(std::ostream &stream, const std::vector<std::string> &items) {
    stream << '[';
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (index != 0U) {
            stream << ", ";
        }
        stream << '"' << json_escape(items[index]) << '"';
    }
    stream << ']';
}

bool write_report(const fs::path &output_dir, const fs::path &docx_path,
                  const featherdoc::template_validation_result &header_result,
                  const featherdoc::template_validation_result &footer_result) {
    std::ofstream markdown(output_dir / "validation_summary.md", std::ios::binary);
    if (!markdown.good()) {
        return false;
    }

    markdown << "# Part Template Validation Sample\n\n";
    markdown << "- DOCX: `" << docx_path.filename().string() << "`\n";
    markdown << "- Header validation passed: "
             << (static_cast<bool>(header_result) ? "true" : "false") << "\n";
    markdown << "- Footer validation passed: "
             << (static_cast<bool>(footer_result) ? "true" : "false") << "\n";
    markdown << "- Footer missing required: `footer_signature`\n";
    markdown << "- Footer duplicate bookmarks: `footer_company`\n";
    markdown << "- Footer malformed placeholders: `footer_summary`\n";

    std::ofstream json(output_dir / "validation_summary.json", std::ios::binary);
    if (!json.good()) {
        return false;
    }

    json << "{\n";
    json << "  \"document\": \"" << json_escape(docx_path.filename().string()) << "\",\n";
    json << "  \"header\": {\n";
    json << "    \"passed\": " << (static_cast<bool>(header_result) ? "true" : "false")
         << ",\n";
    json << "    \"missing_required\": ";
    write_json_array(json, header_result.missing_required);
    json << ",\n    \"duplicate_bookmarks\": ";
    write_json_array(json, header_result.duplicate_bookmarks);
    json << ",\n    \"malformed_placeholders\": ";
    write_json_array(json, header_result.malformed_placeholders);
    json << "\n  },\n";
    json << "  \"footer\": {\n";
    json << "    \"passed\": " << (static_cast<bool>(footer_result) ? "true" : "false")
         << ",\n";
    json << "    \"missing_required\": ";
    write_json_array(json, footer_result.missing_required);
    json << ",\n    \"duplicate_bookmarks\": ";
    write_json_array(json, footer_result.duplicate_bookmarks);
    json << ",\n    \"malformed_placeholders\": ";
    write_json_array(json, footer_result.malformed_placeholders);
    json << "\n  }\n}\n";

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_dir =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "output" / "part-template-validation-visual";
    const fs::path output_docx = output_dir / "part_template_validation.docx";

    std::error_code directory_error;
    fs::create_directories(output_dir, directory_error);
    if (directory_error) {
        std::cerr << "failed to create output directory: " << output_dir.string() << '\n';
        return 1;
    }

    fs::remove(output_docx, directory_error);
    directory_error.clear();

    if (!write_docx(output_docx)) {
        return 1;
    }

    featherdoc::Document doc(output_docx);
    if (const auto error = doc.open()) {
        print_document_error(doc, "open");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto header_template = doc.section_header_template(0);
    if (!header_template) {
        print_document_error(doc, "section_header_template");
        return 1;
    }
    const auto header_result = header_template.validate_template(
        {
            {"header_title", featherdoc::template_slot_kind::text, true},
            {"header_note", featherdoc::template_slot_kind::block, true},
            {"header_rows", featherdoc::template_slot_kind::table_rows, true},
        });
    if (doc.last_error()) {
        print_document_error(doc, "header_template.validate_template");
        return 1;
    }

    auto footer_template = doc.section_footer_template(0);
    if (!footer_template) {
        print_document_error(doc, "section_footer_template");
        return 1;
    }
    const auto footer_result = footer_template.validate_template(
        {
            {"footer_company", featherdoc::template_slot_kind::text, true},
            {"footer_summary", featherdoc::template_slot_kind::block, true},
            {"footer_signature", featherdoc::template_slot_kind::text, true},
        });
    if (doc.last_error()) {
        print_document_error(doc, "footer_template.validate_template");
        return 1;
    }

    const featherdoc::template_validation_result expected_header{};
    const featherdoc::template_validation_result expected_footer{
        {"footer_signature"},
        {"footer_company"},
        {"footer_summary"},
    };

    if (!matches_expected(header_result, expected_header) ||
        !matches_expected(footer_result, expected_footer)) {
        std::cerr << "template validation result mismatch\n";
        return 1;
    }

    if (!write_report(output_dir, output_docx, header_result, footer_result)) {
        std::cerr << "failed to write validation reports\n";
        return 1;
    }

    std::cout << "wrote part template validation sample to " << output_dir.string()
              << '\n';
    return 0;
}
