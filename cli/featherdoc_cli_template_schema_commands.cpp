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
    if (command == "validate-template") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "validate-template expects an input path and validation options",
                json_output);
            return 2;
        }

        validate_template_options options;
        std::string error_message;
        if (!parse_validate_template_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "inspect", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto result = selected.part.validate_template(options.requirements);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, options.json_output);
            return 1;
        }

        print_template_validation_result(selected, result, options.json_output);
        return 0;
    }

    if (command == "validate-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "validate-template-schema expects an input path and schema validation options",
                json_output);
            return 2;
        }

        validate_template_schema_options options;
        std::string error_message;
        if (!parse_validate_template_schema_options(arguments, 2U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        for (const auto &schema_file : options.schema_files) {
            if (!append_validate_template_schema_file_targets(
                    schema_file, options.targets, error_message)) {
                print_parse_error(command, error_message, json_output);
                return 2;
            }
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::template_schema schema;
        std::size_t entry_count = 0U;
        for (const auto &target : options.targets) {
            entry_count += target.requirements.size();
        }
        schema.entries.reserve(entry_count);

        for (const auto &target : options.targets) {
            featherdoc::template_schema_part_selector selector{};
            selector.part = to_template_schema_part_kind(target.part);
            selector.part_index = target.part_index;
            selector.section_index = target.section_index;
            selector.reference_kind = target.reference_kind;

            for (const auto &requirement : target.requirements) {
                schema.entries.push_back({selector, requirement});
            }
        }

        const auto result = doc.validate_template_schema(schema);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, options.json_output);
            return 1;
        }

        print_template_schema_validation_result(result, options.json_output);
        return 0;
    }

    if (command == "export-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "export-template-schema expects an input path",
                              json_output);
            return 2;
        }

        export_template_schema_options options;
        std::string error_message;
        if (!parse_export_template_schema_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        exported_template_schema_result result;
        if (!build_exported_template_schema(
                doc, options.section_targets, options.resolved_section_targets, result,
                command, options.json_output)) {
            return 1;
        }

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write template schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_exported_template_schema_summary(result, options.output_path,
                                               options.json_output);
        return 0;
    }

    if (command == "normalize-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "normalize-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        normalize_template_schema_options options;
        std::string error_message;
        if (!parse_normalize_template_schema_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write normalized schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_normalized_template_schema_summary(result, options.output_path,
                                                 options.json_output);
        return 0;
    }

    if (command == "lint-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "lint-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        lint_template_schema_options options;
        std::string error_message;
        if (!parse_lint_template_schema_options(arguments, 2U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        const auto lint = lint_template_schema(result);
        print_linted_template_schema_result(lint, options.json_output);
        if (!lint.clean()) {
            return 1;
        }
        return 0;
    }

    if (command == "repair-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "repair-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        repair_template_schema_options options;
        std::string error_message;
        if (!parse_repair_template_schema_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result input;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), input,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result result;
        repaired_template_schema_summary summary{};
        repair_template_schema_result(input, result, summary);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write repaired schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_repaired_template_schema_summary(result, summary, options.output_path,
                                               options.json_output);
        return 0;
    }

    if (command == "merge-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "merge-template-schema expects at least two schema paths",
                              json_output);
            return 2;
        }

        merge_template_schema_options options;
        std::string error_message;
        if (!parse_merge_template_schema_options(arguments, 1U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        merged_template_schema_summary summary;
        for (const auto &schema_path : options.schema_paths) {
            exported_template_schema_result input;
            if (!read_template_schema_file(schema_path, input, error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            merge_template_schema_result(result, input, summary);
        }
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write merged schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_merged_template_schema_summary(result, summary, options.output_path,
                                             options.json_output);
        return 0;
    }

    if (command == "patch-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "patch-template-schema expects a schema path and "
                              "--patch-file <path>",
                              json_output);
            return 2;
        }

        patch_template_schema_options options;
        std::string error_message;
        if (!parse_patch_template_schema_options(arguments, 2U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        template_schema_patch_document patch;
        if (!read_template_schema_patch_file(*options.patch_path, patch,
                                             error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        patched_template_schema_summary summary;
        apply_template_schema_patch(result, patch, summary);
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write patched schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_patched_template_schema_summary(result, summary, options.output_path,
                                              options.json_output);
        return 0;
    }

    if (command == "preview-template-schema-patch") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "preview-template-schema-patch expects a schema path "
                              "and --patch-file <path> or <right-schema.json>",
                              json_output);
            return 2;
        }

        preview_template_schema_patch_options options;
        std::string error_message;
        if (!parse_preview_template_schema_patch_options(arguments, 2U, options,
                                                         error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left_result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])),
                                       left_result, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(left_result);
        if (has_template_schema_resolved_target_metadata(left_result)) {
            print_parse_error(command,
                              "preview-template-schema-patch does not support "
                              "schemas with resolved-section target metadata",
                              options.json_output);
            return 2;
        }

        previewed_template_schema_patch_summary summary{};
        summary.left_slot_count = left_result.slot_count();
        summary.review_json_path = options.review_json_path;
        const auto left_schema = to_template_schema(left_result);

        if (options.patch_path.has_value()) {
            template_schema_patch_document patch_document;
            if (!read_template_schema_patch_file(*options.patch_path, patch_document,
                                                 error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            if (has_template_schema_resolved_target_metadata(patch_document)) {
                print_parse_error(command,
                                  "preview-template-schema-patch does not support "
                                  "patches with resolved-section target metadata",
                                  options.json_output);
                return 2;
            }

            const auto patch = to_template_schema_patch(patch_document);
            summary.upsert_slot_count = patch.upsert_slots.size();
            summary.remove_target_count = patch.remove_targets.size();
            summary.remove_slot_count = patch.remove_slots.size();
            summary.rename_slot_count = patch.rename_slots.size();
            summary.update_slot_count = patch.update_slots.size();
            summary.applied =
                featherdoc::preview_template_schema_patch(left_schema, patch);
            if (options.review_json_path.has_value()) {
                const auto review =
                    featherdoc::make_template_schema_patch_review_summary(
                        left_schema, patch);
                if (!write_template_schema_patch_review_file(
                        *options.review_json_path, review, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(
                        command, "review-json",
                        "failed to write schema patch review output", error_info,
                        options.json_output);
                    return 1;
                }
            }
            if (options.output_patch_path.has_value()) {
                if (!write_template_schema_patch_file(*options.output_patch_path,
                                                      patch_document, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(command, "output-patch",
                                             "failed to write previewed patch output",
                                             error_info, options.json_output);
                    return 1;
                }
            }
        } else {
            exported_template_schema_result right_result;
            if (!read_template_schema_file(*options.right_schema_path, right_result,
                                           error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            normalize_template_schema_result(right_result);
            if (has_template_schema_resolved_target_metadata(right_result)) {
                print_parse_error(command,
                                  "preview-template-schema-patch does not support "
                                  "schemas with resolved-section target metadata",
                                  options.json_output);
                return 2;
            }

            summary.right_slot_count = right_result.slot_count();
            const auto diff = diff_template_schema_results(left_result, right_result);
            built_template_schema_patch_summary built_summary{};
            const auto patch_document =
                build_template_schema_patch_document(diff, built_summary);
            const auto patch = to_template_schema_patch(patch_document);
            summary.upsert_slot_count = patch.upsert_slots.size();
            summary.remove_target_count = patch.remove_targets.size();
            summary.remove_slot_count = patch.remove_slots.size();
            summary.rename_slot_count = patch.rename_slots.size();
            summary.update_slot_count = patch.update_slots.size();
            summary.applied =
                featherdoc::preview_template_schema_patch(left_schema, patch);
            if (options.review_json_path.has_value()) {
                auto review = featherdoc::make_template_schema_patch_review_summary(
                    left_schema, patch);
                review.generated_slot_count = right_result.slot_count();
                if (!write_template_schema_patch_review_file(
                        *options.review_json_path, review, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(
                        command, "review-json",
                        "failed to write schema patch review output", error_info,
                        options.json_output);
                    return 1;
                }
            }
            if (options.output_patch_path.has_value()) {
                if (!write_template_schema_patch_file(*options.output_patch_path,
                                                      patch_document, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(command, "output-patch",
                                             "failed to write previewed generated patch output",
                                             error_info, options.json_output);
                    return 1;
                }
            }
        }

        summary.output_patch_path = options.output_patch_path;
        print_previewed_template_schema_patch_summary(summary, options.json_output);
        return 0;
    }

    if (command == "build-template-schema-patch") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U || arguments[1].starts_with("--") ||
            arguments[2].starts_with("--")) {
            print_parse_error(
                command,
                "build-template-schema-patch expects left and right schema paths",
                json_output);
            return 2;
        }

        build_template_schema_patch_options options;
        std::string error_message;
        if (!parse_build_template_schema_patch_options(arguments, 3U, options,
                                                       error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), left,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result right;
        if (!read_template_schema_file(path_type(std::string(arguments[2])), right,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        normalize_template_schema_result(left);
        normalize_template_schema_result(right);
        const auto diff = diff_template_schema_results(left, right);
        built_template_schema_patch_summary summary{};
        const auto patch = build_template_schema_patch_document(diff, summary);

        if (options.review_json_path.has_value()) {
            const auto left_schema = to_template_schema(left);
            const auto patch_model = to_template_schema_patch(patch);
            auto review = featherdoc::make_template_schema_patch_review_summary(
                left_schema, patch_model);
            review.generated_slot_count = right.slot_count();
            if (!write_template_schema_patch_review_file(
                    *options.review_json_path, review, error_message)) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::io_error);
                error_info.detail = std::move(error_message);
                report_operation_failure(
                    command, "review-json",
                    "failed to write schema patch review output", error_info,
                    options.json_output);
                return 1;
            }
        }

        if (!options.output_path.has_value()) {
            write_json_template_schema_patch_document(std::cout, patch);
            return 0;
        }

        if (!write_template_schema_patch_file(*options.output_path, patch,
                                              error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write generated patch output",
                                     error_info, options.json_output);
            return 1;
        }

        print_built_template_schema_patch_summary(
            summary, options.output_path, options.review_json_path,
            options.json_output);
        return 0;
    }

    if (command == "diff-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "diff-template-schema expects left and right schema paths",
                              json_output);
            return 2;
        }

        diff_template_schema_options options;
        std::string error_message;
        if (!parse_diff_template_schema_options(arguments, 3U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), left,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result right;
        if (!read_template_schema_file(path_type(std::string(arguments[2])), right,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        normalize_template_schema_result(left);
        normalize_template_schema_result(right);
        const auto result = diff_template_schema_results(left, right);
        print_template_schema_diff_result(result, options.json_output);
        if (options.fail_on_diff && !result.equal()) {
            return 1;
        }
        return 0;
    }

    if (command == "check-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "check-template-schema expects an input path and --schema-file <path>",
                json_output);
            return 2;
        }

        check_template_schema_options options;
        std::string error_message;
        if (!parse_check_template_schema_options(arguments, 2U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result baseline;
        if (!read_template_schema_file(*options.schema_path, baseline, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(baseline);

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        exported_template_schema_result generated;
        if (!build_exported_template_schema(
                doc, options.section_targets, options.resolved_section_targets,
                generated, command, options.json_output)) {
            return 1;
        }
        normalize_template_schema_result(generated);

        if (options.output_path.has_value() &&
            !write_exported_template_schema_file(*options.output_path, generated,
                                                error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write generated schema output",
                                     error_info, options.json_output);
            return 1;
        }

        const auto result = diff_template_schema_results(baseline, generated);
        print_checked_template_schema_result(*options.schema_path, result,
                                             options.output_path,
                                             options.json_output);
        if (!result.equal()) {
            return 1;
        }
        return 0;
    }

    return 2;
}

} // namespace featherdoc_cli
