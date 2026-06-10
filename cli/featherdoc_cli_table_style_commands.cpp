#include "featherdoc_cli_table_style_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_style_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_ensure_options_parse.hpp"
#include "featherdoc_cli_style_output.hpp"
#include "featherdoc_cli_table_cell_options_parse.hpp"
#include "featherdoc_cli_table_style_look_options_parse.hpp"
#include "featherdoc_cli_table_style_output.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

#include "featherdoc_cli_table_style_command_helpers.inc"

#include "featherdoc_cli_table_style_id_look_commands.inc"

#include "featherdoc_cli_table_style_definition_commands.inc"

#include "featherdoc_cli_table_style_audit_quality_commands.inc"

#include "featherdoc_cli_table_style_look_consistency_commands.inc"

} // namespace

#include "featherdoc_cli_table_style_dispatch.inc"

} // namespace featherdoc_cli
