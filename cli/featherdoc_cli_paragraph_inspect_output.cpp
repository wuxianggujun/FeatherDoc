#include "featherdoc_cli_paragraph_inspect_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_body_paragraph_summary(
    std::ostream &stream, const inspected_body_paragraph &paragraph) {
    stream << "{\"index\":" << paragraph.index
           << ",\"section_index\":" << paragraph.section_index
           << ",\"style_id\":";
    write_json_optional_string(stream, paragraph.style_id);
    stream << ",\"numbering\":";
    if (paragraph.numbering_id.has_value() ||
        paragraph.numbering_level.has_value()) {
        stream << "{\"num_id\":";
        write_json_optional_u32(stream, paragraph.numbering_id);
        stream << ",\"level\":";
        write_json_optional_u32(stream, paragraph.numbering_level);
        stream << '}';
    } else {
        stream << "null";
    }
    stream << ",\"section_break\":" << json_bool(paragraph.has_section_break)
           << ",\"text\":";
    write_json_string(stream, paragraph.text);
    stream << '}';
}

void write_json_body_run_summary(std::ostream &stream,
                                 const inspected_body_run &run) {
    stream << "{\"index\":" << run.index << ",\"style_id\":";
    write_json_optional_string(stream, run.style_id);
    stream << ",\"font_family\":";
    write_json_optional_string(stream, run.font_family);
    stream << ",\"east_asia_font_family\":";
    write_json_optional_string(stream, run.east_asia_font_family);
    stream << ",\"text_color\":";
    write_json_optional_string(stream, run.text_color);
    stream << ",\"bold\":";
    write_json_optional_bool(stream, run.bold);
    stream << ",\"italic\":";
    write_json_optional_bool(stream, run.italic);
    stream << ",\"underline\":";
    write_json_optional_bool(stream, run.underline);
    stream << ",\"font_size_points\":";
    write_json_optional_double(stream, run.font_size_points);
    stream << ",\"language\":";
    write_json_optional_string(stream, run.language);
    stream << ",\"text\":";
    write_json_string(stream, run.text);
    stream << '}';
}
void print_body_paragraph_numbering(
    std::ostream &stream, const inspected_body_paragraph &paragraph) {
    if (!paragraph.numbering_id.has_value() &&
        !paragraph.numbering_level.has_value()) {
        stream << "none";
        return;
    }

    stream << "num_id=";
    if (paragraph.numbering_id.has_value()) {
        stream << *paragraph.numbering_id;
    } else {
        stream << "unknown";
    }

    stream << " level=";
    if (paragraph.numbering_level.has_value()) {
        stream << *paragraph.numbering_level;
    } else {
        stream << "unknown";
    }
}

void print_body_paragraph_summary(
    std::ostream &stream, const inspected_body_paragraph &paragraph) {
    stream << "paragraph[" << paragraph.index
           << "]: section_index=" << paragraph.section_index << " style_id=";
    if (paragraph.style_id.has_value()) {
        stream << *paragraph.style_id;
    } else {
        stream << "none";
    }

    stream << " numbering=";
    print_body_paragraph_numbering(stream, paragraph);
    stream << " section_break="
           << (paragraph.has_section_break ? "yes" : "no")
           << " text=" << format_paragraph_text(paragraph.text);
}

void print_body_run_summary(std::ostream &stream,
                            const inspected_body_run &run) {
    stream << "run[" << run.index << "]: style_id=";
    if (run.style_id.has_value()) {
        stream << *run.style_id;
    } else {
        stream << "none";
    }
    stream << " font_family=";
    if (run.font_family.has_value()) {
        stream << *run.font_family;
    } else {
        stream << "none";
    }
    stream << " east_asia_font_family=";
    if (run.east_asia_font_family.has_value()) {
        stream << *run.east_asia_font_family;
    } else {
        stream << "none";
    }
    stream << " text_color=";
    if (run.text_color.has_value()) {
        stream << *run.text_color;
    } else {
        stream << "none";
    }
    stream << " bold=";
    if (run.bold.has_value()) {
        stream << yes_no(*run.bold);
    } else {
        stream << "none";
    }
    stream << " italic=";
    if (run.italic.has_value()) {
        stream << yes_no(*run.italic);
    } else {
        stream << "none";
    }
    stream << " underline=";
    if (run.underline.has_value()) {
        stream << yes_no(*run.underline);
    } else {
        stream << "none";
    }
    stream << " font_size_points=";
    if (run.font_size_points.has_value()) {
        stream << *run.font_size_points;
    } else {
        stream << "none";
    }
    stream << " language=";
    if (run.language.has_value()) {
        stream << *run.language;
    } else {
        stream << "none";
    }
    stream << " text=" << format_paragraph_text(run.text);
}

} // namespace

void inspect_paragraphs(
    const std::vector<inspected_body_paragraph> &paragraphs,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << paragraphs.size()
                  << ",\"paragraphs\":[";
        for (std::size_t index = 0; index < paragraphs.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_body_paragraph_summary(std::cout, paragraphs[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "paragraphs: " << paragraphs.size() << '\n';
    for (const auto &paragraph : paragraphs) {
        print_body_paragraph_summary(std::cout, paragraph);
        std::cout << '\n';
    }
}

void inspect_paragraph(const inspected_body_paragraph &paragraph,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"paragraph\":";
        write_json_body_paragraph_summary(std::cout, paragraph);
        std::cout << "}\n";
        return;
    }

    std::cout << "index: " << paragraph.index << '\n'
              << "section_index: " << paragraph.section_index << '\n'
              << "style_id: ";
    if (paragraph.style_id.has_value()) {
        std::cout << *paragraph.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "numbering: ";
    print_body_paragraph_numbering(std::cout, paragraph);
    std::cout << '\n'
              << "section_break: "
              << (paragraph.has_section_break ? "yes" : "no") << '\n'
              << "text: " << format_paragraph_text(paragraph.text) << '\n';
}

void inspect_runs(std::size_t paragraph_index,
                  const std::vector<inspected_body_run> &runs,
                  bool json_output) {
    if (json_output) {
        std::cout << "{\"paragraph_index\":" << paragraph_index
                  << ",\"count\":" << runs.size() << ",\"runs\":[";
        for (std::size_t index = 0; index < runs.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_body_run_summary(std::cout, runs[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "paragraph_index: " << paragraph_index << '\n'
              << "runs: " << runs.size() << '\n';
    for (const auto &run : runs) {
        print_body_run_summary(std::cout, run);
        std::cout << '\n';
    }
}

void inspect_run(std::size_t paragraph_index, const inspected_body_run &run,
                 bool json_output) {
    if (json_output) {
        std::cout << "{\"paragraph_index\":" << paragraph_index << ",\"run\":";
        write_json_body_run_summary(std::cout, run);
        std::cout << "}\n";
        return;
    }

    std::cout << "paragraph_index: " << paragraph_index << '\n'
              << "index: " << run.index << '\n'
              << "style_id: ";
    if (run.style_id.has_value()) {
        std::cout << *run.style_id;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "font_family: ";
    if (run.font_family.has_value()) {
        std::cout << *run.font_family;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "east_asia_font_family: ";
    if (run.east_asia_font_family.has_value()) {
        std::cout << *run.east_asia_font_family;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text_color: ";
    if (run.text_color.has_value()) {
        std::cout << *run.text_color;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "bold: ";
    if (run.bold.has_value()) {
        std::cout << yes_no(*run.bold);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "italic: ";
    if (run.italic.has_value()) {
        std::cout << yes_no(*run.italic);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "underline: ";
    if (run.underline.has_value()) {
        std::cout << yes_no(*run.underline);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "font_size_points: ";
    if (run.font_size_points.has_value()) {
        std::cout << *run.font_size_points;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "language: ";
    if (run.language.has_value()) {
        std::cout << *run.language;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text: " << format_paragraph_text(run.text) << '\n';
}

} // namespace featherdoc_cli
