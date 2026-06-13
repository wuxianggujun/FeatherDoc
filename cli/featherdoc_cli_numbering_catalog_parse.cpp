#include "featherdoc_cli_numbering_catalog_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_definition_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse_support.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

auto read_numbering_catalog_file(const path_type &catalog_path,
                                 featherdoc::numbering_catalog &catalog,
                                 std::string &error_message) -> bool {
    std::string content;
    std::size_t index = 0U;
    if (!read_template_table_json_content(catalog_path, content, index,
                                          error_message)) {
        if (error_message.rfind("failed to read JSON patch file:", 0U) == 0U) {
            error_message.replace(0U,
                                  std::string("failed to read JSON patch file:").size(),
                                  "failed to read numbering catalog file:");
        }
        return false;
    }

    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '{') {
        return report_json_input_error("numbering catalog file", index,
                                       "root must be an object", error_message);
    }

    bool saw_definition_count = false;
    bool saw_instance_count = false;
    bool saw_definitions = false;

    catalog.definitions.clear();
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
                    "expected ':' after root object member", error_message);
            }

            ++index;
            if (member_name == "definition_count") {
                if (saw_definition_count) {
                    error_message = "JSON numbering catalog root member "
                                    "'definition_count' must not be duplicated";
                    return false;
                }
                saw_definition_count = true;
                std::uint32_t ignored_count = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, ignored_count, member_name, error_message)) {
                    return false;
                }
            } else if (member_name == "instance_count") {
                if (saw_instance_count) {
                    error_message = "JSON numbering catalog root member "
                                    "'instance_count' must not be duplicated";
                    return false;
                }
                saw_instance_count = true;
                std::uint32_t ignored_count = 0U;
                if (!parse_json_numbering_catalog_uint32(
                        content, index, ignored_count, member_name, error_message)) {
                    return false;
                }
            } else if (member_name == "definitions") {
                if (saw_definitions) {
                    error_message = "JSON numbering catalog root member 'definitions' "
                                    "must not be duplicated";
                    return false;
                }
                saw_definitions = true;
                if (!parse_numbering_catalog_definitions(
                        content, index, catalog.definitions, error_message)) {
                    return false;
                }
            } else {
                return report_json_input_error("numbering catalog file", index,
                                               "unknown root member", error_message);
            }

            bool closed = false;
            if (!consume_json_numbering_catalog_separator(
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
        return report_json_input_error("numbering catalog file", index,
                                       "unexpected trailing content after root object",
                                       error_message);
    }

    if (!saw_definitions) {
        error_message =
            "JSON numbering catalog file must contain a 'definitions' array";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
