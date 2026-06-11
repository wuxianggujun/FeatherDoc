#include "featherdoc_cli_template_schema_patch_parse_slot_mutation.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_schema_patch_parse_update_slot_state.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace featherdoc_cli {
namespace detail {
namespace {

auto parse_template_schema_patch_update_slot(
    std::string_view content, std::size_t &index,
    template_schema_patch_update_slot &operation, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected update-slot object",
                                       error_message);
    }

    auto state = template_schema_patch_update_slot_parse_state{};
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == '}') {
        ++index;
    } else {
        while (index < content.size()) {
            std::string member_name;
            if (!parse_json_patch_string(content, index, member_name,
                                         error_message)) {
                return false;
            }
            skip_json_patch_whitespace(content, index);
            if (index >= content.size() || content[index] != ':') {
                return report_json_input_error("JSON schema patch file", index,
                                               "expected ':' after object member",
                                               error_message);
            }

            ++index;
            if (!parse_template_schema_patch_update_slot_member(
                    content, index, member_name, state, error_message)) {
                return false;
            }

            skip_json_patch_whitespace(content, index);
            if (index >= content.size()) {
                break;
            }
            if (content[index] == ',') {
                ++index;
                skip_json_patch_whitespace(content, index);
                continue;
            }
            if (content[index] == '}') {
                ++index;
                break;
            }
            return report_json_input_error("JSON schema patch file", index,
                                           "expected ',' or '}' after object member",
                                           error_message);
        }
    }

    return finalize_template_schema_patch_update_slot(state, operation,
                                                      error_message);
}

} // namespace

auto parse_template_schema_patch_update_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_update_slot> &operations,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected update_slots array",
                                       error_message);
    }

    operations.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        template_schema_patch_update_slot operation;
        if (!parse_template_schema_patch_update_slot(content, index, operation,
                                                     error_message)) {
            return false;
        }
        operations.push_back(std::move(operation));

        skip_json_patch_whitespace(content, index);
        if (index >= content.size()) {
            break;
        }
        if (content[index] == ',') {
            ++index;
            skip_json_patch_whitespace(content, index);
            continue;
        }
        if (content[index] == ']') {
            ++index;
            return true;
        }
        return report_json_input_error(
            "JSON schema patch file", index,
            "expected ',' or ']' after update-slot entry", error_message);
    }

    return report_json_input_error("JSON schema patch file", index,
                                   "unterminated update_slots array",
                                   error_message);
}

} // namespace detail
} // namespace featherdoc_cli
