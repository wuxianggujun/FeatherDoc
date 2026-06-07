#include "featherdoc_cli_numbering_inspect_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_inspect_numbering_options_parse.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_json.hpp"
#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_numbering_definition_summary(
    std::ostream &stream,
    const featherdoc::numbering_definition_summary &definition) {
    stream << "{\"definition_id\":" << definition.definition_id << ",\"name\":";
    write_json_string(stream, definition.name);
    stream << ",\"levels\":[";
    for (std::size_t index = 0; index < definition.levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream, definition.levels[index]);
    }
    stream << "],\"instance_ids\":[";
    for (std::size_t index = 0; index < definition.instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << definition.instance_ids[index];
    }
    stream << "],\"instances\":[";
    for (std::size_t index = 0; index < definition.instances.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_instance_summary(stream, definition.instances[index]);
    }
    stream << "]}";
}

void print_numbering_instance_ids(
    std::ostream &stream, const std::vector<std::uint32_t> &instance_ids) {
    if (instance_ids.empty()) {
        stream << "none";
        return;
    }

    for (std::size_t index = 0; index < instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << instance_ids[index];
    }
}

void print_numbering_definition_summary(
    std::ostream &stream,
    const featherdoc::numbering_definition_summary &definition) {
    stream << "id=" << definition.definition_id << " name=" << definition.name
           << " levels=" << definition.levels.size() << " instances=";
    print_numbering_instance_ids(stream, definition.instance_ids);
}

void print_numbering_level_override_summary(
    std::ostream &stream,
    const featherdoc::numbering_level_override_summary &level_override) {
    stream << "level=" << level_override.level << " start_override=";
    if (level_override.start_override.has_value()) {
        stream << *level_override.start_override;
    } else {
        stream << "none";
    }

    if (level_override.level_definition.has_value()) {
        stream << " definition_kind="
               << list_kind_name(level_override.level_definition->kind)
               << " definition_start=" << level_override.level_definition->start
               << " definition_text_pattern="
               << level_override.level_definition->text_pattern;
    }
}

void print_numbering_instance_summary(
    std::ostream &stream, const featherdoc::numbering_instance_summary &instance,
    std::string_view indent = {}) {
    stream << indent << "instance[" << instance.instance_id
           << "]: override_count=" << instance.level_overrides.size() << '\n';
    for (std::size_t index = 0; index < instance.level_overrides.size(); ++index) {
        stream << indent << "  override[" << index << "]: ";
        print_numbering_level_override_summary(stream,
                                               instance.level_overrides[index]);
        stream << '\n';
    }
}

void inspect_numbering(
    const std::vector<featherdoc::numbering_definition_summary> &definitions,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << definitions.size()
                  << ",\"definitions\":[";
        for (std::size_t index = 0; index < definitions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_definition_summary(std::cout, definitions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "definitions: " << definitions.size() << '\n';
    for (std::size_t index = 0; index < definitions.size(); ++index) {
        std::cout << "definition[" << index << "]: ";
        print_numbering_definition_summary(std::cout, definitions[index]);
        std::cout << '\n';
        for (const auto &level : definitions[index].levels) {
            std::cout << "  level[" << level.level << "]: kind="
                      << list_kind_name(level.kind) << " start=" << level.start
                      << " text_pattern=" << level.text_pattern << '\n';
        }
        for (const auto &instance : definitions[index].instances) {
            print_numbering_instance_summary(std::cout, instance, "  ");
        }
    }
}

void inspect_numbering(
    const featherdoc::numbering_definition_summary &definition,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"definition\":";
        write_json_numbering_definition_summary(std::cout, definition);
        std::cout << "}\n";
        return;
    }

    std::cout << "definition_id: " << definition.definition_id << '\n';
    std::cout << "name: " << definition.name << '\n';
    std::cout << "instances: ";
    print_numbering_instance_ids(std::cout, definition.instance_ids);
    std::cout << '\n';
    std::cout << "levels: " << definition.levels.size() << '\n';
    for (const auto &level : definition.levels) {
        std::cout << "level[" << level.level
                  << "]: kind=" << list_kind_name(level.kind)
                  << " start=" << level.start
                  << " text_pattern=" << level.text_pattern << '\n';
    }
    std::cout << "instance_count: " << definition.instances.size() << '\n';
    for (const auto &instance : definition.instances) {
        print_numbering_instance_summary(std::cout, instance);
    }
}

void inspect_numbering(
    const featherdoc::numbering_instance_lookup_summary &instance_lookup,
    bool json_output) {
    if (json_output) {
        std::cout << "{\"instance\":{\"definition_id\":"
                  << instance_lookup.definition_id
                  << ",\"definition_name\":";
        write_json_string(std::cout, instance_lookup.definition_name);
        std::cout << ",\"instance\":";
        write_json_numbering_instance_summary(std::cout,
                                              instance_lookup.instance);
        std::cout << "}}\n";
        return;
    }

    std::cout << "definition_id: " << instance_lookup.definition_id << '\n';
    std::cout << "name: " << instance_lookup.definition_name << '\n';
    print_numbering_instance_summary(std::cout, instance_lookup.instance);
}

auto run_inspect_numbering_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    const auto json_output = has_json_flag(arguments);
    if (arguments.size() < 2U) {
        print_parse_error(command, "inspect-numbering expects an input path",
                          json_output);
        return 2;
    }

    inspect_numbering_options options;
    std::string error_message;
    if (!parse_inspect_numbering_options(arguments, 2U, options,
                                         error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    if (!open_document(path_type(std::string(arguments[1])), doc, command,
                       options.json_output)) {
        return 1;
    }

    if (options.instance_id.has_value()) {
        const auto instance = doc.find_numbering_instance(*options.instance_id);
        if (!instance.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (options.definition_id.has_value() &&
            instance->definition_id != *options.definition_id) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::invalid_argument);
            error_info.detail =
                "numbering instance id '" +
                std::to_string(*options.instance_id) +
                "' was not found under definition id '" +
                std::to_string(*options.definition_id) +
                "' in word/numbering.xml";
            error_info.entry_name = "word/numbering.xml";
            report_operation_failure(command, "inspect",
                                     "numbering instance was not found",
                                     error_info, options.json_output);
            return 1;
        }

        inspect_numbering(*instance, options.json_output);
        return 0;
    }

    if (options.definition_id.has_value()) {
        const auto definition =
            doc.find_numbering_definition(*options.definition_id);
        if (!definition.has_value()) {
            report_document_error(command, "inspect", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        inspect_numbering(*definition, options.json_output);
        return 0;
    }

    const auto definitions = doc.list_numbering_definitions();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        report_document_error(command, "inspect", error_info,
                              options.json_output);
        return 1;
    }

    inspect_numbering(definitions, options.json_output);
    return 0;
}

} // namespace

auto is_numbering_inspect_command(std::string_view command) -> bool {
    return command == "inspect-numbering";
}

auto run_numbering_inspect_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "inspect-numbering") {
        return run_inspect_numbering_command(command, arguments, doc);
    }

    print_parse_error(command,
                      "unsupported numbering inspect command: " +
                          std::string(command),
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
