#include "featherdoc_cli_image_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_image_options_parse.hpp"
#include "featherdoc_cli_image_output.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_text.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

#include "featherdoc_cli_image_command_output_helpers.inc"

#include "featherdoc_cli_image_command_part_load.inc"

#include "featherdoc_cli_image_command_inspect_extract.inc"

#include "featherdoc_cli_image_command_mutation.inc"

#include "featherdoc_cli_image_command_append.inc"

} // namespace

#include "featherdoc_cli_image_command_dispatch.inc"

} // namespace featherdoc_cli
