#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "featherdoc_cli_template_schema_ops.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace {

auto make_slot(std::string bookmark_name,
               featherdoc::template_slot_source_kind source =
                   featherdoc::template_slot_source_kind::bookmark)
    -> featherdoc::template_slot_requirement {
    featherdoc::template_slot_requirement requirement;
    requirement.bookmark_name = std::move(bookmark_name);
    requirement.source = source;
    requirement.kind = featherdoc::template_slot_kind::text;
    requirement.required = true;
    return requirement;
}

auto make_target(featherdoc_cli::validation_part_family part)
    -> featherdoc_cli::exported_template_schema_target {
    featherdoc_cli::exported_template_schema_target target;
    target.part = part;
    return target;
}

} // namespace

TEST_CASE("cli template schema ops compare slots and build updates") {
    auto left = make_slot("customer");
    left.kind = featherdoc::template_slot_kind::text;
    left.required = true;
    left.min_occurrences = 1U;
    left.max_occurrences = 2U;

    auto right = left;
    right.kind = featherdoc::template_slot_kind::image;
    right.required = false;
    right.min_occurrences.reset();
    right.max_occurrences = 3U;

    CHECK(featherdoc_cli::compare_template_slot_requirement(left, left) == 0);
    CHECK(featherdoc_cli::compare_template_slot_requirement(left, right) != 0);
    CHECK(featherdoc_cli::compare_template_slot_requirement_shape(left, right) !=
          0);

    const auto update = featherdoc_cli::make_template_schema_slot_update(left, right);
    REQUIRE(update.kind.has_value());
    CHECK(*update.kind == featherdoc::template_slot_kind::image);
    REQUIRE(update.required.has_value());
    CHECK_FALSE(*update.required);
    CHECK(update.clear_min_occurrences);
    REQUIRE(update.max_occurrences.has_value());
    CHECK(*update.max_occurrences == 3U);
}

TEST_CASE("cli template schema ops reports clean schemas") {
    featherdoc_cli::exported_template_schema_result result;
    auto target = make_target(featherdoc_cli::validation_part_family::body);
    target.requirements.push_back(make_slot("alpha"));
    target.requirements.push_back(make_slot("beta"));
    result.targets.push_back(target);

    const auto lint = featherdoc_cli::lint_template_schema(result);

    CHECK(lint.clean());
    CHECK(lint.target_count == 1U);
    CHECK(lint.slot_count == 2U);
    CHECK(lint.issues.empty());
    CHECK(featherdoc_cli::template_schema_lint_issue_name(
              featherdoc_cli::template_schema_lint_issue_kind::slot_order) ==
          "slot_order");
}

TEST_CASE("cli template schema ops reports ordering and duplicate issues") {
    featherdoc_cli::exported_template_schema_result result;

    auto footer = make_target(featherdoc_cli::validation_part_family::footer);
    footer.requirements.push_back(make_slot("footer_slot"));
    result.targets.push_back(footer);

    auto body = make_target(featherdoc_cli::validation_part_family::body);
    body.entry_name = "word/document.xml";
    body.requirements.push_back(make_slot("zeta"));
    body.requirements.push_back(make_slot("alpha"));
    body.requirements.push_back(make_slot("alpha"));
    result.targets.push_back(body);

    auto duplicate_body = make_target(featherdoc_cli::validation_part_family::body);
    duplicate_body.requirements.push_back(make_slot("zzzz"));
    result.targets.push_back(duplicate_body);

    const auto lint = featherdoc_cli::lint_template_schema(result);

    CHECK_FALSE(lint.clean());
    CHECK(lint.target_count == 3U);
    CHECK(lint.slot_count == 5U);
    CHECK(lint.issue_count(
              featherdoc_cli::template_schema_lint_issue_kind::target_order) ==
          1U);
    CHECK(lint.issue_count(
              featherdoc_cli::template_schema_lint_issue_kind::
                  duplicate_target_identity) == 1U);
    CHECK(lint.issue_count(
              featherdoc_cli::template_schema_lint_issue_kind::slot_order) ==
          1U);
    CHECK(lint.issue_count(
              featherdoc_cli::template_schema_lint_issue_kind::
                  duplicate_slot_name) == 1U);
    CHECK(lint.issue_count(
              featherdoc_cli::template_schema_lint_issue_kind::
                  entry_name_present) == 1U);
}
