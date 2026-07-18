#pragma once

#include <featherdoc/detail/path.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] inline auto path_from_cli_utf8(std::string_view text)
    -> std::filesystem::path {
    return featherdoc::detail::path_from_utf8(text);
}

[[nodiscard]] inline auto path_to_cli_utf8(const std::filesystem::path &path)
    -> std::string {
    return featherdoc::detail::path_to_utf8(path);
}

[[nodiscard]] auto parse_cli_path_utf8(std::string_view text,
                                       std::string_view label,
                                       std::filesystem::path &path,
                                       std::string &error_message) -> bool;

[[nodiscard]] auto quote_cli_argument(std::string_view value) -> std::string;
[[nodiscard]] auto yes_no(bool value) -> const char *;
[[nodiscard]] auto json_bool(bool value) -> const char *;
[[nodiscard]] auto format_paragraph_text(std::string_view text) -> std::string;
[[nodiscard]] auto
optional_display_value(const std::optional<std::string> &value) -> std::string;
[[nodiscard]] auto
optional_size_display_value(const std::optional<std::size_t> &value)
    -> std::string;
[[nodiscard]] auto strip_utf8_bom(std::string text) -> std::string;
[[nodiscard]] auto lower_ascii_copy(std::string value) -> std::string;
[[nodiscard]] auto is_docx_path(const std::filesystem::path &path) -> bool;
[[nodiscard]] auto is_word_temporary_path(const std::filesystem::path &path)
    -> bool;

} // namespace featherdoc_cli
