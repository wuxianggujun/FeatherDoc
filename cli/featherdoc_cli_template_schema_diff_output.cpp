#include "featherdoc_cli_template_schema_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_template_schema_target_output.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>

namespace featherdoc_cli {
namespace {

void write_json_template_schema_diff_result(
    std::ostream &stream, const template_schema_diff_result &result) {
    stream << "{\"equal\":" << json_bool(result.equal())
           << ",\"added_target_count\":" << result.added_targets.size()
           << ",\"removed_target_count\":" << result.removed_targets.size()
           << ",\"changed_target_count\":" << result.changed_targets.size()
           << ",\"added_targets\":[";
    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.added_targets[index]);
    }
    stream << "],\"removed_targets\":[";
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.removed_targets[index]);
    }
    stream << "],\"changed_targets\":[";
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"left\":";
        write_json_exported_template_schema_target(stream,
                                                   result.changed_targets[index].left);
        stream << ",\"right\":";
        write_json_exported_template_schema_target(stream,
                                                   result.changed_targets[index].right);
        stream << '}';
    }
    stream << "]}\n";
}

} // namespace

void print_template_schema_diff_result(const template_schema_diff_result &result,
                                       bool json_output) {
    if (json_output) {
        write_json_template_schema_diff_result(std::cout, result);
        return;
    }

    std::cout << "equal: " << yes_no(result.equal()) << '\n'
              << "added_target_count: " << result.added_targets.size() << '\n'
              << "removed_target_count: " << result.removed_targets.size() << '\n'
              << "changed_target_count: " << result.changed_targets.size() << '\n';

    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        std::cout << '\n' << "added_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.added_targets[index]);
    }
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        std::cout << '\n' << "removed_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.removed_targets[index]);
    }
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        std::cout << '\n' << "changed_target[" << index << "].left\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].left);
        std::cout << '\n' << "changed_target[" << index << "].right\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].right);
    }
}

void print_checked_template_schema_result(
    const path_type &schema_path, const template_schema_diff_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"check-template-schema\",\"matches\":"
                  << json_bool(result.equal()) << ",\"schema_file\":";
        write_json_string(std::cout, schema_path.string());
        if (output_path.has_value()) {
            std::cout << ",\"generated_output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"added_target_count\":" << result.added_targets.size()
                  << ",\"removed_target_count\":" << result.removed_targets.size()
                  << ",\"changed_target_count\":" << result.changed_targets.size()
                  << ",\"added_targets\":[";
        for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_target(std::cout,
                                                       result.added_targets[index]);
        }
        std::cout << "],\"removed_targets\":[";
        for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_target(
                std::cout, result.removed_targets[index]);
        }
        std::cout << "],\"changed_targets\":[";
        for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            std::cout << "{\"left\":";
            write_json_exported_template_schema_target(
                std::cout, result.changed_targets[index].left);
            std::cout << ",\"right\":";
            write_json_exported_template_schema_target(
                std::cout, result.changed_targets[index].right);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "schema_file: " << schema_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "generated_output_path: " << output_path->string() << '\n';
    }
    std::cout << "matches: " << yes_no(result.equal()) << '\n'
              << "added_target_count: " << result.added_targets.size() << '\n'
              << "removed_target_count: " << result.removed_targets.size() << '\n'
              << "changed_target_count: " << result.changed_targets.size() << '\n';

    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        std::cout << '\n' << "added_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.added_targets[index]);
    }
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        std::cout << '\n' << "removed_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.removed_targets[index]);
    }
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        std::cout << '\n' << "changed_target[" << index << "].baseline\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].left);
        std::cout << '\n' << "changed_target[" << index << "].generated\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].right);
    }
}

} // namespace featherdoc_cli
