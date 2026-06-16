#include "featherdoc_cli_run_recipe_parse_support.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <fstream>
#include <iterator>

namespace featherdoc_cli {

auto read_run_recipe_utf8_text_file(
    const std::filesystem::path &path, std::string_view label,
    std::string &content, std::string &error_message) -> bool {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error_message = "failed to open " + std::string(label) + ": " +
                        path.string();
        return false;
    }

    content.assign(std::istreambuf_iterator<char>(stream),
                   std::istreambuf_iterator<char>());
    if (stream.bad()) {
        error_message = "failed to read " + std::string(label) + ": " +
                        path.string();
        return false;
    }

    return true;
}

auto consume_run_recipe_json_object_separator(
    std::string_view content, std::size_t &index, std::string &error_message,
    bool &done) -> bool {
    skip_json_patch_whitespace(content, index);
    if (index >= content.size()) {
        return report_json_patch_error(index, "unexpected end of object",
                                       error_message);
    }

    if (content[index] == ',') {
        ++index;
        skip_json_patch_whitespace(content, index);
        done = false;
        return true;
    }

    if (content[index] == '}') {
        ++index;
        done = true;
        return true;
    }

    return report_json_patch_error(index,
                                   "expected ',' or '}' after object member",
                                   error_message);
}

} // namespace featherdoc_cli
