#include "featherdoc_cli_paragraph_inspect_commands.hpp"

#include "featherdoc_cli_template_paragraph_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_content_options_parse.hpp"
#include "featherdoc_cli_paragraph_inspect_output.hpp"
#include "featherdoc_cli_paragraph_run_options_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <featherdoc.hpp>

#include <pugixml.hpp>
#include <zip.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

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
} // namespace

auto is_paragraph_inspect_command(std::string_view command) -> bool {
    return command == "inspect-paragraphs" || command == "inspect-runs" ||
           is_template_paragraph_inspect_command(command);
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
    if (is_template_paragraph_inspect_command(command)) {
        return run_template_paragraph_inspect_command(command, arguments);
    }

    print_parse_error(command,
                      "unsupported paragraph inspect command: " +
                          std::string(command),
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
