#include "featherdoc_cli_numbering_catalog_patch_override_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_override_entry_parse.hpp"
#include "featherdoc_cli_numbering_catalog_patch_parse_support.hpp"

namespace featherdoc_cli {

auto parse_numbering_catalog_override_patch_array(
    std::string_view content, std::size_t &index,
    std::vector<numbering_catalog_override_patch> &patches,
    std::string_view member_name, bool allow_override_values,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size() || content[index] != '[') {
        return report_json_input_error(
            "numbering catalog patch file", index,
            "expected " + std::string(member_name) + " array", error_message);
    }

    patches.clear();
    ++index;
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content[index] == ']') {
        ++index;
        return true;
    }

    while (index < content.size()) {
        numbering_catalog_override_patch patch;
        if (!parse_numbering_catalog_override_patch(
                content, index, patch, allow_override_values, error_message)) {
            return false;
        }
        patches.push_back(std::move(patch));

        bool closed = false;
        if (!consume_json_numbering_catalog_patch_separator(
                content, index, ']',
                "expected ',' or ']' after override patch entry", closed,
                error_message)) {
            return false;
        }
        if (closed) {
            return true;
        }
    }

    return report_json_input_error(
        "numbering catalog patch file", index,
        "unterminated " + std::string(member_name) + " array", error_message);
}

} // namespace featherdoc_cli
