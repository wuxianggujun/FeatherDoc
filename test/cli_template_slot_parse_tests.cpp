#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_slot_parse.hpp"

#include <string>

namespace {

auto parse_slot(std::string_view text,
                featherdoc::template_slot_requirement &requirement,
                std::string &error_message) -> bool {
    error_message.clear();
    requirement = {};
    return featherdoc_cli::parse_template_slot_requirement(text, requirement,
                                                           error_message);
}

} // namespace

TEST_CASE("cli template slot parser accepts documented kind tokens") {
    featherdoc::template_slot_kind kind{};
    CHECK(featherdoc_cli::parse_template_slot_kind("text", kind));
    CHECK(kind == featherdoc::template_slot_kind::text);
    CHECK(featherdoc_cli::parse_template_slot_kind("table_rows", kind));
    CHECK(kind == featherdoc::template_slot_kind::table_rows);
    CHECK(featherdoc_cli::parse_template_slot_kind("table-rows", kind));
    CHECK(kind == featherdoc::template_slot_kind::table_rows);
    CHECK(featherdoc_cli::parse_template_slot_kind("table", kind));
    CHECK(kind == featherdoc::template_slot_kind::table);
    CHECK(featherdoc_cli::parse_template_slot_kind("image", kind));
    CHECK(kind == featherdoc::template_slot_kind::image);
    CHECK(featherdoc_cli::parse_template_slot_kind("floating_image", kind));
    CHECK(kind == featherdoc::template_slot_kind::floating_image);
    CHECK(featherdoc_cli::parse_template_slot_kind("floating-image", kind));
    CHECK(kind == featherdoc::template_slot_kind::floating_image);
    CHECK(featherdoc_cli::parse_template_slot_kind("block", kind));
    CHECK(kind == featherdoc::template_slot_kind::block);

    CHECK_FALSE(featherdoc_cli::parse_template_slot_kind("unknown", kind));
}

TEST_CASE("cli template slot parser accepts bookmark and occurrence requirements") {
    featherdoc::template_slot_requirement requirement{};
    std::string error_message;
    REQUIRE(parse_slot("customer:text:optional:min=1:max=3", requirement,
                       error_message));

    CHECK(requirement.bookmark_name == "customer");
    CHECK(requirement.kind == featherdoc::template_slot_kind::text);
    CHECK_FALSE(requirement.required);
    REQUIRE(requirement.min_occurrences.has_value());
    CHECK(*requirement.min_occurrences == 1U);
    REQUIRE(requirement.max_occurrences.has_value());
    CHECK(*requirement.max_occurrences == 3U);
    CHECK(requirement.source == featherdoc::template_slot_source_kind::bookmark);

    REQUIRE(parse_slot("items:table-rows:count=2", requirement, error_message));
    CHECK(requirement.bookmark_name == "items");
    CHECK(requirement.kind == featherdoc::template_slot_kind::table_rows);
    CHECK(requirement.required);
    REQUIRE(requirement.min_occurrences.has_value());
    CHECK(*requirement.min_occurrences == 2U);
    REQUIRE(requirement.max_occurrences.has_value());
    CHECK(*requirement.max_occurrences == 2U);
}

TEST_CASE("cli template slot parser accepts content control source aliases") {
    featherdoc::template_slot_requirement requirement{};
    std::string error_message;
    REQUIRE(parse_slot("content_control_tag=customer_name:text", requirement,
                       error_message));
    CHECK(requirement.bookmark_name == "customer_name");
    CHECK(requirement.source ==
          featherdoc::template_slot_source_kind::content_control_tag);

    REQUIRE(parse_slot("content-control-tag=order_no:text", requirement,
                       error_message));
    CHECK(requirement.bookmark_name == "order_no");
    CHECK(requirement.source ==
          featherdoc::template_slot_source_kind::content_control_tag);

    REQUIRE(parse_slot("cc-tag=status:text", requirement, error_message));
    CHECK(requirement.bookmark_name == "status");
    CHECK(requirement.source ==
          featherdoc::template_slot_source_kind::content_control_tag);

    REQUIRE(parse_slot("content_control_alias=Line Items:table_rows", requirement,
                       error_message));
    CHECK(requirement.bookmark_name == "Line Items");
    CHECK(requirement.source ==
          featherdoc::template_slot_source_kind::content_control_alias);

    REQUIRE(parse_slot("content-control-alias=Header Review:block", requirement,
                       error_message));
    CHECK(requirement.bookmark_name == "Header Review");
    CHECK(requirement.source ==
          featherdoc::template_slot_source_kind::content_control_alias);

    REQUIRE(parse_slot("cc-alias=Signature:floating-image", requirement,
                       error_message));
    CHECK(requirement.bookmark_name == "Signature");
    CHECK(requirement.kind == featherdoc::template_slot_kind::floating_image);
    CHECK(requirement.source ==
          featherdoc::template_slot_source_kind::content_control_alias);
}

TEST_CASE("cli template slot parser reports invalid slot text") {
    featherdoc::template_slot_requirement requirement{};
    std::string error_message;

    CHECK_FALSE(parse_slot("customer", requirement, error_message));
    CHECK(error_message ==
          "invalid --slot value: expected <bookmark>:<kind>"
          "[:required|optional][:count=<n>|min=<n>|max=<n>...]");

    CHECK_FALSE(parse_slot("cc-tag=:text", requirement, error_message));
    CHECK(error_message == "invalid --slot value: slot name must not be empty");

    CHECK_FALSE(parse_slot("customer:unknown", requirement, error_message));
    CHECK(error_message == "invalid --slot kind: unknown");

    CHECK_FALSE(parse_slot("customer:text:optional:required", requirement,
                           error_message));
    CHECK(error_message == "duplicate --slot requirement segment: required");

    CHECK_FALSE(parse_slot("customer:text:count=2:min=1", requirement,
                           error_message));
    CHECK(error_message == "duplicate --slot min occurrence segment");

    CHECK_FALSE(parse_slot("customer:text:min=1:count=2", requirement,
                           error_message));
    CHECK(error_message == "conflicting --slot occurrence segment: count=2");

    CHECK_FALSE(parse_slot("customer:text:min=4:max=2", requirement,
                           error_message));
    CHECK(error_message ==
          "invalid --slot occurrence range: max must be greater than or equal to min");

    CHECK_FALSE(parse_slot("customer:text:min=abc", requirement, error_message));
    CHECK(error_message == "invalid --slot occurrence value: abc");

    CHECK_FALSE(parse_slot("customer:text:", requirement, error_message));
    CHECK(error_message == "invalid --slot segment: value must not be empty");
}
