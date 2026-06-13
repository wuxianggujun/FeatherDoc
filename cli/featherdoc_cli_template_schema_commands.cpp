#include "featherdoc_cli_template_schema_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_schema_convert.hpp"
#include "featherdoc_cli_template_schema_export.hpp"
#include "featherdoc_cli_template_schema_model.hpp"
#include "featherdoc_cli_template_schema_ops.hpp"
#include "featherdoc_cli_template_schema_options_parse.hpp"
#include "featherdoc_cli_template_schema_output.hpp"
#include "featherdoc_cli_template_schema_patch_ops.hpp"
#include "featherdoc_cli_template_schema_patch_parse.hpp"
#include "featherdoc_cli_template_schema_parse.hpp"
#include "featherdoc_cli_template_schema_validation_output.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto append_validate_template_schema_file_targets(
    const path_type &schema_path,
    std::vector<validate_template_schema_target_options> &targets,
    std::string &error_message) -> bool {
    exported_template_schema_result schema_file;
    if (!read_template_schema_file(schema_path, schema_file, error_message)) {
        return false;
    }

    for (const auto &schema_target : schema_file.targets) {
        validate_template_schema_target_options target{};
        target.part = schema_target.part;
        target.part_index = schema_target.part_index;
        target.section_index = schema_target.section_index;
        target.requirements = schema_target.requirements;
        if (schema_target.reference_kind.has_value()) {
            target.reference_kind = *schema_target.reference_kind;
            target.has_kind = true;
        }
        targets.push_back(std::move(target));
    }

    return true;
}

} // namespace

auto is_template_schema_command(std::string_view command) -> bool {
    return command == "validate-template" ||
           command == "validate-template-schema" ||
           command == "export-template-schema" ||
           command == "normalize-template-schema" ||
           command == "lint-template-schema" ||
           command == "repair-template-schema" ||
           command == "merge-template-schema" ||
           command == "patch-template-schema" ||
           command == "preview-template-schema-patch" ||
           command == "build-template-schema-patch" ||
           command == "diff-template-schema" ||
           command == "check-template-schema";
}

auto run_template_schema_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
#include "featherdoc_cli_template_schema_validate_commands.inc"

#include "featherdoc_cli_template_schema_maintenance_commands.inc"

#include "featherdoc_cli_template_schema_patch_commands.inc"

#include "featherdoc_cli_template_schema_diff_check_commands.inc"

    return 2;
}

} // namespace featherdoc_cli
