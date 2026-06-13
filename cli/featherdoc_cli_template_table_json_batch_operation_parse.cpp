#include "featherdoc_cli_template_table_json_batch_operation_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_table_json_batch_operation_parse_detail.hpp"
#include "featherdoc_cli_template_table_json_patch_parse.hpp"

#include <string>
#include <utility>

namespace featherdoc_cli {

namespace {

auto parse_template_table_json_batch_operation(
    std::string_view content, std::size_t &index,
    template_table_json_batch_operation &operation,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_patch_error(index, "expected operation object",
                                       error_message);
    }

    auto state = detail::template_table_json_batch_operation_parse_state{};
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
                return report_json_patch_error(
                    index, "expected ':' after object member", error_message);
            }

            ++index;
            if (!detail::parse_template_table_json_batch_operation_member(
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
            return report_json_patch_error(
                index, "expected ',' or '}' after object member",
                error_message);
        }
    }

    return detail::finalize_template_table_json_batch_operation(
        std::move(state), operation, error_message);
}

} // namespace

auto parse_template_table_json_batch_operations_array(
    std::string_view content, std::size_t &index,
    std::vector<template_table_json_batch_operation> &operations,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_patch_error(index, "expected operations array",
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
        template_table_json_batch_operation operation;
        if (!parse_template_table_json_batch_operation(content, index,
                                                       operation,
                                                       error_message)) {
            error_message = prefix_template_table_json_batch_message(
                operations.size(), error_message);
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
        return report_json_patch_error(index,
                                       "expected ',' or ']' after operation",
                                       error_message);
    }

    return report_json_patch_error(index, "unterminated operations array",
                                   error_message);
}

} // namespace featherdoc_cli
