#include "featherdoc_cli_section_part_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_style_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_section_management_commands.hpp"
#include "featherdoc_cli_section_options_parse.hpp"
#include "featherdoc_cli_section_part_management_commands.hpp"
#include "featherdoc_cli_section_part_reference_commands.hpp"
#include "featherdoc_cli_section_part_support.hpp"
#include "featherdoc_cli_text.hpp"

#include <pugixml.hpp>

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <zip.h>

namespace featherdoc_cli {
namespace {

enum class zip_entry_read_status {
    missing,
    read_failed,
    ok,
};

struct inspected_part_reference {
    std::size_t section_index = 0;
    std::string kind;
};

struct inspected_part_entry {
    std::string relationship_id;
    std::string entry_name;
    std::vector<inspected_part_reference> references;
    std::vector<std::string> paragraphs;
};

[[nodiscard]] auto section_reference_name(section_part_family family) -> const char * {
    return family == section_part_family::header ? "w:headerReference"
                                                 : "w:footerReference";
}

[[nodiscard]] auto section_part_paragraphs(featherdoc::Document &doc,
                                           section_part_family family,
                                           std::size_t index)
    -> featherdoc::Paragraph & {
    if (family == section_part_family::header) {
        return doc.header_paragraphs(index);
    }

    return doc.footer_paragraphs(index);
}

[[nodiscard]] auto section_part_paragraphs(
    featherdoc::Document &doc, section_part_family family,
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) -> featherdoc::Paragraph & {
    if (family == section_part_family::header) {
        return doc.section_header_paragraphs(section_index, reference_kind);
    }

    return doc.section_footer_paragraphs(section_index, reference_kind);
}

[[nodiscard]] auto has_section_header(
    featherdoc::Document &doc, std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) -> bool {
    return doc.section_header_paragraphs(section_index, reference_kind).has_next();
}

[[nodiscard]] auto has_section_footer(
    featherdoc::Document &doc, std::size_t section_index,
    featherdoc::section_reference_kind reference_kind) -> bool {
    return doc.section_footer_paragraphs(section_index, reference_kind).has_next();
}

[[nodiscard]] auto collect_paragraph_text(featherdoc::Paragraph paragraph)
    -> std::string {
    std::string text;
    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
        text += run.get_text();
    }
    return text;
}

[[nodiscard]] auto collect_part_lines(featherdoc::Paragraph paragraph)
    -> std::vector<std::string> {
    std::vector<std::string> lines;
    for (; paragraph.has_next(); paragraph.next()) {
        lines.push_back(collect_paragraph_text(paragraph));
    }
    return lines;
}

[[nodiscard]] auto read_zip_entry_text(zip_t *archive,
                                       std::string_view entry_name,
                                       std::string &content)
    -> zip_entry_read_status {
    if (zip_entry_open(archive, std::string(entry_name).c_str()) < 0) {
        return zip_entry_read_status::missing;
    }

    void *buffer = nullptr;
    std::size_t size = 0;
    if (zip_entry_read(archive, &buffer, &size) < 0) {
        zip_entry_close(archive);
        return zip_entry_read_status::read_failed;
    }

    if (buffer != nullptr && size != 0U) {
        content.assign(static_cast<const char *>(buffer), size);
    } else {
        content.clear();
    }

    std::free(buffer);
    zip_entry_close(archive);
    return zip_entry_read_status::ok;
}

[[nodiscard]] auto load_inspected_part_entries(
    const path_type &input_path, featherdoc::Document &doc,
    section_part_family family, std::vector<inspected_part_entry> &parts,
    std::string &error_message) -> bool {
    parts.clear();

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(input_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        error_message = "failed to open source archive '" + input_path.string() + "'";
        return false;
    }

    auto close_archive = [&]() { zip_close(archive); };

    std::string relationships_xml_text;
    const auto relationships_status = read_zip_entry_text(
        archive, "word/_rels/document.xml.rels", relationships_xml_text);
    if (relationships_status == zip_entry_read_status::read_failed) {
        close_archive();
        error_message =
            "failed to read relationships entry 'word/_rels/document.xml.rels'";
        return false;
    }

    if (relationships_status == zip_entry_read_status::ok) {
        pugi::xml_document relationships_xml;
        const auto parse_result = relationships_xml.load_buffer(
            relationships_xml_text.data(), relationships_xml_text.size());
        if (!parse_result) {
            close_archive();
            error_message =
                "failed to parse relationships entry 'word/_rels/document.xml.rels'";
            return false;
        }

        const auto expected_type =
            family == section_part_family::header
                ? std::string_view{
                      "http://schemas.openxmlformats.org/officeDocument/2006/"
                      "relationships/header"}
                : std::string_view{
                      "http://schemas.openxmlformats.org/officeDocument/2006/"
                      "relationships/footer"};

        std::vector<std::string> seen_entries;
        const auto relationships = relationships_xml.child("Relationships");
        for (auto relationship = relationships.child("Relationship");
             relationship != pugi::xml_node{};
             relationship = relationship.next_sibling("Relationship")) {
            const auto type =
                std::string_view{relationship.attribute("Type").value()};
            if (type != expected_type) {
                continue;
            }

            const auto target =
                std::string_view{relationship.attribute("Target").value()};
            if (target.empty()) {
                continue;
            }

            const auto entry_name = normalize_word_part_entry(target);
            bool already_seen = false;
            for (const auto &seen_entry : seen_entries) {
                if (seen_entry == entry_name) {
                    already_seen = true;
                    break;
                }
            }
            if (already_seen) {
                continue;
            }

            seen_entries.push_back(entry_name);
            parts.push_back(inspected_part_entry{
                relationship.attribute("Id").value(), entry_name, {}, {}});
        }
    }

    std::string document_xml_text;
    const auto document_status =
        read_zip_entry_text(archive, "word/document.xml", document_xml_text);
    if (document_status != zip_entry_read_status::ok) {
        close_archive();
        error_message = "failed to read XML entry from archive 'word/document.xml'";
        return false;
    }

    pugi::xml_document document_xml;
    const auto document_parse_result =
        document_xml.load_buffer(document_xml_text.data(), document_xml_text.size());
    if (!document_parse_result) {
        close_archive();
        error_message = "failed to parse XML entry 'word/document.xml'";
        return false;
    }

    close_archive();

    const auto body = document_xml.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        error_message =
            "document.xml does not contain the expected w:document/w:body structure";
        return false;
    }

    const auto collect_references = [&](pugi::xml_node section_properties,
                                        std::size_t section_index) {
        for (auto reference = section_properties.child(section_reference_name(family));
             reference != pugi::xml_node{};
             reference = reference.next_sibling(section_reference_name(family))) {
            const auto relationship_id =
                std::string_view{reference.attribute("r:id").value()};
            if (relationship_id.empty()) {
                continue;
            }

            const auto kind_attribute =
                std::string_view{reference.attribute("w:type").value()};
            const auto kind = kind_attribute.empty() ? std::string{"default"}
                                                     : std::string{kind_attribute};

            for (auto &part : parts) {
                if (part.relationship_id == relationship_id) {
                    part.references.push_back(
                        inspected_part_reference{section_index, kind});
                    break;
                }
            }
        }
    };

    std::size_t section_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (const auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            collect_references(section_properties, section_index);
            ++section_index;
        }
    }

    if (const auto final_section_properties = body.child("w:sectPr");
        final_section_properties != pugi::xml_node{}) {
        collect_references(final_section_properties, section_index);
    }

    const auto expected_count = family == section_part_family::header
                                    ? doc.header_count()
                                    : doc.footer_count();
    if (parts.size() != expected_count) {
        error_message = "CLI part inspection order does not match loaded document parts";
        return false;
    }

    for (std::size_t index = 0; index < parts.size(); ++index) {
        parts[index].paragraphs =
            collect_part_lines(section_part_paragraphs(doc, family, index));
    }

    return true;
}

[[nodiscard]] auto show_section_text(
    featherdoc::Document &doc, section_part_family family,
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind, bool json_output) -> bool {
    const auto lines = collect_part_lines(
        section_part_paragraphs(doc, family, section_index, reference_kind));

    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, section_part_name(family));
        std::cout << ",\"section\":" << section_index << ",\"kind\":";
        write_json_string(std::cout,
                          featherdoc::to_xml_reference_type(reference_kind));
        std::cout << ",\"present\":" << (!lines.empty() ? "true" : "false")
                  << ",\"paragraphs\":";
        write_json_lines(std::cout, lines);
        std::cout << "}\n";
        return true;
    }

    if (lines.empty()) {
        std::cerr << "section " << section_part_name(family)
                  << " reference not found\n";
        return false;
    }

    for (const auto &line : lines) {
        std::cout << line << '\n';
    }

    return true;
}

[[nodiscard]] auto replace_section_text(
    featherdoc::Document &doc, section_part_family family,
    std::size_t section_index, std::string_view replacement_text,
    featherdoc::section_reference_kind reference_kind,
    std::string_view command = {}, bool json_output = false) -> bool {
    const auto success =
        family == section_part_family::header
            ? doc.replace_section_header_text(section_index, replacement_text,
                                              reference_kind)
            : doc.replace_section_footer_text(section_index, replacement_text,
                                              reference_kind);

    if (!success) {
        return report_document_error(command, "mutate", doc.last_error(),
                                     json_output);
    }

    return success;
}

#include "featherdoc_cli_section_part_inspect_commands.inc"

#include "featherdoc_cli_section_part_text_commands.inc"

} // namespace

#include "featherdoc_cli_section_part_dispatch.inc"

} // namespace featherdoc_cli
