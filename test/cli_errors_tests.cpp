#include <sstream>
#include <system_error>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_errors.hpp"

TEST_CASE("cli write_json_command_error writes compact error result") {
    featherdoc::document_error_info error_info{};
    error_info.detail = "bad \"value\"";
    error_info.entry_name = "word/document.xml";
    error_info.xml_offset = 42U;

    std::ostringstream stream;
    featherdoc_cli::write_json_command_error(stream, "inspect", "parse",
                                             "invalid argument", &error_info);

    CHECK_EQ(stream.str(),
             "{\"command\":\"inspect\",\"ok\":false,\"stage\":\"parse\","
             "\"message\":\"invalid argument\",\"detail\":\"bad \\\"value\\\"\","
             "\"entry\":\"word/document.xml\",\"xml_offset\":42}\n");
}

TEST_CASE("cli write_json_command_error omits absent document details") {
    std::ostringstream stream;
    featherdoc_cli::write_json_command_error(stream, "run-recipe", "input",
                                             "missing file");

    CHECK_EQ(stream.str(),
             "{\"command\":\"run-recipe\",\"ok\":false,\"stage\":\"input\","
             "\"message\":\"missing file\"}\n");
}
