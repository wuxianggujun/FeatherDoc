#pragma once

#include <featherdoc.hpp>

#include <iosfwd>
#include <optional>

namespace featherdoc_cli {

void write_json_style_summary(std::ostream &stream,
                              const featherdoc::style_summary &style);

void write_json_style_usage_summary(
    std::ostream &stream, const featherdoc::style_usage_summary &usage);

void print_style_summary(std::ostream &stream,
                         const featherdoc::style_summary &style);

void inspect_style(
    const featherdoc::style_summary &style,
    const std::optional<featherdoc::style_usage_summary> &usage,
    bool json_output);

void write_json_style_prune_plan(std::ostream &stream,
                                 const featherdoc::style_prune_plan &plan);

void print_style_prune_plan(const featherdoc::style_prune_plan &plan);

void write_json_style_prune_summary(
    std::ostream &stream, const featherdoc::style_prune_summary &summary);

void print_style_prune_summary(const featherdoc::style_prune_summary &summary);

} // namespace featherdoc_cli
