#include "featherdoc_cli_template_schema_output.hpp"

#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_template_schema_ops.hpp"
#include "featherdoc_cli_template_schema_output_detail.hpp"
#include "featherdoc_cli_text.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <iostream>
#include <ostream>

namespace featherdoc_cli {

void print_template_schema_patch_selector_summary(
    std::ostream &stream, const template_schema_patch_target_selector &selector) {
    stream << "part=" << validation_part_name(selector.part);
    if (selector.part_index.has_value()) {
        stream << " index=" << *selector.part_index;
    }
    if (selector.section_index.has_value()) {
        stream << " section=" << *selector.section_index;
    }
    if (selector.reference_kind.has_value()) {
        stream << " kind="
               << featherdoc::to_xml_reference_type(*selector.reference_kind);
    }
    if (selector.resolved_from_section_index.has_value()) {
        stream << " resolved_from_section="
               << *selector.resolved_from_section_index;
    }
    if (selector.linked_to_previous) {
        stream << " linked_to_previous=yes";
    }
}

void print_linted_template_schema_result(const template_schema_lint_result &result,
                                         bool json_output) {
    const auto duplicate_target_count =
        result.issue_count(template_schema_lint_issue_kind::duplicate_target_identity);
    const auto duplicate_slot_count =
        result.issue_count(template_schema_lint_issue_kind::duplicate_slot_name);
    const auto target_order_issue_count =
        result.issue_count(template_schema_lint_issue_kind::target_order);
    const auto slot_order_issue_count =
        result.issue_count(template_schema_lint_issue_kind::slot_order);
    const auto entry_name_issue_count =
        result.issue_count(template_schema_lint_issue_kind::entry_name_present);

    if (json_output) {
        std::cout << "{\"command\":\"lint-template-schema\",\"ok\":true,"
                  << "\"clean\":" << json_bool(result.clean())
                  << ",\"target_count\":" << result.target_count
                  << ",\"slot_count\":" << result.slot_count
                  << ",\"issue_count\":" << result.issues.size()
                  << ",\"duplicate_target_count\":" << duplicate_target_count
                  << ",\"duplicate_slot_count\":" << duplicate_slot_count
                  << ",\"target_order_issue_count\":" << target_order_issue_count
                  << ",\"slot_order_issue_count\":" << slot_order_issue_count
                  << ",\"entry_name_issue_count\":" << entry_name_issue_count
                  << ",\"issues\":[";
        for (std::size_t index = 0U; index < result.issues.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_schema_lint_issue(std::cout, result.issues[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "clean: " << yes_no(result.clean()) << '\n'
              << "target_count: " << result.target_count << '\n'
              << "slot_count: " << result.slot_count << '\n'
              << "issue_count: " << result.issues.size() << '\n'
              << "duplicate_target_count: " << duplicate_target_count << '\n'
              << "duplicate_slot_count: " << duplicate_slot_count << '\n'
              << "target_order_issue_count: " << target_order_issue_count << '\n'
              << "slot_order_issue_count: " << slot_order_issue_count << '\n'
              << "entry_name_issue_count: " << entry_name_issue_count << '\n';
    if (result.issues.empty()) {
        std::cout << "issues: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.issues.size(); ++index) {
        const auto &issue = result.issues[index];
        std::cout << "issue[" << index << "]: issue="
                  << template_schema_lint_issue_name(issue.kind)
                  << " target_index=" << issue.target_index << ' ';
        print_template_schema_patch_selector_summary(std::cout, issue.target);
        if (issue.previous_target_index.has_value()) {
            std::cout << " previous_target_index=" << *issue.previous_target_index;
        }
        if (issue.slot_index.has_value()) {
            std::cout << " slot_index=" << *issue.slot_index;
        }
        if (issue.previous_slot_index.has_value()) {
            std::cout << " previous_slot_index=" << *issue.previous_slot_index;
        }
        if (!issue.bookmark_name.empty()) {
            std::cout << " bookmark=" << issue.bookmark_name;
        }
        if (!issue.previous_bookmark_name.empty()) {
            std::cout << " previous_bookmark=" << issue.previous_bookmark_name;
        }
        if (!issue.entry_name.empty()) {
            std::cout << " entry_name=" << issue.entry_name;
        }
        std::cout << '\n';
    }
}

} // namespace featherdoc_cli
