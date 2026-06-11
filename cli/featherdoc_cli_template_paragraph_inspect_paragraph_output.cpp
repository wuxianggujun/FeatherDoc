#include "featherdoc_cli_template_paragraph_inspect_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>

namespace featherdoc_cli {
namespace {

void write_json_template_paragraph_numbering_summary(
    std::ostream &stream,
    const featherdoc::paragraph_inspection_summary::numbering_summary
        &numbering) {
    stream << "{\"num_id\":";
    write_json_optional_u32(stream, numbering.num_id);
    stream << ",\"level\":";
    write_json_optional_u32(stream, numbering.level);
    stream << ",\"definition_id\":";
    write_json_optional_u32(stream, numbering.definition_id);
    stream << ",\"definition_name\":";
    write_json_optional_string(stream, numbering.definition_name);
    stream << '}';
}

void write_json_template_paragraph_summary(
    std::ostream &stream,
    const featherdoc::paragraph_inspection_summary &paragraph) {
    stream << "{\"index\":" << paragraph.index << ",\"style_id\":";
    write_json_optional_string(stream, paragraph.style_id);
    stream << ",\"bidi\":";
    write_json_optional_bool(stream, paragraph.bidi);
    stream << ",\"numbering\":";
    if (paragraph.numbering.has_value()) {
        write_json_template_paragraph_numbering_summary(
            stream, *paragraph.numbering);
    } else {
        stream << "null";
    }
    stream << ",\"run_count\":" << paragraph.run_count << ",\"text\":";
    write_json_string(stream, paragraph.text);
    stream << '}';
}

void print_template_paragraph_numbering(
    std::ostream &stream,
    const featherdoc::paragraph_inspection_summary &paragraph) {
    if (!paragraph.numbering.has_value()) {
        stream << "none";
        return;
    }

    stream << "num_id=";
    if (paragraph.numbering->num_id.has_value()) {
        stream << *paragraph.numbering->num_id;
    } else {
        stream << "none";
    }
    stream << " level=";
    if (paragraph.numbering->level.has_value()) {
        stream << *paragraph.numbering->level;
    } else {
        stream << "none";
    }
    stream << " definition_id=";
    if (paragraph.numbering->definition_id.has_value()) {
        stream << *paragraph.numbering->definition_id;
    } else {
        stream << "none";
    }
    stream << " definition_name=";
    if (paragraph.numbering->definition_name.has_value()) {
        stream << *paragraph.numbering->definition_name;
    } else {
        stream << "none";
    }
}

void print_template_paragraph_summary(
    std::ostream &stream,
    const featherdoc::paragraph_inspection_summary &paragraph) {
    stream << "paragraph[" << paragraph.index << "]: style_id=";
    if (paragraph.style_id.has_value()) {
        stream << *paragraph.style_id;
    } else {
        stream << "none";
    }
    stream << " bidi=";
    if (paragraph.bidi.has_value()) {
        stream << yes_no(*paragraph.bidi);
    } else {
        stream << "none";
    }
    stream << " numbering=";
    print_template_paragraph_numbering(stream, paragraph);
    stream << " runs=" << paragraph.run_count
           << " text=" << format_paragraph_text(paragraph.text);
}

} // namespace

void inspect_template_paragraphs(
    const selected_template_part &selected,
    const std::vector<featherdoc::paragraph_inspection_summary> &paragraphs,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"count\":" << paragraphs.size()
                  << ",\"paragraphs\":[";
        for (std::size_t index = 0; index < paragraphs.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_paragraph_summary(std::cout, paragraphs[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "paragraphs: " << paragraphs.size() << '\n';
    for (const auto &paragraph : paragraphs) {
        print_template_paragraph_summary(std::cout, paragraph);
        std::cout << '\n';
    }
}

void inspect_template_paragraph(
    const selected_template_part &selected,
    const featherdoc::paragraph_inspection_summary &paragraph,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"paragraph\":";
        write_json_template_paragraph_summary(std::cout, paragraph);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "index: " << paragraph.index << '\n' << "style_id: ";
    if (paragraph.style_id.has_value()) {
        std::cout << *paragraph.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "bidi: ";
    if (paragraph.bidi.has_value()) {
        std::cout << yes_no(*paragraph.bidi);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "numbering: ";
    print_template_paragraph_numbering(std::cout, paragraph);
    std::cout << '\n' << "run_count: " << paragraph.run_count << '\n'
              << "text: " << format_paragraph_text(paragraph.text) << '\n';
}

} // namespace featherdoc_cli
