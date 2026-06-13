#include "featherdoc_cli_paragraph_inspect_load.hpp"

#include "featherdoc_cli_parse.hpp"

#include <pugixml.hpp>
#include <zip.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

} // namespace

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

} // namespace featherdoc_cli
