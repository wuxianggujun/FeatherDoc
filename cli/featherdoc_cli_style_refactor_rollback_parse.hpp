#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include <featherdoc.hpp>

namespace featherdoc_cli {

[[nodiscard]] auto read_style_refactor_rollback_file(
    const std::filesystem::path &rollback_path,
    const std::vector<std::size_t> &entry_indexes,
    const std::vector<std::string> &source_style_ids,
    const std::vector<std::string> &target_style_ids,
    std::vector<featherdoc::style_refactor_rollback_entry> &entries,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli