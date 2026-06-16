#pragma once

#include "featherdoc_cli_template_schema_patch_parse_update_slot_state.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli::detail {

[[nodiscard]] auto parse_update_slot_string_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::string> &value, std::string &error_message) -> bool;

[[nodiscard]] auto parse_update_slot_index_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::size_t> &value, std::string &error_message) -> bool;

[[nodiscard]] auto parse_update_slot_selector_index_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<std::size_t> &value, std::string &error_message) -> bool;

[[nodiscard]] auto parse_update_slot_bool_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<bool> &value, std::string &error_message) -> bool;

[[nodiscard]] auto parse_update_slot_selector_bool_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    std::optional<bool> &value, std::string &error_message) -> bool;

[[nodiscard]] auto parse_update_slot_kind_member(
    std::string_view content, std::size_t &index,
    template_schema_patch_update_slot_parse_state &state,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_update_slot_reference_kind_member(
    std::string_view content, std::size_t &index, std::string_view member_name,
    template_schema_patch_update_slot_parse_state &state,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_update_slot_slot_kind_member(
    std::string_view content, std::size_t &index,
    template_schema_patch_update_slot_parse_state &state,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli::detail
