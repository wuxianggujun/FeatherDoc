#include "featherdoc_cli_numbering_catalog_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_catalog_output_json.hpp"
#include "featherdoc_cli_numbering_json.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>

namespace featherdoc_cli {
namespace {

void write_json_numbering_catalog_changed_level(
    std::ostream &stream, const changed_numbering_catalog_level &changed_level) {
    stream << "{\"left\":";
    write_json_numbering_level_definition(stream, changed_level.left);
    stream << ",\"right\":";
    write_json_numbering_level_definition(stream, changed_level.right);
    stream << '}';
}

void write_json_numbering_catalog_changed_override(
    std::ostream &stream,
    const changed_numbering_catalog_override &changed_override) {
    stream << "{\"left\":";
    write_json_numbering_level_override_summary(stream, changed_override.left);
    stream << ",\"right\":";
    write_json_numbering_level_override_summary(stream, changed_override.right);
    stream << '}';
}

void write_json_numbering_catalog_instance_diff(
    std::ostream &stream, const numbering_catalog_instance_diff_result &diff) {
    stream << "{\"instance_index\":" << diff.instance_index
           << ",\"added_override_count\":" << diff.added_overrides.size()
           << ",\"removed_override_count\":" << diff.removed_overrides.size()
           << ",\"changed_override_count\":" << diff.changed_overrides.size()
           << ",\"added_overrides\":[";
    for (std::size_t index = 0U; index < diff.added_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_override_summary(stream,
                                                    diff.added_overrides[index]);
    }
    stream << "],\"removed_overrides\":[";
    for (std::size_t index = 0U; index < diff.removed_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_override_summary(
            stream, diff.removed_overrides[index]);
    }
    stream << "],\"changed_overrides\":[";
    for (std::size_t index = 0U; index < diff.changed_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_changed_override(
            stream, diff.changed_overrides[index]);
    }
    stream << "]}";
}

void write_json_numbering_catalog_changed_definition(
    std::ostream &stream,
    const changed_numbering_catalog_definition &definition_diff) {
    stream << "{\"name\":";
    write_json_string(stream, definition_diff.name);
    stream << ",\"added_level_count\":" << definition_diff.added_levels.size()
           << ",\"removed_level_count\":" << definition_diff.removed_levels.size()
           << ",\"changed_level_count\":" << definition_diff.changed_levels.size()
           << ",\"added_instance_count\":"
           << definition_diff.added_instances.size()
           << ",\"removed_instance_count\":"
           << definition_diff.removed_instances.size()
           << ",\"changed_instance_count\":"
           << definition_diff.changed_instances.size()
           << ",\"added_levels\":[";
    for (std::size_t index = 0U; index < definition_diff.added_levels.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream,
                                              definition_diff.added_levels[index]);
    }
    stream << "],\"removed_levels\":[";
    for (std::size_t index = 0U; index < definition_diff.removed_levels.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(
            stream, definition_diff.removed_levels[index]);
    }
    stream << "],\"changed_levels\":[";
    for (std::size_t index = 0U; index < definition_diff.changed_levels.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_changed_level(
            stream, definition_diff.changed_levels[index]);
    }
    stream << "],\"added_instances\":[";
    for (std::size_t index = 0U; index < definition_diff.added_instances.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_instance_summary(
            stream, definition_diff.added_instances[index]);
    }
    stream << "],\"removed_instances\":[";
    for (std::size_t index = 0U; index < definition_diff.removed_instances.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_instance_summary(
            stream, definition_diff.removed_instances[index]);
    }
    stream << "],\"changed_instances\":[";
    for (std::size_t index = 0U; index < definition_diff.changed_instances.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_instance_diff(
            stream, definition_diff.changed_instances[index]);
    }
    stream << "]}";
}

void write_json_numbering_catalog_diff_result(
    std::ostream &stream, const numbering_catalog_diff_result &result) {
    stream << "{\"equal\":" << json_bool(result.equal())
           << ",\"added_definition_count\":" << result.added_definitions.size()
           << ",\"removed_definition_count\":"
           << result.removed_definitions.size()
           << ",\"changed_definition_count\":"
           << result.changed_definitions.size() << ",\"added_definitions\":[";
    for (std::size_t index = 0U; index < result.added_definitions.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_definition(stream,
                                                result.added_definitions[index]);
    }
    stream << "],\"removed_definitions\":[";
    for (std::size_t index = 0U; index < result.removed_definitions.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_definition(
            stream, result.removed_definitions[index]);
    }
    stream << "],\"changed_definitions\":[";
    for (std::size_t index = 0U; index < result.changed_definitions.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_changed_definition(
            stream, result.changed_definitions[index]);
    }
    stream << "]}\n";
}

} // namespace

void print_numbering_catalog_diff_result(
    const numbering_catalog_diff_result &result, bool json_output) {
    if (json_output) {
        write_json_numbering_catalog_diff_result(std::cout, result);
        return;
    }

    std::cout << "equal: " << yes_no(result.equal()) << '\n'
              << "added_definition_count: " << result.added_definitions.size()
              << '\n'
              << "removed_definition_count: "
              << result.removed_definitions.size() << '\n'
              << "changed_definition_count: "
              << result.changed_definitions.size() << '\n';
}

void print_checked_numbering_catalog_result(
    const path_type &catalog_path,
    const numbering_catalog_lint_result &baseline_lint,
    const numbering_catalog_lint_result &generated_lint,
    const numbering_catalog_diff_result &diff,
    const std::optional<path_type> &output_path, bool json_output) {
    const auto clean = baseline_lint.clean() && generated_lint.clean();
    if (json_output) {
        std::cout << "{\"command\":\"check-numbering-catalog\","
                  << "\"matches\":" << json_bool(diff.equal())
                  << ",\"clean\":" << json_bool(clean)
                  << ",\"catalog_file\":";
        write_json_string(std::cout, catalog_path.string());
        if (output_path.has_value()) {
            std::cout << ",\"generated_output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"baseline_issue_count\":" << baseline_lint.issues.size()
                  << ",\"generated_issue_count\":"
                  << generated_lint.issues.size()
                  << ",\"added_definition_count\":"
                  << diff.added_definitions.size()
                  << ",\"removed_definition_count\":"
                  << diff.removed_definitions.size()
                  << ",\"changed_definition_count\":"
                  << diff.changed_definitions.size()
                  << ",\"baseline_issues\":[";
        write_json_numbering_catalog_lint_issues(std::cout,
                                                 baseline_lint.issues);
        std::cout << "],\"generated_issues\":[";
        write_json_numbering_catalog_lint_issues(std::cout,
                                                 generated_lint.issues);
        std::cout << "],\"added_definitions\":[";
        for (std::size_t index = 0U; index < diff.added_definitions.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_catalog_definition(
                std::cout, diff.added_definitions[index]);
        }
        std::cout << "],\"removed_definitions\":[";
        for (std::size_t index = 0U; index < diff.removed_definitions.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_catalog_definition(
                std::cout, diff.removed_definitions[index]);
        }
        std::cout << "],\"changed_definitions\":[";
        for (std::size_t index = 0U; index < diff.changed_definitions.size();
             ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_catalog_changed_definition(
                std::cout, diff.changed_definitions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "matches: " << yes_no(diff.equal()) << '\n'
              << "clean: " << yes_no(clean) << '\n'
              << "catalog_file: " << catalog_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "generated_output_path: " << output_path->string()
                  << '\n';
    }
    std::cout << "baseline_issue_count: " << baseline_lint.issues.size()
              << '\n'
              << "generated_issue_count: " << generated_lint.issues.size()
              << '\n'
              << "added_definition_count: " << diff.added_definitions.size()
              << '\n'
              << "removed_definition_count: " << diff.removed_definitions.size()
              << '\n'
              << "changed_definition_count: " << diff.changed_definitions.size()
              << '\n';
}

} // namespace featherdoc_cli
