#pragma once

#include <featherdoc/detail/path.hpp>
#include <featherdoc/detail/utf8.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace featherdoc_cli {

inline constexpr std::size_t max_cli_input_bytes = 16U * 1024U * 1024U;
inline constexpr std::size_t max_json_nesting_depth = 128U;

[[nodiscard]] inline auto read_bounded_utf8_file(
    const std::filesystem::path &path, std::string_view label,
    std::string &content, std::string &error_message,
    std::size_t byte_limit = max_cli_input_bytes) -> bool {
    content.clear();
    error_message.clear();

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error_message = "failed to open " + std::string{label} + ": " +
                        featherdoc::detail::path_to_utf8(path);
        return false;
    }

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
    if (!featherdoc::detail::is_valid_utf8(content)) {
        error_message = std::string{label} + " is not valid UTF-8";
        content.clear();
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
