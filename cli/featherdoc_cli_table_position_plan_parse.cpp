#include "featherdoc_cli_table_position_plan_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <algorithm>
#include <cstdint>
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

#include "featherdoc_cli_table_position_plan_preset_parse.inc"

#include "featherdoc_cli_table_position_plan_value_parse.inc"

#include "featherdoc_cli_table_position_plan_fingerprint_parse.inc"

#include "featherdoc_cli_table_position_plan_file_parse.inc"
} // namespace featherdoc_cli
