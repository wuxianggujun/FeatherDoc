#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto parse_template_schema_json_target(
    std::string_view content, std::size_t &index,
    exported_template_schema_target &target, std::string &error_message)
    -> bool;
[[nodiscard]] auto read_template_schema_file(
    const std::filesystem::path &schema_path,
    exported_template_schema_result &result, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
