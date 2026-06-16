#include "featherdoc_cli_image_append_options_parse_support.hpp"

namespace featherdoc_cli {

auto parse_append_image_floating_flag_option(
    std::string_view argument, bool &floating_flag_seen,
    append_image_options &options, std::string &error_message)
    -> option_parse_result {
    if (argument != "--floating") {
        return option_parse_result::not_matched;
    }

    if (floating_flag_seen) {
        error_message = "duplicate --floating option";
        return option_parse_result::error;
    }

    options.floating = true;
    floating_flag_seen = true;
    return option_parse_result::matched;
}

} // namespace featherdoc_cli
