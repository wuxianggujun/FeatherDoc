auto make_semantic_sequence_change(std::string_view field)
    -> featherdoc::document_semantic_diff_change {
    auto change = featherdoc::document_semantic_diff_change{};
    change.field = std::string{field};
    return change;
}

void append_semantic_added_change(
    std::string_view field, const std::vector<std::string> &right_values,
    std::size_t right_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.added_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::added;
    change.right_index = right_index;
    change.right_value = right_values[right_index];
    changes.push_back(std::move(change));
}

void append_semantic_removed_change(
    std::string_view field, const std::vector<std::string> &left_values,
    std::size_t left_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.removed_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::removed;
    change.left_index = left_index;
    change.left_value = left_values[left_index];
    changes.push_back(std::move(change));
}

void append_semantic_changed_change(
    std::string_view field, const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::size_t left_index,
    std::size_t right_index,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    ++summary.changed_count;
    auto change = make_semantic_sequence_change(field);
    change.kind = featherdoc::document_semantic_diff_change_kind::changed;
    change.left_index = left_index;
    change.right_index = right_index;
    change.left_value = left_values[left_index];
    change.right_value = right_values[right_index];
    change.field_changes = semantic_field_changes(change.left_value, change.right_value);
    changes.push_back(std::move(change));
}

void compare_semantic_values_by_index(
    const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::string_view field,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    const auto item_count = std::max(left_values.size(), right_values.size());
    for (std::size_t item_index = 0; item_index < item_count; ++item_index) {
        if (item_index >= left_values.size()) {
            append_semantic_added_change(field, right_values, item_index, summary,
                                         changes);
            continue;
        }

        if (item_index >= right_values.size()) {
            append_semantic_removed_change(field, left_values, item_index, summary,
                                           changes);
            continue;
        }

        if (left_values[item_index] != right_values[item_index]) {
            append_semantic_changed_change(field, left_values, right_values,
                                           item_index, item_index, summary,
                                           changes);
        } else {
            ++summary.unchanged_count;
        }
    }
}

auto can_align_semantic_values(
    std::size_t left_count, std::size_t right_count,
    const featherdoc::document_semantic_diff_options &options) -> bool {
    if (!options.align_sequences_by_content) {
        return false;
    }

    const auto row_count = left_count + 1U;
    const auto column_count = right_count + 1U;
    if (row_count == 0U || column_count == 0U) {
        return false;
    }
    return row_count <= options.alignment_cell_limit / column_count;
}

void compare_semantic_values_by_content_alignment(
    const std::vector<std::string> &left_values,
    const std::vector<std::string> &right_values, std::string_view field,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    const auto left_count = left_values.size();
    const auto right_count = right_values.size();
    const auto column_count = right_count + 1U;
    auto cost = std::vector<std::size_t>((left_count + 1U) * column_count, 0U);
    const auto cost_index = [column_count](std::size_t left_position,
                                           std::size_t right_position) {
        return left_position * column_count + right_position;
    };

    for (std::size_t left_position = 1U; left_position <= left_count;
         ++left_position) {
        cost[cost_index(left_position, 0U)] = left_position;
    }
    for (std::size_t right_position = 1U; right_position <= right_count;
         ++right_position) {
        cost[cost_index(0U, right_position)] = right_position;
    }

    for (std::size_t left_position = 1U; left_position <= left_count;
         ++left_position) {
        for (std::size_t right_position = 1U; right_position <= right_count;
             ++right_position) {
            const auto substitution_cost =
                left_values[left_position - 1U] == right_values[right_position - 1U]
                    ? 0U
                    : 1U;
            auto best_cost = cost[cost_index(left_position - 1U,
                                             right_position - 1U)] +
                             substitution_cost;
            const auto removal_cost =
                cost[cost_index(left_position - 1U, right_position)] + 1U;
            if (removal_cost < best_cost) {
                best_cost = removal_cost;
            }
            const auto addition_cost =
                cost[cost_index(left_position, right_position - 1U)] + 1U;
            if (addition_cost < best_cost) {
                best_cost = addition_cost;
            }
            cost[cost_index(left_position, right_position)] = best_cost;
        }
    }

    auto aligned_changes = std::vector<featherdoc::document_semantic_diff_change>{};
    std::size_t left_position = left_count;
    std::size_t right_position = right_count;
    while (left_position > 0U || right_position > 0U) {
        const auto current_cost = cost[cost_index(left_position, right_position)];
        if (left_position > 0U && right_position > 0U) {
            const auto left_index = left_position - 1U;
            const auto right_index = right_position - 1U;
            const auto substitution_cost =
                left_values[left_index] == right_values[right_index] ? 0U : 1U;
            const auto diagonal_cost =
                cost[cost_index(left_position - 1U, right_position - 1U)] +
                substitution_cost;
            if (current_cost == diagonal_cost) {
                if (substitution_cost == 0U) {
                    ++summary.unchanged_count;
                } else {
                    append_semantic_changed_change(field, left_values, right_values,
                                                   left_index, right_index, summary,
                                                   aligned_changes);
                }
                --left_position;
                --right_position;
                continue;
            }
        }

        if (left_position > 0U &&
            current_cost == cost[cost_index(left_position - 1U, right_position)] +
                                1U) {
            append_semantic_removed_change(field, left_values, left_position - 1U,
                                           summary, aligned_changes);
            --left_position;
            continue;
        }

        append_semantic_added_change(field, right_values, right_position - 1U,
                                     summary, aligned_changes);
        --right_position;
    }

    std::reverse(aligned_changes.begin(), aligned_changes.end());
    changes.insert(changes.end(), aligned_changes.begin(), aligned_changes.end());
}

template <typename Item, typename Fingerprint>
void compare_semantic_sequence(
    const std::vector<Item> &left_items, const std::vector<Item> &right_items,
    std::string_view field, Fingerprint fingerprint,
    const featherdoc::document_semantic_diff_options &options,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    summary.left_count = left_items.size();
    summary.right_count = right_items.size();

    auto left_values = std::vector<std::string>{};
    auto right_values = std::vector<std::string>{};
    left_values.reserve(left_items.size());
    right_values.reserve(right_items.size());
    for (const auto &left_item : left_items) {
        left_values.push_back(fingerprint(left_item));
    }
    for (const auto &right_item : right_items) {
        right_values.push_back(fingerprint(right_item));
    }

    if (can_align_semantic_values(left_values.size(), right_values.size(), options)) {
        compare_semantic_values_by_content_alignment(left_values, right_values, field,
                                                     summary, changes);
        return;
    }

    compare_semantic_values_by_index(left_values, right_values, field, summary,
                                     changes);
}

template <typename Item, typename Key, typename Fingerprint>
void compare_semantic_sequence_by_key(
    const std::vector<Item> &left_items, const std::vector<Item> &right_items,
    std::string_view field, Key key, Fingerprint fingerprint,
    featherdoc::document_semantic_diff_category_summary &summary,
    std::vector<featherdoc::document_semantic_diff_change> &changes) {
    summary.left_count = left_items.size();
    summary.right_count = right_items.size();

    auto matched_right_items = std::vector<bool>(right_items.size(), false);
    for (std::size_t left_index = 0U; left_index < left_items.size(); ++left_index) {
        const auto left_key = key(left_items[left_index]);
        auto right_index = std::optional<std::size_t>{};
        for (std::size_t candidate_index = 0U; candidate_index < right_items.size();
             ++candidate_index) {
            if (!matched_right_items[candidate_index] &&
                key(right_items[candidate_index]) == left_key) {
                right_index = candidate_index;
                break;
            }
        }

        if (!right_index.has_value()) {
            auto values = std::vector<std::string>{fingerprint(left_items[left_index])};
            append_semantic_removed_change(field, values, 0U, summary, changes);
            changes.back().left_index = left_index;
            continue;
        }

        matched_right_items[*right_index] = true;
        const auto left_value = fingerprint(left_items[left_index]);
        const auto right_value = fingerprint(right_items[*right_index]);
        if (left_value != right_value) {
            auto left_values = std::vector<std::string>{left_value};
            auto right_values = std::vector<std::string>{right_value};
            append_semantic_changed_change(field, left_values, right_values, 0U, 0U,
                                           summary, changes);
            changes.back().left_index = left_index;
            changes.back().right_index = *right_index;
        } else {
            ++summary.unchanged_count;
        }
    }

    for (std::size_t right_index = 0U; right_index < right_items.size(); ++right_index) {
        if (!matched_right_items[right_index]) {
            auto values = std::vector<std::string>{fingerprint(right_items[right_index])};
            append_semantic_added_change(field, values, 0U, summary, changes);
            changes.back().right_index = right_index;
        }
    }
}
