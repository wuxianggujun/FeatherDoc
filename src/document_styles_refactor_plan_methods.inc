std::optional<featherdoc::style_refactor_plan> Document::plan_style_refactor(
    const std::vector<featherdoc::style_refactor_request> &requests) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before planning style refactors",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style_summaries = this->list_styles();
    if (const auto &error_info = this->last_error(); error_info.code) {
        return std::nullopt;
    }

    auto styles_by_id =
        std::unordered_map<std::string, featherdoc::style_summary>{};
    styles_by_id.reserve(style_summaries.size());
    for (auto &style : style_summaries) {
        if (!style.style_id.empty()) {
            styles_by_id.emplace(style.style_id, std::move(style));
        }
    }

    auto plan = featherdoc::style_refactor_plan{};
    plan.operation_count = requests.size();
    plan.operations.reserve(requests.size());

    const auto append_issue =
        [](featherdoc::style_refactor_operation_plan &operation,
           std::string code, std::string message) {
            operation.issues.push_back(featherdoc::style_refactor_issue{
                std::move(code), std::move(message)});
        };

    for (const auto &request : requests) {
        auto operation = featherdoc::style_refactor_operation_plan{};
        operation.action = request.action;
        operation.source_style_id = request.source_style_id;
        operation.target_style_id = request.target_style_id;

        if (operation.source_style_id.empty()) {
            append_issue(operation, "empty_source_style_id",
                         "source style id must not be empty");
        }
        if (operation.target_style_id.empty()) {
            append_issue(operation, "empty_target_style_id",
                         "target style id must not be empty");
        }
        if (!operation.source_style_id.empty() &&
            operation.source_style_id == operation.target_style_id) {
            append_issue(operation, "same_style_id",
                         "source and target style ids must be different");
        }

        if (!operation.source_style_id.empty()) {
            const auto source_iterator =
                styles_by_id.find(operation.source_style_id);
            if (source_iterator == styles_by_id.end()) {
                append_issue(operation, "missing_source_style",
                             "source style id '" + operation.source_style_id +
                                 "' was not found in word/styles.xml");
            } else {
                operation.source_style = source_iterator->second;
                if (source_iterator->second.is_default) {
                    append_issue(operation, "default_source_style",
                                 "default source style id '" +
                                     operation.source_style_id +
                                     "' cannot be refactored");
                }

                auto source_usage =
                    this->find_style_usage(operation.source_style_id);
                if (!source_usage.has_value()) {
                    return std::nullopt;
                }
                operation.source_usage = std::move(*source_usage);
            }
        }

        if (!operation.target_style_id.empty()) {
            const auto target_iterator =
                styles_by_id.find(operation.target_style_id);
            if (target_iterator != styles_by_id.end()) {
                operation.target_style = target_iterator->second;
            }
        }

        if (operation.action == featherdoc::style_refactor_action::rename) {
            if (operation.target_style.has_value()) {
                append_issue(operation, "target_style_exists",
                             "target style id '" + operation.target_style_id +
                                 "' already exists in word/styles.xml");
            }
        } else {
            if (!operation.target_style.has_value() &&
                !operation.target_style_id.empty()) {
                append_issue(operation, "missing_target_style",
                             "target style id '" + operation.target_style_id +
                                 "' was not found in word/styles.xml");
            }
            if (operation.source_style.has_value() &&
                operation.target_style.has_value() &&
                (operation.source_style->kind != operation.target_style->kind ||
                 operation.source_style->type_name !=
                     operation.target_style->type_name)) {
                append_issue(operation, "style_type_mismatch",
                             "source style id '" + operation.source_style_id +
                                 "' has type '" +
                                 operation.source_style->type_name +
                                 "' but target style id '" +
                                 operation.target_style_id + "' has type '" +
                                 operation.target_style->type_name + "'");
            }
        }

        operation.applyable = operation.issues.empty();
        if (operation.applyable) {
            ++plan.applyable_count;
        }
        plan.issue_count += operation.issues.size();
        plan.operations.push_back(std::move(operation));
    }

    this->last_error_info.clear();
    return plan;
}

std::optional<featherdoc::style_refactor_plan>
Document::suggest_style_merges() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before suggesting style merges",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto style_summaries = this->list_styles();
    if (const auto &error_info = this->last_error(); error_info.code) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    struct duplicate_candidate {
        featherdoc::style_summary style{};
        featherdoc::style_usage_summary usage{};
        pugi::xml_node style_node{};
    };

    auto candidates_by_signature =
        std::unordered_map<std::string, std::vector<duplicate_candidate>>{};

    for (const auto &style : style_summaries) {
        if (!style_duplicate_merge_candidate(style)) {
            continue;
        }

        const auto resolved = this->resolve_style_properties(style.style_id);
        if (!resolved.has_value()) {
            return std::nullopt;
        }
        const auto usage = this->find_style_usage(style.style_id);
        if (!usage.has_value()) {
            return std::nullopt;
        }
        const auto style_node = find_style_node(styles_root, style.style_id);

        candidates_by_signature[style_duplicate_signature(style, *resolved)]
            .push_back(duplicate_candidate{style, *usage, style_node});
    }

    auto requests = std::vector<featherdoc::style_refactor_request>{};
    auto suggestions = std::vector<featherdoc::style_refactor_suggestion>{};
    for (auto &entry : candidates_by_signature) {
        auto &candidates = entry.second;
        if (candidates.size() < 2U) {
            continue;
        }

        std::sort(candidates.begin(), candidates.end(),
                  [](const auto &left, const auto &right) {
                      const auto left_count = left.usage.total_count();
                      const auto right_count = right.usage.total_count();
                      if (left_count != right_count) {
                          return left_count > right_count;
                      }
                      return left.style.style_id < right.style.style_id;
                  });

        const auto target_style_id = candidates.front().style.style_id;
        const auto target_reference_count =
            candidates.front().usage.total_count();
        const auto target_style_node = candidates.front().style_node;
        for (std::size_t index = 1U; index < candidates.size(); ++index) {
            requests.push_back({featherdoc::style_refactor_action::merge,
                                candidates[index].style.style_id,
                                target_style_id});
            suggestions.push_back(style_duplicate_merge_suggestion(
                target_reference_count > candidates[index].usage.total_count(),
                compare_style_duplicate_xml(candidates[index].style_node,
                                            target_style_node)));
        }
    }

    auto plan = this->plan_style_refactor(requests);
    if (!plan.has_value()) {
        return std::nullopt;
    }

    for (std::size_t index = 0U;
         index < plan->operations.size() && index < suggestions.size();
         ++index) {
        auto &operation = plan->operations[index];
        if (operation.action == featherdoc::style_refactor_action::merge &&
            operation.source_style_id == requests[index].source_style_id &&
            operation.target_style_id == requests[index].target_style_id) {
            operation.suggestion = suggestions[index];
        }
    }

    return plan;
}

