#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class numbering_catalog_lint_issue_kind {
    empty_definition_name,
    duplicate_definition_name,
    empty_levels,
    duplicate_level,
    invalid_level,
    invalid_start,
    empty_text_pattern,
    duplicate_instance_id,
    duplicate_override_level,
    invalid_override_level,
    invalid_override_start,
    invalid_override_definition,
};

struct numbering_catalog_lint_issue {
    numbering_catalog_lint_issue_kind kind =
        numbering_catalog_lint_issue_kind::empty_definition_name;
    std::size_t definition_index{};
    std::string definition_name;
    std::optional<std::size_t> instance_index;
    std::optional<std::uint32_t> instance_id;
    std::optional<std::size_t> level_index;
    std::optional<std::size_t> override_index;
    std::optional<std::uint32_t> level;
    std::string detail;
};

struct numbering_catalog_lint_result {
    std::size_t definition_count{};
    std::size_t instance_count{};
    std::size_t level_count{};
    std::size_t override_count{};
    std::vector<numbering_catalog_lint_issue> issues;

    [[nodiscard]] bool clean() const noexcept {
        return this->issues.empty();
    }
};

[[nodiscard]] auto numbering_catalog_lint_issue_name(
    numbering_catalog_lint_issue_kind kind) -> std::string_view;
[[nodiscard]] auto lint_numbering_catalog(
    const featherdoc::numbering_catalog &catalog) -> numbering_catalog_lint_result;

} // namespace featherdoc_cli