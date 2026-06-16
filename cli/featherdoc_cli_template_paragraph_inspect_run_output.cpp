#include "featherdoc_cli_template_paragraph_inspect_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>

namespace featherdoc_cli {
namespace {

void write_json_template_run_summary(
    std::ostream &stream, const featherdoc::run_inspection_summary &run) {
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
    stream << ",\"east_asia_language\":";
    write_json_optional_string(stream, run.east_asia_language);
    stream << ",\"bidi_language\":";
    write_json_optional_string(stream, run.bidi_language);
    stream << ",\"rtl\":";
    write_json_optional_bool(stream, run.rtl);
    stream << ",\"text\":";
    write_json_string(stream, run.text);
    stream << '}';
}

void print_template_run_summary(std::ostream &stream,
                                const featherdoc::run_inspection_summary &run) {
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
    stream << " east_asia_language=";
    if (run.east_asia_language.has_value()) {
        stream << *run.east_asia_language;
    } else {
        stream << "none";
    }
    stream << " bidi_language=";
    if (run.bidi_language.has_value()) {
        stream << *run.bidi_language;
    } else {
        stream << "none";
    }
    stream << " rtl=";
    if (run.rtl.has_value()) {
        stream << yes_no(*run.rtl);
    } else {
        stream << "none";
    }
    stream << " text=" << format_paragraph_text(run.text);
}

} // namespace

void inspect_template_runs(
    const selected_template_part &selected, std::size_t paragraph_index,
    const std::vector<featherdoc::run_inspection_summary> &runs,
    bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"paragraph_index\":" << paragraph_index
                  << ",\"count\":" << runs.size() << ",\"runs\":[";
        for (std::size_t index = 0; index < runs.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_run_summary(std::cout, runs[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
    std::cout << "paragraph_index: " << paragraph_index << '\n'
              << "runs: " << runs.size() << '\n';
    for (const auto &run : runs) {
        print_template_run_summary(std::cout, run);
        std::cout << '\n';
    }
}

void inspect_template_run(const selected_template_part &selected,
                          std::size_t paragraph_index,
                          const featherdoc::run_inspection_summary &run,
                          bool json_output) {
    if (json_output) {
        std::cout << '{';
        write_json_selected_template_part(std::cout, selected);
        std::cout << ",\"paragraph_index\":" << paragraph_index << ",\"run\":";
        write_json_template_run_summary(std::cout, run);
        std::cout << "}\n";
        return;
    }

    print_selected_template_part(std::cout, selected);
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
    std::cout << '\n' << "east_asia_language: ";
    if (run.east_asia_language.has_value()) {
        std::cout << *run.east_asia_language;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "bidi_language: ";
    if (run.bidi_language.has_value()) {
        std::cout << *run.bidi_language;
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "rtl: ";
    if (run.rtl.has_value()) {
        std::cout << yes_no(*run.rtl);
    } else {
        std::cout << "none";
    }
    std::cout << '\n' << "text: " << format_paragraph_text(run.text) << '\n';
}

} // namespace featherdoc_cli
