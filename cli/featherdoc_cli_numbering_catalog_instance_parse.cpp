#include "featherdoc_cli_numbering_catalog_instance_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_level_override_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse_support.hpp"

#include <utility>

namespace featherdoc_cli {
namespace {

auto parse_numbering_catalog_instance(
    std::string_view content, std::size_t &index,
    featherdoc::numbering_instance_summary &instance,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog file", index,
                                       "numbering instance must be an object",
                                       error_message);
    }

    bool saw_instance_id = false;
    bool saw_level_overrides = false;

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
                return report_json_input_error(
                    "numbering catalog file", index,
                    "expected ':' after numbering instance member", error_message);
            }

            ++index;
            if (member_name == "instance_id") {
                if (saw_instance_id) {
                    error_message = "JSON numbering catalog instance member "
                                    "'instance_id' must not be duplicated";
                    return false;
                }
                saw_instance_id = true;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, instance.instance_id, member_name,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "level_overrides") {
                if (saw_level_overrides) {
                    error_message = "JSON numbering catalog instance member "
                                    "'level_overrides' must not be duplicated";
                    return false;
                }
                saw_level_overrides = true;
                if (!parse_numbering_catalog_level_overrides(
                        content, index, instance.level_overrides, error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog file", index,
                                               "unknown numbering instance member",
                                               error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_separator(
                    content, index, '}',
                    "expected ',' or '}' after numbering instance member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    if (!saw_level_overrides) {
        error_message =
            "JSON numbering catalog instance must contain 'level_overrides'";
        return false;
    }

    return true;
}

} // namespace

auto parse_numbering_catalog_instances(
    std::string_view content, std::size_t &index,
    std::vector<featherdoc::numbering_instance_summary> &instances,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error("numbering catalog file", index,
                                       "expected instances array", error_message);
    }

    instances.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        featherdoc::numbering_instance_summary instance;
        if (!parse_numbering_catalog_instance(content, index, instance,
                                              error_message)) {
            return false;
        }
        instances.push_back(std::move(instance));

        bool closed = false;
        if (!consume_json_numbering_catalog_separator(
                content, index, ']',
                "expected ',' or ']' after numbering instance", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error("numbering catalog file", index,
                                   "unterminated instances array", error_message);
}

} // namespace featherdoc_cli
