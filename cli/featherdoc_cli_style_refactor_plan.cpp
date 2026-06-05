#include "featherdoc_cli_style_refactor_plan.hpp"

#include <algorithm>
#include <utility>

namespace featherdoc_cli {
namespace {

auto style_refactor_filter_contains(const std::vector<std::string> &style_ids,
                                    std::string_view style_id) -> bool {
    return std::any_of(style_ids.begin(), style_ids.end(),
                       [style_id](const auto &candidate) {
                           return std::string_view(candidate) == style_id;
                       });
}

void recount_style_refactor_plan(featherdoc::style_refactor_plan &plan) {
    plan.operation_count = plan.operations.size();
    plan.applyable_count = 0U;
    plan.issue_count = 0U;
    for (const auto &operation : plan.operations) {
        if (operation.applyable) {
            ++plan.applyable_count;
        }
        plan.issue_count += operation.issues.size();
    }
}

} // namespace

auto style_refactor_action_name(featherdoc::style_refactor_action action) -> const char * {
    switch (action) {
    case featherdoc::style_refactor_action::rename:
        return "rename";
    case featherdoc::style_refactor_action::merge:
        return "merge";
    }

    return "rename";
}

auto style_refactor_command_template(
    const featherdoc::style_refactor_operation_plan &operation) -> std::string {
    auto command = std::string{"featherdoc_cli "};
    command += operation.action == featherdoc::style_refactor_action::rename
                   ? "rename-style"
                   : "merge-style";
    command += " <input.docx> ";
    command += operation.source_style_id;
    command += ' ';
    command += operation.target_style_id;
    command += " --output <output.docx> --json";
    return command;
}

auto style_refactor_rollback_command_template(
    const featherdoc::style_refactor_rollback_entry &entry) -> std::string {
    if (!entry.automatic) {
        return {};
    }

    auto command = std::string{"featherdoc_cli "};
    command += entry.action == featherdoc::style_refactor_action::rename
                   ? "rename-style"
                   : "merge-style";
    command += " <input.docx> ";
    command += entry.source_style_id;
    command += ' ';
    command += entry.target_style_id;
    command += " --output <output.docx> --json";
    return command;
}

auto style_merge_suggestion_confidence_profile_min_confidence(
    std::string_view profile) -> std::optional<std::uint32_t> {
    if (profile == "strict") {
        return 90U;
    }
    if (profile == "review") {
        return 75U;
    }
    if (profile == "exploratory") {
        return 0U;
    }
    return std::nullopt;
}

auto style_merge_suggestion_confidence_profile_is_valid(
    std::string_view profile) -> bool {
    return profile == "recommended" ||
           style_merge_suggestion_confidence_profile_min_confidence(profile)
               .has_value();
}

auto filter_style_refactor_plan_by_min_confidence(
    featherdoc::style_refactor_plan plan, std::uint32_t min_confidence)
    -> featherdoc::style_refactor_plan {
    if (min_confidence == 0U) {
        return plan;
    }

    auto filtered_operations =
        std::vector<featherdoc::style_refactor_operation_plan>{};
    filtered_operations.reserve(plan.operations.size());
    for (auto &operation : plan.operations) {
        if (operation.suggestion.has_value() &&
            operation.suggestion->confidence >= min_confidence) {
            filtered_operations.push_back(std::move(operation));
        }
    }

    plan.operations = std::move(filtered_operations);
    recount_style_refactor_plan(plan);
    return plan;
}

auto filter_style_refactor_plan_by_style_ids(
    featherdoc::style_refactor_plan plan,
    const std::vector<std::string> &source_style_ids,
    const std::vector<std::string> &target_style_ids)
    -> featherdoc::style_refactor_plan {
    if (source_style_ids.empty() && target_style_ids.empty()) {
        return plan;
    }

    auto filtered_operations =
        std::vector<featherdoc::style_refactor_operation_plan>{};
    filtered_operations.reserve(plan.operations.size());
    for (auto &operation : plan.operations) {
        const auto source_matches =
            source_style_ids.empty() ||
            style_refactor_filter_contains(source_style_ids,
                                           operation.source_style_id);
        const auto target_matches =
            target_style_ids.empty() ||
            style_refactor_filter_contains(target_style_ids,
                                           operation.target_style_id);
        if (source_matches && target_matches) {
            filtered_operations.push_back(std::move(operation));
        }
    }

    plan.operations = std::move(filtered_operations);
    recount_style_refactor_plan(plan);
    return plan;
}

} // namespace featherdoc_cli
