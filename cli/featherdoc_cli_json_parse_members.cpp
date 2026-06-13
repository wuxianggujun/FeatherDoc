#include "featherdoc_cli_json_parse.hpp"

#include "featherdoc_cli_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_json_patch_index_value(std::string_view text, std::size_t &index,
                                  std::size_t &value,
                                  std::string_view member_name,
                                  std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(text, index);
    if (index >= text.size()) {
        return report_json_patch_error(index, "expected integer value",
                                       error_message);
    }

    if (text[index] == '"') {
        if (!parse_json_patch_string(text, index, token, error_message)) {
            return false;
        }
    } else if (text[index] == '-' || (text[index] >= '0' && text[index] <= '9')) {
        if (!parse_json_patch_number(text, index, token, error_message)) {
            return false;
        }
    } else {
        return report_json_patch_error(index, "expected integer value",
                                       error_message);
    }

    if (!parse_index(token, value)) {
        error_message = "JSON patch member '" + std::string(member_name) +
                        "' must be an unsigned integer";
        return false;
    }

    return true;
}

auto resolve_json_patch_string_member(
    const std::optional<std::string> &primary_value,
    const std::optional<std::string> &alias_value,
    std::string_view primary_name, std::string_view alias_name,
    std::optional<std::string> &resolved_value,
    std::string &error_message) -> bool {
    if (primary_value.has_value() && alias_value.has_value() &&
        *primary_value != *alias_value) {
        error_message = "JSON patch members '" + std::string(primary_name) +
                        "' and '" + std::string(alias_name) +
                        "' must match when both are present";
        return false;
    }

    if (primary_value.has_value()) {
        resolved_value = primary_value;
    } else {
        resolved_value = alias_value;
    }

    return true;
}

auto resolve_json_patch_index_member(std::optional<std::size_t> primary_value,
                                     std::optional<std::size_t> alias_value,
                                     std::string_view primary_name,
                                     std::string_view alias_name,
                                     std::optional<std::size_t> &resolved_value,
                                     std::string &error_message) -> bool {
    if (primary_value.has_value() && alias_value.has_value() &&
        *primary_value != *alias_value) {
        error_message = "JSON patch members '" + std::string(primary_name) +
                        "' and '" + std::string(alias_name) +
                        "' must match when both are present";
        return false;
    }

    if (primary_value.has_value()) {
        resolved_value = primary_value;
    } else {
        resolved_value = alias_value;
    }

    return true;
}

auto parse_json_patch_bool_member_value(
    std::string_view text, std::size_t &index, bool &value,
    std::string_view member_name, std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    if (index >= text.size()) {
        return report_json_input_error("JSON schema file", index,
                                       "expected boolean value", error_message);
    }

    if (text[index] == '"') {
        std::string token;
        if (!parse_json_patch_string(text, index, token, error_message)) {
            return false;
        }
        if (!parse_bool(token, value)) {
            error_message = "JSON schema member '" + std::string(member_name) +
                            "' must be a boolean";
            return false;
        }
        return true;
    }

    if (text.substr(index, 4U) == "true") {
        value = true;
        index += 4U;
        return true;
    }

    if (text.substr(index, 5U) == "false") {
        value = false;
        index += 5U;
        return true;
    }

    error_message = "JSON schema member '" + std::string(member_name) +
                    "' must be a boolean";
    return false;
}

} // namespace featherdoc_cli
