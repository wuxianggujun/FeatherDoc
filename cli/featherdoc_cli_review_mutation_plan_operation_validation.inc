    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
        if (!paragraph_fields_ok || !saw_comment_text) {
            error_message =
                "JSON review mutation plan append_paragraph_text_comment operation must contain 'paragraph_index', 'text_offset', 'text_length', and 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan append_paragraph_text_comment operation member 'comment_text' must not be empty";
            return false;
        }
        if (saw_text) {
            error_message =
                "JSON review mutation plan append_paragraph_text_comment operation does not accept 'text'";
            return false;
        }
        if (saw_comment_index || saw_resolved || saw_expected_resolved ||
            saw_expected_comment_text || saw_expected_parent_index ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan append_paragraph_text_comment operation does not accept 'comment_index', 'resolved', 'expected_resolved', 'expected_comment_text', 'expected_parent_index', or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::append_comment_reply:
        if (!saw_comment_index || !saw_comment_text) {
            error_message =
                "JSON review mutation plan append_comment_reply operation must contain 'comment_index' and 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan append_comment_reply operation member 'comment_text' must not be empty";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_resolved || clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan append_comment_reply operation only accepts 'comment_index', 'comment_text', 'expected_text', 'expected_comment_text', 'expected_resolved', 'expected_parent_index', 'author', 'initials', and 'date'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::replace_comment:
        if (!saw_comment_index || !saw_comment_text) {
            error_message =
                "JSON review mutation plan replace_comment operation must contain 'comment_index' and 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan replace_comment operation member 'comment_text' must not be empty";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_resolved || saw_author || saw_initials || saw_date ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan replace_comment operation only accepts 'comment_index', 'comment_text', 'expected_text', 'expected_comment_text', 'expected_resolved', and 'expected_parent_index'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::remove_comment:
        if (!saw_comment_index) {
            error_message =
                "JSON review mutation plan remove_comment operation must contain 'comment_index'";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_comment_text || saw_resolved || saw_author || saw_initials ||
            saw_date || clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan remove_comment operation only accepts 'comment_index', 'expected_text', 'expected_comment_text', 'expected_resolved', and 'expected_parent_index'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::set_comment_resolved:
        if (!saw_comment_index || !saw_resolved) {
            error_message =
                "JSON review mutation plan set_comment_resolved operation must contain 'comment_index' and 'resolved'";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_comment_text || saw_author || saw_initials || saw_date ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan set_comment_resolved operation only accepts 'comment_index', 'resolved', 'expected_text', 'expected_comment_text', 'expected_resolved', and 'expected_parent_index'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::set_comment_metadata:
        if (!saw_comment_index) {
            error_message =
                "JSON review mutation plan set_comment_metadata operation must contain 'comment_index'";
            return false;
        }
        if (!has_comment_metadata_update) {
            error_message =
                "JSON review mutation plan set_comment_metadata operation must contain at least one metadata update field";
            return false;
        }
        if ((saw_author && operation.clear_author) ||
            (saw_initials && operation.clear_initials) ||
            (saw_date && operation.clear_date)) {
            error_message =
                "JSON review mutation plan set_comment_metadata operation cannot set and clear the same metadata field";
            return false;
        }
        if (saw_paragraph_index || saw_text_offset || saw_text_length ||
            saw_start_paragraph_index || saw_start_text_offset ||
            saw_end_paragraph_index || saw_end_text_offset || saw_text ||
            saw_comment_text || saw_resolved) {
            error_message =
                "JSON review mutation plan set_comment_metadata operation only accepts 'comment_index', metadata update fields, 'expected_text', 'expected_comment_text', 'expected_resolved', and 'expected_parent_index'";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        if (!saw_paragraph_index || !saw_text_offset || !saw_text) {
            error_message =
                "JSON review mutation plan insert_paragraph_text_revision operation must contain 'paragraph_index', 'text_offset', and 'text'";
            return false;
        }
        if (saw_text_length || saw_comment_text || saw_expected_text ||
            saw_expected_comment_text || saw_expected_resolved ||
            saw_expected_parent_index || saw_comment_index || saw_resolved ||
            saw_initials || clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan insert_paragraph_text_revision operation does not accept comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
        if (!paragraph_fields_ok) {
            error_message =
                "JSON review mutation plan delete_paragraph_text_revision operation must contain 'paragraph_index', 'text_offset', and 'text_length'";
            return false;
        }
        if (saw_text || saw_comment_text || saw_expected_comment_text ||
            saw_expected_resolved || saw_expected_parent_index ||
            saw_comment_index || saw_resolved || saw_initials ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan delete_paragraph_text_revision operation does not accept text, comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        if (!paragraph_fields_ok || !saw_text) {
            error_message =
                "JSON review mutation plan replace_paragraph_text_revision operation must contain 'paragraph_index', 'text_offset', 'text_length', and 'text'";
            return false;
        }
        if (saw_comment_text || saw_expected_comment_text ||
            saw_expected_resolved || saw_expected_parent_index ||
            saw_comment_index || saw_resolved || saw_initials ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan replace_paragraph_text_revision operation does not accept comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::append_text_range_comment:
        if (!text_range_fields_ok || !saw_comment_text) {
            error_message =
                "JSON review mutation plan append_text_range_comment operation must contain 'start_paragraph_index', 'start_text_offset', 'end_paragraph_index', 'end_text_offset', and 'comment_text'";
            return false;
        }
        if (operation.text.empty()) {
            error_message =
                "JSON review mutation plan append_text_range_comment operation member 'comment_text' must not be empty";
            return false;
        }
        if (saw_text) {
            error_message =
                "JSON review mutation plan append_text_range_comment operation does not accept 'text'";
            return false;
        }
        if (saw_comment_index || saw_resolved || saw_expected_resolved ||
            saw_expected_comment_text || saw_expected_parent_index ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan append_text_range_comment operation does not accept 'comment_index', 'resolved', 'expected_resolved', 'expected_comment_text', 'expected_parent_index', or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        if (!text_range_insertion_fields_ok || !saw_text) {
            error_message =
                "JSON review mutation plan insert_text_range_revision operation must contain 'start_paragraph_index', 'start_text_offset', and 'text'";
            return false;
        }
        if (saw_end_paragraph_index || saw_end_text_offset ||
            saw_comment_text || saw_expected_text ||
            saw_expected_comment_text || saw_expected_resolved ||
            saw_expected_parent_index || saw_comment_index || saw_resolved ||
            saw_initials || clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan insert_text_range_revision operation does not accept range end, comment, expected text/comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::delete_text_range_revision:
        if (!text_range_fields_ok) {
            error_message =
                "JSON review mutation plan delete_text_range_revision operation must contain 'start_paragraph_index', 'start_text_offset', 'end_paragraph_index', and 'end_text_offset'";
            return false;
        }
        if (saw_text || saw_comment_text || saw_expected_comment_text ||
            saw_expected_resolved || saw_expected_parent_index ||
            saw_comment_index || saw_resolved || saw_initials ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan delete_text_range_revision operation does not accept text, comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        if (!text_range_fields_ok || !saw_text) {
            error_message =
                "JSON review mutation plan replace_text_range_revision operation must contain 'start_paragraph_index', 'start_text_offset', 'end_paragraph_index', 'end_text_offset', and 'text'";
            return false;
        }
        if (saw_comment_text || saw_expected_comment_text ||
            saw_expected_resolved || saw_expected_parent_index ||
            saw_comment_index || saw_resolved || saw_initials ||
            clear_metadata_fields_seen) {
            error_message =
                "JSON review mutation plan replace_text_range_revision operation does not accept comment, expected comment, resolved, initials, or clear metadata fields";
            return false;
        }
        return true;
    }

    return false;
