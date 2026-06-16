#include "featherdoc_cli_template_schema_patch_parse_rename_slot_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <utility>

namespace featherdoc_cli::detail {
namespace {

[[nodiscard]] auto resolve_rename_slot_source(
    const template_schema_patch_rename_slot_members &members,
    std::optional<std::string> &resolved_bookmark_name,
    featherdoc::template_slot_source_kind &source,
    std::string &resolved_slot_name, std::string &error_message) -> bool {
    if (!resolve_json_patch_string_member(
            members.bookmark_value, members.bookmark_name_value, "bookmark",
            "bookmark_name", resolved_bookmark_name, error_message)) {
        return false;
    }

    std::size_t selector_count = 0U;
    selector_count += resolved_bookmark_name.has_value() ? 1U : 0U;
    selector_count += members.content_control_tag_value.has_value() ? 1U : 0U;
    selector_count += members.content_control_alias_value.has_value() ? 1U : 0U;
    if (selector_count != 1U) {
        error_message =
            "JSON schema patch rename-slot object must contain exactly one of "
            "'bookmark', 'bookmark_name', 'content_control_tag', or "
            "'content_control_alias'";
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
        error_message = "JSON schema patch rename-slot selector must not be empty";
        return false;
    }
    return true;
}

[[nodiscard]] auto resolve_rename_slot_new_source(
    const template_schema_patch_rename_slot_members &members,
    featherdoc::template_slot_source_kind source,
    featherdoc::template_slot_source_kind &new_source,
    std::string &resolved_new_slot_name, std::string &error_message) -> bool {
    std::optional<std::string> resolved_new_bookmark_name;
    if (!resolve_json_patch_string_member(
            members.new_bookmark_value, members.new_bookmark_name_value,
            "new_bookmark", "new_bookmark_name", resolved_new_bookmark_name,
            error_message)) {
        return false;
    }

    std::size_t new_selector_count = 0U;
    new_selector_count += resolved_new_bookmark_name.has_value() ? 1U : 0U;
    new_selector_count += members.new_content_control_tag_value.has_value() ? 1U : 0U;
    new_selector_count +=
        members.new_content_control_alias_value.has_value() ? 1U : 0U;
    if (new_selector_count != 1U) {
        if (source == featherdoc::template_slot_source_kind::bookmark &&
            new_selector_count == 0U) {
            error_message =
                "JSON schema patch rename-slot object must contain "
                "'new_bookmark' or 'new_bookmark_name'";
        } else {
            error_message =
                "JSON schema patch rename-slot object must contain exactly one "
                "matching new selector";
        }
        return false;
    }

    new_source = featherdoc::template_slot_source_kind::bookmark;
    if (resolved_new_bookmark_name.has_value()) {
        resolved_new_slot_name = *resolved_new_bookmark_name;
    } else if (members.new_content_control_tag_value.has_value()) {
        new_source = featherdoc::template_slot_source_kind::content_control_tag;
        resolved_new_slot_name = *members.new_content_control_tag_value;
    } else {
        new_source = featherdoc::template_slot_source_kind::content_control_alias;
        resolved_new_slot_name = *members.new_content_control_alias_value;
    }

    if (new_source != source) {
        error_message =
            "JSON schema patch rename-slot cannot change slot source; use "
            "remove_slots and upsert_targets instead";
        return false;
    }
    if (resolved_new_slot_name.empty()) {
        error_message =
            "JSON schema patch rename-slot new selector must not be empty";
        return false;
    }
    return true;
}

} // namespace

auto finalize_template_schema_patch_rename_slot(
    const template_schema_patch_rename_slot_members &members,
    template_schema_patch_rename_slot &operation,
    std::string &error_message) -> bool {
    operation = {};
    if (!finalize_template_schema_patch_selector(
            members.part_value, members.index_value, members.part_index_value,
            members.section_value, members.kind_value,
            members.resolved_from_section_value,
            members.linked_to_previous_value, operation.target, error_message)) {
        return false;
    }

    std::optional<std::string> resolved_bookmark_name;
    auto source = featherdoc::template_slot_source_kind::bookmark;
    std::string resolved_slot_name;
    if (!resolve_rename_slot_source(members, resolved_bookmark_name, source,
                                    resolved_slot_name, error_message)) {
        return false;
    }

    auto new_source = featherdoc::template_slot_source_kind::bookmark;
    std::string resolved_new_slot_name;
    if (!resolve_rename_slot_new_source(members, source, new_source,
                                        resolved_new_slot_name,
                                        error_message)) {
        return false;
    }

    operation.bookmark_name = std::move(resolved_slot_name);
    operation.new_bookmark_name = std::move(resolved_new_slot_name);
    operation.source = source;
    return true;
}

} // namespace featherdoc_cli::detail
