#include "featherdoc_cli_review_comment_commands.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_review_comment_commands_detail.hpp"

namespace featherdoc_cli {

auto is_review_comment_command(std::string_view command) -> bool {
    return command == "append-paragraph-text-comment" ||
           command == "append-text-range-comment" ||
           command == "set-paragraph-text-comment-range" ||
           command == "set-text-range-comment-range" ||
           command == "append-comment-reply" ||
           command == "set-comment-metadata" ||
           command == "set-comment-resolved" ||
           command == "append-comment" ||
           command == "replace-comment" ||
           command == "remove-comment";
}

auto run_review_comment_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "append-paragraph-text-comment" ||
        command == "append-text-range-comment") {
        return run_append_text_comment_command(command, arguments, doc);
    }
    if (command == "set-paragraph-text-comment-range" ||
        command == "set-text-range-comment-range") {
        return run_set_text_comment_range_command(command, arguments, doc);
    }
    if (command == "append-comment-reply") {
        return run_append_comment_reply_command(command, arguments, doc);
    }
    if (command == "set-comment-metadata") {
        return run_set_comment_metadata_command(command, arguments, doc);
    }
    if (command == "set-comment-resolved") {
        return run_set_comment_resolved_command(command, arguments, doc);
    }
    if (command == "append-comment" || command == "replace-comment" ||
        command == "remove-comment") {
        return run_edit_comment_thread_command(command, arguments, doc);
    }

    print_parse_error(command, "unknown review comment command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
