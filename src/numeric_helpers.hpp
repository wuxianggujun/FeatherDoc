#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>

namespace featherdoc::detail {

template <typename Integer>
[[nodiscard]] auto parse_integer_strict(std::string_view text)
    -> std::optional<Integer> {
    static_assert(std::is_integral_v<Integer>);
    if (text.empty() || text.front() == '+' ||
        (std::is_unsigned_v<Integer> && text.front() == '-')) {
        return std::nullopt;
    }

    Integer value{};
    const auto *begin = text.data();
    const auto *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 10);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

template <typename Integer>
[[nodiscard]] auto parse_integer_strict(const char *text)
    -> std::optional<Integer> {
    if (text == nullptr) {
        return std::nullopt;
    }
    return parse_integer_strict<Integer>(std::string_view{text});
}

[[nodiscard]] inline auto increment_identifier(std::uint32_t maximum)
    -> std::optional<std::uint32_t> {
    if (maximum == std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return maximum + 1U;
}

} // namespace featherdoc::detail
