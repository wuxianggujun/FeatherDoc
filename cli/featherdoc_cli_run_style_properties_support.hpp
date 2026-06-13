#pragma once

#include "featherdoc_cli_command_support.hpp"

#include <featherdoc.hpp>

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct run_properties_summary {
    std::optional<std::string> font_family;
    std::optional<std::string> east_asia_font_family;
    std::optional<std::string> language;
    std::optional<std::string> east_asia_language;
    std::optional<std::string> bidi_language;
    std::optional<bool> rtl;
    std::optional<bool> paragraph_bidi;
};

struct paragraph_style_properties_summary {
    std::optional<std::string> based_on;
    std::optional<std::string> next_style;
    std::optional<std::uint32_t> outline_level;
};

struct materialized_style_run_property_summary {
    std::string field;
    std::string source_style_id;
};

[[nodiscard]] auto open_document_for_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc, bool json_output) -> bool;

void write_json_run_properties_summary(std::ostream &stream,
                                       const run_properties_summary &summary);

void print_run_properties_summary(std::ostream &stream,
                                  const run_properties_summary &summary);

void write_json_paragraph_style_properties_summary(
    std::ostream &stream, const paragraph_style_properties_summary &summary);

void print_paragraph_style_properties_summary(
    std::ostream &stream, const paragraph_style_properties_summary &summary);

void write_json_cleared_fields(std::ostream &stream,
                               const std::vector<std::string> &cleared_fields);

[[nodiscard]] auto collect_materialized_style_run_properties(
    std::string_view style_id,
    const featherdoc::resolved_style_properties_summary &resolved)
    -> std::vector<materialized_style_run_property_summary>;

void write_json_materialized_style_run_properties(
    std::ostream &stream,
    const std::vector<materialized_style_run_property_summary>
        &materialized_properties);

[[nodiscard]] auto resolve_style_properties_for_command(
    std::string_view command, featherdoc::Document &doc,
    const std::string &style_id, bool json_output)
    -> std::optional<featherdoc::resolved_style_properties_summary>;

} // namespace featherdoc_cli
