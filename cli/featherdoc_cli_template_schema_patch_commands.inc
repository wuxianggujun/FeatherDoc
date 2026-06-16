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
