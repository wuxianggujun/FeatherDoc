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

[[nodiscard]] inline auto path_from_utf8(std::string_view text)
    -> std::filesystem::path {
    const auto *begin = reinterpret_cast<const char8_t *>(text.data());
    return std::filesystem::path{std::u8string{begin, begin + text.size()}};
}

} // namespace featherdoc::detail
