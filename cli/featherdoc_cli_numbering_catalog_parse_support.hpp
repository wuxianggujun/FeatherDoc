#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {

[[nodiscard]] auto consume_json_numbering_catalog_separator(
    std::string_view content, std::size_t &index, char closing_character,
    std::string_view after_item_detail, bool &closed,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
