#pragma once

#include "featherdoc_cli_bookmark_text_options_parse.hpp"

namespace featherdoc_cli {

[[nodiscard]] auto parse_set_content_control_form_state_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_content_control_form_state_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
