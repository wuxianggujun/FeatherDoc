#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto read_review_mutation_plan_build_request_content(
    const std::filesystem::path &request_path, std::string &content,
    std::string &error_message) -> bool;

[[nodiscard]] auto consume_review_mutation_plan_build_request_separator(
    std::string_view content, std::size_t &index, char close_char,
    std::string_view error_detail, bool &closed, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_review_mutation_plan_build_request_bool_value(
    std::string_view content, std::size_t &index, bool &value,
    std::string_view member_name, std::string &error_message) -> bool;

} // namespace featherdoc_cli
