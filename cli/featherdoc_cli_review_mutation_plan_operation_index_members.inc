        if (member_name == "kind") {
            if (saw_kind) {
                error_message =
                    "JSON review mutation plan operation member 'kind' must not be duplicated";
                return false;
            }
            saw_kind = true;
            if (!parse_review_mutation_plan_operation_kind(
                    content, index, operation.kind, error_message)) {
                return false;
            }
        } else if (member_name == "paragraph_index") {
            if (saw_paragraph_index) {
                error_message =
                    "JSON review mutation plan operation member 'paragraph_index' must not be duplicated";
                return false;
            }
            saw_paragraph_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.paragraph_index,
                                              "paragraph_index",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "text_offset") {
            if (saw_text_offset) {
                error_message =
                    "JSON review mutation plan operation member 'text_offset' must not be duplicated";
                return false;
            }
            saw_text_offset = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.text_offset,
                                              "text_offset",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "text_length") {
            if (saw_text_length) {
                error_message =
                    "JSON review mutation plan operation member 'text_length' must not be duplicated";
                return false;
            }
            saw_text_length = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.text_length,
                                              "text_length",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "start_paragraph_index") {
            if (saw_start_paragraph_index) {
                error_message =
                    "JSON review mutation plan operation member 'start_paragraph_index' must not be duplicated";
                return false;
            }
            saw_start_paragraph_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.start_paragraph_index,
                                              "start_paragraph_index",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "start_text_offset") {
            if (saw_start_text_offset) {
                error_message =
                    "JSON review mutation plan operation member 'start_text_offset' must not be duplicated";
                return false;
            }
            saw_start_text_offset = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.start_text_offset,
                                              "start_text_offset",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "end_paragraph_index") {
            if (saw_end_paragraph_index) {
                error_message =
                    "JSON review mutation plan operation member 'end_paragraph_index' must not be duplicated";
                return false;
            }
            saw_end_paragraph_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.end_paragraph_index,
                                              "end_paragraph_index",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "end_text_offset") {
            if (saw_end_text_offset) {
                error_message =
                    "JSON review mutation plan operation member 'end_text_offset' must not be duplicated";
                return false;
            }
            saw_end_text_offset = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.end_text_offset,
                                              "end_text_offset",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "comment_index") {
            if (saw_comment_index) {
                error_message =
                    "JSON review mutation plan operation member 'comment_index' must not be duplicated";
                return false;
            }
            saw_comment_index = true;
            if (!parse_json_patch_index_value(content, index,
                                              operation.comment_index,
                                              "comment_index",
                                              error_message)) {
                return false;
            }
        } else if (member_name == "resolved") {
            if (saw_resolved) {
                error_message =
                    "JSON review mutation plan operation member 'resolved' must not be duplicated";
                return false;
            }
            saw_resolved = true;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, operation.resolved, "resolved",
                    error_message)) {
                return false;
            }
