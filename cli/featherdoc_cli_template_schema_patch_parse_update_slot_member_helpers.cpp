#include "featherdoc_cli_template_schema_patch_parse_update_slot_member_helpers.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_slot_parse.hpp"

#include <string>

namespace featherdoc_cli::detail {

auto parse_update_slot_string_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::string> &value, std::string &error_message) -> bool {
    if (value.has_value()) {
        error_message = "JSON schema patch member '" + std::string(member_name) +
                        "' must not be duplicated";
        return false;
    }
    skip_json_patch_whitespace(content, index);
    value.emplace();
    return parse_json_patch_string(content, index, *value, error_message);
}

auto parse_update_slot_index_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::size_t> &value, std::string &error_message) -> bool {
    if (value.has_value()) {
        error_message = "JSON schema patch update-slot member '" +
                        std::string(member_name) + "' must not be duplicated";
        return false;
    }
    std::size_t parsed_index = 0U;
    if (!parse_json_patch_index_value(content, index, parsed_index, member_name,
                                      error_message)) {
        return false;
    }
    value = parsed_index;
    return true;
}

auto parse_update_slot_selector_index_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::size_t> &value, std::string &error_message) -> bool {
    if (value.has_value()) {
        error_message = "JSON schema patch member '" + std::string(member_name) +
                        "' must not be duplicated";
        return false;
    }
    std::size_t parsed_index = 0U;
    if (!parse_json_patch_index_value(content, index, parsed_index, member_name,
                                      error_message)) {
        return false;
    }
    value = parsed_index;
    return true;
}

auto parse_update_slot_bool_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<bool> &value, std::string &error_message) -> bool {
    if (value.has_value()) {
        error_message = "JSON schema patch update-slot member '" +
                        std::string(member_name) + "' must not be duplicated";
        return false;
    }
    bool parsed_value = false;
    if (!parse_json_patch_bool_member_value(content, index, parsed_value,
                                            member_name, error_message)) {
        return false;
    }
    value = parsed_value;
    return true;
}

auto parse_update_slot_selector_bool_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<bool> &value, std::string &error_message) -> bool {
    if (value.has_value()) {
        error_message = "JSON schema patch member '" + std::string(member_name) +
                        "' must not be duplicated";
        return false;
    }
    bool parsed_value = false;
    if (!parse_json_patch_bool_member_value(content, index, parsed_value,
                                            member_name, error_message)) {
        return false;
    }
    value = parsed_value;
    return true;
}

auto parse_update_slot_kind_member(
    std::string_view content, std::size_t &index,
    template_schema_patch_update_slot_parse_state &state,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    std::string token;
    if (!parse_json_patch_string(content, index, token, error_message)) {
        return false;
    }

    featherdoc::section_reference_kind parsed_reference_kind{};
    featherdoc::template_slot_kind parsed_slot_kind{};
    if (parse_reference_kind(token, parsed_reference_kind)) {
        if (state.kind_value.has_value()) {
            error_message = "JSON schema patch member 'kind' must not be duplicated";
            return false;
        }
        state.kind_value = parsed_reference_kind;
        return true;
    }
    if (parse_template_slot_kind(token, parsed_slot_kind)) {
        if (state.slot_kind_value.has_value()) {
            error_message =
                "JSON schema patch update-slot member 'kind' must not be duplicated";
            return false;
        }
        state.slot_kind_value = parsed_slot_kind;
        return true;
    }

    error_message = "JSON schema patch update-slot member 'kind' must be a section "
                    "reference kind or template slot kind";
    return false;
}

auto parse_update_slot_reference_kind_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_patch_update_slot_parse_state &state,
    std::string &error_message) -> bool {
    if (state.kind_value.has_value()) {
        error_message = "JSON schema patch member '" + std::string(member_name) +
                        "' must not be duplicated";
        return false;
    }
    featherdoc::section_reference_kind parsed_kind{};
    if (!parse_json_patch_reference_kind_value(content, index, parsed_kind,
                                               error_message)) {
        return false;
    }
    state.kind_value = parsed_kind;
    return true;
}

auto parse_update_slot_slot_kind_member(
    std::string_view content, std::size_t &index,
    template_schema_patch_update_slot_parse_state &state,
    std::string &error_message) -> bool {
    if (state.slot_kind_value.has_value()) {
        error_message =
            "JSON schema patch update-slot member 'slot_kind' must not be duplicated";
        return false;
    }

    std::string token;
    skip_json_patch_whitespace(content, index);
    if (!parse_json_patch_string(content, index, token, error_message)) {
        return false;
    }

    featherdoc::template_slot_kind parsed_kind{};
    if (!parse_template_slot_kind(token, parsed_kind)) {
        error_message = "JSON schema patch update-slot member 'slot_kind' must be "
                        "one of 'text', 'table_rows', 'table', 'image', "
                        "'floating_image', or 'block'";
        return false;
    }
    state.slot_kind_value = parsed_kind;
    return true;
}

} // namespace featherdoc_cli::detail
