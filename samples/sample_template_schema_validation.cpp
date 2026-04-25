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
    <w:p><w:r><w:t>Template schema validation sample</w:t></w:r></w:p>
    <w:p><w:r><w:t>The section header should pass and the footer should fail.</w:t></w:r></w:p>
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

auto same_strings(const std::vector<std::string> &actual,
                  std::initializer_list<const char *> expected) -> bool {
    if (actual.size() != expected.size()) {
        return false;
    }

    auto actual_it = actual.begin();
    auto expected_it = expected.begin();
    for (; actual_it != actual.end(); ++actual_it, ++expected_it) {
        if (*actual_it != *expected_it) {
            return false;
        }
    }

    return true;
}

bool matches_expected(const featherdoc::template_schema_validation_result &result) {
    if (result.part_results.size() != 2U) {
        return false;
    }

    const auto &header_result = result.part_results[0];
    if (!header_result.available || header_result.entry_name != "word/header1.xml" ||
        !static_cast<bool>(header_result) ||
        header_result.target.part !=
            featherdoc::template_schema_part_kind::section_header ||
        !header_result.target.section_index.has_value() ||
        *header_result.target.section_index != 0U ||
        !header_result.validation.missing_required.empty() ||
        !header_result.validation.duplicate_bookmarks.empty() ||
        !header_result.validation.malformed_placeholders.empty() ||
        !header_result.validation.unexpected_bookmarks.empty() ||
        !header_result.validation.kind_mismatches.empty() ||
        !header_result.validation.occurrence_mismatches.empty()) {
        return false;
    }

    const auto &footer_result = result.part_results[1];
    return footer_result.available && footer_result.entry_name == "word/footer1.xml" &&
           !static_cast<bool>(footer_result) &&
           footer_result.target.part ==
               featherdoc::template_schema_part_kind::section_footer &&
           footer_result.target.section_index.has_value() &&
           *footer_result.target.section_index == 0U &&
           same_strings(footer_result.validation.missing_required,
                        {"footer_signature"}) &&
           same_strings(footer_result.validation.duplicate_bookmarks,
                        {"footer_company"}) &&
           same_strings(footer_result.validation.malformed_placeholders,
                        {"footer_summary"}) &&
           footer_result.validation.unexpected_bookmarks.empty() &&
           footer_result.validation.kind_mismatches.empty() &&
           footer_result.validation.occurrence_mismatches.empty();
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

const char *schema_part_name(featherdoc::template_schema_part_kind part) {
    switch (part) {
    case featherdoc::template_schema_part_kind::body:
        return "body";
    case featherdoc::template_schema_part_kind::header:
        return "header";
    case featherdoc::template_schema_part_kind::footer:
        return "footer";
    case featherdoc::template_schema_part_kind::section_header:
        return "section-header";
    case featherdoc::template_schema_part_kind::section_footer:
        return "section-footer";
    }

    return "body";
}

bool write_report(const fs::path &output_dir, const fs::path &docx_path,
                  const featherdoc::template_schema_validation_result &result) {
    std::ofstream markdown(output_dir / "validation_summary.md", std::ios::binary);
    if (!markdown.good()) {
        return false;
    }

    markdown << "# Template Schema Validation Sample\n\n";
    markdown << "- DOCX: `" << docx_path.filename().string() << "`\n";
    markdown << "- Passed: `" << (static_cast<bool>(result) ? "true" : "false")
             << "`\n";
    for (std::size_t index = 0U; index < result.part_results.size(); ++index) {
        const auto &part_result = result.part_results[index];
        markdown << "- Part " << index << ": `" << schema_part_name(part_result.target.part)
                 << "` entry=`" << part_result.entry_name << "`"
                 << " available=`" << (part_result.available ? "true" : "false")
                 << "` passed=`" << (static_cast<bool>(part_result) ? "true" : "false")
                 << "`\n";
    }

    std::ofstream json(output_dir / "validation_summary.json", std::ios::binary);
    if (!json.good()) {
        return false;
    }

    json << "{\n";
    json << "  \"document\": \"" << json_escape(docx_path.filename().string()) << "\",\n";
    json << "  \"passed\": " << (static_cast<bool>(result) ? "true" : "false")
         << ",\n";
    json << "  \"part_results\": [\n";
    for (std::size_t index = 0U; index < result.part_results.size(); ++index) {
        const auto &part_result = result.part_results[index];
        if (index != 0U) {
            json << ",\n";
        }
        json << "    {\n";
        json << "      \"part\": \"" << schema_part_name(part_result.target.part)
             << "\",\n";
        json << "      \"entry_name\": \"" << json_escape(part_result.entry_name)
             << "\",\n";
        json << "      \"available\": "
             << (part_result.available ? "true" : "false") << ",\n";
        json << "      \"passed\": "
             << (static_cast<bool>(part_result) ? "true" : "false") << ",\n";
        json << "      \"missing_required\": ";
        write_json_array(json, part_result.validation.missing_required);
        json << ",\n      \"duplicate_bookmarks\": ";
        write_json_array(json, part_result.validation.duplicate_bookmarks);
        json << ",\n      \"malformed_placeholders\": ";
        write_json_array(json, part_result.validation.malformed_placeholders);
        json << "\n    }";
    }
    json << "\n  ]\n}\n";

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_dir =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "output" / "template-schema-validation";
    const fs::path output_docx = output_dir / "template_schema_validation.docx";

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

    featherdoc::template_schema schema{
        {
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt,
                 0U, featherdoc::section_reference_kind::default_reference},
                {"header_title", featherdoc::template_slot_kind::text, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt,
                 0U, featherdoc::section_reference_kind::default_reference},
                {"header_note", featherdoc::template_slot_kind::block, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_header, std::nullopt,
                 0U, featherdoc::section_reference_kind::default_reference},
                {"header_rows", featherdoc::template_slot_kind::table_rows, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt,
                 0U, featherdoc::section_reference_kind::default_reference},
                {"footer_company", featherdoc::template_slot_kind::text, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt,
                 0U, featherdoc::section_reference_kind::default_reference},
                {"footer_summary", featherdoc::template_slot_kind::block, true},
            },
            {
                {featherdoc::template_schema_part_kind::section_footer, std::nullopt,
                 0U, featherdoc::section_reference_kind::default_reference},
                {"footer_signature", featherdoc::template_slot_kind::text, true},
            },
        }};

    const auto result = doc.validate_template_schema(schema);
    if (doc.last_error()) {
        print_document_error(doc, "validate_template_schema");
        return 1;
    }

    if (!matches_expected(result)) {
        std::cerr << "template schema validation result mismatch\n";
        return 1;
    }

    if (!write_report(output_dir, output_docx, result)) {
        std::cerr << "failed to write validation reports\n";
        return 1;
    }

    std::cout << "wrote template schema validation sample to " << output_dir.string()
              << '\n';
    return 0;
}
