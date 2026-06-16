#pragma once

#include "featherdoc_cli_style_refactor_options_parse.hpp"

#include <featherdoc.hpp>

#include <iosfwd>

namespace featherdoc_cli {

void write_json_style_refactor_restore_selection(
    std::ostream &stream, const style_merge_restore_options &options);

void write_json_style_refactor_restore_result_fields(
    std::ostream &stream,
    const featherdoc::style_refactor_restore_result &result);

void inspect_style_refactor_restore_result(
    const featherdoc::style_refactor_restore_result &result, bool json_output,
    const style_merge_restore_options *options = nullptr);

} // namespace featherdoc_cli
