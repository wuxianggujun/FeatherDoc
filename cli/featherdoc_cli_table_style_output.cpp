#include "featherdoc_cli_table_style_output.hpp"

#include "featherdoc_cli_text.hpp"

#include <ostream>

namespace featherdoc_cli {

void write_json_table_style_look(
    std::ostream &stream, const featherdoc::table_style_look &style_look) {
    stream << "{\"first_row\":" << json_bool(style_look.first_row)
           << ",\"last_row\":" << json_bool(style_look.last_row)
           << ",\"first_column\":" << json_bool(style_look.first_column)
           << ",\"last_column\":" << json_bool(style_look.last_column)
           << ",\"banded_rows\":" << json_bool(style_look.banded_rows)
           << ",\"banded_columns\":" << json_bool(style_look.banded_columns)
           << '}';
}

} // namespace featherdoc_cli
