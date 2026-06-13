#include "featherdoc_cli_bookmark_commands_detail.hpp"

#include "featherdoc_cli_bookmark_output.hpp"
#include "featherdoc_cli_bookmark_support.hpp"
#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_inspect_bookmarks_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-bookmarks expects an input path",
                          json_output);
        return 2;
    }

    inspect_bookmarks_options options;
    std::string error_message;
    if (!parse_inspect_bookmarks_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, "inspect", error_message,
                                 doc.last_error(), options.json_output);
        return 1;
    }

    if (options.bookmark_name.has_value()) {
        const auto bookmark = selected.part.find_bookmark(*options.bookmark_name);
        if (!bookmark.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        inspect_bookmark(selected, *bookmark, options.json_output);
        return 0;
    }

    const auto bookmarks = selected.part.list_bookmarks();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    inspect_bookmarks(selected, bookmarks, options.json_output);
    return 0;
}

} // namespace featherdoc_cli
