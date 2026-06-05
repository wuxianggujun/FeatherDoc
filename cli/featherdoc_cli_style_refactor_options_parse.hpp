#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <featherdoc.hpp>

namespace featherdoc_cli {

struct style_refactor_plan_options {
    std::vector<featherdoc::style_refactor_request> requests{};
    std::optional<std::filesystem::path> output_plan_path;
    bool json_output = false;
};

struct style_merge_suggestion_options {
    std::optional<std::filesystem::path> output_plan_path;
    std::optional<std::uint32_t> min_confidence;
    std::optional<std::string> confidence_profile;
    std::vector<std::string> source_style_ids{};
    std::vector<std::string> target_style_ids{};
    bool fail_on_suggestion = false;
    bool json_output = false;
};

struct style_refactor_apply_options {
    std::vector<featherdoc::style_refactor_request> requests{};
    std::optional<std::filesystem::path> plan_file_path;
    std::optional<std::filesystem::path> rollback_plan_path;
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct style_merge_restore_options {
    std::optional<std::filesystem::path> rollback_plan_path;
    std::vector<std::size_t> entry_indexes;
    std::vector<std::string> source_style_ids;
    std::vector<std::string> target_style_ids;
    std::optional<std::filesystem::path> output_path;
    bool dry_run = false;
    bool json_output = false;
};

[[nodiscard]] auto parse_style_refactor_plan_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    style_refactor_plan_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_style_merge_suggestion_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    style_merge_suggestion_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_style_refactor_apply_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    style_refactor_apply_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_style_merge_restore_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    style_merge_restore_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
