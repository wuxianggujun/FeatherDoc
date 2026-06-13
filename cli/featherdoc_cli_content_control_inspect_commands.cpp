#include "featherdoc_cli_content_control_inspect_commands.hpp"

#include "featherdoc_cli_bookmark_text_options_parse.hpp"
#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
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

} // namespace

auto is_content_control_inspect_command(std::string_view command) -> bool {
    return command == "inspect-content-controls";
}

auto run_content_control_inspect_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command != "inspect-content-controls") {
        print_parse_error(command, "unsupported content control inspect command",
                          has_json_flag(arguments));
        return 2;
    }

    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-content-controls expects an input path",
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

} // namespace featherdoc_cli
