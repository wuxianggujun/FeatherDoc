#include "featherdoc_cli_numbering_catalog_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_json.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>

namespace featherdoc_cli {
namespace {

auto numbering_catalog_instance_count(
    const featherdoc::numbering_catalog &catalog) -> std::size_t {
    std::size_t count = 0U;
    for (const auto &definition : catalog.definitions) {
        count += definition.instances.size();
    }
    return count;
}

void write_json_numbering_catalog_definition(
    std::ostream &stream,
    const featherdoc::numbering_catalog_definition &definition) {
    stream << "{\"name\":";
    write_json_string(stream, definition.definition.name);
    stream << ",\"levels\":[";
    for (std::size_t index = 0; index < definition.definition.levels.size();
         ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream,
                                              definition.definition.levels[index]);
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

void write_json_numbering_catalog_patch_summary(
    std::ostream &stream, const numbering_catalog_patch_summary &summary) {
    stream << "\"upserted_level_count\":" << summary.upserted_level_count
           << ",\"upserted_override_count\":" << summary.upserted_override_count
           << ",\"removed_override_count\":" << summary.removed_override_count
           << ",\"missing_override_count\":" << summary.missing_override_count;
}

void write_json_numbering_catalog_lint_issue(
    std::ostream &stream, const numbering_catalog_lint_issue &issue) {
    stream << "{\"issue\":";
    write_json_string(stream, numbering_catalog_lint_issue_name(issue.kind));
    stream << ",\"definition_index\":" << issue.definition_index
           << ",\"definition_name\":";
    write_json_string(stream, issue.definition_name);
    if (issue.instance_index.has_value()) {
        stream << ",\"instance_index\":" << *issue.instance_index;
    }
    if (issue.instance_id.has_value()) {
        stream << ",\"instance_id\":" << *issue.instance_id;
    }
    if (issue.level_index.has_value()) {
        stream << ",\"level_index\":" << *issue.level_index;
    }
    if (issue.override_index.has_value()) {
        stream << ",\"override_index\":" << *issue.override_index;
    }
    if (issue.level.has_value()) {
        stream << ",\"level\":" << *issue.level;
    }
    stream << ",\"detail\":";
    write_json_string(stream, issue.detail);
    stream << '}';
}

void write_json_numbering_catalog_lint_issues(
    std::ostream &stream,
    const std::vector<numbering_catalog_lint_issue> &issues) {
    for (std::size_t index = 0U; index < issues.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_lint_issue(stream, issues[index]);
    }
}

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

void write_json_numbering_catalog(std::ostream &stream,
                                  const featherdoc::numbering_catalog &catalog) {
    stream << "{\"definition_count\":" << catalog.definitions.size()
           << ",\"instance_count\":" << numbering_catalog_instance_count(catalog)
           << ",\"definitions\":[";
    for (std::size_t index = 0; index < catalog.definitions.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_catalog_definition(stream, catalog.definitions[index]);
    }
    stream << "]}";
}

auto write_numbering_catalog_file(const path_type &output_path,
                                  const featherdoc::numbering_catalog &catalog,
                                  std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open numbering catalog output path: " + output_path.string();
        return false;
    }

    write_json_numbering_catalog(stream, catalog);
    stream << '\n';
    if (!stream.good()) {
        error_message =
            "failed to write numbering catalog output path: " + output_path.string();
        return false;
    }

    return true;
}

void print_exported_numbering_catalog_summary(
    const featherdoc::numbering_catalog &catalog,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"export-numbering-catalog\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"definition_count\":" << catalog.definitions.size()
                  << ",\"instance_count\":"
                  << numbering_catalog_instance_count(catalog) << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "definition_count: " << catalog.definitions.size() << '\n'
              << "instance_count: " << numbering_catalog_instance_count(catalog)
              << '\n';
}

void print_patched_numbering_catalog_summary(
    const featherdoc::numbering_catalog &catalog,
    const numbering_catalog_patch_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"patch-numbering-catalog\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"definition_count\":" << catalog.definitions.size()
                  << ",\"instance_count\":"
                  << numbering_catalog_instance_count(catalog) << ',';
        write_json_numbering_catalog_patch_summary(std::cout, summary);
        std::cout << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "definition_count: " << catalog.definitions.size() << '\n'
              << "instance_count: " << numbering_catalog_instance_count(catalog)
              << '\n'
              << "upserted_level_count: " << summary.upserted_level_count
              << '\n'
              << "upserted_override_count: " << summary.upserted_override_count
              << '\n'
              << "removed_override_count: " << summary.removed_override_count
              << '\n'
              << "missing_override_count: " << summary.missing_override_count
              << '\n';
}

void print_linted_numbering_catalog_result(
    const numbering_catalog_lint_result &result, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"lint-numbering-catalog\",\"ok\":true,"
                  << "\"clean\":" << json_bool(result.clean())
                  << ",\"definition_count\":" << result.definition_count
                  << ",\"instance_count\":" << result.instance_count
                  << ",\"level_count\":" << result.level_count
                  << ",\"override_count\":" << result.override_count
                  << ",\"issue_count\":" << result.issues.size()
                  << ",\"issues\":[";
        write_json_numbering_catalog_lint_issues(std::cout, result.issues);
        std::cout << "]}\n";
        return;
    }

    std::cout << "clean: " << yes_no(result.clean()) << '\n'
              << "definition_count: " << result.definition_count << '\n'
              << "instance_count: " << result.instance_count << '\n'
              << "level_count: " << result.level_count << '\n'
              << "override_count: " << result.override_count << '\n'
              << "issue_count: " << result.issues.size() << '\n';
    if (result.issues.empty()) {
        std::cout << "issues: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.issues.size(); ++index) {
        const auto &issue = result.issues[index];
        std::cout << "issue[" << index << "]: issue="
                  << numbering_catalog_lint_issue_name(issue.kind)
                  << " definition_index=" << issue.definition_index
                  << " definition_name=" << issue.definition_name;
        if (issue.instance_index.has_value()) {
            std::cout << " instance_index=" << *issue.instance_index;
        }
        if (issue.instance_id.has_value()) {
            std::cout << " instance_id=" << *issue.instance_id;
        }
        if (issue.level_index.has_value()) {
            std::cout << " level_index=" << *issue.level_index;
        }
        if (issue.override_index.has_value()) {
            std::cout << " override_index=" << *issue.override_index;
        }
        if (issue.level.has_value()) {
            std::cout << " level=" << *issue.level;
        }
        std::cout << " detail=" << issue.detail << '\n';
    }
}

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
