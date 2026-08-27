#pragma once

#include <string_view>
#include <vector>

namespace featherdoc {
class Document;
}

namespace featherdoc_cli {

[[nodiscard]] auto is_package_command(std::string_view command) -> bool;
[[nodiscard]] auto run_package_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &document) -> int;

} // namespace featherdoc_cli
