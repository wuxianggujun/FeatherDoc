#include "featherdoc_cli_section_part_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_style_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_section_management_commands.hpp"
#include "featherdoc_cli_section_options_parse.hpp"
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

void write_json_section_flags(std::ostream &stream, featherdoc::Document &doc,
                              std::size_t section_index,
                              section_part_family family) {
    const auto has_default =
        family == section_part_family::header
            ? has_section_header(
                  doc, section_index,
                  featherdoc::section_reference_kind::default_reference)
            : has_section_footer(
                  doc, section_index,
                  featherdoc::section_reference_kind::default_reference);
    const auto has_first =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::first_page)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::first_page);
    const auto has_even =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::even_page)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::even_page);

    stream << "{\"default\":" << (has_default ? "true" : "false")
           << ",\"first\":" << (has_first ? "true" : "false")
           << ",\"even\":" << (has_even ? "true" : "false") << '}';
}

void write_json_part_references(
    std::ostream &stream,
    const std::vector<inspected_part_reference> &references) {
    stream << '[';
    for (std::size_t index = 0; index < references.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"section\":" << references[index].section_index
               << ",\"kind\":";
        write_json_string(stream, references[index].kind);
        stream << '}';
    }
    stream << ']';
}

void inspect_sections(featherdoc::Document &doc, bool json_output) {
    if (json_output) {
        std::cout << "{\"sections\":" << doc.section_count()
                  << ",\"headers\":" << doc.header_count()
                  << ",\"footers\":" << doc.footer_count()
                  << ",\"section_layouts\":[";
        for (std::size_t section_index = 0; section_index < doc.section_count();
             ++section_index) {
            if (section_index != 0U) {
                std::cout << ',';
            }

            std::cout << "{\"index\":" << section_index << ",\"header\":";
            write_json_section_flags(std::cout, doc, section_index,
                                     section_part_family::header);
            std::cout << ",\"footer\":";
            write_json_section_flags(std::cout, doc, section_index,
                                     section_part_family::footer);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "sections: " << doc.section_count() << '\n';
    std::cout << "headers: " << doc.header_count() << '\n';
    std::cout << "footers: " << doc.footer_count() << '\n';

    for (std::size_t section_index = 0; section_index < doc.section_count();
         ++section_index) {
        std::cout
            << "section[" << section_index << "]: header(default="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::default_reference))
            << ", first="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::first_page))
            << ", even="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::even_page))
            << ") footer(default="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::default_reference))
            << ", first="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::first_page))
            << ", even="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::even_page))
            << ")\n";
    }
}

void inspect_related_parts(const std::vector<inspected_part_entry> &parts,
                           section_part_family family, bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, section_part_name(family));
        std::cout << ",\"count\":" << parts.size() << ",\"parts\":[";
        for (std::size_t index = 0; index < parts.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }

            const auto &part = parts[index];
            std::cout << "{\"index\":" << index << ",\"relationship_id\":";
            write_json_string(std::cout, part.relationship_id);
            std::cout << ",\"entry\":";
            write_json_string(std::cout, part.entry_name);
            std::cout << ",\"references\":";
            write_json_part_references(std::cout, part.references);
            std::cout << ",\"paragraphs\":";
            write_json_lines(std::cout, part.paragraphs);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << section_part_name(family) << " parts: " << parts.size() << '\n';
    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto &part = parts[index];
        std::cout << "part[" << index << "]: entry=" << part.entry_name
                  << " relationship=" << part.relationship_id << " references=";
        if (part.references.empty()) {
            std::cout << "none";
        } else {
            for (std::size_t reference_index = 0;
                 reference_index < part.references.size(); ++reference_index) {
                if (reference_index != 0U) {
                    std::cout << ", ";
                }
                std::cout << "section["
                          << part.references[reference_index].section_index
                          << "]:" << part.references[reference_index].kind;
            }
        }
        std::cout << '\n';

        for (std::size_t paragraph_index = 0;
             paragraph_index < part.paragraphs.size(); ++paragraph_index) {
            std::cout << "  paragraph[" << paragraph_index
                      << "]: " << part.paragraphs[paragraph_index] << '\n';
        }
    }
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

[[nodiscard]] auto remove_part(featherdoc::Document &doc,
                               section_part_family family,
                               std::size_t part_index,
                               std::string_view command,
                               bool json_output) -> bool {
    const auto success = family == section_part_family::header
                             ? doc.remove_header_part(part_index)
                             : doc.remove_footer_part(part_index);
    if (!success) {
        std::string message =
            std::string(section_part_name(family)) + " part not found";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

[[nodiscard]] auto move_part(featherdoc::Document &doc, section_part_family family,
                             std::size_t source_index,
                             std::size_t target_index,
                             std::string_view command,
                             bool json_output) -> bool {
    const auto success = family == section_part_family::header
                             ? doc.move_header_part(source_index, target_index)
                             : doc.move_footer_part(source_index, target_index);
    if (!success) {
        std::string message =
            std::string(section_part_name(family)) + " part could not be reordered";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
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

[[nodiscard]] auto run_inspect_sections_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-sections expects an input path",
                          json_output);
        return 2;
    }

    inspect_options options;
    std::string error_message;
    if (!parse_inspect_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    inspect_sections(doc, options.json_output);
    return 0;
}

[[nodiscard]] auto run_inspect_related_parts_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "inspect-header/footer-parts expects an input path",
                          json_output);
        return 2;
    }

    inspect_options options;
    std::string error_message;
    if (!parse_inspect_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto family = part_family_for_command(command);
    const auto input_path = path_type(std::string(arguments[1]));
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    std::vector<inspected_part_entry> parts;
    if (!load_inspected_part_entries(input_path, doc, family, parts,
                                     error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "inspect", error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    inspect_related_parts(parts, family, options.json_output);
    return 0;
}

[[nodiscard]] auto run_remove_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "remove-header/footer-part expects an input path and "
                          "a part index",
                          json_output);
        return 2;
    }

    std::size_t part_index = 0;
    if (!parse_index(arguments[2], part_index)) {
        print_parse_error(command,
                          "invalid part index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    section_part_command_options options;
    std::string error_message;
    if (!parse_section_part_command_options(arguments, 3U, false, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto family = part_family_for_command(command);
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!remove_part(doc, family, part_index, command, options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [family, part_index](std::ostream &stream) {
                stream << ",\"part\":";
                write_json_string(stream, section_part_name(family));
                stream << ",\"part_index\":" << part_index;
            });
    }

    return 0;
}

[[nodiscard]] auto run_move_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(command,
                          "move-header/footer-part expects an input path, a "
                          "source index, and a target index",
                          json_output);
        return 2;
    }

    std::size_t source_index = 0;
    std::size_t target_index = 0;
    if (!parse_index(arguments[2], source_index)) {
        print_parse_error(command,
                          "invalid source index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }
    if (!parse_index(arguments[3], target_index)) {
        print_parse_error(command,
                          "invalid target index: " + std::string(arguments[3]),
                          json_output);
        return 2;
    }

    section_part_command_options options;
    std::string error_message;
    if (!parse_section_part_command_options(arguments, 4U, false, options,
                                            error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto family = part_family_for_command(command);
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!move_part(doc, family, source_index, target_index, command,
                   options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [family, source_index, target_index](std::ostream &stream) {
                stream << ",\"part\":";
                write_json_string(stream, section_part_name(family));
                stream << ",\"source\":" << source_index
                       << ",\"target\":" << target_index;
            });
    }

    return 0;
}

[[nodiscard]] auto run_show_section_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "show-section-header/footer expects an input path and "
                          "a section index",
                          json_output);
        return 2;
    }

    std::size_t section_index = 0;
    if (!parse_index(arguments[2], section_index)) {
        print_parse_error(command,
                          "invalid section index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    section_text_options options;
    std::string error_message;
    if (!parse_section_text_options(arguments, 3U, false, false, true, options,
                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    return show_section_text(doc, part_family_for_command(command), section_index,
                             options.reference_kind, options.json_output)
               ? 0
               : 1;
}

[[nodiscard]] auto run_set_section_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "set-section-header/footer expects an input path and "
                          "a section index",
                          json_output);
        return 2;
    }

    std::size_t section_index = 0;
    if (!parse_index(arguments[2], section_index)) {
        print_parse_error(command,
                          "invalid section index: " + std::string(arguments[2]),
                          json_output);
        return 2;
    }

    section_text_options options;
    std::string error_message;
    if (!parse_section_text_options(arguments, 3U, true, true, true, options,
                                    error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    std::string replacement_text;
    if (!read_text_source(options, replacement_text, error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "input", error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    const auto family = part_family_for_command(command);
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (!replace_section_text(doc, family, section_index, replacement_text,
                              options.reference_kind, command,
                              options.json_output)) {
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [family, section_index, &options](std::ostream &stream) {
                stream << ",\"part\":";
                write_json_string(stream, section_part_name(family));
                stream << ",\"section\":" << section_index << ",\"kind\":";
                write_json_string(
                    stream, featherdoc::to_xml_reference_type(options.reference_kind));
            });
    }

    return 0;
}

} // namespace

auto is_section_part_command(std::string_view command) -> bool {
    return command == "inspect-sections" ||
           command == "inspect-header-parts" ||
           command == "inspect-footer-parts" ||
           is_section_management_command(command) ||
           is_section_part_reference_command(command) ||
           command == "remove-header-part" ||
           command == "remove-footer-part" ||
           command == "move-header-part" || command == "move-footer-part" ||
           command == "show-section-header" ||
           command == "show-section-footer" ||
           command == "set-section-header" ||
           command == "set-section-footer";
}

auto run_section_part_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-sections") {
        return run_inspect_sections_command(command, arguments, doc);
    }
    if (command == "inspect-header-parts" || command == "inspect-footer-parts") {
        return run_inspect_related_parts_command(command, arguments, doc);
    }
    if (is_section_management_command(command)) {
        return run_section_management_command(command, arguments, doc);
    }
    if (is_section_part_reference_command(command)) {
        return run_section_part_reference_command(command, arguments, doc);
    }
    if (command == "remove-header-part" || command == "remove-footer-part") {
        return run_remove_part_command(command, arguments, doc);
    }
    if (command == "move-header-part" || command == "move-footer-part") {
        return run_move_part_command(command, arguments, doc);
    }
    if (command == "show-section-header" || command == "show-section-footer") {
        return run_show_section_part_command(command, arguments, doc);
    }
    if (command == "set-section-header" || command == "set-section-footer") {
        return run_set_section_part_command(command, arguments, doc);
    }

    print_parse_error(command, "unsupported section part command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
