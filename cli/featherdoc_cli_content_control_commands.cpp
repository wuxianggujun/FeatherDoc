#include "featherdoc_cli_content_control_commands.hpp"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_image_output.hpp"
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

void write_json_content_control_summary(
    std::ostream &stream,
    const featherdoc::content_control_summary &content_control) {
    stream << "{\"index\":" << content_control.index << ",\"kind\":";
    write_json_string(stream, content_control_kind_name(content_control.kind));
    stream << ",\"form_kind\":";
    write_json_string(
        stream, content_control_form_kind_name(content_control.form_kind));
    stream << ",\"tag\":";
    write_json_optional_string(stream, content_control.tag);
    stream << ",\"alias\":";
    write_json_optional_string(stream, content_control.alias);
    stream << ",\"id\":";
    write_json_optional_string(stream, content_control.id);
    stream << ",\"lock\":";
    write_json_optional_string(stream, content_control.lock);
    stream << ",\"data_binding_store_item_id\":";
    write_json_optional_string(stream,
                               content_control.data_binding_store_item_id);
    stream << ",\"data_binding_xpath\":";
    write_json_optional_string(stream, content_control.data_binding_xpath);
    stream << ",\"data_binding_prefix_mappings\":";
    write_json_optional_string(stream,
                               content_control.data_binding_prefix_mappings);
    stream << ",\"checked\":";
    write_json_optional_bool(stream, content_control.checked);
    stream << ",\"date_format\":";
    write_json_optional_string(stream, content_control.date_format);
    stream << ",\"date_locale\":";
    write_json_optional_string(stream, content_control.date_locale);
    stream << ",\"selected_list_item\":";
    write_json_optional_size(stream, content_control.selected_list_item);
    stream << ",\"list_items\":[";
    for (std::size_t index = 0; index < content_control.list_items.size();
         ++index) {
        if (index > 0U) {
            stream << ',';
        }
        stream << "{\"display_text\":";
        write_json_string(stream,
                          content_control.list_items[index].display_text);
        stream << ",\"value\":";
        write_json_string(stream, content_control.list_items[index].value);
        stream << '}';
    }
    stream << "],\"showing_placeholder\":"
           << json_bool(content_control.showing_placeholder)
           << ",\"text\":";
    write_json_string(stream, content_control.text);
    stream << '}';
}

void print_content_control_summary(
    std::ostream &stream,
    const featherdoc::content_control_summary &content_control) {
    stream << "index=" << content_control.index
           << " kind=" << content_control_kind_name(content_control.kind)
           << " form_kind="
           << content_control_form_kind_name(content_control.form_kind)
           << " tag=" << optional_display_value(content_control.tag)
           << " alias=" << optional_display_value(content_control.alias)
           << " id=" << optional_display_value(content_control.id)
           << " lock=" << optional_display_value(content_control.lock)
           << " data_binding_store_item_id="
           << optional_display_value(
                  content_control.data_binding_store_item_id)
           << " data_binding_xpath="
           << optional_display_value(content_control.data_binding_xpath)
           << " data_binding_prefix_mappings="
           << optional_display_value(
                  content_control.data_binding_prefix_mappings)
           << " checked="
           << (content_control.checked.has_value()
                   ? yes_no(*content_control.checked)
                   : "-")
           << " date_format=" << optional_display_value(content_control.date_format)
           << " date_locale=" << optional_display_value(content_control.date_locale)
           << " list_items=" << content_control.list_items.size()
           << " showing_placeholder="
           << yes_no(content_control.showing_placeholder) << " text=";
    write_json_string(stream, content_control.text);
}

auto filter_content_controls(
    const std::vector<featherdoc::content_control_summary> &content_controls,
    const inspect_content_controls_options &options)
    -> std::vector<featherdoc::content_control_summary> {
    auto filtered = std::vector<featherdoc::content_control_summary>{};
    filtered.reserve(content_controls.size());
    for (const auto &content_control : content_controls) {
        if (options.tag.has_value() &&
            (!content_control.tag.has_value() ||
             *content_control.tag != *options.tag)) {
            continue;
        }
        if (options.alias.has_value() &&
            (!content_control.alias.has_value() ||
             *content_control.alias != *options.alias)) {
            continue;
        }
        filtered.push_back(content_control);
    }
    return filtered;
}

void inspect_content_controls(
    const selected_template_part &selected,
    const std::vector<featherdoc::content_control_summary> &content_controls,
    const inspect_content_controls_options &options, bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
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
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        if (options.tag.has_value() || options.alias.has_value()) {
            std::cout << ",\"filters\":{\"tag\":";
            write_json_optional_string(std::cout, options.tag);
            std::cout << ",\"alias\":";
            write_json_optional_string(std::cout, options.alias);
            std::cout << '}';
        }
        std::cout << ",\"count\":" << content_controls.size()
                  << ",\"content_controls\":[";
        for (std::size_t index = 0; index < content_controls.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_content_control_summary(std::cout,
                                               content_controls[index]);
        }
        std::cout << "]}\n";
        return;
    }

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
    if (options.tag.has_value()) {
        std::cout << "filter_tag: " << *options.tag << '\n';
    }
    if (options.alias.has_value()) {
        std::cout << "filter_alias: " << *options.alias << '\n';
    }
    std::cout << "content_controls: " << content_controls.size() << '\n';
    for (std::size_t index = 0; index < content_controls.size(); ++index) {
        std::cout << "content_control[" << index << "]: ";
        print_content_control_summary(std::cout, content_controls[index]);
        std::cout << '\n';
    }
}

void write_json_content_control_text_result(
    std::ostream &stream, const selected_template_part &selected,
    const replace_content_control_text_options &options, std::size_t replaced) {
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
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    stream << ",\"selector\":{\"kind\":";
    write_json_string(stream, options.tag.has_value() ? "tag" : "alias");
    stream << ",\"value\":";
    write_json_string(stream, options.tag.has_value() ? *options.tag
                                                 : *options.alias);
    stream << "},\"replaced\":" << replaced << ",\"text\":";
    write_json_string(stream, options.text);
}

void print_content_control_text_result(
    const selected_template_part &selected,
    const replace_content_control_text_options &options,
    const std::optional<path_type> &output_path, std::size_t replaced) {
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
    std::cout << "selector_kind: "
              << (options.tag.has_value() ? "tag" : "alias") << '\n';
    std::cout << "selector_value: "
              << (options.tag.has_value() ? *options.tag : *options.alias)
              << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "text: " << options.text << '\n';
}

void write_json_content_control_selector(
    std::ostream &stream, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias) {
    stream << ",\"selector\":{\"kind\":";
    write_json_string(stream, tag.has_value() ? "tag" : "alias");
    stream << ",\"value\":";
    write_json_string(stream, tag.has_value() ? *tag : *alias);
    stream << '}';
}

void write_json_content_control_part_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::optional<std::string> &tag,
    const std::optional<std::string> &alias) {
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
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selected.reference_kind));
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, std::string(selected.part.entry_name()));
    write_json_content_control_selector(stream, tag, alias);
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

void write_json_content_control_table_result(
    std::ostream &stream, const selected_template_part &selected,
    const content_control_table_replacement_options &options,
    const std::vector<std::vector<std::string>> &rows, std::size_t replaced) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << ",\"replaced\":" << replaced << ",\"row_count\":"
           << rows.size() << ",\"rows\":[";
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index != 0U) {
            stream << ',';
        }
        stream << '[';
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            if (cell_index != 0U) {
                stream << ',';
            }
            write_json_string(stream, rows[row_index][cell_index]);
        }
        stream << ']';
    }
    stream << ']';
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

void write_json_content_control_form_state_result(
    std::ostream &stream, const selected_template_part &selected,
    const set_content_control_form_state_options &options, std::size_t updated) {
    write_json_content_control_part_result(stream, selected, options.tag,
                                           options.alias);
    stream << R"(,"updated":)" << updated << R"(,"form_state":)";
    write_json_content_control_form_state_options(stream, options.state);
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

void print_content_control_common_result(
    const selected_template_part &selected, const std::optional<std::string> &tag,
    const std::optional<std::string> &alias,
    const std::optional<path_type> &output_path, std::size_t replaced) {
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
    std::cout << "selector_kind: " << (tag.has_value() ? "tag" : "alias")
              << '\n';
    std::cout << "selector_value: " << (tag.has_value() ? *tag : *alias)
              << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
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

void print_content_control_table_result(
    const selected_template_part &selected,
    const content_control_table_replacement_options &options,
    const std::vector<std::vector<std::string>> &rows, std::size_t replaced) {
    print_content_control_common_result(selected, options.tag, options.alias,
                                        options.output_path, replaced);
    std::cout << "row_count: " << rows.size() << '\n';
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        std::cout << "row[" << row_index << "]:";
        for (std::size_t cell_index = 0; cell_index < rows[row_index].size();
             ++cell_index) {
            std::cout << (cell_index == 0U ? " " : " | ")
                      << rows[row_index][cell_index];
        }
        std::cout << '\n';
    }
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

auto resolve_bookmark_table_row_sources(
    const content_control_table_replacement_options &options,
    std::vector<std::vector<std::string>> &rows, std::string &error_message)
    -> bool {
    rows.clear();
    rows.reserve(options.row_sources.size());

    for (const auto &row_sources : options.row_sources) {
        std::vector<std::string> row;
        row.reserve(row_sources.size());

        for (const auto &source : row_sources) {
            std::string text;
            if (!read_text_source(source, text, error_message)) {
                return false;
            }
            row.push_back(std::move(text));
        }

        rows.push_back(std::move(row));
    }

    return true;
}

} // namespace

auto is_content_control_command(std::string_view command) -> bool {
    return command == "inspect-content-controls" ||
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
    if (command == "inspect-content-controls") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(
                command, "inspect-content-controls expects an input path",
                json_output);
            return 2;
        }

        inspect_content_controls_options options;
        std::string error_message;
        if (!parse_inspect_content_controls_options(arguments, 2U, options,
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
            report_operation_failure(command, "inspect", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto content_controls = selected.part.list_content_controls();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        const auto filtered_content_controls =
            filter_content_controls(content_controls, options);
        inspect_content_controls(selected, filtered_content_controls, options,
                                 options.json_output);
        return 0;
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
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              std::string(command) + " expects an input path",
                              json_output);
            return 2;
        }

        content_control_table_replacement_options options;
        std::string error_message;
        const bool allow_empty_rows =
            command == "replace-content-control-table-rows";
        if (!parse_content_control_table_replacement_options(
                arguments, 2U, options, allow_empty_rows, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::vector<std::vector<std::string>> rows;
        if (!resolve_bookmark_table_row_sources(options, rows, error_message)) {
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
            command == "replace-content-control-table"
                ? (options.tag.has_value()
                       ? selected.part.replace_content_control_with_table_by_tag(
                             *options.tag, rows)
                       : selected.part.replace_content_control_with_table_by_alias(
                             *options.alias, rows))
                : (options.tag.has_value()
                       ? selected.part.replace_content_control_with_table_rows_by_tag(
                             *options.tag, rows)
                       : selected.part.replace_content_control_with_table_rows_by_alias(
                             *options.alias, rows));
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
                [&selected, &options, &rows,
                 replaced](std::ostream &stream) {
                    write_json_content_control_table_result(
                        stream, selected, options, rows, replaced);
                });
        } else {
            print_content_control_table_result(selected, options, rows, replaced);
        }

        return 0;
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
