#include "featherdoc_cli_style_refactor_rollback_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_refactor_plan_parse.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;
auto consume_style_refactor_rollback_separator(std::string_view content,
                                                std::size_t &index,
                                                char close_char,
                                                std::string_view error_detail,
                                                bool &closed,
                                                std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("style refactor rollback file", index,
                                       "unexpected end of JSON", error_message);
    }
    if (content[index] == ',') {
        ++index;
        skip_json_patch_whitespace(content, index);
        closed = false;
        return true;
    }
    if (content[index] == close_char) {
        ++index;
        closed = true;
        return true;
    }

    return report_json_input_error("style refactor rollback file", index,
                                   error_detail, error_message);
}

auto parse_json_bool_value(std::string_view content, std::size_t &index,
                           bool &value, std::string_view member_name,
                           std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "true") {
        value = true;
        index += 4U;
        return true;
    }
    if (content.substr(index, 5U) == "false") {
        value = false;
        index += 5U;
        return true;
    }
    if (index < content.size() && content[index] == '"') {
        std::string token;
        if (!parse_json_patch_string(content, index, token, error_message)) {
            return false;
        }
        if (parse_bool(token, value)) {
            return true;
        }
    }

    error_message = "JSON style refactor rollback member '" +
                    std::string(member_name) + "' must be a boolean";
    return false;
}

auto consume_json_null_value(std::string_view content, std::size_t &index,
                             std::string_view member_name,
                             std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "null") {
        index += 4U;
        return true;
    }

    error_message = "JSON style refactor rollback member '" +
                    std::string(member_name) + "' must be null";
    return false;
}

auto parse_nullable_json_string_value(std::string_view content,
                                      std::size_t &index,
                                      std::string &value,
                                      std::string_view member_name,
                                      std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "null") {
        value.clear();
        index += 4U;
        return true;
    }

    if (index < content.size() && content[index] == '"') {
        return parse_json_patch_string(content, index, value, error_message);
    }

    error_message = "JSON style refactor rollback member '" +
                    std::string(member_name) + "' must be a string or null";
    return false;
}

auto parse_style_usage_part_kind_value(
    std::string_view content, std::size_t &index,
    featherdoc::style_usage_part_kind &part,
    std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(content, index);
    if (!parse_json_patch_string(content, index, token, error_message)) {
        return false;
    }

    if (token == "body") {
        part = featherdoc::style_usage_part_kind::body;
        return true;
    }
    if (token == "header") {
        part = featherdoc::style_usage_part_kind::header;
        return true;
    }
    if (token == "footer") {
        part = featherdoc::style_usage_part_kind::footer;
        return true;
    }

    error_message = "JSON style usage hit member 'part' must be 'body', "
                    "'header', or 'footer'";
    return false;
}

auto parse_style_usage_hit_kind_value(
    std::string_view content, std::size_t &index,
    featherdoc::style_usage_hit_kind &kind,
    std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(content, index);
    if (!parse_json_patch_string(content, index, token, error_message)) {
        return false;
    }

    if (token == "paragraph") {
        kind = featherdoc::style_usage_hit_kind::paragraph;
        return true;
    }
    if (token == "run") {
        kind = featherdoc::style_usage_hit_kind::run;
        return true;
    }
    if (token == "table") {
        kind = featherdoc::style_usage_hit_kind::table;
        return true;
    }

    error_message = "JSON style usage hit member 'kind' must be 'paragraph', "
                    "'run', or 'table'";
    return false;
}

auto parse_style_usage_hit_section_index(
    std::string_view content, std::size_t &index,
    std::optional<std::size_t> &section_index,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (content.substr(index, 4U) == "null") {
        section_index.reset();
        index += 4U;
        return true;
    }

    std::size_t parsed_index = 0U;
    if (!parse_json_patch_index_value(content, index, parsed_index,
                                      "section_index", error_message)) {
        return false;
    }
    section_index = parsed_index;
    return true;
}

auto parse_style_usage_hit_object(
    std::string_view content, std::size_t &index,
    featherdoc::style_usage_hit &hit, std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected style usage hit object",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_part = false;
    bool saw_entry_name = false;
    bool saw_ordinal = false;
    bool saw_kind = false;
    bool saw_node_ordinal = false;

    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor rollback file", index,
                                       "style usage hit object must not be empty",
                                       error_message);
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name, error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "style refactor rollback file", index,
                "expected ':' after style usage hit member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "part") {
            saw_part = true;
            if (!parse_style_usage_part_kind_value(content, index, hit.part,
                                                   error_message)) {
                return false;
            }
        } else if (member_name == "entry_name") {
            saw_entry_name = true;
            if (!parse_json_patch_string(content, index, hit.entry_name,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "section_index") {
            if (!parse_style_usage_hit_section_index(content, index,
                                                     hit.section_index,
                                                     error_message)) {
                return false;
            }
        } else if (member_name == "ordinal") {
            saw_ordinal = true;
            if (!parse_json_patch_index_value(content, index, hit.ordinal,
                                              "ordinal", error_message)) {
                return false;
            }
        } else if (member_name == "node_ordinal") {
            saw_node_ordinal = true;
            if (!parse_json_patch_index_value(content, index, hit.node_ordinal,
                                              "node_ordinal", error_message)) {
                return false;
            }
        } else if (member_name == "kind") {
            saw_kind = true;
            if (!parse_style_usage_hit_kind_value(content, index, hit.kind,
                                                  error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, '}',
                "expected ',' or '}' after style usage hit member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_part || !saw_entry_name || !saw_ordinal || !saw_kind) {
        error_message = "JSON style usage hit must contain 'part', 'entry_name', "
                        "'ordinal', and 'kind'";
        return false;
    }
    if (!saw_node_ordinal) {
        hit.node_ordinal = hit.ordinal;
    }

    return true;
}

auto parse_style_usage_hits_array(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::style_usage_hit> &hits,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected style usage hits array",
                                       error_message);
    }

    hits.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        auto hit = featherdoc::style_usage_hit{};
        if (!parse_style_usage_hit_object(content, index, hit, error_message)) {
            return false;
        }
        hits.push_back(std::move(hit));

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, ']',
                "expected ',' or ']' after style usage hit", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("style refactor rollback file", index,
                                   "unterminated style usage hits array",
                                   error_message);
}

auto parse_style_usage_summary_object(
    std::string_view content, std::size_t &index,
    featherdoc::style_usage_summary &usage,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected style usage object",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_style_id = false;
    bool saw_hits = false;
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor rollback file", index,
                                       "style usage object must not be empty",
                                       error_message);
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name, error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "style refactor rollback file", index,
                "expected ':' after style usage object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "style_id") {
            saw_style_id = true;
            if (!parse_json_patch_string(content, index, usage.style_id,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "paragraph_count") {
            if (!parse_json_patch_index_value(content, index,
                                              usage.paragraph_count,
                                              "paragraph_count",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "run_count") {
            if (!parse_json_patch_index_value(content, index, usage.run_count,
                                              "run_count", error_message)) {
                return false;
            }
        } else if (member_name == "table_count") {
            if (!parse_json_patch_index_value(content, index, usage.table_count,
                                              "table_count", error_message)) {
                return false;
            }
        } else if (member_name == "hits") {
            saw_hits = true;
            if (!parse_style_usage_hits_array(content, index, usage.hits,
                                              error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, '}',
                "expected ',' or '}' after style usage object member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_style_id || !saw_hits) {
        error_message = "JSON style usage object must contain 'style_id' and 'hits'";
        return false;
    }

    return true;
}

auto parse_style_refactor_rollback_entry(
    std::string_view content, std::size_t &index,
    featherdoc::style_refactor_rollback_entry &entry,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected rollback operation object",
                                       error_message);
    }

    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_action = false;
    bool saw_source = false;
    bool saw_target = false;
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor rollback file", index,
                                       "rollback operation object must not be empty",
                                       error_message);
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name, error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "style refactor rollback file", index,
                "expected ':' after rollback operation member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "action") {
            saw_action = true;
            if (!parse_style_refactor_plan_action(content, index, entry.action,
                                                  error_message)) {
                return false;
            }
        } else if (member_name == "source_style_id") {
            saw_source = true;
            if (!parse_json_patch_string(content, index, entry.source_style_id,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "target_style_id") {
            saw_target = true;
            if (!parse_json_patch_string(content, index, entry.target_style_id,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "automatic") {
            if (!parse_json_bool_value(content, index, entry.automatic,
                                       member_name, error_message)) {
                return false;
            }
        } else if (member_name == "restorable") {
            if (!parse_json_bool_value(content, index, entry.restorable,
                                       member_name, error_message)) {
                return false;
            }
        } else if (member_name == "note") {
            if (!parse_nullable_json_string_value(content, index, entry.note,
                                                  member_name, error_message)) {
                return false;
            }
        } else if (member_name == "source_style_xml") {
            if (!parse_nullable_json_string_value(content, index,
                                                  entry.source_style_xml,
                                                  member_name, error_message)) {
                return false;
            }
        } else if (member_name == "source_usage") {
            skip_json_patch_whitespace(content, index);
            if (content.substr(index, 4U) == "null") {
                if (!consume_json_null_value(content, index, member_name,
                                             error_message)) {
                    return false;
                }
                entry.source_usage.reset();
            } else {
                auto usage = featherdoc::style_usage_summary{};
                if (!parse_style_usage_summary_object(content, index, usage,
                                                      error_message)) {
                    return false;
                }
                entry.source_usage = std::move(usage);
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, '}',
                "expected ',' or '}' after rollback operation member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    if (!saw_action || !saw_source || !saw_target) {
        error_message = "JSON style refactor rollback operation must contain "
                        "'action', 'source_style_id', and 'target_style_id'";
        return false;
    }

    return true;
}

auto parse_style_refactor_rollback_entries(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::style_refactor_rollback_entry> &entries,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected rollback_operations array",
                                       error_message);
    }

    entries.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        auto entry = featherdoc::style_refactor_rollback_entry{};
        if (!parse_style_refactor_rollback_entry(content, index, entry,
                                                 error_message)) {
            return false;
        }
        entries.push_back(std::move(entry));

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, ']',
                "expected ',' or ']' after rollback operation", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("style refactor rollback file", index,
                                   "unterminated rollback_operations array",
                                   error_message);
}

auto read_style_refactor_rollback_content(const path_type &rollback_path,
                                          std::string &content,
                                          std::string &error_message) -> bool {
    std::ifstream stream(rollback_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to read style refactor rollback file: " +
                        rollback_path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    return true;
}

auto read_style_refactor_rollback_file(
    const path_type &rollback_path, const std::vector<std::size_t> &entry_indexes,
    const std::vector<std::string> &source_style_ids,
    const std::vector<std::string> &target_style_ids,
    std::vector<featherdoc::style_refactor_rollback_entry> &entries,
    std::string &error_message) -> bool {
    std::string content;
    if (!read_style_refactor_rollback_content(rollback_path, content,
                                              error_message)) {
        return false;
    }

    std::size_t index = 0U;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("style refactor rollback file", index,
                                       "expected root object", error_message);
    }

    auto parsed_entries = std::vector<featherdoc::style_refactor_rollback_entry>{};
    ++index;
    skip_json_patch_whitespace(content, index);
    bool saw_rollback_operations = false;
    if (index < content.size() && content[index] == '}') {
        return report_json_input_error("style refactor rollback file", index,
                                       "root object must not be empty",
                                       error_message);
    }

    while (index < content.size()) {
        std::string member_name;
        if (!parse_json_patch_string(content, index, member_name, error_message)) {
            return false;
        }
        skip_json_patch_whitespace(content, index);
        if (index >= content.size() || content[index] != ':') {
            return report_json_input_error(
                "style refactor rollback file", index,
                "expected ':' after root object member", error_message);
        }
        ++index;
        skip_json_patch_whitespace(content, index);

        if (member_name == "rollback_operations") {
            if (saw_rollback_operations) {
                error_message = "JSON style refactor rollback root member "
                                "'rollback_operations' must not be duplicated";
                return false;
            }
            saw_rollback_operations = true;
            if (!parse_style_refactor_rollback_entries(content, index,
                                                       parsed_entries,
                                                       error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }

        bool closed = false;
        if (!consume_style_refactor_rollback_separator(
                content, index, '}',
                "expected ',' or '}' after root object member", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            break;
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error("style refactor rollback file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }
    if (!saw_rollback_operations) {
        error_message = "JSON style refactor rollback file must contain a "
                        "'rollback_operations' array";
        return false;
    }

    const auto matches_style_selection = [&](const auto &entry) {
        if (!source_style_ids.empty() &&
            std::find(source_style_ids.begin(), source_style_ids.end(),
                      entry.source_style_id) == source_style_ids.end()) {
            return false;
        }
        if (!target_style_ids.empty() &&
            std::find(target_style_ids.begin(), target_style_ids.end(),
                      entry.target_style_id) == target_style_ids.end()) {
            return false;
        }
        return true;
    };

    entries.clear();
    if (!entry_indexes.empty()) {
        for (const auto entry_index : entry_indexes) {
            if (entry_index >= parsed_entries.size()) {
                error_message = "style refactor rollback --entry index is out of range";
                return false;
            }
            if (parsed_entries[entry_index].action !=
                featherdoc::style_refactor_action::merge) {
                error_message = "style refactor rollback --entry must reference a "
                                "merge rollback operation";
                return false;
            }
            entries.push_back(std::move(parsed_entries[entry_index]));
        }
    } else {
        for (auto &entry : parsed_entries) {
            if (entry.action == featherdoc::style_refactor_action::merge &&
                matches_style_selection(entry)) {
                entries.push_back(std::move(entry));
            }
        }
    }

    if (entries.empty()) {
        if (!source_style_ids.empty() || !target_style_ids.empty()) {
            error_message = "JSON style refactor rollback file does not contain any "
                            "merge rollback operations matching restore selection";
        } else {
            error_message = "JSON style refactor rollback file does not contain any "
                            "merge rollback operations to restore";
        }
        return false;
    }

    return true;
}
} // namespace featherdoc_cli
