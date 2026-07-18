#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto package_part_name_identity(std::string_view part_name)
    -> std::optional<std::string>;

[[nodiscard]] auto package_part_names_equivalent(std::string_view left,
                                                 std::string_view right)
    -> bool;

} // namespace featherdoc_cli
