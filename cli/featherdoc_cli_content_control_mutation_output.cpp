#include "featherdoc_cli_content_control_mutation_output.hpp"

#include "featherdoc_cli_content_control_output.hpp"
#include "featherdoc_cli_image_output.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <featherdoc.hpp>

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>

namespace featherdoc_cli {
namespace {

void write_json_content_control_form_state_options(
    std::ostream &stream,
    const featherdoc::content_control_form_state_options &state) {
    stream << R"({"clear_lock":)" << (state.clear_lock ? "true" : "false");
    stream << R"(,"clear_data_binding":)"
           << (state.clear_data_binding ? "true" : "false");
    if (state.lock.has_value()) {
        stream << R"(,"lock":)";
        write_json_string(stream, *state.lock);
    }
    if (state.data_binding_store_item_id.has_value()) {
        stream << R"(,"data_binding_store_item_id":)";
        write_json_string(stream, *state.data_binding_store_item_id);
    }
    if (state.data_binding_xpath.has_value()) {
        stream << R"(,"data_binding_xpath":)";
        write_json_string(stream, *state.data_binding_xpath);
    }
    if (state.data_binding_prefix_mappings.has_value()) {
        stream << R"(,"data_binding_prefix_mappings":)";
        write_json_string(stream, *state.data_binding_prefix_mappings);
    }
    if (state.checked.has_value()) {
        stream << R"(,"checked":)" << (*state.checked ? "true" : "false");
    }
    if (state.selected_list_item.has_value()) {
        stream << R"(,"selected_item":)";
        write_json_string(stream, *state.selected_list_item);
    }
    if (state.date_text.has_value()) {
        stream << R"(,"date_text":)";
        write_json_string(stream, *state.date_text);
    }
    if (state.date_format.has_value()) {
        stream << R"(,"date_format":)";
        write_json_string(stream, *state.date_format);
    }
    if (state.date_locale.has_value()) {
        stream << R"(,"date_locale":)";
        write_json_string(stream, *state.date_locale);
    }
    stream << '}';
}

void write_json_custom_xml_sync_item(
    std::ostream &stream,
    const featherdoc::custom_xml_data_binding_sync_item &item) {
    stream << R"({"part_entry_name":)";
    write_json_string(stream, item.part_entry_name);
    stream << R"(,"content_control_index":)" << item.content_control_index
           << R"(,"tag":)";
    write_json_optional_string(stream, item.tag);
    stream << R"(,"alias":)";
    write_json_optional_string(stream, item.alias);
    stream << R"(,"store_item_id":)";
    write_json_string(stream, item.store_item_id);
    stream << R"(,"xpath":)";
    write_json_string(stream, item.xpath);
    stream << R"(,"previous_text":)";
    write_json_string(stream, item.previous_text);
    stream << R"(,"value":)";
    write_json_string(stream, item.value);
    stream << '}';
}

void write_json_custom_xml_sync_issue(
    std::ostream &stream,
    const featherdoc::custom_xml_data_binding_sync_issue &issue) {
    stream << R"({"part_entry_name":)";
    write_json_string(stream, issue.part_entry_name);
    stream << R"(,"content_control_index":)";
    write_json_optional_size(stream, issue.content_control_index);
    stream << R"(,"tag":)";
    write_json_optional_string(stream, issue.tag);
    stream << R"(,"alias":)";
    write_json_optional_string(stream, issue.alias);
    stream << R"(,"store_item_id":)";
    write_json_string(stream, issue.store_item_id);
    stream << R"(,"xpath":)";
    write_json_string(stream, issue.xpath);
    stream << R"(,"reason":)";
    write_json_string(stream, issue.reason);
    stream << '}';
}

} // namespace

void write_json_content_control_text_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_text_options &options, std::size_t replaced) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << ",\"replaced\":" << replaced << ",\"text\":";
    write_json_string(stream, options.text);
}

void print_content_control_text_result(
    const selected_template_part &selected,
    const replace_content_control_text_options &options,
    const std::optional<path_type> &output_path, std::size_t replaced) {
    print_content_control_common_result(selected, options.tag, options.alias,
                                        output_path, replaced);
    std::cout << "text: " << options.text << '\n';
}

void write_json_content_control_paragraphs_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_paragraphs_options &options,
    const std::vector<std::string> &paragraphs, std::size_t replaced) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << ",\"replaced\":" << replaced
           << ",\"paragraph_count\":" << paragraphs.size()
           << ",\"paragraphs\":";
    write_json_strings(stream, paragraphs);
}

void print_content_control_paragraphs_result(
    const selected_template_part &selected,
    const replace_content_control_paragraphs_options &options,
    const std::vector<std::string> &paragraphs, std::size_t replaced) {
    print_content_control_common_result(selected, options.tag, options.alias,
                                        options.output_path, replaced);
    std::cout << "paragraph_count: " << paragraphs.size() << '\n';
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        std::cout << "paragraph[" << index << "]: " << paragraphs[index] << '\n';
    }
}

void write_json_content_control_image_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_image_options &options,
    const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << ",\"image_path\":";
    write_json_string(stream, image_path.string());
    stream << ",\"replaced\":" << inserted_images.size() << ",\"images\":[";
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_drawing_image_summary(stream, inserted_images[index]);
    }
    stream << ']';
}

void print_content_control_image_result(
    const selected_template_part &selected,
    const replace_content_control_image_options &options,
    const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    print_content_control_common_result(selected, options.tag, options.alias,
                                        options.output_path,
                                        inserted_images.size());
    std::cout << "image_path: " << image_path.string() << '\n';
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        std::cout << "image[" << index << "]: ";
        print_drawing_image_summary(std::cout, inserted_images[index]);
        std::cout << '\n';
    }
}

void write_json_content_control_form_state_result(
    std::ostream &stream, const selected_template_part &selected,
    const set_content_control_form_state_options &options, std::size_t updated) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << R"(,"updated":)" << updated << R"(,"form_state":)";
    write_json_content_control_form_state_options(stream, options.state);
}

void print_content_control_form_state_result(
    const selected_template_part &selected,
    const set_content_control_form_state_options &options, std::size_t updated) {
    print_selected_template_part(std::cout, selected);
    std::cout << "selector_kind: " << (options.tag.has_value() ? "tag" : "alias")
              << '\n';
    std::cout << "selector_value: "
              << (options.tag.has_value() ? *options.tag : *options.alias)
              << '\n';
    if (options.output_path.has_value()) {
        std::cout << "output_path: " << options.output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "updated: " << updated << '\n';
    const auto &state = options.state;
    if (state.lock.has_value()) {
        std::cout << "lock: " << *state.lock << '\n';
    }
    if (state.clear_lock) {
        std::cout << "clear_lock: true\n";
    }
    if (state.clear_data_binding) {
        std::cout << "clear_data_binding: true\n";
    }
    if (state.data_binding_store_item_id.has_value()) {
        std::cout << "data_binding_store_item_id: "
                  << *state.data_binding_store_item_id << "\n";
    }
    if (state.data_binding_xpath.has_value()) {
        std::cout << "data_binding_xpath: " << *state.data_binding_xpath
                  << "\n";
    }
    if (state.data_binding_prefix_mappings.has_value()) {
        std::cout << "data_binding_prefix_mappings: "
                  << *state.data_binding_prefix_mappings << "\n";
    }
    if (state.checked.has_value()) {
        std::cout << "checked: " << (*state.checked ? "true" : "false")
                  << '\n';
    }
    if (state.selected_list_item.has_value()) {
        std::cout << "selected_item: " << *state.selected_list_item << '\n';
    }
    if (state.date_text.has_value()) {
        std::cout << "date_text: " << *state.date_text << '\n';
    }
    if (state.date_format.has_value()) {
        std::cout << "date_format: " << *state.date_format << '\n';
    }
    if (state.date_locale.has_value()) {
        std::cout << "date_locale: " << *state.date_locale << '\n';
    }
}

void write_json_custom_xml_sync_result(
    std::ostream &stream,
    const featherdoc::custom_xml_data_binding_sync_result &result) {
    stream << R"(,"scanned_content_controls":)" << result.scanned_content_controls
           << R"(,"bound_content_controls":)" << result.bound_content_controls
           << R"(,"synced_content_controls":)" << result.synced_content_controls
           << R"(,"issue_count":)" << result.issues.size()
           << R"(,"synced_items":[)";
    for (std::size_t index = 0; index < result.synced_items.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_custom_xml_sync_item(stream, result.synced_items[index]);
    }
    stream << R"(],"issues":[)";
    for (std::size_t index = 0; index < result.issues.size(); ++index) {
        if (index > 0U) {
            stream << ',';
        }
        write_json_custom_xml_sync_issue(stream, result.issues[index]);
    }
    stream << ']';
}

void print_custom_xml_sync_result(
    const std::optional<path_type> &output_path,
    const featherdoc::custom_xml_data_binding_sync_result &result) {
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "scanned_content_controls: " << result.scanned_content_controls << '\n';
    std::cout << "bound_content_controls: " << result.bound_content_controls << '\n';
    std::cout << "synced_content_controls: " << result.synced_content_controls << '\n';
    std::cout << "issue_count: " << result.issues.size() << '\n';
    for (const auto &item : result.synced_items) {
        std::cout << "synced[" << item.content_control_index << "]: "
                  << item.part_entry_name << " " << item.xpath << " = "
                  << item.value << '\n';
    }
    for (const auto &issue : result.issues) {
        std::cout << "issue";
        if (issue.content_control_index.has_value()) {
            std::cout << '[' << *issue.content_control_index << ']';
        }
        std::cout << ": " << issue.part_entry_name << " " << issue.reason
                  << " " << issue.xpath << '\n';
    }
}

} // namespace featherdoc_cli
