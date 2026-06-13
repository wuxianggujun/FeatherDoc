#include "featherdoc_cli_style_refactor_rollback_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_style_refactor_plan_parse.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace featherdoc_cli {

using path_type = std::filesystem::path;

#include "featherdoc_cli_style_refactor_rollback_value_parse.inc"

#include "featherdoc_cli_style_refactor_rollback_usage_parse.inc"

#include "featherdoc_cli_style_refactor_rollback_entry_parse.inc"

#include "featherdoc_cli_style_refactor_rollback_file_parse.inc"

} // namespace featherdoc_cli
