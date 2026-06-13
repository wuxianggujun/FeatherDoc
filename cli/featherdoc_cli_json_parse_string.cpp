#include "featherdoc_cli_json_parse.hpp"

#include <cstdint>
#include <string>

namespace featherdoc_cli {
namespace {

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

} // namespace

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

} // namespace featherdoc_cli
