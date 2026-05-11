#include "featherdoc_cli_parse.hpp"

#include <limits>

namespace featherdoc_cli {

auto has_json_flag(const std::vector<std::string_view> &arguments) -> bool {
    for (const auto argument : arguments) {
        if (argument == "--json") {
            return true;
        }
    }

    return false;
}

template <typename UInt>
auto parse_unsigned_integer(std::string_view text, UInt &value) -> bool {
    if (text.empty()) {
        return false;
    }

    UInt result = 0;
    for (const auto ch : text) {
        if (ch < '0' || ch > '9') {
            return false;
        }

        const auto digit = static_cast<UInt>(ch - '0');
        if (result > (std::numeric_limits<UInt>::max() - digit) / 10) {
            return false;
        }
        result = static_cast<UInt>(result * 10 + digit);
    }

    value = result;
    return true;
}

auto parse_index(std::string_view text, std::size_t &value) -> bool {
    return parse_unsigned_integer(text, value);
}

auto parse_uint32(std::string_view text, std::uint32_t &value) -> bool {
    return parse_unsigned_integer(text, value);
}

auto parse_int32(std::string_view text, std::int32_t &value) -> bool {
    if (text.empty()) {
        return false;
    }

    auto digits = text;
    bool negative = false;
    if (digits.front() == '+' || digits.front() == '-') {
        negative = digits.front() == '-';
        digits.remove_prefix(1);
    }
    if (digits.empty()) {
        return false;
    }

    std::uint64_t magnitude = 0;
    for (const auto ch : digits) {
        if (ch < '0' || ch > '9') {
            return false;
        }

        const auto digit = static_cast<std::uint64_t>(ch - '0');
        constexpr auto positive_limit =
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
        constexpr auto negative_limit = positive_limit + 1;
        const auto limit = negative ? negative_limit : positive_limit;
        if (magnitude > (limit - digit) / 10) {
            return false;
        }
        magnitude = magnitude * 10 + digit;
    }

    if (negative) {
        if (magnitude == static_cast<std::uint64_t>(
                             std::numeric_limits<std::int32_t>::max()) +
                             1ULL) {
            value = std::numeric_limits<std::int32_t>::min();
            return true;
        }

        value = -static_cast<std::int32_t>(magnitude);
        return true;
    }

    value = static_cast<std::int32_t>(magnitude);
    return true;
}

auto parse_bool(std::string_view text, bool &value) -> bool {
    if (text == "true" || text == "yes" || text == "1") {
        value = true;
        return true;
    }
    if (text == "false" || text == "no" || text == "0") {
        value = false;
        return true;
    }

    return false;
}

} // namespace featherdoc_cli
