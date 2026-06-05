#pragma once

#include "featherdoc_cli_template_schema_model.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {

[[nodiscard]] auto read_template_schema_patch_file(
    const std::filesystem::path &patch_path,
    template_schema_patch_document &patch, std::string &error_message) -> bool;

} // namespace featherdoc_cli