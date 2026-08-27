#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_input.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace featherdoc_cli {

void skip_json_patch_whitespace(std::string_view text, std::size_t &index) {
    while (index < text.size()) {
        const auto character = text[index];
        if (character != ' ' && character != '\t' && character != '\n' &&
            character != '\r') {
            break;
        }
        ++index;
    }
}

auto report_json_input_error(std::string_view input_label, std::size_t offset,
                             std::string_view detail,
                             std::string &error_message) -> bool {
    error_message = "invalid " + std::string(input_label) + " at byte offset '" +
                    std::to_string(offset) + "': " + std::string(detail);
    return false;
}

auto report_json_patch_error(std::size_t offset, std::string_view detail,
                             std::string &error_message) -> bool {
    return report_json_input_error("JSON patch", offset, detail, error_message);
}

auto read_template_table_json_content(const std::filesystem::path &patch_path,
                                      std::string &content, std::size_t &index,
                                      std::string &error_message) -> bool {
    index = 0U;
    if (!read_bounded_utf8_file(patch_path, "JSON patch file", content,
                                error_message)) {
        return false;
    }
    if (content.size() >= 3U &&
        static_cast<unsigned char>(content[0]) == 0xEFU &&
        static_cast<unsigned char>(content[1]) == 0xBBU &&
        static_cast<unsigned char>(content[2]) == 0xBFU) {
        index = 3U;
    }

    return true;
}

} // namespace featherdoc_cli
