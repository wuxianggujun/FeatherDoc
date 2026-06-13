#pragma once

#include <featherdoc.hpp>

#include <string_view>
#include <vector>

namespace featherdoc_cli {

[[nodiscard]] auto run_inspect_bookmarks_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_replace_bookmark_paragraphs_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_remove_bookmark_block_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_replace_bookmark_text_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_fill_bookmarks_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

[[nodiscard]] auto run_replace_bookmark_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

} // namespace featherdoc_cli
