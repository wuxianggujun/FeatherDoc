auto has_style_refactor_issue(
    const featherdoc::style_refactor_operation_plan &operation,
    std::string_view code) -> bool {
    return std::any_of(
        operation.issues.begin(), operation.issues.end(),
        [code](const auto &issue) { return issue.code == code; });
}

void append_style_refactor_issue_once(
    featherdoc::style_refactor_operation_plan &operation, std::string code,
    std::string message) {
    if (has_style_refactor_issue(operation, code)) {
        return;
    }
    operation.issues.push_back(
        featherdoc::style_refactor_issue{std::move(code), std::move(message)});
}

void recalculate_style_refactor_plan_counts(
    featherdoc::style_refactor_plan &plan) {
    plan.operation_count = plan.operations.size();
    plan.applyable_count = 0U;
    plan.issue_count = 0U;
    for (auto &operation : plan.operations) {
        operation.applyable = operation.issues.empty();
        if (operation.applyable) {
            ++plan.applyable_count;
        }
        plan.issue_count += operation.issues.size();
    }
}

auto style_duplicate_string_key(const std::optional<std::string> &value)
    -> std::string {
    return value.has_value() ? *value : std::string{};
}

auto style_duplicate_bool_key(std::optional<bool> value) -> char {
    if (!value.has_value()) {
        return '?';
    }
    return *value ? '1' : '0';
}

void append_style_duplicate_key_part(std::string &key, std::string_view value) {
    key.append(std::to_string(value.size()));
    key.push_back(':');
    key.append(value);
    key.push_back('|');
}

auto style_duplicate_numbering_key(
    const std::optional<featherdoc::style_summary::numbering_summary>
        &numbering) -> std::string {
    if (!numbering.has_value()) {
        return {};
    }

    auto key = std::string{};
    append_style_duplicate_key_part(
        key, numbering->num_id.has_value() ? std::to_string(*numbering->num_id)
                                           : std::string{});
    append_style_duplicate_key_part(key, numbering->level.has_value()
                                             ? std::to_string(*numbering->level)
                                             : std::string{});
    append_style_duplicate_key_part(
        key, numbering->definition_id.has_value()
                 ? std::to_string(*numbering->definition_id)
                 : std::string{});
    append_style_duplicate_key_part(
        key, numbering->definition_name.value_or(std::string{}));
    return key;
}

auto style_duplicate_signature(
    const featherdoc::style_summary &style,
    const featherdoc::resolved_style_properties_summary &resolved)
    -> std::string {
    auto key = std::string{};
    append_style_duplicate_key_part(key, style.type_name);
    append_style_duplicate_key_part(key,
                                    style_duplicate_string_key(style.based_on));
    append_style_duplicate_key_part(key, style.is_semi_hidden ? "1" : "0");
    append_style_duplicate_key_part(key, style.is_unhide_when_used ? "1" : "0");
    append_style_duplicate_key_part(key, style.is_quick_format ? "1" : "0");
    append_style_duplicate_key_part(
        key, style_duplicate_numbering_key(style.numbering));
    append_style_duplicate_key_part(
        key, resolved.run_font_family.value.value_or(std::string{}));
    append_style_duplicate_key_part(
        key, resolved.run_east_asia_font_family.value.value_or(std::string{}));
    append_style_duplicate_key_part(
        key, resolved.run_language.value.value_or(std::string{}));
    append_style_duplicate_key_part(
        key, resolved.run_east_asia_language.value.value_or(std::string{}));
    append_style_duplicate_key_part(
        key, resolved.run_bidi_language.value.value_or(std::string{}));
    append_style_duplicate_key_part(
        key, std::string{style_duplicate_bool_key(resolved.run_rtl.value)});
    append_style_duplicate_key_part(key, std::string{style_duplicate_bool_key(
                                             resolved.paragraph_bidi.value)});
    return key;
}

auto style_duplicate_merge_candidate(const featherdoc::style_summary &style)
    -> bool {
    if (style.style_id.empty() || style.is_default || !style.is_custom) {
        return false;
    }
    return style.kind == featherdoc::style_kind::paragraph ||
           style.kind == featherdoc::style_kind::character;
}

struct style_duplicate_xml_comparison {
    bool definitions_match{false};
    std::vector<std::string> differences{};
};

auto style_xml_ignored_attribute(std::string_view attribute_name,
                                 bool root_node) -> bool {
    return root_node && attribute_name == "w:styleId";
}

auto style_xml_ignored_child(std::string_view child_name,
                             bool root_node) -> bool {
    return root_node && child_name == "w:name";
}

void append_style_canonical_xml(std::string &output, pugi::xml_node node,
                                bool root_node = false) {
    if (node == pugi::xml_node{}) {
        output.append("<missing/>");
        return;
    }

    switch (node.type()) {
    case pugi::node_element: {
        output.push_back('<');
        output.append(node.name());

        auto attributes = std::vector<std::pair<std::string, std::string>>{};
        for (auto attribute = node.first_attribute();
             attribute != pugi::xml_attribute{};
             attribute = attribute.next_attribute()) {
            if (!style_xml_ignored_attribute(attribute.name(), root_node)) {
                attributes.emplace_back(attribute.name(), attribute.value());
            }
        }
        std::sort(attributes.begin(), attributes.end());
        for (const auto &attribute : attributes) {
            output.push_back(' ');
            output.append(attribute.first);
            output.push_back('=');
            output.push_back('\'');
            output.append(attribute.second);
            output.push_back('\'');
        }
        output.push_back('>');

        for (auto child = node.first_child(); child != pugi::xml_node{};
             child = child.next_sibling()) {
            if (child.type() == pugi::node_element &&
                style_xml_ignored_child(child.name(), root_node)) {
                continue;
            }
            append_style_canonical_xml(output, child, false);
        }

        output.append("</");
        output.append(node.name());
        output.push_back('>');
        return;
    }
    case pugi::node_pcdata:
    case pugi::node_cdata:
        output.append(node.value());
        return;
    default:
        return;
    }
}

auto style_canonical_xml(pugi::xml_node node,
                         bool root_node = false) -> std::string {
    auto output = std::string{};
    append_style_canonical_xml(output, node, root_node);
    return output;
}

auto style_xml_snapshot(pugi::xml_node style) -> std::string {
    if (style == pugi::xml_node{}) {
        return {};
    }

    auto stream = std::ostringstream{};
    style.print(stream, "", pugi::format_raw);
    return stream.str();
}

auto style_refactor_restore_issue_suggestion(std::string_view code)
    -> std::string {
    if (code == "unsupported_rollback_action") {
        return "Select merge rollback entries only, or use the original "
               "rename-style command template for rename rollback entries.";
    }
    if (code == "empty_source_style_id" || code == "empty_target_style_id") {
        return "Regenerate the rollback plan from a valid apply-style-refactor "
               "run before retrying restore.";
    }
    if (code == "not_restorable" || code == "missing_source_style_xml" ||
        code == "missing_source_usage") {
        return "Re-run apply-style-refactor with --rollback-plan so merge "
               "rollback records include source style XML and original usage "
               "hits.";
    }
    if (code == "source_usage_style_mismatch" ||
        code == "source_style_xml_mismatch") {
        return "Do not edit rollback JSON by hand; regenerate the rollback "
               "plan from the same style refactor plan.";
    }
    if (code == "source_style_exists") {
        return "The source style already exists; skip this rollback entry or "
               "restore from a document where the merge is still applied.";
    }
    if (code == "missing_target_style") {
        return "Restore against the merged document that still contains the "
               "target style, or recreate the target style before retrying.";
    }
    if (code == "invalid_source_style_xml") {
        return "Regenerate the rollback plan; the captured source style XML "
               "snapshot is not usable.";
    }
    if (code == "missing_usage_part" || code == "missing_usage_hit") {
        return "The document structure changed after the merge; restore from "
               "the original merged document or skip this entry after review.";
    }
    if (code == "usage_hit_not_target_style") {
        return "The captured reference no longer points at the merge target; "
               "inspect current style usage before deciding whether to skip "
               "this entry.";
    }
    if (code == "source_style_insert_failed") {
        return "Check styles.xml integrity and retry restore after repairing "
               "the styles part.";
    }
    return "Review the rollback entry and current document state before "
           "retrying restore.";
}

void append_style_refactor_restore_issue(
    std::vector<featherdoc::style_refactor_restore_issue> &issues,
    std::string code, std::string message) {
    auto issue = featherdoc::style_refactor_restore_issue{};
    issue.code = std::move(code);
    issue.message = std::move(message);
    issue.suggestion = style_refactor_restore_issue_suggestion(issue.code);
    issues.push_back(std::move(issue));
}

auto style_usage_hit_node_name(featherdoc::style_usage_hit_kind hit_kind)
    -> const char * {
    switch (hit_kind) {
    case featherdoc::style_usage_hit_kind::paragraph:
        return "w:p";
    case featherdoc::style_usage_hit_kind::run:
        return "w:r";
    case featherdoc::style_usage_hit_kind::table:
        return "w:tbl";
    }

    return "";
}

auto style_usage_hit_properties_name(featherdoc::style_usage_hit_kind hit_kind)
    -> const char * {
    switch (hit_kind) {
    case featherdoc::style_usage_hit_kind::paragraph:
        return "w:pPr";
    case featherdoc::style_usage_hit_kind::run:
        return "w:rPr";
    case featherdoc::style_usage_hit_kind::table:
        return "w:tblPr";
    }

    return "";
}

auto style_usage_hit_style_name(featherdoc::style_usage_hit_kind hit_kind)
    -> const char * {
    switch (hit_kind) {
    case featherdoc::style_usage_hit_kind::paragraph:
        return "w:pStyle";
    case featherdoc::style_usage_hit_kind::run:
        return "w:rStyle";
    case featherdoc::style_usage_hit_kind::table:
        return "w:tblStyle";
    }

    return "";
}

auto style_usage_hit_precise_ordinal(const featherdoc::style_usage_hit &hit)
    -> std::size_t {
    return hit.node_ordinal != 0U ? hit.node_ordinal : hit.ordinal;
}

struct style_reference_restore_hit_result {
    bool found{false};
    bool changed{false};
};

void restore_style_reference_hit_in_tree(
    pugi::xml_node node, const featherdoc::style_usage_hit &hit,
    std::string_view target_style_id, std::string_view source_style_id,
    bool apply_change, style_usage_node_ordinals &node_ordinals,
    style_reference_restore_hit_result &result) {
    if (node == pugi::xml_node{} || result.found) {
        return;
    }

    const auto node_name = std::string_view{node.name()};
    if (node_name == style_usage_hit_node_name(hit.kind)) {
        const auto node_ordinal =
            increment_style_usage_node_ordinal(node_ordinals, hit.kind);
        if (node_ordinal == style_usage_hit_precise_ordinal(hit)) {
            result.found = true;
            if (apply_change) {
                result.changed = rewrite_style_reference_child(
                    node, style_usage_hit_properties_name(hit.kind),
                    style_usage_hit_style_name(hit.kind), target_style_id,
                    source_style_id);
            } else {
                result.changed = node_style_reference_matches(
                    node, style_usage_hit_properties_name(hit.kind),
                    style_usage_hit_style_name(hit.kind), target_style_id);
            }
            return;
        }
    }

    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        restore_style_reference_hit_in_tree(child, hit, target_style_id,
                                            source_style_id, apply_change,
                                            node_ordinals, result);
        if (result.found) {
            return;
        }
    }
}

auto restore_style_reference_hit(
    pugi::xml_node root, const featherdoc::style_usage_hit &hit,
    std::string_view target_style_id, std::string_view source_style_id,
    bool apply_change = true) -> style_reference_restore_hit_result {
    auto result = style_reference_restore_hit_result{};
    if (root == pugi::xml_node{} ||
        style_usage_hit_precise_ordinal(hit) == 0U) {
        return result;
    }

    auto node_ordinals = style_usage_node_ordinals{};
    restore_style_reference_hit_in_tree(root, hit, target_style_id,
                                        source_style_id, apply_change,
                                        node_ordinals, result);
    return result;
}

void append_style_child_xml_difference(std::vector<std::string> &differences,
                                       pugi::xml_node source_style,
                                       pugi::xml_node target_style,
                                       const char *child_name) {
    const auto source_child = source_style.child(child_name);
    const auto target_child = target_style.child(child_name);
    if (source_child == pugi::xml_node{} && target_child == pugi::xml_node{}) {
        return;
    }
    if (style_canonical_xml(source_child) !=
        style_canonical_xml(target_child)) {
        differences.emplace_back(child_name);
    }
}

auto compare_style_duplicate_xml(pugi::xml_node source_style,
                                 pugi::xml_node target_style)
    -> style_duplicate_xml_comparison {
    auto comparison = style_duplicate_xml_comparison{};
    if (source_style == pugi::xml_node{} || target_style == pugi::xml_node{}) {
        comparison.differences.emplace_back("missing_style_xml");
        return comparison;
    }

    comparison.definitions_match = style_canonical_xml(source_style, true) ==
                                   style_canonical_xml(target_style, true);
    if (comparison.definitions_match) {
        return comparison;
    }

    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:basedOn");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:next");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:link");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:pPr");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:rPr");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:tblPr");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:uiPriority");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:qFormat");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:semiHidden");
    append_style_child_xml_difference(comparison.differences, source_style,
                                      target_style, "w:unhideWhenUsed");
    if (comparison.differences.empty()) {
        comparison.differences.emplace_back("other_style_xml");
    }
    return comparison;
}

auto style_duplicate_merge_suggestion(
    bool target_has_more_references,
    const style_duplicate_xml_comparison &xml_comparison)
    -> featherdoc::style_refactor_suggestion {
    auto suggestion = featherdoc::style_refactor_suggestion{};
    suggestion.reason_code = xml_comparison.definitions_match
                                 ? "matching_style_signature_and_xml"
                                 : "matching_resolved_style_signature";
    suggestion.reason = xml_comparison.definitions_match
                            ? "Source and target share the supported resolved "
                              "style signature, and "
                              "their style definitions also match after "
                              "ignoring styleId and name."
                            : "Source and target share the supported resolved "
                              "style signature, but "
                              "their raw style definitions still differ in "
                              "unsupported or metadata XML.";
    suggestion.confidence = xml_comparison.definitions_match
                                ? (target_has_more_references ? 95U : 90U)
                                : (target_has_more_references ? 80U : 75U);
    suggestion.evidence = {
        "same_style_type",
        "same_based_on",
        "same_visibility_flags",
        "same_numbering_summary",
        "same_resolved_run_properties",
        xml_comparison.definitions_match ? "style_definition_xml_matches"
                                         : "style_definition_xml_differs",
        target_has_more_references ? "target_has_more_references"
                                   : "target_selected_by_style_id_tiebreaker",
    };
    suggestion.differences = xml_comparison.differences;
    return suggestion;
}

void add_style_refactor_apply_conflicts(featherdoc::style_refactor_plan &plan) {
    auto source_operations = std::unordered_map<std::string, std::size_t>{};
    auto rename_targets = std::unordered_map<std::string, std::size_t>{};

    for (std::size_t index = 0U; index < plan.operations.size(); ++index) {
        auto &operation = plan.operations[index];
        if (!operation.source_style_id.empty()) {
            const auto [iterator, inserted] =
                source_operations.emplace(operation.source_style_id, index);
            if (!inserted) {
                const auto message = "source style id '" +
                                     operation.source_style_id +
                                     "' appears in multiple apply operations";
                append_style_refactor_issue_once(
                    plan.operations[iterator->second],
                    "duplicate_source_operation", message);
                append_style_refactor_issue_once(
                    operation, "duplicate_source_operation", message);
            }
        }

        if (operation.action == featherdoc::style_refactor_action::rename &&
            !operation.target_style_id.empty()) {
            const auto [iterator, inserted] =
                rename_targets.emplace(operation.target_style_id, index);
            if (!inserted) {
                const auto message =
                    "rename target style id '" + operation.target_style_id +
                    "' is created by multiple apply operations";
                append_style_refactor_issue_once(
                    plan.operations[iterator->second],
                    "duplicate_rename_target_operation", message);
                append_style_refactor_issue_once(
                    operation, "duplicate_rename_target_operation", message);
            }
        }
    }

    recalculate_style_refactor_plan_counts(plan);
}
