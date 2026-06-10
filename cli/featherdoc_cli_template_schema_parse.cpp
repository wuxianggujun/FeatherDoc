#include "featherdoc_cli_template_schema_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_template_schema_slot_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto parse_template_schema_json_target(
    std::string_view content, std::size_t &index,
    exported_template_schema_target &target, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema file", index,
                                       "expected target object", error_message);
    }

    std::optional<std::string> part_value;
    std::optional<std::size_t> index_value;
    std::optional<std::size_t> section_value;
    std::optional<featherdoc::section_reference_kind> kind_value;
    std::optional<std::size_t> resolved_from_section_value;
    std::optional<bool> linked_to_previous_value;
    std::optional<std::string> entry_name_value;
    bool saw_slots = false;
    std::vector<featherdoc::template_slot_requirement> slots;

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
            if (member_name == "part") {
                if (part_value.has_value()) {
                    error_message =
                        "JSON schema member 'part' must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                part_value.emplace();
                if (!parse_json_patch_string(content, index, *part_value,
                                             error_message)) {
                    return false;
                }
            } else if (member_name == "index") {
                if (index_value.has_value()) {
                    error_message =
                        "JSON schema member 'index' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index, "index",
                                                  error_message)) {
                    return false;
                }
                index_value = parsed_index;
            } else if (member_name == "section") {
                if (section_value.has_value()) {
                    error_message =
                        "JSON schema member 'section' must not be duplicated";
                    return false;
                }
                std::size_t parsed_index = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_index,
                                                  "section", error_message)) {
                    return false;
                }
                section_value = parsed_index;
            } else if (member_name == "kind") {
                if (kind_value.has_value()) {
                    error_message =
                        "JSON schema member 'kind' must not be duplicated";
                    return false;
                }
                featherdoc::section_reference_kind parsed_kind{};
                if (!parse_json_patch_reference_kind_value(content, index,
                                                           parsed_kind,
                                                           error_message)) {
                    return false;
                }
                kind_value = parsed_kind;
            } else if (member_name == "slots") {
                if (saw_slots) {
                    error_message =
                        "JSON schema member 'slots' must not be duplicated";
                    return false;
                }
                if (!parse_template_schema_json_slots(content, index, slots,
                                                      error_message)) {
                    return false;
                }
                saw_slots = true;
            } else if (member_name == "linked_to_previous") {
                if (linked_to_previous_value.has_value()) {
                    error_message = "JSON schema member 'linked_to_previous' "
                                    "must not be duplicated";
                    return false;
                }
                bool parsed_value = false;
                if (!parse_json_patch_bool_member_value(content, index, parsed_value,
                                                        "linked_to_previous",
                                                        error_message)) {
                    return false;
                }
                linked_to_previous_value = parsed_value;
            } else if (member_name == "resolved_from_section") {
                if (resolved_from_section_value.has_value()) {
                    error_message = "JSON schema member 'resolved_from_section' "
                                    "must not be duplicated";
                    return false;
                }
                std::size_t parsed_value = 0U;
                if (!parse_json_patch_index_value(content, index, parsed_value,
                                                  "resolved_from_section",
                                                  error_message)) {
                    return false;
                }
                resolved_from_section_value = parsed_value;
            } else if (member_name == "entry_name") {
                if (entry_name_value.has_value()) {
                    error_message = "JSON schema member 'entry_name' must not be duplicated";
                    return false;
                }
                skip_json_patch_whitespace(content, index);
                entry_name_value.emplace();
                if (!parse_json_patch_string(content, index, *entry_name_value,
                                             error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("JSON schema file", index,
                                               "unknown target member",
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
            return report_json_input_error("JSON schema file", index,
                                           "expected ',' or '}' after object member",
                                           error_message);
        }
    }

    if (!part_value.has_value()) {
        error_message = "JSON schema target must contain 'part'";
        return false;
    }
    if (!saw_slots) {
        error_message = "JSON schema target must contain a 'slots' array";
        return false;
    }
    if (slots.empty()) {
        error_message = "JSON schema target 'slots' array must not be empty";
        return false;
    }

    target = {};
    if (!parse_validation_part(*part_value, target.part)) {
        error_message =
            "JSON schema member 'part' must be one of "
            "'body', 'header', 'footer', 'section-header', or 'section-footer'";
        return false;
    }
    target.part_index = index_value;
    target.section_index = section_value;
    target.requirements = std::move(slots);
    target.entry_name = entry_name_value.value_or(std::string{});
    target.reference_kind = kind_value;
    target.resolved_from_section_index = resolved_from_section_value;
    target.linked_to_previous = linked_to_previous_value.value_or(false);

    return validate_template_part_selection(target.part, target.part_index,
                                            target.section_index,
                                            target.reference_kind.has_value(),
                                            "schema validation", error_message);
}

auto read_template_schema_file(const path_type &schema_path,
                               exported_template_schema_result &result,
                               std::string &error_message) -> bool {
    std::string content;
    std::size_t index = 0U;
    if (!read_template_table_json_content(schema_path, content, index,
                                          error_message)) {
        if (error_message.rfind("failed to read JSON patch file:", 0U) == 0U) {
            error_message.replace(0U, std::string("failed to read JSON patch file:").size(),
                                  "failed to read JSON schema file:");
        }
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("JSON schema file", index,
                                       "root must be an object", error_message);
    }

    result = {};
    bool saw_targets = false;
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
            if (member_name == "targets") {
                if (saw_targets) {
                    error_message =
                        "JSON schema member 'targets' must not be duplicated";
                    return false;
                }

                skip_json_patch_whitespace(content, index);
                if (index >= content.size() || content[index] != '[') {
                    return report_json_input_error("JSON schema file", index,
                                                   "expected targets array",
                                                   error_message);
                }

                ++index;
                skip_json_patch_whitespace(content, index);
                if (index < content.size() && content[index] == ']') {
                    ++index;
                    saw_targets = true;
                } else {
                    while (index < content.size()) {
                        exported_template_schema_target target;
                        if (!parse_template_schema_json_target(content, index, target,
                                                               error_message)) {
                            return false;
                        }
                        result.targets.push_back(std::move(target));

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
                            saw_targets = true;
                            break;
                        }
                        return report_json_input_error(
                            "JSON schema file", index,
                            "expected ',' or ']' after target entry", error_message);
                    }
                }
            } else {
                return report_json_input_error("JSON schema file", index,
                                               "unknown root member", error_message);
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
            return report_json_input_error("JSON schema file", index,
                                           "expected ',' or '}' after object member",
                                           error_message);
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error("JSON schema file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }

    if (!saw_targets) {
        error_message = "JSON schema file must contain a 'targets' array";
        return false;
    }

    return true;
}
} // namespace featherdoc_cli
