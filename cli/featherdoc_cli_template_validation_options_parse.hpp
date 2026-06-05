#pragma once

#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct validate_template_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::vector<featherdoc::template_slot_requirement> requirements;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct validate_template_schema_target_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::vector<featherdoc::template_slot_requirement> requirements;
    bool has_kind = false;
};

struct validate_template_schema_options {
    std::vector<validate_template_schema_target_options> targets;
    std::vector<std::filesystem::path> schema_files;
    bool json_output = false;
};

[[nodiscard]] auto parse_validate_template_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    validate_template_options &options, std::string &error_message) -> bool;

[[nodiscard]] auto parse_validate_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    validate_template_schema_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
