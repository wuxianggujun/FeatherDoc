#include "featherdoc_cli_template_schema_patch_parse.hpp"

#include "featherdoc_cli_template_schema_patch_parse_detail.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_schema_parse.hpp"
#include "featherdoc_cli_template_slot_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;
using detail::parse_template_schema_patch_remove_slots_array;
using detail::parse_template_schema_patch_rename_slots_array;
using detail::parse_template_schema_patch_target_selectors_array;
using detail::parse_template_schema_patch_update_slots_array;
using detail::rewrite_template_schema_patch_error;

auto read_template_schema_patch_file(const path_type &patch_path,
                                     template_schema_patch_document &patch,
                                     std::string &error_message) -> bool {
    std::string content;
    std::size_t index = 0U;
    if (!read_template_table_json_content(patch_path, content, index,
                                          error_message)) {
        if (error_message.rfind("failed to read JSON patch file:", 0U) == 0U) {
            error_message.replace(0U, std::string("failed to read JSON patch file:").size(),
                                  "failed to read JSON schema patch file:");
        }
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema patch file", index,
                                       "root must be an object", error_message);
    }

    patch = {};
    bool saw_upsert_targets = false;
    bool saw_remove_targets = false;
    bool saw_remove_slots = false;
    bool saw_rename_slots = false;
    bool saw_update_slots = false;

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
            if (member_name == "upsert_targets") {
                if (saw_upsert_targets) {
                    error_message = "JSON schema patch member 'upsert_targets' "
                                    "must not be duplicated";
                    return false;
                }
                saw_upsert_targets = true;

                skip_json_patch_whitespace(content, index);
                if (index >= content.size() || content[index] != '[') {
                    return report_json_input_error("JSON schema patch file", index,
                                                   "expected upsert_targets array",
                                                   error_message);
                }

                ++index;
                skip_json_patch_whitespace(content, index);
                if (index < content.size() && content[index] == ']') {
                    ++index;
                } else {
                    while (index < content.size()) {
                        exported_template_schema_target target;
                        if (!parse_template_schema_json_target(content, index, target,
                                                               error_message)) {
                            rewrite_template_schema_patch_error(error_message);
                            return false;
                        }
                        patch.upsert_targets.push_back(std::move(target));

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
                            break;
                        }
                        return report_json_input_error(
                            "JSON schema patch file", index,
                            "expected ',' or ']' after upsert target entry",
                            error_message);
                    }
                }
            } else if (member_name == "remove_targets") {
                if (saw_remove_targets) {
                    error_message = "JSON schema patch member 'remove_targets' "
                                    "must not be duplicated";
                    return false;
                }
                saw_remove_targets = true;
                if (!parse_template_schema_patch_target_selectors_array(
                        content, index, patch.remove_targets, error_message)) {
                    return false;
                }
            } else if (member_name == "remove_slots") {
                if (saw_remove_slots) {
                    error_message = "JSON schema patch member 'remove_slots' "
                                    "must not be duplicated";
                    return false;
                }
                saw_remove_slots = true;
                if (!parse_template_schema_patch_remove_slots_array(
                        content, index, patch.remove_slots, error_message)) {
                    return false;
                }
            } else if (member_name == "rename_slots") {
                if (saw_rename_slots) {
                    error_message = "JSON schema patch member 'rename_slots' "
                                    "must not be duplicated";
                    return false;
                }
                saw_rename_slots = true;
                if (!parse_template_schema_patch_rename_slots_array(
                        content, index, patch.rename_slots, error_message)) {
                    return false;
                }
            } else if (member_name == "update_slots") {
                if (saw_update_slots) {
                    error_message = "JSON schema patch member 'update_slots' "
                                    "must not be duplicated";
                    return false;
                }
                saw_update_slots = true;
                if (!parse_template_schema_patch_update_slots_array(
                        content, index, patch.update_slots, error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("JSON schema patch file", index,
                                               "unknown root member",
                                               error_message);
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

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error("JSON schema patch file", index,
                                       "unexpected trailing content",
                                       error_message);
    }

    return true;
}

} // namespace featherdoc_cli
