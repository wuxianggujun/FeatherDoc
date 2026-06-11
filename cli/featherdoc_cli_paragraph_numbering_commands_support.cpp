#include "featherdoc_cli_paragraph_numbering_commands_support.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <system_error>

namespace featherdoc_cli {

auto parse_paragraph_numbering_index_argument(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::uint32_t &paragraph_index, bool json_output) -> bool {
    if (parse_uint32(arguments[2], paragraph_index)) {
        return true;
    }

    print_parse_error(command,
                      "invalid paragraph index: " +
                          std::string(arguments[2]),
                      json_output);
    return false;
}

auto resolve_paragraph_numbering_body_paragraph(
    std::string_view command, featherdoc::Document &doc,
    std::uint32_t paragraph_index, featherdoc::Paragraph &paragraph,
    bool json_output) -> bool {
    paragraph = doc.paragraphs();
    for (std::uint32_t current_index = 0U;
         current_index < paragraph_index && paragraph.has_next();
         ++current_index) {
        paragraph.next();
    }

    if (paragraph.has_next()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail =
        "paragraph index '" + std::to_string(paragraph_index) + "' is out of range";
    error_info.entry_name = "word/document.xml";
    return report_operation_failure(command, "mutate",
                                    "paragraph index is out of range", error_info,
                                    json_output);
}

void write_json_numbering_definition_levels(
    std::ostream &stream,
    const std::vector<featherdoc::numbering_level_definition> &levels) {
    stream << '[';
    for (std::size_t index = 0U; index < levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream, levels[index]);
    }
    stream << ']';
}

void write_json_paragraph_style_numbering_links(
    std::ostream &stream,
    const std::vector<featherdoc::paragraph_style_numbering_link> &style_links) {
    stream << '[';
    for (std::size_t index = 0U; index < style_links.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_paragraph_style_numbering_link(stream, style_links[index]);
    }
    stream << ']';
}

auto open_paragraph_numbering_document_for_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc, bool json_output) -> bool {
    return open_document(path_type(std::string(arguments[1])), doc, command,
                         json_output);
}

} // namespace featherdoc_cli
