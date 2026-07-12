#pragma once

#include <featherdoc/detail/path.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace featherdoc_cli {

inline constexpr std::size_t max_cli_input_bytes = 16U * 1024U * 1024U;
inline constexpr std::size_t max_json_nesting_depth = 128U;

[[nodiscard]] inline auto is_valid_utf8(std::string_view text) noexcept -> bool {
    std::size_t index = 0U;
    while (index < text.size()) {
        const auto first = static_cast<unsigned char>(text[index]);
        if (first <= 0x7FU) {
            ++index;
            continue;
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
            return false;
        }

        if (continuation_count > text.size() - index - 1U) {
            return false;
        }
        for (std::size_t offset = 1U; offset <= continuation_count; ++offset) {
            const auto continuation =
                static_cast<unsigned char>(text[index + offset]);
            if ((continuation & 0xC0U) != 0x80U) {
                return false;
            }
            code_point = (code_point << 6U) | (continuation & 0x3FU);
        }
        if (code_point < minimum || code_point > 0x10FFFFU ||
            (code_point >= 0xD800U && code_point <= 0xDFFFU)) {
            return false;
        }
        index += continuation_count + 1U;
    }
    return true;
}

[[nodiscard]] inline auto read_bounded_utf8_file(
    const std::filesystem::path &path, std::string_view label,
    std::string &content, std::string &error_message,
    std::size_t byte_limit = max_cli_input_bytes) -> bool {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error_message = "failed to open " + std::string{label} + ": " +
                        featherdoc::detail::path_to_utf8(path);
        return false;
    }

    content.clear();
    std::array<char, 8192U> buffer{};
    while (stream) {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes_read = static_cast<std::size_t>(stream.gcount());
        if (bytes_read > byte_limit - content.size()) {
            error_message = std::string{label} + " exceeds the " +
                            std::to_string(byte_limit) + " byte input limit";
            content.clear();
            return false;
        }
        content.append(buffer.data(), bytes_read);
    }
    if (!stream.eof()) {
        error_message = "failed to read " + std::string{label} + ": " +
                        featherdoc::detail::path_to_utf8(path);
        content.clear();
        return false;
    }
    if (!is_valid_utf8(content)) {
        error_message = std::string{label} + " is not valid UTF-8";
        content.clear();
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
