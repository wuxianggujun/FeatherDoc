template_schema_normalization_summary normalize_template_schema(
    featherdoc::template_schema &schema) {
    featherdoc::template_schema_normalization_summary summary{};
    summary.original_slots = schema.entries.size();

    const auto original_entries = schema.entries;
    std::vector<featherdoc::template_schema_entry> normalized_entries;
    normalized_entries.reserve(schema.entries.size());
    for (auto entry : schema.entries) {
        entry.target = canonical_template_schema_selector(entry.target);
        const auto existing_it = find_template_schema_slot(
            normalized_entries, entry.target, entry.requirement.source,
            entry.requirement.bookmark_name);
        if (existing_it == normalized_entries.end()) {
            normalized_entries.push_back(std::move(entry));
            continue;
        }

        *existing_it = std::move(entry);
        ++summary.duplicate_slots_removed;
    }

    std::sort(normalized_entries.begin(), normalized_entries.end(),
              template_schema_entry_less);
    summary.final_slots = normalized_entries.size();
    summary.reordered_or_normalized =
        !template_schema_entries_equal(original_entries, normalized_entries);
    schema.entries = std::move(normalized_entries);
    return summary;
}

template_schema_patch_summary merge_template_schema(
    featherdoc::template_schema &base, const featherdoc::template_schema &overlay) {
    featherdoc::template_schema_patch_summary summary{};

    (void)normalize_template_schema(base);
    auto normalized_overlay = overlay;
    (void)normalize_template_schema(normalized_overlay);

    for (const auto &overlay_entry : normalized_overlay.entries) {
        const auto existing_it = find_template_schema_slot(
            base.entries, overlay_entry.target, overlay_entry.requirement.source,
            overlay_entry.requirement.bookmark_name);
        if (existing_it == base.entries.end()) {
            base.entries.push_back(overlay_entry);
            ++summary.inserted_slots;
            continue;
        }

        if (!template_schema_entry_equals(*existing_it, overlay_entry)) {
            *existing_it = overlay_entry;
            ++summary.replaced_slots;
        }
    }

    (void)normalize_template_schema(base);
    return summary;
}

template_schema_patch_summary apply_template_schema_patch(
    featherdoc::template_schema &schema, const featherdoc::template_schema_patch &patch) {
    featherdoc::template_schema_patch_summary summary{};
    (void)normalize_template_schema(schema);

    for (const auto &raw_target : patch.remove_targets) {
        const auto target = canonical_template_schema_selector(raw_target);
        const auto previous_size = schema.entries.size();
        schema.entries.erase(
            std::remove_if(schema.entries.begin(), schema.entries.end(),
                           [&](const featherdoc::template_schema_entry &entry) {
                               return template_schema_selector_equals(entry.target, target);
                           }),
            schema.entries.end());
        if (schema.entries.size() != previous_size) {
            ++summary.removed_targets;
        }
    }

    for (const auto &remove_slot : patch.remove_slots) {
        const auto target = canonical_template_schema_selector(remove_slot.target);
        const auto previous_size = schema.entries.size();
        schema.entries.erase(
            std::remove_if(schema.entries.begin(), schema.entries.end(),
                           [&](const featherdoc::template_schema_entry &entry) {
                               return template_schema_selector_equals(entry.target, target) &&
                                      entry.requirement.source == remove_slot.source &&
                                       entry.requirement.bookmark_name ==
                                          remove_slot.bookmark_name;
                           }),
            schema.entries.end());
        summary.removed_slots += previous_size - schema.entries.size();
    }

    for (const auto &rename_slot : patch.rename_slots) {
        const auto target = canonical_template_schema_selector(rename_slot.slot.target);
        for (auto &entry : schema.entries) {
            if (!template_schema_selector_equals(entry.target, target) ||
                entry.requirement.source != rename_slot.slot.source ||
                entry.requirement.bookmark_name != rename_slot.slot.bookmark_name ||
                entry.requirement.bookmark_name == rename_slot.new_bookmark_name) {
                continue;
            }

            entry.requirement.bookmark_name = rename_slot.new_bookmark_name;
            ++summary.renamed_slots;
        }
    }

    for (const auto &update_slot : patch.update_slots) {
        const auto update_summary = update_template_schema_slot(
            schema, update_slot.slot, update_slot.update);
        summary.replaced_slots += update_summary.replaced_slots;
    }

    if (!patch.upsert_slots.empty()) {
        featherdoc::template_schema overlay{};
        overlay.entries = patch.upsert_slots;
        const auto merge_summary = merge_template_schema(schema, overlay);
        summary.inserted_slots += merge_summary.inserted_slots;
        summary.replaced_slots += merge_summary.replaced_slots;
    }

    (void)normalize_template_schema(schema);
    return summary;
}

template_schema_patch_summary preview_template_schema_patch(
    const featherdoc::template_schema &schema,
    const featherdoc::template_schema_patch &patch) {
    auto preview_schema = schema;
    return apply_template_schema_patch(preview_schema, patch);
}

template_schema_patch_summary preview_template_schema_patch(
    const featherdoc::template_schema &left,
    const featherdoc::template_schema &right) {
    const auto patch = build_template_schema_patch(left, right);
    return preview_template_schema_patch(left, patch);
}

featherdoc::template_schema_patch_review_summary
make_template_schema_patch_review_summary(
    const featherdoc::template_schema &baseline,
    const featherdoc::template_schema_patch &patch) {
    featherdoc::template_schema_patch_review_summary summary{};
    summary.baseline_slot_count = template_schema_slot_count(baseline);
    summary.patch_upsert_slot_count = patch.upsert_slots.size();
    summary.patch_remove_target_count = patch.remove_targets.size();
    summary.patch_remove_slot_count = patch.remove_slots.size();
    summary.patch_rename_slot_count = patch.rename_slots.size();
    summary.patch_update_slot_count = patch.update_slots.size();
    summary.preview = preview_template_schema_patch(baseline, patch);
    return summary;
}

featherdoc::template_schema_patch_review_summary
make_template_schema_patch_review_summary(
    const featherdoc::template_schema &baseline,
    const featherdoc::template_schema &generated) {
    const auto patch = build_template_schema_patch(baseline, generated);
    auto summary = make_template_schema_patch_review_summary(baseline, patch);
    summary.generated_slot_count = template_schema_slot_count(generated);
    return summary;
}

void write_template_schema_patch_review_json(
    std::ostream &stream,
    const featherdoc::template_schema_patch_review_summary &summary) {
    stream << '{';
    stream << "\"schema\":";
    write_schema_review_json_string(
        stream, "featherdoc.template_schema_patch_review.v1");
    stream << ",\"baseline_slot_count\":" << summary.baseline_slot_count;
    stream << ",\"generated_slot_count\":";
    if (summary.generated_slot_count.has_value()) {
        stream << *summary.generated_slot_count;
    } else {
        stream << "null";
    }
    stream << ",\"patch\":{"
           << "\"upsert_slot_count\":" << summary.patch_upsert_slot_count
           << ",\"remove_target_count\":" << summary.patch_remove_target_count
           << ",\"remove_slot_count\":" << summary.patch_remove_slot_count
           << ",\"rename_slot_count\":" << summary.patch_rename_slot_count
           << ",\"update_slot_count\":" << summary.patch_update_slot_count
           << "}";
    stream << ",\"preview\":{"
           << "\"removed_targets\":" << summary.preview.removed_targets
           << ",\"removed_slots\":" << summary.preview.removed_slots
           << ",\"renamed_slots\":" << summary.preview.renamed_slots
           << ",\"inserted_slots\":" << summary.preview.inserted_slots
           << ",\"replaced_slots\":" << summary.preview.replaced_slots
           << ",\"changed\":" << (summary.preview.changed() ? "true" : "false")
           << "}";
    stream << ",\"changed\":" << (summary.changed() ? "true" : "false");
    stream << '}';
}

std::string template_schema_patch_review_json(
    const featherdoc::template_schema_patch_review_summary &summary) {
    std::ostringstream stream;
    write_template_schema_patch_review_json(stream, summary);
    return stream.str();
}

template_schema_patch_summary replace_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target,
    std::span<const featherdoc::template_schema_entry> entries) {
    featherdoc::template_schema_patch patch{};
    patch.remove_targets.push_back(target);
    patch.upsert_slots.insert(patch.upsert_slots.end(), entries.begin(), entries.end());
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary replace_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target,
    std::initializer_list<featherdoc::template_schema_entry> entries) {
    return replace_template_schema_target(
        schema, target, std::span<const featherdoc::template_schema_entry>{entries.begin(), entries.size()});
}

template_schema_patch_summary upsert_template_schema_slot(
    featherdoc::template_schema &schema, const featherdoc::template_schema_entry &entry) {
    featherdoc::template_schema_patch patch{};
    patch.upsert_slots.push_back(entry);
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary remove_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &target) {
    featherdoc::template_schema_patch patch{};
    patch.remove_targets.push_back(target);
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary rename_template_schema_target(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_part_selector &source_target,
    const featherdoc::template_schema_part_selector &target) {
    (void)normalize_template_schema(schema);

    const auto source = canonical_template_schema_selector(source_target);
    const auto destination = canonical_template_schema_selector(target);
    if (template_schema_selector_equals(source, destination)) {
        return {};
    }

    featherdoc::template_schema_patch patch{};
    for (const auto &entry : schema.entries) {
        if (!template_schema_selector_equals(entry.target, source)) {
            continue;
        }

        auto moved_entry = entry;
        moved_entry.target = destination;
        patch.upsert_slots.push_back(std::move(moved_entry));
    }

    if (patch.upsert_slots.empty()) {
        return {};
    }

    patch.remove_targets.push_back(source);
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary remove_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot) {
    featherdoc::template_schema_patch patch{};
    patch.remove_slots.push_back(slot);
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary rename_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot,
    std::string_view new_bookmark_name) {
    featherdoc::template_schema_patch patch{};
    featherdoc::template_schema_slot_rename rename_slot{};
    rename_slot.slot = slot;
    rename_slot.new_bookmark_name = std::string{new_bookmark_name};
    patch.rename_slots.push_back(std::move(rename_slot));
    return apply_template_schema_patch(schema, patch);
}

template_schema_patch_summary update_template_schema_slot(
    featherdoc::template_schema &schema,
    const featherdoc::template_schema_slot_selector &slot,
    const featherdoc::template_schema_slot_update &update) {
    featherdoc::template_schema_patch_summary summary{};
    (void)normalize_template_schema(schema);

    const auto target = canonical_template_schema_selector(slot.target);
    const auto existing_it = find_template_schema_slot(
        schema.entries, target, slot.source, slot.bookmark_name);
    if (existing_it == schema.entries.end()) {
        return summary;
    }

    auto updated_entry = *existing_it;
    if (update.kind.has_value()) {
        updated_entry.requirement.kind = *update.kind;
    }
    if (update.required.has_value()) {
        updated_entry.requirement.required = *update.required;
    }
    if (update.clear_min_occurrences) {
        updated_entry.requirement.min_occurrences.reset();
    }
    if (update.min_occurrences.has_value()) {
        updated_entry.requirement.min_occurrences = *update.min_occurrences;
    }
    if (update.clear_max_occurrences) {
        updated_entry.requirement.max_occurrences.reset();
    }
    if (update.max_occurrences.has_value()) {
        updated_entry.requirement.max_occurrences = *update.max_occurrences;
    }

    if (template_schema_entry_equals(*existing_it, updated_entry)) {
        return summary;
    }

    *existing_it = std::move(updated_entry);
    ++summary.replaced_slots;
    (void)normalize_template_schema(schema);
    return summary;
}

template_schema_patch build_template_schema_patch(
    const featherdoc::template_schema &left, const featherdoc::template_schema &right) {
    auto normalized_left = left;
    auto normalized_right = right;
    (void)normalize_template_schema(normalized_left);
    (void)normalize_template_schema(normalized_right);

    featherdoc::template_schema_patch patch{};

    std::vector<featherdoc::template_schema_entry> removed_entries;
    std::vector<featherdoc::template_schema_entry> added_entries;

    for (const auto &left_entry : normalized_left.entries) {
        if (!template_schema_has_target(normalized_right, left_entry.target)) {
            if (std::none_of(patch.remove_targets.begin(), patch.remove_targets.end(),
                             [&](const featherdoc::template_schema_part_selector &target) {
                                 return template_schema_selector_equals(target,
                                                                        left_entry.target);
                             })) {
                patch.remove_targets.push_back(left_entry.target);
            }
            continue;
        }

        const auto right_it = find_template_schema_slot(
            normalized_right.entries, left_entry.target, left_entry.requirement.source,
            left_entry.requirement.bookmark_name);
        if (right_it == normalized_right.entries.end()) {
            removed_entries.push_back(left_entry);
            continue;
        }
        if (!template_schema_entry_equals(left_entry, *right_it)) {
            featherdoc::template_schema_slot_patch_update update_slot{};
            update_slot.slot.target = left_entry.target;
            update_slot.slot.source = left_entry.requirement.source;
            update_slot.slot.bookmark_name = left_entry.requirement.bookmark_name;
            update_slot.update = make_template_schema_slot_update(
                left_entry.requirement, right_it->requirement);
            patch.update_slots.push_back(std::move(update_slot));
        }
    }

    for (const auto &right_entry : normalized_right.entries) {
        if (!template_schema_has_target(normalized_left, right_entry.target)) {
            patch.upsert_slots.push_back(right_entry);
            continue;
        }

        const auto left_it = find_template_schema_slot(
            normalized_left.entries, right_entry.target, right_entry.requirement.source,
            right_entry.requirement.bookmark_name);
        if (left_it == normalized_left.entries.end()) {
            added_entries.push_back(right_entry);
        }
    }

    std::vector<bool> removed_consumed(removed_entries.size(), false);
    std::vector<bool> added_consumed(added_entries.size(), false);
    const auto find_unique_renamed_slot = [&](std::size_t removed_index,
                                              int match_level)
        -> std::optional<std::size_t> {
        const auto matches = [&](const featherdoc::template_schema_entry &removed,
                                 const featherdoc::template_schema_entry &added) {
            if (!template_schema_selector_equals(removed.target, added.target) ||
                removed.requirement.source != added.requirement.source) {
                return false;
            }
            if (match_level == 0) {
                return compare_template_slot_requirement_shape(removed.requirement,
                                                               added.requirement) == 0;
            }
            if (match_level == 1) {
                return removed.requirement.kind == added.requirement.kind;
            }
            return true;
        };

        std::optional<std::size_t> matched_added_index;
        std::size_t matched_added_count = 0U;
        for (std::size_t added_index = 0U; added_index < added_entries.size();
             ++added_index) {
            if (added_consumed[added_index] ||
                !matches(removed_entries[removed_index], added_entries[added_index])) {
                continue;
            }
            matched_added_index = added_index;
            ++matched_added_count;
        }
        if (matched_added_count != 1U || !matched_added_index.has_value()) {
            return std::nullopt;
        }

        std::size_t matched_removed_count = 0U;
        for (std::size_t candidate_removed_index = 0U;
             candidate_removed_index < removed_entries.size();
             ++candidate_removed_index) {
            if (removed_consumed[candidate_removed_index] ||
                !matches(removed_entries[candidate_removed_index],
                         added_entries[*matched_added_index])) {
                continue;
            }
            ++matched_removed_count;
        }
        if (matched_removed_count != 1U) {
            return std::nullopt;
        }
        return matched_added_index;
    };

    for (std::size_t removed_index = 0U; removed_index < removed_entries.size();
         ++removed_index) {
        auto matched_added_index = find_unique_renamed_slot(removed_index, 0);
        if (!matched_added_index.has_value()) {
            matched_added_index = find_unique_renamed_slot(removed_index, 1);
        }
        if (!matched_added_index.has_value()) {
            matched_added_index = find_unique_renamed_slot(removed_index, 2);
        }
        if (!matched_added_index.has_value()) {
            continue;
        }

        featherdoc::template_schema_slot_rename rename_slot{};
        rename_slot.slot.target = removed_entries[removed_index].target;
        rename_slot.slot.source = removed_entries[removed_index].requirement.source;
        rename_slot.slot.bookmark_name =
            removed_entries[removed_index].requirement.bookmark_name;
        rename_slot.new_bookmark_name =
            added_entries[*matched_added_index].requirement.bookmark_name;
        patch.rename_slots.push_back(std::move(rename_slot));

        if (compare_template_slot_requirement_shape(
                removed_entries[removed_index].requirement,
                added_entries[*matched_added_index].requirement) != 0) {
            featherdoc::template_schema_slot_patch_update update_slot{};
            update_slot.slot.target = removed_entries[removed_index].target;
            update_slot.slot.source = removed_entries[removed_index].requirement.source;
            update_slot.slot.bookmark_name =
                added_entries[*matched_added_index].requirement.bookmark_name;
            update_slot.update = make_template_schema_slot_update(
                removed_entries[removed_index].requirement,
                added_entries[*matched_added_index].requirement);
            patch.update_slots.push_back(std::move(update_slot));
        }

        removed_consumed[removed_index] = true;
        added_consumed[*matched_added_index] = true;
    }

    for (std::size_t removed_index = 0U; removed_index < removed_entries.size();
         ++removed_index) {
        if (removed_consumed[removed_index]) {
            continue;
        }

        featherdoc::template_schema_slot_selector remove_slot{};
        remove_slot.target = removed_entries[removed_index].target;
        remove_slot.source = removed_entries[removed_index].requirement.source;
        remove_slot.bookmark_name =
            removed_entries[removed_index].requirement.bookmark_name;
        patch.remove_slots.push_back(std::move(remove_slot));
    }

    for (std::size_t added_index = 0U; added_index < added_entries.size();
         ++added_index) {
        if (!added_consumed[added_index]) {
            patch.upsert_slots.push_back(added_entries[added_index]);
        }
    }

    return patch;
}
