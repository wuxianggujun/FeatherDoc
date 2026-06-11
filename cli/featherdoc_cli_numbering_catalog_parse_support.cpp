#include "featherdoc_cli_numbering_catalog_parse_support.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_numbering_catalog_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_json_numbering_catalog_uint32(
    std::string_view content, std::size_t &index, std::uint32_t &value,
    std::string_view member_name, std::string &error_message) -> bool {
    std::string token;
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("numbering catalog file", index,
                                       "expected unsigned integer value",
                                       error_message);
    }

    if (content[index] == '"') {
        if (!parse_json_patch_string(content, index, token, error_message)) {
            return false;
        }
    } else if (content[index] >= '0' && content[index] <= '9') {
        if (!parse_json_patch_number(content, index, token, error_message)) {
            return false;
        }
    } else {
        return report_json_input_error("numbering catalog file", index,
                                       "expected unsigned integer value",
                                       error_message);
    }

    if (!parse_uint32(token, value)) {
        error_message = "JSON numbering catalog member '" +
                        std::string(member_name) +
                        "' must be an unsigned integer";
        return false;
    }

    return true;
}

auto parse_json_numbering_catalog_optional_uint32(
    std::string_view content, std::size_t &index,
    std::optional<std::uint32_t> &value, std::string_view member_name,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index < content.size() && content.substr(index, 4U) == "null") {
        index += 4U;
        value.reset();
        return true;
    }

    std::uint32_t parsed_value = 0U;
    if (!parse_json_numbering_catalog_uint32(content, index, parsed_value,
                                             member_name, error_message)) {
        return false;
    }
    value = parsed_value;
    return true;
}

auto consume_json_numbering_catalog_separator(
    std::string_view content, std::size_t &index, char closing_character,
    std::string_view after_item_detail, bool &closed,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error("numbering catalog file", index,
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

    return report_json_input_error("numbering catalog file", index,
                                   after_item_detail, error_message);
}

} // namespace featherdoc_cli
