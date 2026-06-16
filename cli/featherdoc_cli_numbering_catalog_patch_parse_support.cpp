#include "featherdoc_cli_numbering_catalog_patch_parse_support.hpp"

#include "featherdoc_cli_json_parse.hpp"

namespace featherdoc_cli {

auto consume_json_numbering_catalog_patch_separator(
    std::string_view content, std::size_t &index, char closing_character,
    std::string_view after_item_detail, bool &closed,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("numbering catalog patch file", index,
                                       "unterminated JSON container",
                                       error_message);
    }
    if (content[index] == ',') {
        ++index;
        skip_json_patch_whitespace(content, index);
        closed = false;
        return true;
    }
    if (content[index] == closing_character) {
        ++index;
        closed = true;
        return true;
    }

    return report_json_input_error("numbering catalog patch file", index,
                                   after_item_detail, error_message);
}

} // namespace featherdoc_cli
