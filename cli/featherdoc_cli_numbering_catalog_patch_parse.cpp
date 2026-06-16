#include "featherdoc_cli_numbering_catalog_patch_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_level_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_override_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_parse_support.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto read_numbering_catalog_patch_file(
    const path_type &patch_path, numbering_catalog_patch_document &patch,
    std::string &error_message) -> bool {
    std::string content;
    std::size_t index = 0U;
    if (!read_template_table_json_content(patch_path, content, index,
                                          error_message)) {
        if (error_message.rfind("failed to read JSON patch file:", 0U) == 0U) {
            error_message.replace(0U,
                                  std::string("failed to read JSON patch file:").size(),
                                  "failed to read numbering catalog patch file:");
        }
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog patch file", index,
                                       "root must be an object", error_message);
    }

    patch = {};
    bool saw_upsert_levels = false;
    bool saw_upsert_overrides = false;
    bool saw_remove_overrides = false;

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
                    "numbering catalog patch file", index,
                    "expected ':' after root object member", error_message);
            }

            ++index;
            if (member_name == "upsert_levels") {
                if (saw_upsert_levels) {
                    error_message = "JSON numbering catalog patch root member "
                                    "'upsert_levels' must not be duplicated";
                    return false;
                }
                saw_upsert_levels = true;
                if (!parse_numbering_catalog_level_patch_array(
                        content, index, patch.upsert_levels, member_name,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "upsert_overrides") {
                if (saw_upsert_overrides) {
                    error_message = "JSON numbering catalog patch root member "
                                    "'upsert_overrides' must not be duplicated";
                    return false;
                }
                saw_upsert_overrides = true;
                if (!parse_numbering_catalog_override_patch_array(
                        content, index, patch.upsert_overrides, member_name, true,
                        error_message)) {
                    return false;
                }
            } else if (member_name == "remove_overrides") {
                if (saw_remove_overrides) {
                    error_message = "JSON numbering catalog patch root member "
                                    "'remove_overrides' must not be duplicated";
                    return false;
                }
                saw_remove_overrides = true;
                if (!parse_numbering_catalog_override_patch_array(
                        content, index, patch.remove_overrides, member_name, false,
                        error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog patch file", index,
                                               "unknown root member", error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_patch_separator(
                    content, index, '}',
                    "expected ',' or '}' after root object member", closed,
                    error_message)) {
                return false;
            }
            if (closed) {
                break;
            }
        }
    }

    skip_json_patch_whitespace(content, index);
    if (index != content.size()) {
        return report_json_input_error("numbering catalog patch file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }

    return true;
}

} // namespace featherdoc_cli
