#pragma once

#include "featherdoc_cli_style_numbering_audit.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace featherdoc_cli {

struct style_numbering_repair_target {
    std::optional<std::uint32_t> definition_id;
    std::optional<std::string> definition_name;
    std::optional<std::uint32_t> level;
};

[[nodiscard]] auto make_style_numbering_repair_target(
    const featherdoc::style_summary::numbering_summary &numbering)
    -> style_numbering_repair_target;
[[nodiscard]] auto make_style_numbering_repair_target(
    const featherdoc::numbering_definition_summary &definition,
    std::uint32_t level) -> style_numbering_repair_target;
[[nodiscard]] auto build_style_numbering_repair_suggestion(
    style_numbering_audit_issue_kind kind,
    const featherdoc::style_summary &style,
    const style_numbering_repair_target *target = nullptr)
    -> style_numbering_repair_suggestion;

} // namespace featherdoc_cli
