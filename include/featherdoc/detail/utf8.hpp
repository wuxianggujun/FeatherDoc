#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace featherdoc::detail {

struct utf8_decode_result {
    bool valid{false};
    std::size_t length{0U};
    std::uint32_t code_point{0U};
};

[[nodiscard]] inline auto hex_digit_upper(unsigned char value) noexcept
    -> char {
    return static_cast<char>(value < 10U ? ('0' + value)
                                         : ('A' + (value - 10U)));
}

inline void append_diagnostic_byte_escape(std::string &output,
                                          unsigned char byte) {
    output.push_back('\\');
    output.push_back('x');
    output.push_back(hex_digit_upper(static_cast<unsigned char>(byte >> 4U)));
    output.push_back(hex_digit_upper(static_cast<unsigned char>(byte & 0x0FU)));
}

[[nodiscard]] inline auto decode_next_utf8(std::string_view text,
                                           std::size_t index) noexcept
    -> utf8_decode_result {
    if (index >= text.size()) {
        return {};
    }

    const auto first = static_cast<unsigned char>(text[index]);
    if (first <= 0x7FU) {
        return {true, 1U, first};
    }

    std::size_t continuation_count = 0U;
    std::uint32_t code_point = 0U;
    std::uint32_t minimum = 0U;
    if (first >= 0xC2U && first <= 0xDFU) {
        continuation_count = 1U;
        code_point = first & 0x1FU;
        minimum = 0x80U;
    } else if (first >= 0xE0U && first <= 0xEFU) {
        continuation_count = 2U;
        code_point = first & 0x0FU;
        minimum = 0x800U;
    } else if (first >= 0xF0U && first <= 0xF4U) {
        continuation_count = 3U;
        code_point = first & 0x07U;
        minimum = 0x10000U;
    } else {
        return {false, 1U, 0U};
    }

    if (continuation_count > text.size() - index - 1U) {
        return {false, text.size() - index, 0U};
    }
    for (std::size_t offset = 1U; offset <= continuation_count; ++offset) {
        const auto continuation =
            static_cast<unsigned char>(text[index + offset]);
        if ((continuation & 0xC0U) != 0x80U) {
            return {false, 1U, 0U};
        }
        code_point = (code_point << 6U) | (continuation & 0x3FU);
    }

    const auto length = continuation_count + 1U;
    if (code_point < minimum || code_point > 0x10FFFFU ||
        (code_point >= 0xD800U && code_point <= 0xDFFFU)) {
        return {false, length, 0U};
    }

    return {true, length, code_point};
}

[[nodiscard]] inline auto is_valid_utf8(std::string_view text) noexcept
    -> bool {
    std::size_t index = 0U;
    while (index < text.size()) {
        const auto decoded = decode_next_utf8(text, index);
        if (!decoded.valid || decoded.length == 0U) {
            return false;
        }
        index += decoded.length;
    }
    return true;
}

[[nodiscard]] inline auto
is_unicode_control_code_point(std::uint32_t code_point) noexcept -> bool {
    return code_point <= 0x1FU || (code_point >= 0x7FU && code_point <= 0x9FU);
}

[[nodiscard]] inline auto sanitize_utf8_for_diagnostic(std::string_view text)
    -> std::string {
    std::string sanitized;
    sanitized.reserve(text.size());

    std::size_t index = 0U;
    while (index < text.size()) {
        const auto decoded = decode_next_utf8(text, index);
        if (!decoded.valid || decoded.length == 0U) {
            const auto invalid_length =
                decoded.length == 0U ? 1U : decoded.length;
            const auto bounded_length = invalid_length > text.size() - index
                                            ? text.size() - index
                                            : invalid_length;
            for (std::size_t offset = 0U; offset < bounded_length; ++offset) {
                append_diagnostic_byte_escape(
                    sanitized,
                    static_cast<unsigned char>(text[index + offset]));
            }
            index += bounded_length;
            continue;
        }

        if (is_unicode_control_code_point(decoded.code_point)) {
            for (std::size_t offset = 0U; offset < decoded.length; ++offset) {
                append_diagnostic_byte_escape(
                    sanitized,
                    static_cast<unsigned char>(text[index + offset]));
            }
        } else {
            sanitized.append(text.substr(index, decoded.length));
        }
        index += decoded.length;
    }

    return sanitized;
}

} // namespace featherdoc::detail
