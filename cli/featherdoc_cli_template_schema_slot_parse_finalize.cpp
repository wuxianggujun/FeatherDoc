#include "featherdoc_cli_template_schema_slot_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_slot_parse.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto resolve_template_schema_slot_selector(
    const template_schema_json_slot_members &members,
    featherdoc::template_slot_source_kind &source,
    std::string &resolved_slot_name, std::string &error_message) -> bool {
    std::optional<std::string> resolved_bookmark_name;
    if (!resolve_json_patch_string_member(
            members.bookmark_value, members.bookmark_name_value, "bookmark",
            "bookmark_name", resolved_bookmark_name, error_message)) {
        return false;
    }

    std::size_t selector_count = 0U;
    selector_count += resolved_bookmark_name.has_value() ? 1U : 0U;
    selector_count += members.content_control_tag_value.has_value() ? 1U : 0U;
    selector_count +=
        members.content_control_alias_value.has_value() ? 1U : 0U;
    if (selector_count != 1U) {
        error_message =
            "JSON schema slot must contain exactly one of 'bookmark', "
            "'bookmark_name', 'content_control_tag', or 'content_control_alias'";
        return false;
    }

    source = featherdoc::template_slot_source_kind::bookmark;
    if (resolved_bookmark_name.has_value()) {
        resolved_slot_name = *resolved_bookmark_name;
    } else if (members.content_control_tag_value.has_value()) {
        source = featherdoc::template_slot_source_kind::content_control_tag;
        resolved_slot_name = *members.content_control_tag_value;
    } else {
        source = featherdoc::template_slot_source_kind::content_control_alias;
        resolved_slot_name = *members.content_control_alias_value;
    }
    if (resolved_slot_name.empty()) {
        error_message = "JSON schema slot selector must not be empty";
        return false;
    }
    return true;
}

[[nodiscard]] auto parse_template_schema_slot_kind(
    const template_schema_json_slot_members &members,
    featherdoc::template_slot_kind &kind, std::string &error_message) -> bool {
    if (!members.kind_value.has_value()) {
        error_message = "JSON schema slot must contain 'kind'";
        return false;
    }

    if (!parse_template_slot_kind(*members.kind_value, kind)) {
        error_message =
            "JSON schema slot member 'kind' must be one of "
            "'text', 'table_rows', 'table', 'image', 'floating_image', or "
            "'block'";
        return false;
    }
    return true;
}

[[nodiscard]] auto validate_template_schema_slot_occurrence_range(
    const template_schema_json_slot_members &members,
    std::string &error_message) -> bool {
    if (members.count_value.has_value() &&
        (members.min_value.has_value() || members.max_value.has_value())) {
        error_message =
            "JSON schema slot member 'count' conflicts with 'min'/'max'";
        return false;
    }
    if (members.min_value.has_value() && members.max_value.has_value() &&
        *members.max_value < *members.min_value) {
        error_message =
            "JSON schema slot occurrence range is invalid: max must be greater "
            "than or equal to min";
        return false;
    }
    return true;
}

} // namespace

auto finalize_template_schema_json_slot(
    const template_schema_json_slot_members &members,
    featherdoc::template_slot_requirement &requirement,
    std::string &error_message) -> bool {
    auto source = featherdoc::template_slot_source_kind::bookmark;
    std::string resolved_slot_name;
    if (!resolve_template_schema_slot_selector(
            members, source, resolved_slot_name, error_message)) {
        return false;
    }

    featherdoc::template_slot_kind kind{};
    if (!parse_template_schema_slot_kind(members, kind, error_message) ||
        !validate_template_schema_slot_occurrence_range(members,
                                                        error_message)) {
        return false;
    }

    requirement = {};
    requirement.bookmark_name = std::move(resolved_slot_name);
    requirement.kind = kind;
    requirement.required = members.required_value.value_or(true);
    requirement.source = source;
    if (members.count_value.has_value()) {
        requirement.min_occurrences = *members.count_value;
        requirement.max_occurrences = *members.count_value;
    } else {
        requirement.min_occurrences = members.min_value;
        requirement.max_occurrences = members.max_value;
    }
    return true;
}

} // namespace featherdoc_cli
