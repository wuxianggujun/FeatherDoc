#pragma once

#include <featherdoc.hpp>

#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto run_append_text_comment_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

auto run_set_text_comment_range_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

auto run_append_comment_reply_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

auto run_set_comment_metadata_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

auto run_set_comment_resolved_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

auto run_edit_comment_thread_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int;

} // namespace featherdoc_cli
