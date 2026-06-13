#include "featherdoc_cli_review_mutation_plan_build_request_parse_support.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <fstream>
#include <iterator>
#include <string>

namespace featherdoc_cli {

auto read_review_mutation_plan_build_request_content(
    const std::filesystem::path &request_path, std::string &content,
    std::string &error_message) -> bool {
    std::ifstream stream(request_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to read review mutation plan build request file: " +
                        request_path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    return true;
}

auto consume_review_mutation_plan_build_request_separator(
    std::string_view content, std::size_t &index, char close_char,
    std::string_view error_detail, bool &closed, std::string &error_message)
    -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_input_error(
            "review mutation plan build request file", index,
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

    return report_json_input_error("review mutation plan build request file",
                                   index, error_detail, error_message);
}

auto parse_review_mutation_plan_build_request_bool_value(
    std::string_view content, std::size_t &index, bool &value,
    std::string_view member_name, std::string &error_message) -> bool {
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

    error_message =
        "JSON review mutation plan build request operation member '" +
        std::string(member_name) + "' must be a boolean";
    return false;
}

} // namespace featherdoc_cli
