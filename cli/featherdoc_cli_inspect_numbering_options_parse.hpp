#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct inspect_numbering_options {
    std::optional<std::uint32_t> definition_id;
    std::optional<std::uint32_t> instance_id;
    bool json_output = false;
};

[[nodiscard]] auto parse_inspect_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_numbering_options &options, std::string &error_message) -> bool;

} // namespace featherdoc_cli
