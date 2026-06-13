#include "featherdoc_cli_bookmark_commands_detail.hpp"

#include "featherdoc_cli_bookmark_output.hpp"
#include "featherdoc_cli_bookmark_support.hpp"
#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
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

} // namespace

auto run_replace_bookmark_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 4U) {
        print_parse_error(
            command,
            std::string(command) +
                " expects an input path, bookmark name, and image path",
            json_output);
        return 2;
    }

    const auto bookmark_name = std::string(arguments[2]);
    if (bookmark_name.empty()) {
        print_parse_error(command, "bookmark name must not be empty",
                          json_output);
        return 2;
    }

    append_image_options options;
    std::string error_message;
    const bool floating = command == "replace-bookmark-floating-image";
    if (!parse_bookmark_image_command_options(arguments, 4U, command, floating,
                                              options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    selected_template_part selected;
    if (!select_bookmark_part(command, doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              options.json_output, selected, error_message)) {
        return 1;
    }

    const auto bookmark = selected.part.find_bookmark(bookmark_name);
    if (!bookmark.has_value()) {
        report_document_error(command, "inspect", doc.last_error(),
                              options.json_output);
        return 1;
    }
    const auto bookmark_summary = *bookmark;

    const auto existing_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    const auto image_path = path_type(std::string(arguments[3]));
    std::size_t replaced = 0U;
    if (floating) {
        if (options.width_px.has_value()) {
            replaced = selected.part.replace_bookmark_with_floating_image(
                bookmark_name, image_path, *options.width_px, *options.height_px,
                options.floating_options);
        } else {
            replaced = selected.part.replace_bookmark_with_floating_image(
                bookmark_name, image_path, options.floating_options);
        }
    } else if (options.width_px.has_value()) {
        replaced = selected.part.replace_bookmark_with_image(
            bookmark_name, image_path, *options.width_px, *options.height_px);
    } else {
        replaced =
            selected.part.replace_bookmark_with_image(bookmark_name, image_path);
    }

    if (replaced == 0U) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto updated_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, options.json_output);
        return 1;
    }

    const auto inserted_images =
        collect_new_drawing_images(existing_images, updated_images);
    if (inserted_images.size() != replaced) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::result_out_of_range);
        error_info.detail = "expected " + std::to_string(replaced) +
                            " replaced drawing image(s) in " +
                            std::string(selected.part.entry_name()) +
                            ", found " +
                            std::to_string(inserted_images.size());
        error_info.entry_name = std::string(selected.part.entry_name());
        report_operation_failure(command, "mutate",
                                 "replaced drawing images not found", error_info,
                                 options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &bookmark_summary, &image_path,
             &inserted_images](std::ostream &stream) {
                write_json_bookmark_image_result(
                    stream, selected, bookmark_summary, image_path,
                    inserted_images);
            });
    } else {
        print_bookmark_image_result(selected, bookmark_summary, image_path,
                                    options.output_path, inserted_images);
    }

    return 0;
}

} // namespace featherdoc_cli
