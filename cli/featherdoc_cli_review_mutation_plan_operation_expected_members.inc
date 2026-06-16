        } else if (member_name == "expected_text") {
            if (saw_expected_text) {
                error_message =
                    "JSON review mutation plan operation member 'expected_text' must not be duplicated";
                return false;
            }
            saw_expected_text = true;
            std::string expected_text;
            if (!parse_json_patch_string(content, index, expected_text,
                                         error_message)) {
                return false;
            }
            operation.expected_text = std::move(expected_text);
        } else if (member_name == "expected_comment_text") {
            if (saw_expected_comment_text) {
                error_message =
                    "JSON review mutation plan operation member 'expected_comment_text' must not be duplicated";
                return false;
            }
            saw_expected_comment_text = true;
            std::string expected_comment_text;
            if (!parse_json_patch_string(content, index,
                                         expected_comment_text,
                                         error_message)) {
                return false;
            }
            operation.expected_comment_text =
                std::move(expected_comment_text);
        } else if (member_name == "expected_resolved") {
            if (saw_expected_resolved) {
                error_message =
                    "JSON review mutation plan operation member 'expected_resolved' must not be duplicated";
                return false;
            }
            saw_expected_resolved = true;
            bool expected_resolved = false;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, expected_resolved, "expected_resolved",
                    error_message)) {
                return false;
            }
            operation.expected_resolved = expected_resolved;
        } else if (member_name == "expected_parent_index") {
            if (saw_expected_parent_index) {
                error_message =
                    "JSON review mutation plan operation member 'expected_parent_index' must not be duplicated";
                return false;
            }
            saw_expected_parent_index = true;
            std::size_t expected_parent_index = 0U;
            if (!parse_json_patch_index_value(
                    content, index, expected_parent_index,
                    "expected_parent_index", error_message)) {
                return false;
            }
            operation.expected_parent_index = expected_parent_index;
