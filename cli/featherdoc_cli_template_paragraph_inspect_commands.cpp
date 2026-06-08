#include "featherdoc_cli_template_paragraph_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_text.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

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

auto run_inspect_template_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "inspect-template-paragraphs expects an input path",
                          json_output);
        return 2;
    }

    template_inspect_paragraphs_options options;
    std::string error_message;
    if (!parse_template_inspect_paragraphs_options(arguments, 2U, options,
                                                   error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    if (options.paragraph_index.has_value()) {
        const auto paragraph =
            selected.part.inspect_paragraph(*options.paragraph_index);
        if (!paragraph.has_value()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "paragraph index '" +
                std::to_string(*options.paragraph_index) +
                "' is out of range";
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "inspect",
                                     "paragraph index is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_template_paragraph(selected, *paragraph, options.json_output);
        return 0;
    }

    const auto paragraphs = selected.part.inspect_paragraphs();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_template_paragraphs(selected, paragraphs, options.json_output);
    return 0;
}

auto run_inspect_template_runs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-template-runs expects an input path and a paragraph index",
            json_output);
        return 2;
    }

    std::size_t paragraph_index = 0U;
    if (!parse_index(arguments[2], paragraph_index)) {
        print_parse_error(command,
                          "invalid paragraph index: " +
                              std::string(arguments[2]),
                          json_output);
        return 2;
    }

    template_inspect_runs_options options;
    std::string error_message;
    if (!parse_template_inspect_runs_options(arguments, 3U, options,
                                             error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    featherdoc::Document doc;
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    const auto paragraph = selected.part.inspect_paragraph(paragraph_index);
    if (!paragraph.has_value()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail =
            "paragraph index '" + std::to_string(paragraph_index) +
            "' is out of range";
        error_info.entry_name = std::string(selected.part.entry_name());
        report_operation_failure(command, "inspect",
                                 "paragraph index is out of range", error_info,
                                 options.json_output);
        return 1;
    }

    if (options.run_index.has_value()) {
        const auto run = selected.part.inspect_paragraph_run(
            paragraph_index, *options.run_index);
        if (!run.has_value()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "run index '" + std::to_string(*options.run_index) +
                "' is out of range for paragraph index '" +
                std::to_string(paragraph_index) + "'";
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "inspect",
                                     "run index is out of range", error_info,
                                     options.json_output);
            return 1;
        }

        inspect_template_run(selected, paragraph_index, *run,
                             options.json_output);
        return 0;
    }

    const auto runs = selected.part.inspect_paragraph_runs(paragraph_index);
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_template_runs(selected, paragraph_index, runs, options.json_output);
    return 0;
}

} // namespace

auto is_template_paragraph_inspect_command(std::string_view command) -> bool {
    return command == "inspect-template-paragraphs" ||
           command == "inspect-template-runs";
}

auto run_template_paragraph_inspect_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "inspect-template-paragraphs") {
        return run_inspect_template_paragraphs_command(command, arguments);
    }
    if (command == "inspect-template-runs") {
        return run_inspect_template_runs_command(command, arguments);
    }

    print_parse_error(command,
                      "unsupported template paragraph inspect command: " +
                          std::string(command),
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
