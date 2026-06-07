#include "featherdoc_cli_paragraph_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_content_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_paragraph_run_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_inspect_options_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_text.hpp"

#include <featherdoc.hpp>

#include <pugixml.hpp>
#include <zip.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

struct inspected_body_paragraph {
    std::size_t index = 0;
    std::size_t section_index = 0;
    std::optional<std::string> style_id;
    std::optional<std::uint32_t> numbering_id;
    std::optional<std::uint32_t> numbering_level;
    bool has_section_break = false;
    std::string text;
};

struct inspected_body_run {
    std::size_t index = 0;
    std::optional<std::string> style_id;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<std::string> text_color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<bool> underline;
    std::optional<double> font_size_points;
    std::optional<std::string> language;
    std::string text;
};

enum class zip_entry_read_status {
    missing,
    read_failed,
    ok,
};

auto read_zip_entry_text(zip_t *archive, std::string_view entry_name,
                         std::string &content) -> zip_entry_read_status {
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

void append_xml_paragraph_text(std::string &text, pugi::xml_node node) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto name = std::string_view{child.name()};
        if (name == "w:t" || name == "w:delText") {
            text += child.text().get();
            continue;
        }

        if (name == "w:tab") {
            text.push_back('\t');
            continue;
        }

        if (name == "w:br" || name == "w:cr") {
            text.push_back('\n');
            continue;
        }

        append_xml_paragraph_text(text, child);
    }
}

auto collect_xml_paragraph_text(pugi::xml_node paragraph) -> std::string {
    std::string text;
    append_xml_paragraph_text(text, paragraph);
    return text;
}

auto read_optional_xml_attribute(pugi::xml_node node,
                                 const char *attribute_name)
    -> std::optional<std::string> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute(attribute_name).value()};
    if (value.empty()) {
        return std::nullopt;
    }

    return std::string(value);
}

auto read_optional_run_font_family(pugi::xml_node run_fonts)
    -> std::optional<std::string> {
    if (const auto ascii = read_optional_xml_attribute(run_fonts, "w:ascii");
        ascii.has_value()) {
        return ascii;
    }

    if (const auto hansi = read_optional_xml_attribute(run_fonts, "w:hAnsi");
        hansi.has_value()) {
        return hansi;
    }

    return read_optional_xml_attribute(run_fonts, "w:cs");
}

auto read_optional_on_off_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value.empty()) {
        return true;
    }

    return value != "0" && value != "false" && value != "off";
}

auto read_optional_underline_value(pugi::xml_node node) -> std::optional<bool> {
    if (node == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value.empty()) {
        return true;
    }

    return value != "0" && value != "false" && value != "none";
}

auto read_optional_font_size_points(pugi::xml_node node)
    -> std::optional<double> {
    const auto value = std::string_view{node.attribute("w:val").value()};
    if (value.empty()) {
        return std::nullopt;
    }

    std::uint32_t half_points = 0U;
    if (!parse_uint32(value, half_points) || half_points == 0U) {
        return std::nullopt;
    }

    return static_cast<double>(half_points) / 2.0;
}

auto load_body_paragraph_summaries(
    const path_type &input_path,
    std::vector<inspected_body_paragraph> &paragraphs,
    std::string &error_message) -> bool {
    paragraphs.clear();

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(input_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        error_message =
            "failed to open source archive '" + input_path.string() + "'";
        return false;
    }

    auto close_archive = [&]() { zip_close(archive); };

    std::string document_xml_text;
    const auto document_status =
        read_zip_entry_text(archive, "word/document.xml", document_xml_text);
    if (document_status != zip_entry_read_status::ok) {
        close_archive();
        error_message =
            "failed to read XML entry from archive 'word/document.xml'";
        return false;
    }

    pugi::xml_document document_xml;
    const auto document_parse_result =
        document_xml.load_buffer(document_xml_text.data(),
                                 document_xml_text.size());
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

    std::size_t section_index = 0U;
    std::size_t paragraph_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        inspected_body_paragraph paragraph_summary;
        paragraph_summary.index = paragraph_index;
        paragraph_summary.section_index = section_index;

        const auto properties = child.child("w:pPr");
        if (const auto style = properties.child("w:pStyle");
            style != pugi::xml_node{}) {
            const auto style_id =
                std::string_view{style.attribute("w:val").value()};
            if (!style_id.empty()) {
                paragraph_summary.style_id = std::string(style_id);
            }
        }

        if (const auto numbering = properties.child("w:numPr");
            numbering != pugi::xml_node{}) {
            const auto numbering_level_text = std::string_view{
                numbering.child("w:ilvl").attribute("w:val").value()};
            if (!numbering_level_text.empty()) {
                std::uint32_t numbering_level = 0U;
                if (!parse_uint32(numbering_level_text, numbering_level)) {
                    error_message = "invalid numbering level on paragraph index " +
                                    std::to_string(paragraph_index) +
                                    " in word/document.xml";
                    return false;
                }
                paragraph_summary.numbering_level = numbering_level;
            }

            const auto numbering_id_text = std::string_view{
                numbering.child("w:numId").attribute("w:val").value()};
            if (!numbering_id_text.empty()) {
                std::uint32_t numbering_id = 0U;
                if (!parse_uint32(numbering_id_text, numbering_id)) {
                    error_message =
                        "invalid numbering instance id on paragraph index " +
                        std::to_string(paragraph_index) +
                        " in word/document.xml";
                    return false;
                }
                paragraph_summary.numbering_id = numbering_id;
            }
        }

        paragraph_summary.has_section_break =
            properties.child("w:sectPr") != pugi::xml_node{};
        paragraph_summary.text = collect_xml_paragraph_text(child);
        paragraphs.push_back(std::move(paragraph_summary));

        if (paragraphs.back().has_section_break) {
            ++section_index;
        }
        ++paragraph_index;
    }

    return true;
}

auto find_body_paragraph_node(pugi::xml_node body, std::size_t paragraph_index)
    -> pugi::xml_node {
    std::size_t current_index = 0U;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (current_index == paragraph_index) {
            return paragraph;
        }
        ++current_index;
    }

    return {};
}

auto load_body_run_summaries(const path_type &input_path,
                             std::size_t paragraph_index,
                             std::vector<inspected_body_run> &runs,
                             bool &paragraph_found,
                             std::string &error_message) -> bool {
    runs.clear();
    paragraph_found = false;

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(input_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        error_message =
            "failed to open source archive '" + input_path.string() + "'";
        return false;
    }

    auto close_archive = [&]() { zip_close(archive); };

    std::string document_xml_text;
    const auto document_status =
        read_zip_entry_text(archive, "word/document.xml", document_xml_text);
    if (document_status != zip_entry_read_status::ok) {
        close_archive();
        error_message =
            "failed to read XML entry from archive 'word/document.xml'";
        return false;
    }

    pugi::xml_document document_xml;
    const auto document_parse_result =
        document_xml.load_buffer(document_xml_text.data(),
                                 document_xml_text.size());
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

    const auto paragraph = find_body_paragraph_node(body, paragraph_index);
    if (paragraph == pugi::xml_node{}) {
        return true;
    }

    paragraph_found = true;
    std::size_t current_run_index = 0U;
    for (auto run = paragraph.child("w:r"); run != pugi::xml_node{};
         run = run.next_sibling("w:r")) {
        inspected_body_run run_summary;
        run_summary.index = current_run_index;
        const auto run_properties = run.child("w:rPr");
        const auto run_fonts = run_properties.child("w:rFonts");

        run_summary.style_id =
            read_optional_xml_attribute(run_properties.child("w:rStyle"),
                                        "w:val");
        run_summary.font_family = read_optional_run_font_family(run_fonts);
        run_summary.east_asia_font_family =
            read_optional_xml_attribute(run_fonts, "w:eastAsia");
        run_summary.text_color =
            read_optional_xml_attribute(run_properties.child("w:color"),
                                        "w:val");
        run_summary.bold =
            read_optional_on_off_value(run_properties.child("w:b"));
        run_summary.italic =
            read_optional_on_off_value(run_properties.child("w:i"));
        run_summary.underline =
            read_optional_underline_value(run_properties.child("w:u"));
        run_summary.font_size_points =
            read_optional_font_size_points(run_properties.child("w:sz"));
        run_summary.language =
            read_optional_xml_attribute(run_properties.child("w:lang"),
                                        "w:val");

        run_summary.text = collect_xml_paragraph_text(run);
        runs.push_back(std::move(run_summary));
        ++current_run_index;
    }

    return true;
}

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

auto run_inspect_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-paragraphs expects an input path",
                          json_output);
        return 2;
    }

    inspect_paragraphs_options options;
    std::string error_message;
    if (!parse_inspect_paragraphs_options(arguments, 2U, options,
                                          error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    featherdoc::Document doc;
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    std::vector<inspected_body_paragraph> paragraphs;
    if (!load_body_paragraph_summaries(input_path, paragraphs, error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "inspect",
                                     error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    if (options.paragraph_index.has_value()) {
        if (*options.paragraph_index >= paragraphs.size()) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "paragraph index '" +
                std::to_string(*options.paragraph_index) + "' is out of range";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "paragraph index is out of range",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_paragraph(paragraphs[*options.paragraph_index],
                          options.json_output);
        return 0;
    }

    inspect_paragraphs(paragraphs, options.json_output);
    return 0;
}

auto run_inspect_runs_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(
            command,
            "inspect-runs expects an input path and a paragraph index",
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

    inspect_runs_options options;
    std::string error_message;
    if (!parse_inspect_runs_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    const auto input_path = path_type(std::string(arguments[1]));
    featherdoc::Document doc;
    if (!open_document(input_path, doc, command, options.json_output)) {
        return 1;
    }

    std::vector<inspected_body_run> runs;
    bool paragraph_found = false;
    if (!load_body_run_summaries(input_path, paragraph_index, runs,
                                 paragraph_found, error_message)) {
        if (options.json_output) {
            write_json_command_error(std::cerr, command, "inspect",
                                     error_message);
        } else {
            std::cerr << error_message << '\n';
        }
        return 1;
    }

    if (!paragraph_found) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "paragraph index '" +
                            std::to_string(paragraph_index) +
                            "' is out of range";
        error_info.entry_name = "word/document.xml";
        report_operation_failure(command, "inspect",
                                 "paragraph index is out of range",
                                 error_info, options.json_output);
        return 1;
    }

    if (options.run_index.has_value()) {
        if (*options.run_index >= runs.size()) {
            featherdoc::document_error_info error_info{};
            error_info.code =
                std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "run index '" + std::to_string(*options.run_index) +
                "' is out of range for paragraph index '" +
                std::to_string(paragraph_index) + "'";
            error_info.entry_name = "word/document.xml";
            report_operation_failure(command, "inspect",
                                     "run index is out of range", error_info,
                                     options.json_output);
            return 1;
        }

        inspect_run(paragraph_index, runs[*options.run_index],
                    options.json_output);
        return 0;
    }

    inspect_runs(paragraph_index, runs, options.json_output);
    return 0;
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

auto is_paragraph_inspect_command(std::string_view command) -> bool {
    return command == "inspect-paragraphs" || command == "inspect-runs" ||
           command == "inspect-template-paragraphs" ||
           command == "inspect-template-runs";
}

auto run_paragraph_inspect_command(
    std::string_view command, const std::vector<std::string_view> &arguments)
    -> int {
    if (command == "inspect-paragraphs") {
        return run_inspect_paragraphs_command(command, arguments);
    }
    if (command == "inspect-runs") {
        return run_inspect_runs_command(command, arguments);
    }
    if (command == "inspect-template-paragraphs") {
        return run_inspect_template_paragraphs_command(command, arguments);
    }
    if (command == "inspect-template-runs") {
        return run_inspect_template_runs_command(command, arguments);
    }

    print_parse_error(command,
                      "unsupported paragraph inspect command: " +
                          std::string(command),
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
