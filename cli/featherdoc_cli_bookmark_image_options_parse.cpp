#include "featherdoc_cli_bookmark_text_options_parse.hpp"

#include <string>

namespace featherdoc_cli {
namespace {

auto rewrite_error_command_name(std::string &error_message,
                                std::string_view old_command_name,
                                std::string_view new_command_name) -> void {
    if (error_message.rfind(old_command_name, 0) != 0) {
        return;
    }

    error_message.replace(0, old_command_name.size(),
                          std::string(new_command_name));
}

} // namespace

auto parse_bookmark_image_command_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    std::string_view command_name, bool allow_floating_layout,
    append_image_options &options, std::string &error_message) -> bool {
    if (!parse_append_image_options(arguments, start_index, options,
                                    error_message)) {
        rewrite_error_command_name(error_message, "append-image", command_name);
        return false;
    }

    if (!allow_floating_layout && options.floating) {
        error_message = std::string(command_name) +
                        " does not accept floating layout options; use "
                        "replace-bookmark-floating-image";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
