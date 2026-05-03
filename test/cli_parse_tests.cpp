#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_parse.hpp"

TEST_CASE("cli has_json_flag detects exact json flag") {
    CHECK_FALSE(featherdoc_cli::has_json_flag({}));
    CHECK_FALSE(featherdoc_cli::has_json_flag({"--jsonish"}));
    CHECK(featherdoc_cli::has_json_flag({"inspect", "--json"}));
}

TEST_CASE("cli parse_index accepts unsigned indexes and rejects invalid text") {
    std::size_t value = 7U;
    CHECK(featherdoc_cli::parse_index("0", value));
    CHECK_EQ(value, 0U);
    CHECK(featherdoc_cli::parse_index("42", value));
    CHECK_EQ(value, 42U);

    CHECK_FALSE(featherdoc_cli::parse_index("", value));
    CHECK_FALSE(featherdoc_cli::parse_index("-1", value));
    CHECK_FALSE(featherdoc_cli::parse_index("1.5", value));
}

TEST_CASE("cli parse_uint32 rejects overflow") {
    std::uint32_t value = 0U;
    CHECK(featherdoc_cli::parse_uint32("4294967295", value));
    CHECK_EQ(value, std::numeric_limits<std::uint32_t>::max());

    CHECK_FALSE(featherdoc_cli::parse_uint32("4294967296", value));
    CHECK_FALSE(featherdoc_cli::parse_uint32("+1", value));
}

TEST_CASE("cli parse_int32 accepts signed bounds and rejects overflow") {
    std::int32_t value = 0;
    CHECK(featherdoc_cli::parse_int32("2147483647", value));
    CHECK_EQ(value, std::numeric_limits<std::int32_t>::max());
    CHECK(featherdoc_cli::parse_int32("-2147483648", value));
    CHECK_EQ(value, std::numeric_limits<std::int32_t>::min());
    CHECK(featherdoc_cli::parse_int32("+17", value));
    CHECK_EQ(value, 17);

    CHECK_FALSE(featherdoc_cli::parse_int32("2147483648", value));
    CHECK_FALSE(featherdoc_cli::parse_int32("-2147483649", value));
    CHECK_FALSE(featherdoc_cli::parse_int32("+", value));
    CHECK_FALSE(featherdoc_cli::parse_int32("12x", value));
}

TEST_CASE("cli parse_bool accepts documented bool tokens") {
    bool value = false;
    CHECK(featherdoc_cli::parse_bool("true", value));
    CHECK(value);
    CHECK(featherdoc_cli::parse_bool("yes", value));
    CHECK(value);
    CHECK(featherdoc_cli::parse_bool("1", value));
    CHECK(value);

    CHECK(featherdoc_cli::parse_bool("false", value));
    CHECK_FALSE(value);
    CHECK(featherdoc_cli::parse_bool("no", value));
    CHECK_FALSE(value);
    CHECK(featherdoc_cli::parse_bool("0", value));
    CHECK_FALSE(value);

    CHECK_FALSE(featherdoc_cli::parse_bool("on", value));
}
