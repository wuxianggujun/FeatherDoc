#include "featherdoc_cli_numbering_catalog_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_numbering_catalog_output_json.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>

namespace featherdoc_cli {

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

} // namespace featherdoc_cli
