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

std::optional<featherdoc::style_refactor_apply_result>
Document::apply_style_refactor(
    const std::vector<featherdoc::style_refactor_request> &requests) {
    auto plan = this->plan_style_refactor(requests);
    if (!plan.has_value()) {
        return std::nullopt;
    }

    add_style_refactor_apply_conflicts(*plan);

    auto result = featherdoc::style_refactor_apply_result{};
    result.requested_count = requests.size();
    result.plan = std::move(*plan);

    if (!result.plan.clean()) {
        this->last_error_info.clear();
        return result;
    }

    for (const auto &operation : result.plan.operations) {
        auto rollback = featherdoc::style_refactor_rollback_entry{};
        if (operation.action == featherdoc::style_refactor_action::rename) {
            rollback.action = featherdoc::style_refactor_action::rename;
            rollback.source_style_id = operation.target_style_id;
            rollback.target_style_id = operation.source_style_id;
            rollback.automatic = true;
            rollback.restorable = true;
            rollback.note = "rename style back to its previous style id";
        } else {
            rollback.action = featherdoc::style_refactor_action::merge;
            rollback.source_style_id = operation.source_style_id;
            rollback.target_style_id = operation.target_style_id;
            rollback.automatic = false;
            rollback.source_usage = operation.source_usage;
            if (const auto styles_root = this->styles.child("w:styles");
                styles_root != pugi::xml_node{}) {
                rollback.source_style_xml = style_xml_snapshot(
                    find_style_node(styles_root, operation.source_style_id));
            }
            rollback.restorable = !rollback.source_style_xml.empty() &&
                                  rollback.source_usage.has_value();
            rollback.note =
                rollback.restorable
                    ? "merge source style XML and original source references "
                      "were captured for a future restore workflow"
                    : "merge removed the source style definition; restore the "
                      "source document or a backup before replaying dependent "
                      "references";
        }

        const auto applied =
            operation.action == featherdoc::style_refactor_action::rename
                ? this->rename_style(operation.source_style_id,
                                     operation.target_style_id)
                : this->merge_style(operation.source_style_id,
                                    operation.target_style_id);
        if (!applied) {
            return std::nullopt;
        }
        result.rollback_entries.push_back(std::move(rollback));
        ++result.applied_count;
    }

    result.changed = result.applied_count > 0U;
    this->last_error_info.clear();
    return result;
}

std::optional<featherdoc::style_refactor_restore_result>
Document::plan_style_refactor_restore(
    const std::vector<featherdoc::style_refactor_rollback_entry>
        &rollback_entries) {
    return this->restore_style_refactor(rollback_entries, false);
}

std::optional<featherdoc::style_refactor_restore_result>
Document::restore_style_refactor(
    const std::vector<featherdoc::style_refactor_rollback_entry>
        &rollback_entries) {
    return this->restore_style_refactor(rollback_entries, true);
}

std::optional<featherdoc::style_refactor_restore_result>
Document::restore_style_refactor(
    const std::vector<featherdoc::style_refactor_rollback_entry>
        &rollback_entries,
    bool apply_changes) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before restoring style refactors",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto result = featherdoc::style_refactor_restore_result{};
    result.requested_count = rollback_entries.size();
    result.dry_run = !apply_changes;
    result.operations.reserve(rollback_entries.size());

    const auto find_hit_root = [&](const featherdoc::style_usage_hit &hit) {
        if (hit.part == featherdoc::style_usage_part_kind::body) {
            return document_root.child("w:body");
        }

        if (hit.part == featherdoc::style_usage_part_kind::header) {
            for (const auto &part : this->header_parts) {
                if (part && part->entry_name == hit.entry_name) {
                    return part->xml.child("w:hdr");
                }
            }
            return pugi::xml_node{};
        }

        for (const auto &part : this->footer_parts) {
            if (part && part->entry_name == hit.entry_name) {
                return part->xml.child("w:ftr");
            }
        }
        return pugi::xml_node{};
    };

    for (const auto &entry : rollback_entries) {
        auto operation = featherdoc::style_refactor_restore_operation_result{};
        operation.action = entry.action;
        operation.source_style_id = entry.source_style_id;
        operation.target_style_id = entry.target_style_id;
        operation.restorable =
            entry.restorable ||
            (!entry.source_style_xml.empty() && entry.source_usage.has_value());

        if (entry.action != featherdoc::style_refactor_action::merge) {
            append_style_refactor_restore_issue(
                operation.issues, "unsupported_rollback_action",
                "only merge rollback entries can be restored from captured "
                "XML");
        }
        if (entry.source_style_id.empty()) {
            append_style_refactor_restore_issue(
                operation.issues, "empty_source_style_id",
                "merge rollback source style id must not be empty");
        }
        if (entry.target_style_id.empty()) {
            append_style_refactor_restore_issue(
                operation.issues, "empty_target_style_id",
                "merge rollback target style id must not be empty");
        }
        if (!operation.restorable) {
            append_style_refactor_restore_issue(
                operation.issues, "not_restorable",
                "merge rollback entry does not contain a restorable style XML "
                "and usage snapshot");
        }
        if (entry.source_style_xml.empty()) {
            append_style_refactor_restore_issue(
                operation.issues, "missing_source_style_xml",
                "merge rollback entry does not contain source style XML");
        }
        if (!entry.source_usage.has_value()) {
            append_style_refactor_restore_issue(
                operation.issues, "missing_source_usage",
                "merge rollback entry does not contain original source usage "
                "hits");
        } else if (!entry.source_usage->style_id.empty() &&
                   entry.source_usage->style_id != entry.source_style_id) {
            append_style_refactor_restore_issue(
                operation.issues, "source_usage_style_mismatch",
                "merge rollback source usage style id does not match the "
                "rollback source style id");
        }
        if (find_style_node(styles_root, entry.source_style_id) !=
            pugi::xml_node{}) {
            append_style_refactor_restore_issue(
                operation.issues, "source_style_exists",
                "source style id '" + entry.source_style_id +
                    "' already exists in word/styles.xml");
        }
        if (find_style_node(styles_root, entry.target_style_id) ==
            pugi::xml_node{}) {
            append_style_refactor_restore_issue(
                operation.issues, "missing_target_style",
                "target style id '" + entry.target_style_id +
                    "' was not found in word/styles.xml");
        }

        auto source_style_document = pugi::xml_document{};
        pugi::xml_node source_style;
        if (!entry.source_style_xml.empty()) {
            const auto parsed = source_style_document.load_string(
                entry.source_style_xml.c_str(), pugi::parse_default);
            if (!parsed) {
                append_style_refactor_restore_issue(
                    operation.issues, "invalid_source_style_xml",
                    "source style XML snapshot could not be parsed");
            } else {
                source_style = source_style_document.child("w:style");
                if (source_style == pugi::xml_node{}) {
                    source_style = source_style_document.first_child();
                }
                if (source_style == pugi::xml_node{} ||
                    std::string_view{source_style.name()} != "w:style") {
                    append_style_refactor_restore_issue(
                        operation.issues, "invalid_source_style_xml",
                        "source style XML snapshot must contain a w:style "
                        "root");
                } else if (std::string_view{
                               source_style.attribute("w:styleId").value()} !=
                           entry.source_style_id) {
                    append_style_refactor_restore_issue(
                        operation.issues, "source_style_xml_mismatch",
                        "source style XML snapshot style id does not match the "
                        "rollback source style id");
                }
            }
        }

        if (!operation.issues.empty()) {
            result.operations.push_back(std::move(operation));
            continue;
        }

        for (const auto &hit : entry.source_usage->hits) {
            const auto root = find_hit_root(hit);
            if (root == pugi::xml_node{}) {
                append_style_refactor_restore_issue(
                    operation.issues, "missing_usage_part",
                    "usage hit part '" + hit.entry_name +
                        "' was not found while restoring source references");
                continue;
            }

            const auto hit_result = restore_style_reference_hit(
                root, hit, entry.target_style_id, entry.source_style_id, false);
            if (!hit_result.found) {
                append_style_refactor_restore_issue(
                    operation.issues, "missing_usage_hit",
                    "usage hit ordinal could not be found while restoring "
                    "source references");
                continue;
            }
            if (!hit_result.changed) {
                append_style_refactor_restore_issue(
                    operation.issues, "usage_hit_not_target_style",
                    "usage hit no longer references the merge target style id");
            }
        }

        if (!operation.issues.empty()) {
            result.operations.push_back(std::move(operation));
            continue;
        }

        if (!apply_changes) {
            operation.style_restored = true;
            operation.restored_reference_count =
                entry.source_usage->hits.size();
            operation.restored = true;
            ++result.restored_count;
            ++result.restored_style_count;
            result.restored_reference_count +=
                operation.restored_reference_count;
            result.operations.push_back(std::move(operation));
            continue;
        }

        if (styles_root.append_copy(source_style) == pugi::xml_node{}) {
            append_style_refactor_restore_issue(
                operation.issues, "source_style_insert_failed",
                "source style XML snapshot could not be inserted into "
                "word/styles.xml");
            result.operations.push_back(std::move(operation));
            continue;
        }

        operation.style_restored = true;
        ++result.restored_style_count;

        for (const auto &hit : entry.source_usage->hits) {
            const auto root = find_hit_root(hit);
            if (root == pugi::xml_node{}) {
                append_style_refactor_restore_issue(
                    operation.issues, "missing_usage_part",
                    "usage hit part '" + hit.entry_name +
                        "' was not found while restoring source references");
                continue;
            }

            const auto hit_result = restore_style_reference_hit(
                root, hit, entry.target_style_id, entry.source_style_id);
            if (!hit_result.found) {
                append_style_refactor_restore_issue(
                    operation.issues, "missing_usage_hit",
                    "usage hit ordinal could not be found while restoring "
                    "source references");
                continue;
            }
            if (!hit_result.changed) {
                append_style_refactor_restore_issue(
                    operation.issues, "usage_hit_not_target_style",
                    "usage hit no longer references the merge target style id");
                continue;
            }

            ++operation.restored_reference_count;
            ++result.restored_reference_count;
        }

        operation.restored = operation.issues.empty();
        if (operation.restored) {
            ++result.restored_count;
        }
        if (apply_changes) {
            result.changed = result.changed || operation.style_restored ||
                             operation.restored_reference_count > 0U;
        }
        result.operations.push_back(std::move(operation));
    }

    if (apply_changes && result.changed) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return result;
}

bool Document::rename_style(std::string_view old_style_id,
                            std::string_view new_style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before renaming styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (old_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "old style id must not be empty when renaming styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (new_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "new style id must not be empty when renaming styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto style = find_style_node(styles_root, old_style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{old_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (find_style_node(styles_root, new_style_id) != pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id '" + std::string{new_style_id} +
                           "' already exists in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (read_on_off_attribute(style, "w:default")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default style id '" + std::string{old_style_id} +
                           "' cannot be renamed",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    auto changed = true;
    ensure_attribute_value(style, "w:styleId", new_style_id);

    changed = rewrite_style_metadata_references_in_styles(
                  styles_root, old_style_id, new_style_id) ||
              changed;

    changed = rewrite_style_references_in_tree(document_root, old_style_id,
                                               new_style_id) ||
              changed;

    for (const auto &part : this->header_parts) {
        if (part) {
            changed =
                rewrite_style_references_in_tree(part->xml.child("w:hdr"),
                                                 old_style_id, new_style_id) ||
                changed;
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part) {
            changed =
                rewrite_style_references_in_tree(part->xml.child("w:ftr"),
                                                 old_style_id, new_style_id) ||
                changed;
        }
    }

    if (changed) {
        this->styles_dirty = true;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::merge_style(std::string_view source_style_id,
                           std::string_view target_style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (source_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id must not be empty when merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (target_style_id.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id must not be empty when merging styles",
                       std::string{styles_xml_entry});
        return false;
    }

    if (source_style_id == target_style_id) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "source and target style ids must be different when merging styles",
            std::string{styles_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return false;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto source_style = find_style_node(styles_root, source_style_id);
    if (source_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id '" + std::string{source_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto target_style = find_style_node(styles_root, target_style_id);
    if (target_style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "target style id '" + std::string{target_style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    if (read_on_off_attribute(source_style, "w:default")) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "default style id '" + std::string{source_style_id} +
                           "' cannot be merged",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto source_summary = summarize_style_node(source_style);
    const auto target_summary = summarize_style_node(target_style);
    if (source_summary.kind != target_summary.kind ||
        source_summary.type_name != target_summary.type_name) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "source style id '" + std::string{source_style_id} +
                           "' has type '" + source_summary.type_name +
                           "' but target style id '" +
                           std::string{target_style_id} + "' has type '" +
                           target_summary.type_name + "'",
                       std::string{styles_xml_entry});
        return false;
    }

    const auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::document_xml_parse_failed,
                       "word/document.xml does not contain a w:document root",
                       std::string{document_xml_entry});
        return false;
    }

    if (const auto error = this->ensure_styles_part_attached()) {
        return false;
    }

    rewrite_style_metadata_references_in_styles(styles_root, source_style_id,
                                                target_style_id);
    rewrite_style_references_in_tree(document_root, source_style_id,
                                     target_style_id);

    for (const auto &part : this->header_parts) {
        if (part) {
            rewrite_style_references_in_tree(part->xml.child("w:hdr"),
                                             source_style_id, target_style_id);
        }
    }

    for (const auto &part : this->footer_parts) {
        if (part) {
            rewrite_style_references_in_tree(part->xml.child("w:ftr"),
                                             source_style_id, target_style_id);
        }
    }

    if (!styles_root.remove_child(source_style)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "failed to remove source style id '" +
                           std::string{source_style_id} +
                           "' from word/styles.xml",
                       std::string{styles_xml_entry});
        return false;
    }

    this->styles_dirty = true;
    this->last_error_info.clear();
    return true;
}

#include "document_styles_prune_methods.inc"
