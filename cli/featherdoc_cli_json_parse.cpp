#include "featherdoc_cli_json_parse.hpp"

#include "featherdoc_cli_parse.hpp"

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

void skip_json_patch_whitespace(std::string_view text, std::size_t &index) {
    while (index < text.size() &&
           std::isspace(static_cast<unsigned char>(text[index])) != 0) {
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

auto append_utf8_code_point(std::string &text, std::uint32_t code_point,
                            std::string &error_message) -> bool {
    if (code_point > 0x10FFFFU ||
        (code_point >= 0xD800U && code_point <= 0xDFFFU)) {
        error_message = "invalid Unicode escape sequence in JSON patch";
        return false;
    }

    if (code_point <= 0x7FU) {
        text.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7FFU) {
        text.push_back(static_cast<char>(0xC0U | (code_point >> 6U)));
        text.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
    } else if (code_point <= 0xFFFFU) {
        text.push_back(static_cast<char>(0xE0U | (code_point >> 12U)));
        text.push_back(
            static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
    } else {
        text.push_back(static_cast<char>(0xF0U | (code_point >> 18U)));
        text.push_back(
            static_cast<char>(0x80U | ((code_point >> 12U) & 0x3FU)));
        text.push_back(
            static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
        text.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
    }

    return true;
}

auto parse_json_patch_hex_digit(char digit, std::uint32_t &value) -> bool {
    if (digit >= '0' && digit <= '9') {
        value = static_cast<std::uint32_t>(digit - '0');
        return true;
    }
    if (digit >= 'a' && digit <= 'f') {
        value = static_cast<std::uint32_t>(10 + (digit - 'a'));
        return true;
    }
    if (digit >= 'A' && digit <= 'F') {
        value = static_cast<std::uint32_t>(10 + (digit - 'A'));
        return true;
    }

    return false;
}

auto parse_json_patch_code_unit(std::string_view text, std::size_t &index,
                                std::uint32_t &code_unit,
                                std::string &error_message) -> bool {
    if (index + 4U > text.size()) {
        return report_json_patch_error(index,
                                       "expected four hexadecimal digits after "
                                       "Unicode escape",
                                       error_message);
    }

    code_unit = 0U;
    for (std::size_t offset = 0U; offset < 4U; ++offset) {
        std::uint32_t hex_digit = 0U;
        if (!parse_json_patch_hex_digit(text[index + offset], hex_digit)) {
            return report_json_patch_error(index + offset,
                                           "invalid hexadecimal digit in "
                                           "Unicode escape",
                                           error_message);
        }

        code_unit = static_cast<std::uint32_t>((code_unit << 4U) | hex_digit);
    }

    index += 4U;
    return true;
}

auto parse_json_patch_string(std::string_view text, std::size_t &index,
                             std::string &value, std::string &error_message)
    -> bool {
    if (index >= text.size() || text[index] != '"') {
        return report_json_patch_error(index, "expected string", error_message);
    }

    value.clear();
    ++index;
    while (index < text.size()) {
        const auto ch = static_cast<unsigned char>(text[index]);
        if (ch == '"') {
            ++index;
            return true;
        }
        if (ch < 0x20U) {
            return report_json_patch_error(index,
                                           "unescaped control character in "
                                           "string literal",
                                           error_message);
        }
        if (ch != '\\') {
            value.push_back(static_cast<char>(ch));
            ++index;
            continue;
        }

        ++index;
        if (index >= text.size()) {
            return report_json_patch_error(index,
                                           "unterminated escape sequence",
                                           error_message);
        }

        switch (text[index]) {
        case '"':
            value.push_back('"');
            ++index;
            break;
        case '\\':
            value.push_back('\\');
            ++index;
            break;
        case '/':
            value.push_back('/');
            ++index;
            break;
        case 'b':
            value.push_back('\b');
            ++index;
            break;
        case 'f':
            value.push_back('\f');
            ++index;
            break;
        case 'n':
            value.push_back('\n');
            ++index;
            break;
        case 'r':
            value.push_back('\r');
            ++index;
            break;
        case 't':
            value.push_back('\t');
            ++index;
            break;
        case 'u': {
            ++index;
            std::uint32_t code_unit = 0U;
            if (!parse_json_patch_code_unit(text, index, code_unit,
                                            error_message)) {
                return false;
            }

            std::uint32_t code_point = code_unit;
            if (code_unit >= 0xD800U && code_unit <= 0xDBFFU) {
                if (index + 2U > text.size() || text[index] != '\\' ||
                    text[index + 1U] != 'u') {
                    return report_json_patch_error(
                        index,
                        "expected low surrogate after high surrogate",
                        error_message);
                }

                index += 2U;
                std::uint32_t low_surrogate = 0U;
                if (!parse_json_patch_code_unit(text, index, low_surrogate,
                                                error_message)) {
                    return false;
                }
                if (low_surrogate < 0xDC00U || low_surrogate > 0xDFFFU) {
                    return report_json_patch_error(
                        index - 4U,
                        "expected low surrogate after high surrogate",
                        error_message);
                }

                code_point =
                    0x10000U + (((code_unit - 0xD800U) << 10U) |
                                (low_surrogate - 0xDC00U));
            } else if (code_unit >= 0xDC00U && code_unit <= 0xDFFFU) {
                return report_json_patch_error(index - 4U,
                                               "unexpected low surrogate",
                                               error_message);
            }

            if (!append_utf8_code_point(value, code_point, error_message)) {
                return false;
            }
            break;
        }
        default:
            return report_json_patch_error(index, "unsupported escape sequence",
                                           error_message);
        }
    }

    return report_json_patch_error(index, "unterminated string literal",
                                   error_message);
}

auto parse_json_patch_number(std::string_view text, std::size_t &index,
                             std::string &value, std::string &error_message)
    -> bool {
    if (index >= text.size()) {
        return report_json_patch_error(index, "expected number", error_message);
    }

    const auto start_index = index;
    if (text[index] == '-') {
        ++index;
    }
    if (index >= text.size()) {
        return report_json_patch_error(start_index, "expected number digits",
                                       error_message);
    }

    if (text[index] == '0') {
        ++index;
    } else if (text[index] >= '1' && text[index] <= '9') {
        ++index;
        while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
            ++index;
        }
    } else {
        return report_json_patch_error(start_index, "expected number digits",
                                       error_message);
    }

    if (index < text.size() && text[index] == '.') {
        ++index;
        if (index >= text.size() || text[index] < '0' || text[index] > '9') {
            return report_json_patch_error(index,
                                           "expected digits after decimal point",
                                           error_message);
        }
        while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
            ++index;
        }
    }

    if (index < text.size() && (text[index] == 'e' || text[index] == 'E')) {
        ++index;
        if (index < text.size() && (text[index] == '+' || text[index] == '-')) {
            ++index;
        }
        if (index >= text.size() || text[index] < '0' || text[index] > '9') {
            return report_json_patch_error(index,
                                           "expected exponent digits",
                                           error_message);
        }
        while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
            ++index;
        }
    }

    value = std::string(text.substr(start_index, index - start_index));
    return true;
}

auto skip_json_patch_value(std::string_view text, std::size_t &index,
                           std::string &error_message) -> bool;

auto skip_json_patch_array(std::string_view text, std::size_t &index,
                           std::string &error_message) -> bool {
    if (index >= text.size() || text[index] != '[') {
        return report_json_patch_error(index, "expected array", error_message);
    }

    ++index;
    skip_json_patch_whitespace(text, index);
    if (index < text.size() && text[index] == ']') {
        ++index;
        return true;
    }

    while (index < text.size()) {
        if (!skip_json_patch_value(text, index, error_message)) {
            return false;
        }

        skip_json_patch_whitespace(text, index);
        if (index >= text.size()) {
            break;
        }
        if (text[index] == ',') {
            ++index;
            skip_json_patch_whitespace(text, index);
            continue;
        }
        if (text[index] == ']') {
            ++index;
            return true;
        }
        return report_json_patch_error(index,
                                       "expected ',' or ']' after array item",
                                       error_message);
    }

    return report_json_patch_error(index, "unterminated array", error_message);
}

auto skip_json_patch_object(std::string_view text, std::size_t &index,
                            std::string &error_message) -> bool {
    if (index >= text.size() || text[index] != '{') {
        return report_json_patch_error(index, "expected object", error_message);
    }

    ++index;
    skip_json_patch_whitespace(text, index);
    if (index < text.size() && text[index] == '}') {
        ++index;
        return true;
    }

    while (index < text.size()) {
        std::string key;
        if (!parse_json_patch_string(text, index, key, error_message)) {
            return false;
        }

        skip_json_patch_whitespace(text, index);
        if (index >= text.size() || text[index] != ':') {
            return report_json_patch_error(index,
                                           "expected ':' after object member",
                                           error_message);
        }

        ++index;
        if (!skip_json_patch_value(text, index, error_message)) {
            return false;
        }

        skip_json_patch_whitespace(text, index);
        if (index >= text.size()) {
            break;
        }
        if (text[index] == ',') {
            ++index;
            skip_json_patch_whitespace(text, index);
            continue;
        }
        if (text[index] == '}') {
            ++index;
            return true;
        }
        return report_json_patch_error(
            index, "expected ',' or '}' after object member", error_message);
    }

    return report_json_patch_error(index, "unterminated object", error_message);
}

auto skip_json_patch_value(std::string_view text, std::size_t &index,
                           std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    if (index >= text.size()) {
        return report_json_patch_error(index, "expected JSON value",
                                       error_message);
    }

    if (text[index] == '"') {
        std::string ignored;
        return parse_json_patch_string(text, index, ignored, error_message);
    }
    if (text[index] == '{') {
        return skip_json_patch_object(text, index, error_message);
    }
    if (text[index] == '[') {
        return skip_json_patch_array(text, index, error_message);
    }
    if (text[index] == 't') {
        if (text.substr(index, 4U) != "true") {
            return report_json_patch_error(index, "expected 'true'",
                                           error_message);
        }
        index += 4U;
        return true;
    }
    if (text[index] == 'f') {
        if (text.substr(index, 5U) != "false") {
            return report_json_patch_error(index, "expected 'false'",
                                           error_message);
        }
        index += 5U;
        return true;
    }
    if (text[index] == 'n') {
        if (text.substr(index, 4U) != "null") {
            return report_json_patch_error(index, "expected 'null'",
                                           error_message);
        }
        index += 4U;
        return true;
    }
    if (text[index] == '-' || (text[index] >= '0' && text[index] <= '9')) {
        std::string ignored;
        return parse_json_patch_number(text, index, ignored, error_message);
    }

    return report_json_patch_error(index, "unexpected JSON token",
                                   error_message);
}

auto parse_json_patch_index_value(std::string_view text, std::size_t &index,
                                  std::size_t &value,
                                  std::string_view member_name,
                                  std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(text, index);
    if (index >= text.size()) {
        return report_json_patch_error(index, "expected integer value",
                                       error_message);
    }

    if (text[index] == '"') {
        if (!parse_json_patch_string(text, index, token, error_message)) {
            return false;
        }
    } else if (text[index] == '-' || (text[index] >= '0' && text[index] <= '9')) {
        if (!parse_json_patch_number(text, index, token, error_message)) {
            return false;
        }
    } else {
        return report_json_patch_error(index, "expected integer value",
                                       error_message);
    }

    if (!parse_index(token, value)) {
        error_message = "JSON patch member '" + std::string(member_name) +
                        "' must be an unsigned integer";
        return false;
    }

    return true;
}

auto resolve_json_patch_string_member(
    const std::optional<std::string> &primary_value,
    const std::optional<std::string> &alias_value,
    std::string_view primary_name, std::string_view alias_name,
    std::optional<std::string> &resolved_value,
    std::string &error_message) -> bool {
    if (primary_value.has_value() && alias_value.has_value() &&
        *primary_value != *alias_value) {
        error_message = "JSON patch members '" + std::string(primary_name) +
                        "' and '" + std::string(alias_name) +
                        "' must match when both are present";
        return false;
    }

    if (primary_value.has_value()) {
        resolved_value = primary_value;
    } else {
        resolved_value = alias_value;
    }

    return true;
}

auto resolve_json_patch_index_member(std::optional<std::size_t> primary_value,
                                     std::optional<std::size_t> alias_value,
                                     std::string_view primary_name,
                                     std::string_view alias_name,
                                     std::optional<std::size_t> &resolved_value,
                                     std::string &error_message) -> bool {
    if (primary_value.has_value() && alias_value.has_value() &&
        *primary_value != *alias_value) {
        error_message = "JSON patch members '" + std::string(primary_name) +
                        "' and '" + std::string(alias_name) +
                        "' must match when both are present";
        return false;
    }

    if (primary_value.has_value()) {
        resolved_value = primary_value;
    } else {
        resolved_value = alias_value;
    }

    return true;
}

auto parse_json_patch_bool_member_value(
    std::string_view text, std::size_t &index, bool &value,
    std::string_view member_name, std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    if (index >= text.size()) {
        return report_json_input_error("JSON schema file", index,
                                       "expected boolean value", error_message);
    }

    if (text[index] == '"') {
        std::string token;
        if (!parse_json_patch_string(text, index, token, error_message)) {
            return false;
        }
        if (!parse_bool(token, value)) {
            error_message = "JSON schema member '" + std::string(member_name) +
                            "' must be a boolean";
            return false;
        }
        return true;
    }

    if (text.substr(index, 4U) == "true") {
        value = true;
        index += 4U;
        return true;
    }

    if (text.substr(index, 5U) == "false") {
        value = false;
        index += 5U;
        return true;
    }

    error_message = "JSON schema member '" + std::string(member_name) +
                    "' must be a boolean";
    return false;
}

auto read_template_table_json_content(const std::filesystem::path &patch_path,
                                      std::string &content, std::size_t &index,
                                      std::string &error_message) -> bool {
    std::ifstream stream(patch_path, std::ios::binary);
    if (!stream.good()) {
        error_message =
            "failed to read JSON patch file: " + patch_path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    index = 0U;
    if (content.size() >= 3U &&
        static_cast<unsigned char>(content[0]) == 0xEFU &&
        static_cast<unsigned char>(content[1]) == 0xBBU &&
        static_cast<unsigned char>(content[2]) == 0xBFU) {
        index = 3U;
    }

    return true;
}

} // namespace featherdoc_cli
