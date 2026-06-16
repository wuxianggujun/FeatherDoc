        } else if (member_name == "text") {
            if (saw_text) {
                error_message =
                    "JSON review mutation plan operation member 'text' must not be duplicated";
                return false;
            }
            saw_text = true;
            if (!parse_json_patch_string(content, index, operation.text,
                                         error_message)) {
                return false;
            }
        } else if (member_name == "comment_text") {
            if (saw_comment_text) {
                error_message =
                    "JSON review mutation plan operation member 'comment_text' must not be duplicated";
                return false;
            }
            saw_comment_text = true;
            if (!parse_json_patch_string(content, index, operation.text,
                                         error_message)) {
                return false;
            }
