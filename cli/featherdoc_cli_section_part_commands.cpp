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

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

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

[[nodiscard]] auto load_inspected_part_entries(
    featherdoc::Document &doc, section_part_family family,
    std::vector<inspected_part_entry> &parts,
    std::string &error_message) -> bool {
    parts.clear();
    const auto loaded_parts =
        family == section_part_family::header ? doc.inspect_header_parts()
                                               : doc.inspect_footer_parts();
    if (doc.last_error().code) {
        error_message = !doc.last_error().detail.empty()
                            ? doc.last_error().detail
                            : doc.last_error().code.message();
        return false;
    }

    parts.reserve(loaded_parts.size());
    for (const auto &loaded_part : loaded_parts) {
        auto part = inspected_part_entry{};
        part.relationship_id = loaded_part.relationship_id;
        part.entry_name = loaded_part.entry_name;
        part.references.reserve(loaded_part.references.size());
        for (const auto &reference : loaded_part.references) {
            part.references.push_back(inspected_part_reference{
                reference.section_index,
                std::string{featherdoc::to_xml_reference_type(
                    reference.reference_kind)}});
        }
        part.paragraphs = collect_part_lines(
            section_part_paragraphs(doc, family, loaded_part.index));
        parts.push_back(std::move(part));
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
