#include "featherdoc_cli_bookmark_commands.hpp"

#include "featherdoc_cli_bookmark_commands_detail.hpp"
#include "featherdoc_cli_bookmark_table_commands.hpp"
#include "featherdoc_cli_bookmark_visibility_commands.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto is_bookmark_command(std::string_view command) -> bool {
    return command == "inspect-bookmarks" ||
           command == "replace-bookmark-paragraphs" ||
           command == "replace-bookmark-table" ||
           command == "replace-bookmark-table-rows" ||
           command == "remove-bookmark-block" ||
           command == "replace-bookmark-text" ||
           command == "fill-bookmarks" ||
           is_bookmark_visibility_command(command) ||
           command == "replace-bookmark-image" ||
           command == "replace-bookmark-floating-image";
}

auto run_bookmark_command(std::string_view command,
                          const std::vector<std::string_view> &arguments,
                          featherdoc::Document &doc) -> int {
    if (command == "inspect-bookmarks") {
        return run_inspect_bookmarks_command(command, arguments, doc);
    }
    if (command == "replace-bookmark-paragraphs") {
        return run_replace_bookmark_paragraphs_command(command, arguments, doc);
    }
    if (command == "replace-bookmark-table" ||
        command == "replace-bookmark-table-rows") {
        return run_replace_bookmark_table_command(command, arguments, doc);
    }
    if (command == "remove-bookmark-block") {
        return run_remove_bookmark_block_command(command, arguments, doc);
    }
    if (command == "replace-bookmark-text") {
        return run_replace_bookmark_text_command(command, arguments, doc);
    }
    if (command == "fill-bookmarks") {
        return run_fill_bookmarks_command(command, arguments, doc);
    }
    if (is_bookmark_visibility_command(command)) {
        return run_bookmark_visibility_command(command, arguments, doc);
    }
    if (command == "replace-bookmark-image" ||
        command == "replace-bookmark-floating-image") {
        return run_replace_bookmark_image_command(command, arguments, doc);
    }

    print_parse_error(command, "unsupported bookmark command", has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
