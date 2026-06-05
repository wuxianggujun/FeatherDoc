#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

namespace {

struct validation_part_case {
    std::string_view token;
    featherdoc_cli::validation_part_family family;
};

constexpr validation_part_case validation_part_cases[] = {
    {"body", featherdoc_cli::validation_part_family::body},
    {"header", featherdoc_cli::validation_part_family::header},
    {"footer", featherdoc_cli::validation_part_family::footer},
    {"section-header", featherdoc_cli::validation_part_family::section_header},
    {"section-footer", featherdoc_cli::validation_part_family::section_footer},
};

} // namespace

TEST_CASE("cli validation part parser accepts documented tokens") {
    for (const auto &test_case : validation_part_cases) {
        auto part = featherdoc_cli::validation_part_family::body;
        CHECK(featherdoc_cli::parse_validation_part(test_case.token, part));
        CHECK(part == test_case.family);
    }
}

TEST_CASE("cli validation part parser rejects invalid tokens without mutation") {
    constexpr std::string_view invalid_tokens[] = {
        "",
        "section_header",
        "Body",
        "header ",
        "unknown",
    };

    for (const auto token : invalid_tokens) {
        auto part = featherdoc_cli::validation_part_family::section_footer;
        CHECK_FALSE(featherdoc_cli::parse_validation_part(token, part));
        CHECK(part == featherdoc_cli::validation_part_family::section_footer);
    }
}

TEST_CASE("cli validation part names use canonical tokens") {
    for (const auto &test_case : validation_part_cases) {
        CHECK(std::string_view{featherdoc_cli::validation_part_name(
                  test_case.family)} == test_case.token);
    }
}

TEST_CASE("cli validation part parser round trips through names") {
    for (const auto &test_case : validation_part_cases) {
        auto part = featherdoc_cli::validation_part_family::body;
        REQUIRE(featherdoc_cli::parse_validation_part(test_case.token, part));
        CHECK(std::string_view{featherdoc_cli::validation_part_name(part)} ==
              test_case.token);
    }
}

TEST_CASE("cli validation part selection accepts section targets with section") {
    std::string error_message;
    CHECK(featherdoc_cli::validate_template_part_selection(
        featherdoc_cli::validation_part_family::section_header, std::nullopt,
        std::size_t{1U}, true, "schema validation", error_message));
}

TEST_CASE("cli validation part selection rejects body section filters") {
    std::string error_message;
    CHECK_FALSE(featherdoc_cli::validate_template_part_selection(
        featherdoc_cli::validation_part_family::body, std::nullopt,
        std::size_t{1U}, false, "schema validation", error_message));
    CHECK(error_message.find("--section") != std::string::npos);
}

TEST_CASE("cli validation part selection rejects section targets without section") {
    std::string error_message;
    CHECK_FALSE(featherdoc_cli::validate_template_part_selection(
        featherdoc_cli::validation_part_family::section_footer, std::nullopt,
        std::nullopt, true, "schema validation", error_message));
    CHECK(error_message.find("requires --section") != std::string::npos);
}
