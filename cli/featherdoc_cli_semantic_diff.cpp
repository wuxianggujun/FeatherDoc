#include "featherdoc_cli_semantic_diff.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_semantic_diff_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command, "semantic-diff expects left and right input paths",
                          json_output);
        return 2;
    }

    semantic_diff_options options;
    std::string error_message;
    if (!parse_semantic_diff_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    featherdoc::Document right_doc;
    if (!open_document(path_type(std::string(arguments[2])), right_doc, command,
                       options.json_output)) {
        return 1;
    }

    const auto result = doc.compare_semantic(right_doc, options.diff_options);
    if (!result.has_value()) {
        report_document_error(command, "compare", doc.last_error(),
                              options.json_output);
        return 1;
    }

    output_semantic_diff_result(*result, options);
    return options.fail_on_diff && result->different() ? 1 : 0;
}

} // namespace featherdoc_cli
