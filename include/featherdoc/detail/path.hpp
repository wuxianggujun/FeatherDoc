#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace featherdoc::detail {

[[nodiscard]] inline auto path_to_utf8(const std::filesystem::path &path)
    -> std::string {
    const auto encoded = path.generic_u8string();
    return {reinterpret_cast<const char *>(encoded.data()), encoded.size()};
}

// Keep native separators and Windows extended-path prefixes for file APIs.
// path_to_utf8() remains the stable display/JSON representation.
[[nodiscard]] inline auto
path_to_native_utf8(const std::filesystem::path &path) -> std::string {
    const auto encoded = path.u8string();
    return {reinterpret_cast<const char *>(encoded.data()), encoded.size()};
}

[[nodiscard]] inline auto path_from_utf8(std::string_view text)
    -> std::filesystem::path {
    if (text.empty()) {
        return {};
    }
    const auto *begin = reinterpret_cast<const char8_t *>(text.data());
    return std::filesystem::path{std::u8string{begin, begin + text.size()}};
}

[[nodiscard]] inline auto
path_has_embedded_nul(const std::filesystem::path &path) -> bool {
    const auto &native = path.native();
    return native.find(std::filesystem::path::value_type{}) !=
           std::filesystem::path::string_type::npos;
}

} // namespace featherdoc::detail
