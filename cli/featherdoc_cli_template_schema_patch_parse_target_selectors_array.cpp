#include "featherdoc_cli_template_schema_patch_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"

namespace featherdoc_cli::detail {

auto parse_template_schema_patch_target_selectors_array(
    std::string_view content, std::size_t &index,
    std::vector<template_schema_patch_target_selector> &selectors,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("JSON schema patch file", index,
                                       "expected selector array", error_message);
    }

    selectors.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        template_schema_patch_target_selector selector;
        if (!parse_template_schema_patch_target_selector(content, index, selector,
                                                         error_message)) {
            return false;
        }
        selectors.push_back(std::move(selector));

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
        return report_json_input_error("JSON schema patch file", index,
                                       "expected ',' or ']' after selector entry",
                                       error_message);
    }

    return report_json_input_error("JSON schema patch file", index,
                                   "unterminated selector array", error_message);
}

} // namespace featherdoc_cli::detail
