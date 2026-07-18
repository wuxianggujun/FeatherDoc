#include "featherdoc_cli_text.hpp"

#include <featherdoc/detail/path.hpp>
#include <featherdoc/detail/utf8.hpp>

#include <algorithm>
#include <cctype>

namespace featherdoc_cli {

auto parse_cli_path_utf8(std::string_view text, std::string_view label,
                         std::filesystem::path &path,
                         std::string &error_message) -> bool {
    if (text.empty()) {
        error_message = std::string{label} + " must not be empty";
        return false;
    }
    if (!featherdoc::detail::is_valid_utf8(text)) {
        error_message = std::string{label} + " must be valid UTF-8";
        return false;
    }

    for (std::size_t index = 0U; index < text.size(); ++index) {
        const auto byte = static_cast<unsigned char>(text[index]);
        const bool ascii_control = byte < 0x20U || byte == 0x7FU;
        const bool utf8_c1_control =
            byte == 0xC2U && index + 1U < text.size() &&
            static_cast<unsigned char>(text[index + 1U]) >= 0x80U &&
            static_cast<unsigned char>(text[index + 1U]) <= 0x9FU;
        if (ascii_control || utf8_c1_control) {
            error_message =
                std::string{label} + " must not contain control characters";
            return false;
        }
    }

    path = path_from_cli_utf8(text);
    error_message.clear();
    return true;
}

auto quote_cli_argument(std::string_view value) -> std::string {
    if (value.empty()) {
        return "\"\"";
    }

    const auto needs_quotes =
        std::any_of(value.begin(), value.end(), [](const char character) {
            return std::isspace(static_cast<unsigned char>(character)) ||
                   character == '"' || character == '\'';
        });
    if (!needs_quotes) {
        return std::string(value);
    }

    auto quoted = std::string{"\""};
    for (const auto character : value) {
        if (character == '"') {
            quoted += "\\\"";
        } else {
            quoted += character;
        }
    }
    quoted += '"';
    return quoted;
}

auto yes_no(bool value) -> const char * { return value ? "yes" : "no"; }

auto json_bool(bool value) -> const char * { return value ? "true" : "false"; }

auto format_paragraph_text(std::string_view text) -> std::string {
    if (text.empty()) {
        return "<empty>";
    }

    std::string formatted;
    formatted.reserve(text.size());
    for (const auto character : text) {
        switch (character) {
        case '\n':
            formatted += "\\n";
            break;
        case '\r':
            formatted += "\\r";
            break;
        case '\t':
            formatted += "\\t";
            break;
        default:
            formatted.push_back(character);
            break;
        }
    }
    return formatted;
}

auto optional_display_value(const std::optional<std::string> &value)
    -> std::string {
    return value.has_value() ? *value : std::string{"-"};
}

auto optional_size_display_value(const std::optional<std::size_t> &value)
    -> std::string {
    return value.has_value() ? std::to_string(*value) : std::string{"-"};
}

auto strip_utf8_bom(std::string text) -> std::string {
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.erase(0, 3);
    }
    return text;
}

auto lower_ascii_copy(std::string value) -> std::string {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

auto is_docx_path(const std::filesystem::path &path) -> bool {
    return lower_ascii_copy(
               featherdoc::detail::path_to_utf8(path.extension())) == ".docx";
}

auto is_word_temporary_path(const std::filesystem::path &path) -> bool {
    const auto filename = featherdoc::detail::path_to_utf8(path.filename());
    return filename.size() >= 2U && filename[0] == '~' && filename[1] == '$';
}

} // namespace featherdoc_cli
