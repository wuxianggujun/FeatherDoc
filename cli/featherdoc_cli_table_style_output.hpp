#pragma once

#include <featherdoc.hpp>

#include <iosfwd>

namespace featherdoc_cli {

void write_json_table_style_look(
    std::ostream &stream, const featherdoc::table_style_look &style_look);

} // namespace featherdoc_cli
