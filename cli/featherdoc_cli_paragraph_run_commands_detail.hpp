#pragma once

#include <featherdoc/fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct body_run_target {
    std::uint32_t paragraph_index = 0U;
    std::uint32_t run_index = 0U;
};

auto report_body_paragraph_run_failure(
    std::string_view command, std::string_view summary, std::string detail,
    bool json_output) -> bool;

[[nodiscard]] auto parse_body_paragraph_index(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::size_t argument_index, std::uint32_t &paragraph_index,
    bool json_output) -> bool;

[[nodiscard]] auto parse_body_run_target(
    std::string_view command, const std::vector<std::string_view> &arguments,
    std::string_view usage_message, body_run_target &target,
    bool json_output) -> bool;

[[nodiscard]] auto resolve_body_paragraph_for_command(
    std::string_view command, featherdoc::Document &doc,
    std::uint32_t paragraph_index, featherdoc::Paragraph &paragraph,
    bool json_output) -> bool;

[[nodiscard]] auto resolve_body_run_for_command(
    std::string_view command, featherdoc::Document &doc,
    const body_run_target &target, featherdoc::Run &run, bool json_output)
    -> bool;

} // namespace featherdoc_cli
