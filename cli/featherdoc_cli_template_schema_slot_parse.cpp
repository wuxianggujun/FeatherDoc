#include "featherdoc_cli_template_schema_slot_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_slot_parse.hpp"

#include <string>
#include <utility>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto parse_template_schema_json_slot(
    std::string_view content, std::size_t &index,
    featherdoc::template_slot_requirement &requirement,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("JSON schema file", index,
                                       "expected slot entry", error_message);
    }

    if (content[index] == '"') {
        std::string slot_text;
        if (!parse_json_patch_string(content, index, slot_text,
                                     error_message)) {
            return false;
        }
        return parse_template_slot_requirement(slot_text, requirement,
                                               error_message);
    }

    if (content[index] != '{') {
        return report_json_input_error("JSON schema file", index,
                                       "slot entries must be strings or objects",
                                       error_message);
    }

    template_schema_json_slot_members members;
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
                return report_json_input_error("JSON schema file", index,
                                               "expected ':' after object member",
                                               error_message);
            }

            ++index;
            if (!parse_template_schema_json_slot_member(
                    content, index, member_name, members, error_message)) {
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
            return report_json_input_error(
                "JSON schema file", index,
                "expected ',' or '}' after object member", error_message);
        }
    }

    return finalize_template_schema_json_slot(members, requirement,
                                              error_message);
}

} // namespace

auto parse_template_schema_json_slots(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::template_slot_requirement> &slots,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("JSON schema file", index,
                                       "expected slots array", error_message);
    }

    slots.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        featherdoc::template_slot_requirement requirement;
        if (!parse_template_schema_json_slot(content, index, requirement,
                                             error_message)) {
            return false;
        }
        slots.push_back(std::move(requirement));

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
            "JSON schema file", index, "expected ',' or ']' after slot entry",
            error_message);
    }

    return report_json_input_error("JSON schema file", index,
                                   "unterminated slots array", error_message);
}

} // namespace featherdoc_cli
