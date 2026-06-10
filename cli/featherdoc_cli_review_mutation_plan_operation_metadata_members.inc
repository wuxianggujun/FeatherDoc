        } else if (member_name == "author") {
            if (saw_author) {
                error_message =
                    "JSON review mutation plan operation member 'author' must not be duplicated";
                return false;
            }
            saw_author = true;
            std::string author;
            if (!parse_json_patch_string(content, index, author,
                                         error_message)) {
                return false;
            }
            operation.author = std::move(author);
        } else if (member_name == "initials") {
            if (saw_initials) {
                error_message =
                    "JSON review mutation plan operation member 'initials' must not be duplicated";
                return false;
            }
            saw_initials = true;
            std::string initials;
            if (!parse_json_patch_string(content, index, initials,
                                         error_message)) {
                return false;
            }
            operation.initials = std::move(initials);
        } else if (member_name == "date") {
            if (saw_date) {
                error_message =
                    "JSON review mutation plan operation member 'date' must not be duplicated";
                return false;
            }
            saw_date = true;
            std::string date;
            if (!parse_json_patch_string(content, index, date, error_message)) {
                return false;
            }
            operation.date = std::move(date);
        } else if (member_name == "clear_author") {
            if (saw_clear_author) {
                error_message =
                    "JSON review mutation plan operation member 'clear_author' must not be duplicated";
                return false;
            }
            saw_clear_author = true;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, operation.clear_author, "clear_author",
                    error_message)) {
                return false;
            }
        } else if (member_name == "clear_initials") {
            if (saw_clear_initials) {
                error_message =
                    "JSON review mutation plan operation member 'clear_initials' must not be duplicated";
                return false;
            }
            saw_clear_initials = true;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, operation.clear_initials,
                    "clear_initials", error_message)) {
                return false;
            }
        } else if (member_name == "clear_date") {
            if (saw_clear_date) {
                error_message =
                    "JSON review mutation plan operation member 'clear_date' must not be duplicated";
                return false;
            }
            saw_clear_date = true;
            if (!parse_review_mutation_plan_bool_value(
                    content, index, operation.clear_date, "clear_date",
                    error_message)) {
                return false;
            }
        } else {
            if (!skip_json_patch_value(content, index, error_message)) {
                return false;
            }
        }
