#include "featherdoc_cli_content_control_commands.hpp"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_content_control_inspect_commands.hpp"
#include "featherdoc_cli_content_control_mutation_output.hpp"
#include "featherdoc_cli_content_control_table_commands.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

auto is_same_drawing_image_identity(const featherdoc::drawing_image_info &lhs,
                                    const featherdoc::drawing_image_info &rhs)
    -> bool {
    return lhs.relationship_id == rhs.relationship_id &&
           lhs.entry_name == rhs.entry_name;
}

auto collect_new_drawing_images(
    const std::vector<featherdoc::drawing_image_info> &before_images,
    const std::vector<featherdoc::drawing_image_info> &after_images)
    -> std::vector<featherdoc::drawing_image_info> {
    std::vector<bool> matched_before(before_images.size(), false);
    std::vector<featherdoc::drawing_image_info> inserted_images;
    inserted_images.reserve(after_images.size());

    for (const auto &image : after_images) {
        auto matched_it =
            std::find_if(before_images.begin(), before_images.end(),
                         [&before_images, &matched_before,
                          &image](const featherdoc::drawing_image_info &candidate) {
                             const auto candidate_index = static_cast<std::size_t>(
                                 &candidate - before_images.data());
                             return !matched_before[candidate_index] &&
                                    is_same_drawing_image_identity(candidate, image);
                         });
        if (matched_it == before_images.end()) {
            inserted_images.push_back(image);
            continue;
        }

        matched_before[static_cast<std::size_t>(matched_it - before_images.begin())] =
            true;
    }

    return inserted_images;
}


auto resolve_text_sources(const std::vector<cli_text_source_options> &sources,
                          std::vector<std::string> &texts,
                          std::string &error_message) -> bool {
    texts.clear();
    texts.reserve(sources.size());

    for (const auto &source : sources) {
        std::string text;
        if (!read_text_source(source, text, error_message)) {
            return false;
        }
        texts.push_back(std::move(text));
    }

    return true;
}

} // namespace

auto is_content_control_command(std::string_view command) -> bool {
    return is_content_control_inspect_command(command) ||
           command == "replace-content-control-text" ||
           command == "set-content-control-form-state" ||
           command == "sync-content-controls-from-custom-xml" ||
           command == "replace-content-control-paragraphs" ||
           command == "replace-content-control-table" ||
           command == "replace-content-control-table-rows" ||
           command == "replace-content-control-image";
}

auto run_content_control_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (is_content_control_inspect_command(command)) {
        return run_content_control_inspect_command(command, arguments, doc);
    }

    if (command == "replace-content-control-text") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command, "replace-content-control-text expects an input path",
                json_output);
            return 2;
        }

        replace_content_control_text_options options;
        std::string error_message;
        if (!parse_replace_content_control_text_options(arguments, 2U, options,
                                                        error_message)) {
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto replaced = options.tag.has_value()
                                  ? selected.part.replace_content_control_text_by_tag(
                                        *options.tag, options.text)
                                  : selected.part.replace_content_control_text_by_alias(
                                        *options.alias, options.text);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }
        if (replaced == 0U) {
            report_operation_failure(command, "mutate",
                                     "matching content control not found",
                                     doc.last_error(), options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, replaced](std::ostream &stream) {
                    write_json_content_control_text_result(stream, selected,
                                                           options, replaced);
                });
        } else {
            print_content_control_text_result(selected, options,
                                              options.output_path, replaced);
        }

        return 0;
    }

    if (command == "set-content-control-form-state") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command, "set-content-control-form-state expects an input path",
                json_output);
            return 2;
        }

        set_content_control_form_state_options options;
        std::string error_message;
        if (!parse_set_content_control_form_state_options(arguments, 2U, options,
                                                          error_message)) {
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto updated =
            options.tag.has_value()
                ? selected.part.set_content_control_form_state_by_tag(
                      *options.tag, options.state)
                : selected.part.set_content_control_form_state_by_alias(
                      *options.alias, options.state);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }
        if (updated == 0U) {
            report_operation_failure(command, "mutate",
                                     "matching content control not found",
                                     doc.last_error(), options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, updated](std::ostream &stream) {
                    write_json_content_control_form_state_result(
                        stream, selected, options, updated);
                });
        } else {
            print_content_control_form_state_result(selected, options, updated);
        }

        return 0;
    }

    if (command == "sync-content-controls-from-custom-xml") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command,
                "sync-content-controls-from-custom-xml expects an input path",
                json_output);
            return 2;
        }

        sync_content_controls_from_custom_xml_options options;
        std::string error_message;
        if (!parse_sync_content_controls_from_custom_xml_options(
                arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const auto sync_result = doc.sync_content_controls_from_custom_xml();
        if (!sync_result.has_value()) {
            report_document_error(command, "sync", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&sync_result](std::ostream &stream) {
                    write_json_custom_xml_sync_result(stream, *sync_result);
                });
        } else {
            print_custom_xml_sync_result(options.output_path, *sync_result);
        }

        return 0;
    }

    if (command == "replace-content-control-paragraphs") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command,
                "replace-content-control-paragraphs expects an input path",
                json_output);
            return 2;
        }

        replace_content_control_paragraphs_options options;
        std::string error_message;
        if (!parse_replace_content_control_paragraphs_options(
                arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::vector<std::string> paragraphs;
        if (!resolve_text_sources(options.paragraph_sources, paragraphs,
                                  error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "input",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto replaced =
            options.tag.has_value()
                ? selected.part.replace_content_control_with_paragraphs_by_tag(
                      *options.tag, paragraphs)
                : selected.part.replace_content_control_with_paragraphs_by_alias(
                      *options.alias, paragraphs);
        if (replaced == 0U) {
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "mutate", error_info,
                                      options.json_output);
            } else {
                report_operation_failure(command, "mutate",
                                         "matching content control not found",
                                         doc.last_error(), options.json_output);
            }
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, &paragraphs,
                 replaced](std::ostream &stream) {
                    write_json_content_control_paragraphs_result(
                        stream, selected, options, paragraphs, replaced);
                });
        } else {
            print_content_control_paragraphs_result(selected, options,
                                                    paragraphs, replaced);
        }

        return 0;
    }

    if (command == "replace-content-control-table" ||
        command == "replace-content-control-table-rows") {
        return run_replace_content_control_table_command(command, arguments, doc);
    }

    if (command == "replace-content-control-image") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "replace-content-control-image expects an input path and image path",
                json_output);
            return 2;
        }

        const auto image_path = path_type(std::string(arguments[2]));
        replace_content_control_image_options options;
        std::string error_message;
        if (!parse_replace_content_control_image_options(arguments, 3U, options,
                                                         error_message)) {
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
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto existing_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        const auto replaced =
            options.width_px.has_value()
                ? (options.tag.has_value()
                       ? selected.part.replace_content_control_with_image_by_tag(
                             *options.tag, image_path, *options.width_px,
                             *options.height_px)
                       : selected.part.replace_content_control_with_image_by_alias(
                             *options.alias, image_path, *options.width_px,
                             *options.height_px))
                : (options.tag.has_value()
                       ? selected.part.replace_content_control_with_image_by_tag(
                             *options.tag, image_path)
                       : selected.part.replace_content_control_with_image_by_alias(
                             *options.alias, image_path));
        if (replaced == 0U) {
            if (const auto &error_info = doc.last_error(); error_info.code) {
                report_document_error(command, "mutate", error_info,
                                      options.json_output);
            } else {
                report_operation_failure(command, "mutate",
                                         "matching content control not found",
                                         doc.last_error(), options.json_output);
            }
            return 1;
        }

        const auto updated_images = selected.part.drawing_images();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "mutate", error_info,
                                  options.json_output);
            return 1;
        }

        const auto inserted_images =
            collect_new_drawing_images(existing_images, updated_images);
        if (inserted_images.size() != replaced) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail =
                "expected " + std::to_string(replaced) +
                " replaced drawing image(s) in " +
                std::string(selected.part.entry_name()) + ", found " +
                std::to_string(inserted_images.size());
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "mutate",
                                     "replaced drawing images not found",
                                     error_info, options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, &options, &image_path,
                 &inserted_images](std::ostream &stream) {
                    write_json_content_control_image_result(
                        stream, selected, options, image_path, inserted_images);
                });
        } else {
            print_content_control_image_result(selected, options, image_path,
                                               inserted_images);
        }

        return 0;
    }

    return 2;
}

} // namespace featherdoc_cli
