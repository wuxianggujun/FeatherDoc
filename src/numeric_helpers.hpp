#pragma once

#include <charconv>
#include <cmath>
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

[[nodiscard]] inline auto parse_half_point_size_points(std::string_view text)
    -> std::optional<double> {
    const auto half_points = parse_integer_strict<std::uint32_t>(text);
    if (!half_points.has_value() || *half_points == 0U) {
        return std::nullopt;
    }

    return static_cast<double>(*half_points) / 2.0;
}

[[nodiscard]] inline auto parse_half_point_size_points(const char *text)
    -> std::optional<double> {
    if (text == nullptr) {
        return std::nullopt;
    }
    return parse_half_point_size_points(std::string_view{text});
}

[[nodiscard]] inline auto font_size_points_to_half_points(double points)
    -> std::optional<std::uint32_t> {
    constexpr auto maximum_points =
        static_cast<double>(std::numeric_limits<std::uint32_t>::max()) / 2.0;
    if (!std::isfinite(points) || points <= 0.0 || points > maximum_points) {
        return std::nullopt;
    }

    const auto rounded_half_points = std::llround(points * 2.0);
    if (rounded_half_points <= 0) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(rounded_half_points);
}

} // namespace featherdoc::detail
