#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto normalize_word_part_entry(std::string_view target)
    -> std::string;
[[nodiscard]] auto quote_cli_argument(std::string_view value) -> std::string;
[[nodiscard]] auto yes_no(bool value) -> const char *;
[[nodiscard]] auto json_bool(bool value) -> const char *;
[[nodiscard]] auto format_paragraph_text(std::string_view text) -> std::string;
[[nodiscard]] auto optional_display_value(
    const std::optional<std::string> &value) -> std::string;
[[nodiscard]] auto optional_size_display_value(
    const std::optional<std::size_t> &value) -> std::string;
[[nodiscard]] auto strip_utf8_bom(std::string text) -> std::string;
[[nodiscard]] auto lower_ascii_copy(std::string value) -> std::string;
[[nodiscard]] auto is_docx_path(const std::filesystem::path &path) -> bool;
[[nodiscard]] auto is_word_temporary_path(const std::filesystem::path &path)
    -> bool;

} // namespace featherdoc_cli
