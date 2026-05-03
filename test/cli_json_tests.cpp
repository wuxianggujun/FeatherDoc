#include <sstream>
#include <string>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_json.hpp"

TEST_CASE("cli json_escape preserves plain text") {
    CHECK_EQ(featherdoc_cli::json_escape("plain text 123"), "plain text 123");
}

TEST_CASE("cli json_escape escapes JSON string control characters") {
    const std::string input = "quote\" slash\\ back\b form\f line\nreturn\r tab\t";
    CHECK_EQ(featherdoc_cli::json_escape(input),
             "quote\\\" slash\\\\ back\\b form\\f line\\nreturn\\r tab\\t");
}

TEST_CASE("cli write_json_string wraps escaped text in quotes") {
    std::ostringstream stream;
    featherdoc_cli::write_json_string(stream, "a\"b\nc");
    CHECK_EQ(stream.str(), "\"a\\\"b\\nc\"");
}
