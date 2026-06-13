#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace featherdoc_cli {

struct inspected_body_paragraph {
    std::size_t index = 0;
    std::size_t section_index = 0;
    std::optional<std::string> style_id;
    std::optional<std::uint32_t> numbering_id;
    std::optional<std::uint32_t> numbering_level;
    bool has_section_break = false;
    std::string text;
};

struct inspected_body_run {
    std::size_t index = 0;
    std::optional<std::string> style_id;
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<std::string> text_color;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<bool> underline;
    std::optional<double> font_size_points;
    std::optional<std::string> language;
    std::string text;
};

void inspect_paragraphs(
    const std::vector<inspected_body_paragraph> &paragraphs,
    bool json_output);

void inspect_paragraph(const inspected_body_paragraph &paragraph,
                       bool json_output);

void inspect_runs(std::size_t paragraph_index,
                  const std::vector<inspected_body_run> &runs,
                  bool json_output);

void inspect_run(std::size_t paragraph_index, const inspected_body_run &run,
                 bool json_output);

} // namespace featherdoc_cli