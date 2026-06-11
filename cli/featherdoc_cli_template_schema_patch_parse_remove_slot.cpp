#include "featherdoc_cli_template_schema_patch_parse_remove_slot.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_schema_patch_parse_remove_slot_detail.hpp"

#include <utility>

namespace featherdoc_cli {
namespace detail {
namespace {

[[nodiscard]] auto parse_template_schema_patch_remove_slot(
    std::string_view content, std::size_t &index,
    template_schema_patch_remove_slot &operation, std::string &error_message)
    -> bool {
    template_schema_patch_remove_slot_members members;
    if (!parse_template_schema_patch_remove_slot_members(
            content, index, members, error_message)) {
        return false;
    }
    return finalize_template_schema_patch_remove_slot(members, operation,
                                                      error_message);
}

} // namespace

auto parse_template_schema_patch_remove_slots_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_remove_slot> &operations,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected remove_slots array",
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
        template_schema_patch_remove_slot operation;
        if (!parse_template_schema_patch_remove_slot(content, index, operation,
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
            "expected ',' or ']' after remove-slot entry", error_message);
    }

    return report_json_input_error("JSON schema patch file", index,
                                   "unterminated remove_slots array",
                                   error_message);
}

} // namespace detail
} // namespace featherdoc_cli
