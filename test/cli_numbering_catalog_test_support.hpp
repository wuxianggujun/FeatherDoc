#pragma once

#include "cli_test_support.hpp"

namespace {

void create_cli_paragraph_list_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    REQUIRE(document.paragraphs().add_run("item 0").has_next());
    append_body_paragraph(document, "item 1");
    append_body_paragraph(document, "item 2");

    REQUIRE_FALSE(document.save());
}

} // namespace
