std::optional<featherdoc::style_refactor_apply_result>
Document::apply_style_refactor(
    const std::vector<featherdoc::style_refactor_request> &requests) try {
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

    result.rollback_entries.reserve(result.plan.operations.size());
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

        result.rollback_entries.push_back(std::move(rollback));
    }

    result.applied_count = result.plan.operations.size();
    result.changed = result.applied_count != 0U;

    // Materialize every caller-visible allocation before publishing the work
    // DOM. Returning this already-constructed optional after commit is then a
    // move-only path and cannot expose a committed batch as an allocation
    // failure.
    auto final_result = std::optional<featherdoc::style_refactor_apply_result>{
        std::move(result)};
    if (!this->mutate_style_identities_transaction(requests)) {
        return std::nullopt;
    }
    return final_result;
} catch (const std::bad_alloc &) {
    (void)detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, std::string_view{"word/styles.xml"});
    return std::nullopt;
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
    bool apply_changes) try {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before restoring style refactors",
            this->extension_state().singleton_part_entries.styles);
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::styles_xml_parse_failed,
            "'" + this->extension_state().singleton_part_entries.styles +
                "' does not contain a w:styles root",
            this->extension_state().singleton_part_entries.styles);
        return std::nullopt;
    }

    auto document_root = this->document.child("w:document");
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
    if (rollback_entries.empty()) {
        this->last_error_info.clear();
        return result;
    }
    result.operations.reserve(rollback_entries.size());

    auto working_styles = pugi::xml_document{};
    auto working_document = pugi::xml_document{};
    auto working_header_parts =
        std::vector<std::unique_ptr<pugi::xml_document>>{};
    auto working_footer_parts =
        std::vector<std::unique_ptr<pugi::xml_document>>{};
    if (apply_changes) {
        if (detail::checked_clone_xml_document(this->styles, working_styles) !=
            detail::xml_document_clone_status::success) {
            (void)detail::set_xml_document_clone_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.styles);
            return std::nullopt;
        }
        if (detail::checked_clone_xml_document(this->document,
                                               working_document) !=
            detail::xml_document_clone_status::success) {
            (void)detail::set_xml_document_clone_allocation_failure(
                this->last_error_info, document_xml_entry);
            return std::nullopt;
        }

        const auto clone_related_parts = [&](const auto &source_parts,
                                             auto &working_parts) -> bool {
            working_parts.reserve(source_parts.size());
            for (const auto &source_part : source_parts) {
                if (!source_part) {
                    working_parts.push_back(nullptr);
                    continue;
                }

                auto working_part = std::make_unique<pugi::xml_document>();
                if (detail::checked_clone_xml_document(source_part->xml,
                                                       *working_part) !=
                    detail::xml_document_clone_status::success) {
                    (void)detail::set_xml_document_clone_allocation_failure(
                        this->last_error_info, source_part->entry_name);
                    return false;
                }
                working_parts.push_back(std::move(working_part));
            }
            return true;
        };
        if (!clone_related_parts(this->header_parts, working_header_parts) ||
            !clone_related_parts(this->footer_parts, working_footer_parts)) {
            return std::nullopt;
        }

        styles_root = working_styles.child("w:styles");
        document_root = working_document.child("w:document");
    }

    const auto hit_part_name_is_valid =
        [](const featherdoc::style_usage_hit &hit) {
            return hit.part == featherdoc::style_usage_part_kind::body ||
                   featherdoc::detail::package_part_name_identity(
                       hit.entry_name)
                       .has_value();
        };

    const auto find_hit_root = [&](const featherdoc::style_usage_hit &hit) {
        if (hit.part == featherdoc::style_usage_part_kind::body) {
            return document_root.child("w:body");
        }

        if (hit.part == featherdoc::style_usage_part_kind::header) {
            for (std::size_t index = 0U; index < this->header_parts.size();
                 ++index) {
                const auto &part = this->header_parts[index];
                if (part && featherdoc::detail::package_part_names_equivalent(
                                part->entry_name, hit.entry_name)) {
                    const auto &xml = apply_changes
                                          ? *working_header_parts[index]
                                          : part->xml;
                    return xml.child("w:hdr");
                }
            }
            return pugi::xml_node{};
        }

        for (std::size_t index = 0U; index < this->footer_parts.size();
             ++index) {
            const auto &part = this->footer_parts[index];
            if (part && featherdoc::detail::package_part_names_equivalent(
                            part->entry_name, hit.entry_name)) {
                const auto &xml =
                    apply_changes ? *working_footer_parts[index] : part->xml;
                return xml.child("w:ftr");
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
                    "' already exists in '" +
                    this->extension_state().singleton_part_entries.styles +
                    "'");
        }
        if (find_style_node(styles_root, entry.target_style_id) ==
            pugi::xml_node{}) {
            append_style_refactor_restore_issue(
                operation.issues, "missing_target_style",
                "target style id '" + entry.target_style_id +
                    "' was not found in '" +
                    this->extension_state().singleton_part_entries.styles +
                    "'");
        }

        auto source_style_document = pugi::xml_document{};
        pugi::xml_node source_style;
        if (!entry.source_style_xml.empty()) {
            const auto parsed = source_style_document.load_string(
                entry.source_style_xml.c_str(), pugi::parse_default);
            if (!parsed) {
                if (parsed.status == pugi::status_out_of_memory) {
                    (void)detail::set_xml_document_mutation_allocation_failure(
                        this->last_error_info,
                        this->extension_state().singleton_part_entries.styles);
                    return std::nullopt;
                }
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

        auto usage_hit_identities = std::unordered_set<std::string>{};
        usage_hit_identities.reserve(entry.source_usage->hits.size());
        for (const auto &hit : entry.source_usage->hits) {
            if (!hit_part_name_is_valid(hit)) {
                append_style_refactor_restore_issue(
                    operation.issues, "invalid_usage_part_name",
                    "usage hit part name '" + hit.entry_name +
                        "' is not a valid OPC package PartName");
                continue;
            }

            auto hit_identity = std::string{};
            append_style_duplicate_key_part(
                hit_identity,
                std::to_string(static_cast<unsigned int>(hit.part)));
            append_style_duplicate_key_part(
                hit_identity,
                std::to_string(static_cast<unsigned int>(hit.kind)));
            append_style_duplicate_key_part(
                hit_identity,
                hit.part == featherdoc::style_usage_part_kind::body
                    ? document_xml_entry
                    : *featherdoc::detail::package_part_name_identity(
                          hit.entry_name));
            append_style_duplicate_key_part(
                hit_identity,
                std::to_string(style_usage_hit_precise_ordinal(hit)));
            if (!usage_hit_identities.insert(std::move(hit_identity)).second) {
                append_style_refactor_restore_issue(
                    operation.issues, "duplicate_usage_hit",
                    "rollback source usage contains the same part, node kind, "
                    "and ordinal more than once");
                continue;
            }

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

        if (detail::checked_append_copy_xml_node(source_style, styles_root) !=
            detail::xml_document_clone_status::success) {
            (void)detail::set_xml_document_mutation_allocation_failure(
                this->last_error_info,
                this->extension_state().singleton_part_entries.styles);
            return std::nullopt;
        }

        operation.style_restored = true;
        ++result.restored_style_count;

        for (const auto &hit : entry.source_usage->hits) {
            if (!hit_part_name_is_valid(hit)) {
                append_style_refactor_restore_issue(
                    operation.issues, "invalid_usage_part_name",
                    "usage hit part name '" + hit.entry_name +
                        "' is not a valid OPC package PartName");
                continue;
            }
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
            if (hit_result.allocation_failure) {
                (void)detail::set_xml_document_mutation_allocation_failure(
                    this->last_error_info,
                    hit.part == featherdoc::style_usage_part_kind::body
                        ? document_xml_entry
                        : std::string_view{hit.entry_name});
                return std::nullopt;
            }
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

    auto final_result =
        std::optional<featherdoc::style_refactor_restore_result>{
            std::move(result)};
    if (apply_changes && final_result->changed) {
        this->invalidate_xml_handles();
        this->styles = std::move(working_styles);
        this->document = std::move(working_document);
        for (std::size_t index = 0U; index < this->header_parts.size();
             ++index) {
            if (this->header_parts[index] && working_header_parts[index]) {
                this->header_parts[index]->xml =
                    std::move(*working_header_parts[index]);
            }
        }
        for (std::size_t index = 0U; index < this->footer_parts.size();
             ++index) {
            if (this->footer_parts[index] && working_footer_parts[index]) {
                this->footer_parts[index]->xml =
                    std::move(*working_footer_parts[index]);
            }
        }
        this->styles_dirty = true;
        this->rebind_internal_xml_handles();
    }

    this->last_error_info.clear();
    return final_result;
} catch (const std::bad_alloc &) {
    (void)detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, std::string_view{"word/styles.xml"});
    return std::nullopt;
}
