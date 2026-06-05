#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct semantic_diff_options {
    featherdoc::document_semantic_diff_options diff_options{};
    bool fail_on_diff = false;
    bool json_output = false;
};

[[nodiscard]] auto parse_semantic_diff_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    semantic_diff_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
