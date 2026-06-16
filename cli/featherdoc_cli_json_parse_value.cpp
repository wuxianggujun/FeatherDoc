#include "featherdoc_cli_json_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto skip_json_patch_value(std::string_view text, std::size_t &index,
                           std::string &error_message) -> bool;

auto parse_json_patch_number(std::string_view text, std::size_t &index,
                             std::string &value, std::string &error_message)
    -> bool {
    if (index >= text.size()) {
        return report_json_patch_error(index, "expected number", error_message);
    }

    const auto start_index = index;
    if (text[index] == '-') {
        ++index;
    }
    if (index >= text.size()) {
        return report_json_patch_error(start_index, "expected number digits",
                                       error_message);
    }

    if (text[index] == '0') {
        ++index;
    } else if (text[index] >= '1' && text[index] <= '9') {
        ++index;
        while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
            ++index;
        }
    } else {
        return report_json_patch_error(start_index, "expected number digits",
                                       error_message);
    }

    if (index < text.size() && text[index] == '.') {
        ++index;
        if (index >= text.size() || text[index] < '0' || text[index] > '9') {
            return report_json_patch_error(index,
                                           "expected digits after decimal point",
                                           error_message);
        }
        while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
            ++index;
        }
    }

    if (index < text.size() && (text[index] == 'e' || text[index] == 'E')) {
        ++index;
        if (index < text.size() && (text[index] == '+' || text[index] == '-')) {
            ++index;
        }
        if (index >= text.size() || text[index] < '0' || text[index] > '9') {
            return report_json_patch_error(index, "expected exponent digits",
                                           error_message);
        }
        while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
            ++index;
        }
    }

    value = std::string(text.substr(start_index, index - start_index));
    return true;
}

auto skip_json_patch_array(std::string_view text, std::size_t &index,
                           std::string &error_message) -> bool {
    if (index >= text.size() || text[index] != '[') {
        return report_json_patch_error(index, "expected array", error_message);
    }

    ++index;
    skip_json_patch_whitespace(text, index);
    if (index < text.size() && text[index] == ']') {
        ++index;
        return true;
    }

    while (index < text.size()) {
        if (!skip_json_patch_value(text, index, error_message)) {
            return false;
        }

        skip_json_patch_whitespace(text, index);
        if (index >= text.size()) {
            break;
        }
        if (text[index] == ',') {
            ++index;
            skip_json_patch_whitespace(text, index);
            continue;
        }
        if (text[index] == ']') {
            ++index;
            return true;
        }
        return report_json_patch_error(index,
                                       "expected ',' or ']' after array item",
                                       error_message);
    }

    return report_json_patch_error(index, "unterminated array", error_message);
}

auto skip_json_patch_object(std::string_view text, std::size_t &index,
                            std::string &error_message) -> bool {
    if (index >= text.size() || text[index] != '{') {
        return report_json_patch_error(index, "expected object", error_message);
    }

    ++index;
    skip_json_patch_whitespace(text, index);
    if (index < text.size() && text[index] == '}') {
        ++index;
        return true;
    }

    while (index < text.size()) {
        std::string key;
        if (!parse_json_patch_string(text, index, key, error_message)) {
            return false;
        }

        skip_json_patch_whitespace(text, index);
        if (index >= text.size() || text[index] != ':') {
            return report_json_patch_error(index,
                                           "expected ':' after object member",
                                           error_message);
        }

        ++index;
        if (!skip_json_patch_value(text, index, error_message)) {
            return false;
        }

        skip_json_patch_whitespace(text, index);
        if (index >= text.size()) {
            break;
        }
        if (text[index] == ',') {
            ++index;
            skip_json_patch_whitespace(text, index);
            continue;
        }
        if (text[index] == '}') {
            ++index;
            return true;
        }
        return report_json_patch_error(
            index, "expected ',' or '}' after object member", error_message);
    }

    return report_json_patch_error(index, "unterminated object", error_message);
}

auto skip_json_patch_value(std::string_view text, std::size_t &index,
                           std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    if (index >= text.size()) {
        return report_json_patch_error(index, "expected JSON value",
                                       error_message);
    }

    if (text[index] == '"') {
        std::string ignored;
        return parse_json_patch_string(text, index, ignored, error_message);
    }
    if (text[index] == '{') {
        return skip_json_patch_object(text, index, error_message);
    }
    if (text[index] == '[') {
        return skip_json_patch_array(text, index, error_message);
    }
    if (text[index] == 't') {
        if (text.substr(index, 4U) != "true") {
            return report_json_patch_error(index, "expected 'true'",
                                           error_message);
        }
        index += 4U;
        return true;
    }
    if (text[index] == 'f') {
        if (text.substr(index, 5U) != "false") {
            return report_json_patch_error(index, "expected 'false'",
                                           error_message);
        }
        index += 5U;
        return true;
    }
    if (text[index] == 'n') {
        if (text.substr(index, 4U) != "null") {
            return report_json_patch_error(index, "expected 'null'",
                                           error_message);
        }
        index += 4U;
        return true;
    }
    if (text[index] == '-' || (text[index] >= '0' && text[index] <= '9')) {
        std::string ignored;
        return parse_json_patch_number(text, index, ignored, error_message);
    }

    return report_json_patch_error(index, "unexpected JSON token",
                                   error_message);
}

} // namespace featherdoc_cli
