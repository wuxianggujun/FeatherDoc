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

void write_json_selected_image_part(std::ostream &stream,
                                    const selected_template_part &selected) {
    stream << ",\"part\":";
    write_json_string(stream, validation_part_name(selected.family));
    if (selected.part_index.has_value()) {
        stream << ",\"part_index\":" << *selected.part_index;
    }
    if (selected.section_index.has_value()) {
        stream << ",\"section\":" << *selected.section_index;
    }
    if (selected.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(
            stream, featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
}

void print_selected_image_part(const selected_template_part &selected) {
    const auto entry_name = std::string(selected.part.entry_name());
    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
}

void inspect_images(const selected_template_part &selected,
                    const std::vector<featherdoc::drawing_image_info> &images,
                    const inspect_images_options &options, bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(
                std::cout,
                featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, std::string(selected.part.entry_name()));
        write_json_inspect_image_filters(std::cout, options);
        std::cout << ",\"count\":" << images.size() << ",\"images\":[";
        for (std::size_t index = 0; index < images.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_drawing_image_summary(std::cout, images[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_image_part(selected);
    print_inspect_image_filters(std::cout, options);
    std::cout << "images: " << images.size() << '\n';
    for (std::size_t index = 0; index < images.size(); ++index) {
        std::cout << "image[" << index << "]: ";
        print_drawing_image_summary(std::cout, images[index]);
        std::cout << '\n';
    }
}

void inspect_image(const selected_template_part &selected,
                   const featherdoc::drawing_image_info &image,
                   const inspect_images_options &options, bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(
                std::cout,
                featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, std::string(selected.part.entry_name()));
        write_json_inspect_image_filters(std::cout, options);
        std::cout << ",\"image\":";
        write_json_drawing_image_summary(std::cout, image);
        std::cout << "}\n";
        return;
    }

    print_selected_image_part(selected);
    print_inspect_image_filters(std::cout, options);
    std::cout << "image_index: " << image.index << '\n';
    std::cout << "placement: " << drawing_image_placement_name(image.placement)
              << '\n';
    std::cout << "relationship_id: " << image.relationship_id << '\n';
    std::cout << "image_entry_name: " << image.entry_name << '\n';
    std::cout << "display_name: " << image.display_name << '\n';
    std::cout << "content_type: " << image.content_type << '\n';
    std::cout << "width_px: " << image.width_px << '\n';
    std::cout << "height_px: " << image.height_px << '\n';
    std::cout << "floating: " << yes_no(image.floating_options.has_value()) << '\n';
    if (!image.floating_options.has_value()) {
        return;
    }

    const auto &floating_options = *image.floating_options;
    std::cout << "horizontal_reference: "
              << floating_image_horizontal_reference_name(
                     floating_options.horizontal_reference)
              << '\n';
    std::cout << "horizontal_offset_px: " << floating_options.horizontal_offset_px
              << '\n';
    std::cout << "vertical_reference: "
              << floating_image_vertical_reference_name(
                     floating_options.vertical_reference)
              << '\n';
    std::cout << "vertical_offset_px: " << floating_options.vertical_offset_px
              << '\n';
    std::cout << "behind_text: " << yes_no(floating_options.behind_text) << '\n';
    std::cout << "allow_overlap: " << yes_no(floating_options.allow_overlap)
              << '\n';
    std::cout << "z_order: " << floating_options.z_order << '\n';
    std::cout << "wrap_mode: "
              << floating_image_wrap_mode_name(floating_options.wrap_mode)
              << '\n';
    std::cout << "wrap_distance_left_px: "
              << floating_options.wrap_distance_left_px << '\n';
    std::cout << "wrap_distance_right_px: "
              << floating_options.wrap_distance_right_px << '\n';
    std::cout << "wrap_distance_top_px: "
              << floating_options.wrap_distance_top_px << '\n';
    std::cout << "wrap_distance_bottom_px: "
              << floating_options.wrap_distance_bottom_px << '\n';
    if (!floating_options.crop.has_value()) {
        std::cout << "crop: none\n";
        return;
    }

    std::cout << "crop_left_per_mille: " << floating_options.crop->left_per_mille
              << '\n';
    std::cout << "crop_top_per_mille: " << floating_options.crop->top_per_mille
              << '\n';
    std::cout << "crop_right_per_mille: "
              << floating_options.crop->right_per_mille << '\n';
    std::cout << "crop_bottom_per_mille: "
              << floating_options.crop->bottom_per_mille << '\n';
}

void print_extract_image_result(const selected_template_part &selected,
                                const featherdoc::drawing_image_info &image,
                                const path_type &output_path) {
    print_selected_image_part(selected);
    std::cout << "output_path: " << output_path.string() << '\n';
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void print_replace_image_result(const selected_template_part &selected,
                                const featherdoc::drawing_image_info &image,
                                const path_type &replacement_path,
                                const std::optional<path_type> &output_path) {
    print_selected_image_part(selected);
    std::cout << "replacement_path: " << replacement_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void print_remove_image_result(const selected_template_part &selected,
                               const featherdoc::drawing_image_info &image,
                               const std::optional<path_type> &output_path) {
    print_selected_image_part(selected);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "removed_image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

void print_append_image_result(const selected_template_part &selected,
                               const featherdoc::drawing_image_info &image,
                               const path_type &image_path,
                               const std::optional<path_type> &output_path) {
    print_selected_image_part(selected);
    std::cout << "image_path: " << image_path.string() << '\n';
    std::cout << "floating: " << yes_no(image.floating_options.has_value()) << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "image: ";
    print_drawing_image_summary(std::cout, image);
    std::cout << '\n';
}

[[nodiscard]] auto load_image_part(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const inspect_images_options &options, featherdoc::Document &doc,
    selected_template_part &selected, std::string &error_message,
    std::string_view operation) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    if (!select_template_part(doc, options.part, options.part_index,
                              options.section_index, options.reference_kind,
                              selected, error_message)) {
        report_operation_failure(command, operation, error_message,
                                 doc.last_error(), options.json_output);
        return false;
    }

    return true;
}

[[nodiscard]] auto load_mutable_image_part(
    std::string_view command, const std::vector<std::string_view> &arguments,
    const append_image_options &options, featherdoc::Document &doc,
    selected_template_part &selected, std::string &error_message) -> bool {
    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return false;
    }

    if (!select_mutable_template_part(doc, options.part, options.part_index,
                                      options.section_index,
                                      options.reference_kind, selected,
                                      error_message)) {
        report_operation_failure(command, "mutate", error_message,
                                 doc.last_error(), options.json_output);
        return false;
    }

    return true;
}

[[nodiscard]] auto run_inspect_images_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-images expects an input path",
                          json_output);
        return 2;
    }

    inspect_images_options options;
    std::string error_message;
    if (!parse_inspect_images_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_image_part(command, arguments, options, doc, selected, error_message,
                         "inspect")) {
        return 1;
    }

    const auto images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    if (options.image_index.has_value()) {
        const auto image_it = std::find_if(
            images.begin(), images.end(),
            [&options](const featherdoc::drawing_image_info &image) {
                return image.index == *options.image_index;
            });
        if (image_it == images.end() ||
            !drawing_image_matches_filters(*image_it, options)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::result_out_of_range);
            error_info.detail = "drawing image index " +
                                std::to_string(*options.image_index) +
                                " was not found in " +
                                std::string(selected.part.entry_name());
            if (has_inspect_image_filters(options)) {
                error_info.detail +=
                    " for " + describe_inspect_image_filters(options);
            }
            error_info.entry_name = std::string(selected.part.entry_name());
            report_operation_failure(command, "inspect", "drawing image not found",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_image(selected, *image_it, options, options.json_output);
        return 0;
    }

    inspect_images(selected, filter_drawing_images(images, options), options,
                   options.json_output);
    return 0;
}

[[nodiscard]] auto run_extract_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "extract-image expects an input path and an output path",
                          json_output);
        return 2;
    }

    extract_image_options options;
    std::string error_message;
    if (!parse_extract_image_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_image_part(command, arguments, options, doc, selected, error_message,
                         "inspect")) {
        return 1;
    }

    const auto images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    featherdoc::drawing_image_info target_image{};
    featherdoc::document_error_info selection_error{};
    if (!resolve_selected_drawing_image(images, options, selected.part.entry_name(),
                                        target_image, selection_error)) {
        report_operation_failure(command, "input", "drawing image not found",
                                 selection_error, options.json_output);
        return 1;
    }

    const auto output_path = path_type(std::string(arguments[2]));
    if (!selected.part.extract_drawing_image(target_image.index, output_path)) {
        report_document_error(command, "extract", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (options.json_output) {
        std::cout << "{\"command\":";
        write_json_string(std::cout, command);
        std::cout << ",\"ok\":true";
        write_json_selected_image_part(std::cout, selected);
        std::cout << ",\"output_path\":";
        write_json_string(std::cout, output_path.string());
        write_json_inspect_image_filters(std::cout, options);
        std::cout << ",\"image\":";
        write_json_drawing_image_summary(std::cout, target_image);
        std::cout << "}\n";
    } else {
        print_extract_image_result(selected, target_image, output_path);
    }

    return 0;
}

[[nodiscard]] auto run_replace_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "replace-image expects an input path and an image path",
                          json_output);
        return 2;
    }

    replace_image_options options;
    std::string error_message;
    if (!parse_replace_image_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_image_part(command, arguments, options, doc, selected, error_message,
                         "mutate")) {
        return 1;
    }

    const auto images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    featherdoc::drawing_image_info target_image{};
    featherdoc::document_error_info selection_error{};
    if (!resolve_selected_drawing_image(images, options, selected.part.entry_name(),
                                        target_image, selection_error)) {
        report_operation_failure(command, "input", "drawing image not found",
                                 selection_error, options.json_output);
        return 1;
    }

    const auto replacement_path = path_type(std::string(arguments[2]));
    if (!selected.part.replace_drawing_image(target_image.index, replacement_path)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto updated_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, options.json_output);
        return 1;
    }

    const auto updated_image_it = std::find_if(
        updated_images.begin(), updated_images.end(),
        [&target_image](const featherdoc::drawing_image_info &image) {
            return image.index == target_image.index;
        });
    if (updated_image_it == updated_images.end()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::result_out_of_range);
        error_info.detail = "updated drawing image index " +
                            std::to_string(target_image.index) +
                            " was not found in " +
                            std::string(selected.part.entry_name());
        error_info.entry_name = std::string(selected.part.entry_name());
        report_operation_failure(command, "mutate",
                                 "updated drawing image not found", error_info,
                                 options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &updated_image_it, &replacement_path,
             &options](std::ostream &stream) {
                write_json_selected_image_part(stream, selected);
                stream << ",\"replacement_path\":";
                write_json_string(stream, replacement_path.string());
                write_json_inspect_image_filters(stream, options);
                stream << ",\"image\":";
                write_json_drawing_image_summary(stream, *updated_image_it);
            });
    } else {
        print_replace_image_result(selected, *updated_image_it, replacement_path,
                                   options.output_path);
    }

    return 0;
}

[[nodiscard]] auto run_remove_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command,
                          "remove-image expects an input path and image selectors",
                          json_output);
        return 2;
    }

    remove_image_options options;
    std::string error_message;
    if (!parse_remove_image_options(arguments, 2U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_image_part(command, arguments, options, doc, selected, error_message,
                         "mutate")) {
        return 1;
    }

    const auto images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    featherdoc::drawing_image_info target_image{};
    featherdoc::document_error_info selection_error{};
    if (!resolve_selected_drawing_image(images, options, selected.part.entry_name(),
                                        target_image, selection_error)) {
        report_operation_failure(command, "input", "drawing image not found",
                                 selection_error, options.json_output);
        return 1;
    }

    if (!selected.part.remove_drawing_image(target_image.index)) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &target_image, &options](std::ostream &stream) {
                write_json_selected_image_part(stream, selected);
                write_json_inspect_image_filters(stream, options);
                stream << ",\"image\":";
                write_json_drawing_image_summary(stream, target_image);
            });
    } else {
        print_remove_image_result(selected, target_image, options.output_path);
    }

    return 0;
}

[[nodiscard]] auto run_append_image_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 3U) {
        print_parse_error(command,
                          "append-image expects an input path and an image path",
                          json_output);
        return 2;
    }

    append_image_options options;
    std::string error_message;
    if (!parse_append_image_options(arguments, 3U, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    selected_template_part selected;
    if (!load_mutable_image_part(command, arguments, options, doc, selected,
                                 error_message)) {
        return 1;
    }

    const auto existing_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info, options.json_output);
        return 1;
    }

    const auto image_path = path_type(std::string(arguments[2]));
    bool success = false;
    if (options.floating) {
        if (options.width_px.has_value()) {
            success = selected.part.append_floating_image(
                image_path, *options.width_px, *options.height_px,
                options.floating_options);
        } else {
            success =
                selected.part.append_floating_image(image_path, options.floating_options);
        }
    } else if (options.width_px.has_value()) {
        success = selected.part.append_image(image_path, *options.width_px,
                                             *options.height_px);
    } else {
        success = selected.part.append_image(image_path);
    }

    if (!success) {
        report_document_error(command, "mutate", doc.last_error(),
                              options.json_output);
        return 1;
    }

    const auto updated_images = selected.part.drawing_images();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "mutate", error_info, options.json_output);
        return 1;
    }

    if (updated_images.size() <= existing_images.size()) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::result_out_of_range);
        error_info.detail = "appended drawing image was not found in " +
                            std::string(selected.part.entry_name());
        error_info.entry_name = std::string(selected.part.entry_name());
        report_operation_failure(command, "mutate",
                                 "appended drawing image not found", error_info,
                                 options.json_output);
        return 1;
    }

    const auto &appended_image = updated_images.back();
    if (!save_document(doc, options.output_path, command, options.json_output)) {
        return 1;
    }

    if (options.json_output) {
        write_json_mutation_result(
            command, doc, options.output_path,
            [&selected, &appended_image, &image_path](std::ostream &stream) {
                write_json_selected_image_part(stream, selected);
                stream << ",\"image_path\":";
                write_json_string(stream, image_path.string());
                stream << ",\"floating\":"
                       << json_bool(appended_image.floating_options.has_value());
                stream << ",\"image\":";
                write_json_drawing_image_summary(stream, appended_image);
            });
    } else {
        print_append_image_result(selected, appended_image, image_path,
                                  options.output_path);
    }

    return 0;
}

} // namespace

auto is_image_command(std::string_view command) -> bool {
    return command == "inspect-images" || command == "extract-image" ||
           command == "replace-image" || command == "remove-image" ||
           command == "append-image";
}

auto run_image_command(std::string_view command,
                       const std::vector<std::string_view> &arguments,
                       featherdoc::Document &doc) -> int {
    if (command == "inspect-images") {
        return run_inspect_images_command(command, arguments, doc);
    }
    if (command == "extract-image") {
        return run_extract_image_command(command, arguments, doc);
    }
    if (command == "replace-image") {
        return run_replace_image_command(command, arguments, doc);
    }
    if (command == "remove-image") {
        return run_remove_image_command(command, arguments, doc);
    }
    if (command == "append-image") {
        return run_append_image_command(command, arguments, doc);
    }

    return 2;
}

} // namespace featherdoc_cli
