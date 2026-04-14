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

struct template_case final {
    std::string name;
    std::string title;
    fs::path path;
    std::string document_xml;
    std::vector<featherdoc::template_slot_requirement> requirements;
    featherdoc::template_validation_result expected;
};

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

bool write_docx(const fs::path &path, const std::string &document_xml) {
    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(),
                                   ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                   &zip_error);
    if (zip == nullptr) {
        std::cerr << "failed to create archive: " << path.string()
                  << " (zip_error=" << zip_error << ")\n";
        return false;
    }

    const auto write_entry = [&](const char *entry_name, std::string_view content) {
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
    };

    if (!write_entry(content_types_xml_entry, content_types_xml) ||
        !write_entry(relationships_xml_entry, relationships_xml) ||
        !write_entry(document_xml_entry, document_xml)) {
        zip_close(zip);
        return false;
    }

    zip_close(zip);
    return true;
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

bool write_report(
    const fs::path &output_dir, const std::vector<template_case> &cases,
    const std::vector<featherdoc::template_validation_result> &results) {
    std::ofstream markdown(output_dir / "validation_summary.md", std::ios::binary);
    if (!markdown.good()) {
        return false;
    }

    markdown << "# Template Validation Sample\n\n";
    markdown << "This report is generated by "
             << "`samples/sample_template_validation.cpp`.\n\n";

    for (std::size_t index = 0; index < cases.size(); ++index) {
        const auto &test_case = cases[index];
        const auto &result = results[index];
        markdown << "## " << test_case.title << "\n\n";
        markdown << "- Path: `" << test_case.path.filename().string() << "`\n";
        markdown << "- Validation passed: "
                 << (static_cast<bool>(result) ? "true" : "false") << "\n";
        markdown << "- Missing required: ";
        if (result.missing_required.empty()) {
            markdown << "(none)\n";
        } else {
            markdown << '`';
            for (std::size_t item_index = 0; item_index < result.missing_required.size();
                 ++item_index) {
                if (item_index != 0U) {
                    markdown << "`, `";
                }
                markdown << result.missing_required[item_index];
            }
            markdown << "`\n";
        }
        markdown << "- Duplicate bookmarks: ";
        if (result.duplicate_bookmarks.empty()) {
            markdown << "(none)\n";
        } else {
            markdown << '`';
            for (std::size_t item_index = 0; item_index < result.duplicate_bookmarks.size();
                 ++item_index) {
                if (item_index != 0U) {
                    markdown << "`, `";
                }
                markdown << result.duplicate_bookmarks[item_index];
            }
            markdown << "`\n";
        }
        markdown << "- Malformed placeholders: ";
        if (result.malformed_placeholders.empty()) {
            markdown << "(none)\n";
        } else {
            markdown << '`';
            for (std::size_t item_index = 0;
                 item_index < result.malformed_placeholders.size(); ++item_index) {
                if (item_index != 0U) {
                    markdown << "`, `";
                }
                markdown << result.malformed_placeholders[item_index];
            }
            markdown << "`\n";
        }
        markdown << '\n';
    }

    std::ofstream json(output_dir / "validation_summary.json", std::ios::binary);
    if (!json.good()) {
        return false;
    }

    json << "{\n";
    for (std::size_t index = 0; index < cases.size(); ++index) {
        const auto &test_case = cases[index];
        const auto &result = results[index];
        json << "  \"" << json_escape(test_case.name) << "\": {\n";
        json << "    \"path\": \"" << json_escape(test_case.path.filename().string())
             << "\",\n";
        json << "    \"passed\": " << (static_cast<bool>(result) ? "true" : "false")
             << ",\n";
        json << "    \"missing_required\": ";
        write_json_array(json, result.missing_required);
        json << ",\n";
        json << "    \"duplicate_bookmarks\": ";
        write_json_array(json, result.duplicate_bookmarks);
        json << ",\n";
        json << "    \"malformed_placeholders\": ";
        write_json_array(json, result.malformed_placeholders);
        json << "\n  }";
        if (index + 1U != cases.size()) {
            json << ',';
        }
        json << '\n';
    }
    json << "}\n";

    return true;
}

std::vector<template_case> build_template_cases(const fs::path &output_dir) {
    const std::string valid_document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Template Validation Visual - Valid</w:t></w:r></w:p>
    <w:p>
      <w:r><w:t>Customer: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="intro_block"/>
      <w:r><w:t>intro placeholder paragraph</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:tbl>
      <w:tr>
        <w:tc>
          <w:p>
            <w:bookmarkStart w:id="2" w:name="line_items"/>
            <w:r><w:t>row placeholder</w:t></w:r>
            <w:bookmarkEnd w:id="2"/>
          </w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:p>
      <w:bookmarkStart w:id="3" w:name="summary_table"/>
      <w:r><w:t>table placeholder paragraph</w:t></w:r>
      <w:bookmarkEnd w:id="3"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="4" w:name="signature_image"/>
      <w:r><w:t>inline image placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="4"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="5" w:name="seal_image"/>
      <w:r><w:t>floating image placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="5"/>
    </w:p>
  </w:body>
</w:document>
)";

    const std::string invalid_document_xml =
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Template Validation Visual - Invalid</w:t></w:r></w:p>
    <w:p>
      <w:r><w:t>Customer slot A: </w:t></w:r>
      <w:bookmarkStart w:id="0" w:name="customer"/>
      <w:r><w:t>placeholder one</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:r><w:t>Customer slot B: </w:t></w:r>
      <w:bookmarkStart w:id="1" w:name="customer"/>
      <w:r><w:t>placeholder two</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:p>
      <w:r><w:t>Summary paragraph: prefix </w:t></w:r>
      <w:bookmarkStart w:id="2" w:name="summary_block"/>
      <w:r><w:t>placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="2"/>
      <w:r><w:t> suffix</w:t></w:r>
    </w:p>
    <w:p><w:r><w:t>Line items are intentionally outside a table row.</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="3" w:name="line_items"/>
      <w:r><w:t>not a table row placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="3"/>
    </w:p>
  </w:body>
</w:document>
)";

    return {
        template_case{
            "valid_template",
            "Valid Template",
            output_dir / "valid_template.docx",
            valid_document_xml,
            {
                {"customer", featherdoc::template_slot_kind::text, true},
                {"intro_block", featherdoc::template_slot_kind::block, true},
                {"line_items", featherdoc::template_slot_kind::table_rows, true},
                {"summary_table", featherdoc::template_slot_kind::table, true},
                {"signature_image", featherdoc::template_slot_kind::image, true},
                {"seal_image", featherdoc::template_slot_kind::floating_image, true},
            },
            {},
        },
        template_case{
            "invalid_template",
            "Invalid Template",
            output_dir / "invalid_template.docx",
            invalid_document_xml,
            {
                {"customer", featherdoc::template_slot_kind::text, true},
                {"invoice", featherdoc::template_slot_kind::text, true},
                {"summary_block", featherdoc::template_slot_kind::block, true},
                {"line_items", featherdoc::template_slot_kind::table_rows, true},
            },
            {
                {"invoice"},
                {"customer"},
                {"summary_block", "line_items"},
            },
        },
    };
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_dir =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "output" / "template-validation-visual";

    std::error_code directory_error;
    fs::create_directories(output_dir, directory_error);
    if (directory_error) {
        std::cerr << "failed to create output directory: " << output_dir.string() << '\n';
        return 1;
    }

    const auto cases = build_template_cases(output_dir);
    std::vector<featherdoc::template_validation_result> results;
    results.reserve(cases.size());

    for (const auto &test_case : cases) {
        fs::remove(test_case.path, directory_error);
        directory_error.clear();

        if (!write_docx(test_case.path, test_case.document_xml)) {
            return 1;
        }

        featherdoc::Document doc(test_case.path);
        if (const auto error = doc.open()) {
            print_document_error(doc, "open");
            return static_cast<int>(error.value() == 0 ? 1 : error.value());
        }

        const auto result = doc.validate_template(test_case.requirements);
        if (doc.last_error()) {
            print_document_error(doc, "validate_template");
            return 1;
        }

        if (!matches_expected(result, test_case.expected)) {
            std::cerr << "validation output mismatch for " << test_case.name << '\n';
            return 1;
        }

        results.push_back(result);
    }

    if (!write_report(output_dir, cases, results)) {
        std::cerr << "failed to write validation report files\n";
        return 1;
    }

    std::cout << "wrote template validation samples to " << output_dir.string() << '\n';
    return 0;
}
