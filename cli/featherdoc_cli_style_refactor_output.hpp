#pragma once

#include "featherdoc_cli_command_support.hpp"

#include <featherdoc.hpp>

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

void write_json_style_refactor_apply_result_fields(
    std::ostream &stream,
    const featherdoc::style_refactor_apply_result &result);

void inspect_style_refactor_plan(
    const featherdoc::style_refactor_plan &plan, bool json_output,
    std::string_view command_name = "plan-style-refactor",
    std::optional<bool> fail_on_suggestion = std::nullopt);

void inspect_style_refactor_apply_result(
    const featherdoc::style_refactor_apply_result &result, bool json_output);

[[nodiscard]] auto write_style_refactor_plan_file(
    const path_type &output_path, const featherdoc::style_refactor_plan &plan,
    std::string &error_message,
    std::string_view command_name = "plan-style-refactor") -> bool;

[[nodiscard]] auto write_style_refactor_rollback_plan_file(
    const path_type &output_path,
    const featherdoc::style_refactor_apply_result &result,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli