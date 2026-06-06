#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct table_row_inspection_summary {
    std::size_t row_index = 0U;
    std::size_t cell_count = 0U;
    std::optional<std::uint32_t> height_twips;
    std::optional<featherdoc::row_height_rule> height_rule;
    bool cant_split = false;
    bool repeats_header = false;
    std::vector<std::string> cell_texts;
};

[[nodiscard]] auto make_table_row_summary(featherdoc::TableRow &row,
                                          std::size_t row_index)
    -> table_row_inspection_summary;

[[nodiscard]] auto collect_table_row_summaries(featherdoc::Table &table)
    -> std::vector<table_row_inspection_summary>;

} // namespace featherdoc_cli
