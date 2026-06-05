#include "featherdoc_cli_template_schema_ops.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

namespace featherdoc_cli {

auto compare_optional_index(const std::optional<std::size_t> &left,
                            const std::optional<std::size_t> &right) -> int {
    if (left.has_value() != right.has_value()) {
        return left.has_value() ? 1 : -1;
    }
    if (!left.has_value()) {
        return 0;
    }
    if (*left < *right) {
        return -1;
    }
    if (*right < *left) {
        return 1;
    }
    return 0;
}

auto compare_optional_reference_kind(
    const std::optional<featherdoc::section_reference_kind> &left,
    const std::optional<featherdoc::section_reference_kind> &right) -> int {
    if (left.has_value() != right.has_value()) {
        return left.has_value() ? 1 : -1;
    }
    if (!left.has_value()) {
        return 0;
    }
    if (static_cast<int>(*left) < static_cast<int>(*right)) {
        return -1;
    }
    if (static_cast<int>(*right) < static_cast<int>(*left)) {
        return 1;
    }
    return 0;
}

auto compare_template_slot_requirement(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right) -> int {
    if (static_cast<int>(left.source) < static_cast<int>(right.source)) {
        return -1;
    }
    if (static_cast<int>(right.source) < static_cast<int>(left.source)) {
        return 1;
    }
    if (left.bookmark_name < right.bookmark_name) {
        return -1;
    }
    if (right.bookmark_name < left.bookmark_name) {
        return 1;
    }
    if (static_cast<int>(left.kind) < static_cast<int>(right.kind)) {
        return -1;
    }
    if (static_cast<int>(right.kind) < static_cast<int>(left.kind)) {
        return 1;
    }
    if (left.required != right.required) {
        return left.required ? 1 : -1;
    }
    if (const auto min_compare =
            compare_optional_index(left.min_occurrences, right.min_occurrences);
        min_compare != 0) {
        return min_compare;
    }
    return compare_optional_index(left.max_occurrences, right.max_occurrences);
}

auto make_template_schema_slot_update(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right)
    -> featherdoc::template_schema_slot_update {
    featherdoc::template_schema_slot_update update{};
    if (left.kind != right.kind) {
        update.kind = right.kind;
    }
    if (left.required != right.required) {
        update.required = right.required;
    }
    if (left.min_occurrences != right.min_occurrences) {
        if (right.min_occurrences.has_value()) {
            update.min_occurrences = *right.min_occurrences;
        } else {
            update.clear_min_occurrences = true;
        }
    }
    if (left.max_occurrences != right.max_occurrences) {
        if (right.max_occurrences.has_value()) {
            update.max_occurrences = *right.max_occurrences;
        } else {
            update.clear_max_occurrences = true;
        }
    }
    return update;
}

auto compare_template_slot_requirement_shape(
    const featherdoc::template_slot_requirement &left,
    const featherdoc::template_slot_requirement &right) -> int {
    if (static_cast<int>(left.source) < static_cast<int>(right.source)) {
        return -1;
    }
    if (static_cast<int>(right.source) < static_cast<int>(left.source)) {
        return 1;
    }
    if (static_cast<int>(left.kind) < static_cast<int>(right.kind)) {
        return -1;
    }
    if (static_cast<int>(right.kind) < static_cast<int>(left.kind)) {
        return 1;
    }
    if (left.required != right.required) {
        return left.required ? 1 : -1;
    }
    if (const auto min_compare =
            compare_optional_index(left.min_occurrences, right.min_occurrences);
        min_compare != 0) {
        return min_compare;
    }
    return compare_optional_index(left.max_occurrences, right.max_occurrences);
}

auto compare_template_schema_target_selector(
    const exported_template_schema_target &left,
    const exported_template_schema_target &right) -> int {
    if (static_cast<int>(left.part) < static_cast<int>(right.part)) {
        return -1;
    }
    if (static_cast<int>(right.part) < static_cast<int>(left.part)) {
        return 1;
    }
    if (const auto part_index_compare =
            compare_optional_index(left.part_index, right.part_index);
        part_index_compare != 0) {
        return part_index_compare;
    }
    if (const auto section_index_compare =
            compare_optional_index(left.section_index, right.section_index);
        section_index_compare != 0) {
        return section_index_compare;
    }
    return compare_optional_reference_kind(left.reference_kind, right.reference_kind);
}

auto compare_template_schema_target(const exported_template_schema_target &left,
                                    const exported_template_schema_target &right)
    -> int {
    if (const auto selector_compare =
            compare_template_schema_target_selector(left, right);
        selector_compare != 0) {
        return selector_compare;
    }
    if (const auto resolved_compare = compare_optional_index(
            left.resolved_from_section_index, right.resolved_from_section_index);
        resolved_compare != 0) {
        return resolved_compare;
    }
    if (left.linked_to_previous != right.linked_to_previous) {
        return left.linked_to_previous ? 1 : -1;
    }

    const auto shared_size =
        std::min(left.requirements.size(), right.requirements.size());
    for (std::size_t index = 0U; index < shared_size; ++index) {
        if (const auto requirement_compare = compare_template_slot_requirement(
                left.requirements[index], right.requirements[index]);
            requirement_compare != 0) {
            return requirement_compare;
        }
    }
    if (left.requirements.size() < right.requirements.size()) {
        return -1;
    }
    if (right.requirements.size() < left.requirements.size()) {
        return 1;
    }
    return 0;
}

auto compare_template_schema_target_identity(
    const exported_template_schema_target &left,
    const exported_template_schema_target &right) -> int {
    if (const auto selector_compare =
            compare_template_schema_target_selector(left, right);
        selector_compare != 0) {
        return selector_compare;
    }
    if (const auto resolved_compare = compare_optional_index(
            left.resolved_from_section_index, right.resolved_from_section_index);
        resolved_compare != 0) {
        return resolved_compare;
    }
    if (left.linked_to_previous != right.linked_to_previous) {
        return left.linked_to_previous ? 1 : -1;
    }
    return 0;
}

auto make_template_schema_patch_target_selector(
    const exported_template_schema_target &target)
    -> template_schema_patch_target_selector {
    template_schema_patch_target_selector selector{};
    selector.part = target.part;
    selector.part_index = target.part_index;
    selector.section_index = target.section_index;
    selector.reference_kind = target.reference_kind;
    selector.resolved_from_section_index = target.resolved_from_section_index;
    selector.linked_to_previous = target.linked_to_previous;
    return selector;
}

auto template_schema_lint_issue_name(const template_schema_lint_issue_kind kind)
    -> std::string_view {
    switch (kind) {
    case template_schema_lint_issue_kind::target_order:
        return "target_order";
    case template_schema_lint_issue_kind::slot_order:
        return "slot_order";
    case template_schema_lint_issue_kind::duplicate_target_identity:
        return "duplicate_target_identity";
    case template_schema_lint_issue_kind::duplicate_slot_name:
        return "duplicate_slot_name";
    case template_schema_lint_issue_kind::entry_name_present:
        return "entry_name_present";
    }

    return "unknown";
}

auto lint_template_schema(const exported_template_schema_result &result)
    -> template_schema_lint_result {
    template_schema_lint_result lint{};
    lint.target_count = result.targets.size();
    lint.slot_count = result.slot_count();

    for (std::size_t target_index = 0U; target_index < result.targets.size();
         ++target_index) {
        const auto &target = result.targets[target_index];
        const auto selector = make_template_schema_patch_target_selector(target);

        if (target_index != 0U &&
            compare_template_schema_target(result.targets[target_index - 1U], target) > 0) {
            template_schema_lint_issue issue{};
            issue.kind = template_schema_lint_issue_kind::target_order;
            issue.target_index = target_index;
            issue.target = selector;
            issue.previous_target_index = target_index - 1U;
            lint.issues.push_back(std::move(issue));
        }

        if (!target.entry_name.empty()) {
            template_schema_lint_issue issue{};
            issue.kind = template_schema_lint_issue_kind::entry_name_present;
            issue.target_index = target_index;
            issue.target = selector;
            issue.entry_name = target.entry_name;
            lint.issues.push_back(std::move(issue));
        }

        for (std::size_t previous_target_index = 0U;
             previous_target_index < target_index; ++previous_target_index) {
            if (compare_template_schema_target_identity(
                    result.targets[previous_target_index], target) != 0) {
                continue;
            }

            template_schema_lint_issue issue{};
            issue.kind = template_schema_lint_issue_kind::duplicate_target_identity;
            issue.target_index = target_index;
            issue.target = selector;
            issue.previous_target_index = previous_target_index;
            lint.issues.push_back(std::move(issue));
            break;
        }

        for (std::size_t slot_index = 1U; slot_index < target.requirements.size();
             ++slot_index) {
            if (compare_template_slot_requirement(target.requirements[slot_index - 1U],
                                                  target.requirements[slot_index]) <= 0) {
                continue;
            }

            template_schema_lint_issue issue{};
            issue.kind = template_schema_lint_issue_kind::slot_order;
            issue.target_index = target_index;
            issue.target = selector;
            issue.slot_index = slot_index;
            issue.previous_slot_index = slot_index - 1U;
            issue.bookmark_name = target.requirements[slot_index].bookmark_name;
            issue.previous_bookmark_name =
                target.requirements[slot_index - 1U].bookmark_name;
            lint.issues.push_back(std::move(issue));
        }

        for (std::size_t slot_index = 0U; slot_index < target.requirements.size();
             ++slot_index) {
            for (std::size_t previous_slot_index = 0U;
                 previous_slot_index < slot_index; ++previous_slot_index) {
                if (target.requirements[previous_slot_index].source !=
                        target.requirements[slot_index].source ||
                    target.requirements[previous_slot_index].bookmark_name !=
                        target.requirements[slot_index].bookmark_name) {
                    continue;
                }

                template_schema_lint_issue issue{};
                issue.kind = template_schema_lint_issue_kind::duplicate_slot_name;
                issue.target_index = target_index;
                issue.target = selector;
                issue.slot_index = slot_index;
                issue.previous_slot_index = previous_slot_index;
                issue.bookmark_name = target.requirements[slot_index].bookmark_name;
                lint.issues.push_back(std::move(issue));
                break;
            }
        }
    }

    return lint;
}

} // namespace featherdoc_cli
