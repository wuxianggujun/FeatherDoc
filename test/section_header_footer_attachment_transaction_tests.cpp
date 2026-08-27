#include "doctest.h"

#include <featherdoc.hpp>

#include <string>

TEST_CASE("creating a section related part invalidates only main document "
          "handles and keeps existing story handles live") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto footer = document.ensure_section_footer_paragraphs(0U);
    auto footer_run = footer.add_run("retained footer");
    auto footer_template = document.footer_template(0U);
    REQUIRE(footer.valid());
    REQUIRE(footer_run.valid());
    REQUIRE(footer_template);

    auto body = document.paragraphs();
    auto body_run = body.add_run("body before header attachment");
    auto body_template = document.body_template();
    REQUIRE(body.valid());
    REQUIRE(body_run.valid());
    REQUIRE(body_template);

    auto header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.valid());
    REQUIRE(header.add_run("new header").valid());
    CHECK(body.valid());
    CHECK(body_run.valid());
    CHECK(body_template);
    CHECK(footer.valid());
    CHECK(footer_run.valid());
    CHECK(footer_template);
    CHECK_EQ(footer_run.get_text(), "retained footer");

    auto rebound_body = document.paragraphs();
    auto retained_header = document.section_header_paragraphs(0U);
    auto retained_header_run = retained_header.runs();
    REQUIRE(rebound_body.valid());
    REQUIRE(retained_header.valid());
    REQUIRE(retained_header_run.valid());

    auto &same_header = document.ensure_section_header_paragraphs(0U);
    CHECK(same_header.valid());
    CHECK(rebound_body.valid());
    CHECK(retained_header.valid());
    CHECK(retained_header_run.valid());
    CHECK_EQ(retained_header_run.get_text(), "new header");
    CHECK(footer.valid());
    CHECK(footer_run.valid());
}

TEST_CASE("assigning a section related part publishes atomically and a repeated "
          "assignment is a handle-preserving no-op") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto default_header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(default_header.add_run("default header").valid());
    auto first_header = document.ensure_section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    REQUIRE(first_header.add_run("first header").valid());
    REQUIRE_EQ(document.header_count(), 2U);

    auto retained_default = document.section_header_paragraphs(0U);
    auto retained_default_run = retained_default.runs();
    auto retained_first = document.section_header_paragraphs(
        0U, featherdoc::section_reference_kind::first_page);
    auto retained_first_run = retained_first.runs();
    auto body = document.paragraphs();
    auto body_run = body.add_run("body before assignment");
    REQUIRE(retained_default.valid());
    REQUIRE(retained_first.valid());
    REQUIRE(body.valid());
    REQUIRE(body_run.valid());

    auto &assigned = document.assign_section_header_paragraphs(
        0U, 1U, featherdoc::section_reference_kind::default_reference);
    REQUIRE(assigned.valid());
    CHECK_EQ(document.section_header_paragraphs(0U).runs().get_text(),
             "first header");
    CHECK(body.valid());
    CHECK(body_run.valid());
    CHECK(retained_default.valid());
    CHECK(retained_default_run.valid());
    CHECK(retained_first.valid());
    CHECK(retained_first_run.valid());

    auto rebound_body = document.paragraphs();
    auto current_default = document.section_header_paragraphs(0U);
    REQUIRE(rebound_body.valid());
    REQUIRE(current_default.valid());
    auto &same_assignment = document.assign_section_header_paragraphs(
        0U, 1U, featherdoc::section_reference_kind::default_reference);
    CHECK(same_assignment.valid());
    CHECK(rebound_body.valid());
    CHECK(current_default.valid());
    CHECK_EQ(current_default.runs().get_text(), "first header");
}

TEST_CASE("failed section related part validation preserves all published "
          "handles and parts") {
    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto header = document.ensure_section_header_paragraphs(0U);
    auto header_run = header.add_run("existing header");
    auto body = document.paragraphs();
    auto body_run = body.add_run("existing body");
    REQUIRE(header.valid());
    REQUIRE(header_run.valid());
    REQUIRE(body.valid());
    REQUIRE(body_run.valid());

    auto &invalid_ensure = document.ensure_section_footer_paragraphs(1U);
    CHECK_FALSE(invalid_ensure.valid());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(document.header_count(), 1U);
    CHECK_EQ(document.footer_count(), 0U);
    CHECK(header.valid());
    CHECK(header_run.valid());
    CHECK(body.valid());
    CHECK(body_run.valid());

    auto &invalid_assignment = document.assign_section_header_paragraphs(
        0U, 2U, featherdoc::section_reference_kind::default_reference);
    CHECK_FALSE(invalid_assignment.valid());
    CHECK_EQ(document.last_error().code,
             std::make_error_code(std::errc::invalid_argument));
    CHECK_EQ(document.header_count(), 1U);
    CHECK(header.valid());
    CHECK(header_run.valid());
    CHECK_EQ(header_run.get_text(), "existing header");
    CHECK(body.valid());
    CHECK(body_run.valid());
    CHECK_EQ(body_run.get_text(), "existing body");
}
