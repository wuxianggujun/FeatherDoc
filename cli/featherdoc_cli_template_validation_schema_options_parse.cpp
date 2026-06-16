#include "featherdoc_cli_template_validation_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_slot_parse.hpp"

#include <utility>

namespace featherdoc_cli {

auto parse_validate_template_schema_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    validate_template_schema_options &options, std::string &error_message) -> bool {
    validate_template_schema_target_options *current_target = nullptr;

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--target") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --target";
                return false;
            }

            validation_part_family part = validation_part_family::body;
            if (!parse_validation_part(arguments[index + 1U], part)) {
                error_message =
                    "invalid template schema target: " +
                    std::string(arguments[index + 1U]);
                return false;
            }

            options.targets.push_back({});
            current_target = &options.targets.back();
            current_target->part = part;
            ++index;
            continue;
        }

        if (argument == "--schema-file") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --schema-file";
                return false;
            }

            options.schema_files.emplace_back(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        if (current_target == nullptr) {
            error_message =
                std::string(argument) +
                " requires a preceding --target "
                "<body|header|footer|section-header|section-footer>";
            return false;
        }

        if (argument == "--index") {
            if (current_target->part_index.has_value()) {
                error_message = "duplicate --index option for current --target";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --index";
                return false;
            }

            std::size_t part_index = 0U;
            if (!parse_index(arguments[index + 1U], part_index)) {
                error_message = "invalid part index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            current_target->part_index = part_index;
            ++index;
            continue;
        }

        if (argument == "--section") {
            if (current_target->section_index.has_value()) {
                error_message = "duplicate --section option for current --target";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }

            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            current_target->section_index = section_index;
            ++index;
            continue;
        }

        if (argument == "--kind") {
            if (current_target->has_kind) {
                error_message = "duplicate --kind option for current --target";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }

            if (!parse_reference_kind(arguments[index + 1U],
                                      current_target->reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            current_target->has_kind = true;
            ++index;
            continue;
        }

        if (argument == "--slot") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --slot";
                return false;
            }

            featherdoc::template_slot_requirement requirement;
            if (!parse_template_slot_requirement(arguments[index + 1U], requirement,
                                                 error_message)) {
                return false;
            }

            current_target->requirements.push_back(std::move(requirement));
            ++index;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (options.targets.empty() && options.schema_files.empty()) {
        error_message =
            "expected at least one --schema-file <path> or --target "
            "<body|header|footer|section-header|section-footer>";
        return false;
    }

    for (std::size_t index = 0U; index < options.targets.size(); ++index) {
        const auto &target = options.targets[index];
        if (target.requirements.empty()) {
            error_message =
                "target[" + std::to_string(index) +
                "] requires at least one --slot <bookmark>:<kind>"
                "[:required|optional][:count=<n>|min=<n>|max=<n>...]";
            return false;
        }

        if (!validate_template_part_selection(
                target.part, target.part_index, target.section_index,
                target.has_kind, "schema validation", error_message)) {
            return false;
        }
    }

    return true;
}

} // namespace featherdoc_cli
