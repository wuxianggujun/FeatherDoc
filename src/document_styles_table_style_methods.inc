std::optional<featherdoc::table_style_definition_summary>
Document::find_table_style_definition(std::string_view style_id) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading table "
                       "style definitions",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (style_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when reading table style definitions",
            std::string{styles_xml_entry});
        return std::nullopt;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return std::nullopt;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    const auto style = find_style_node(styles_root, style_id);
    if (style == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' was not found in word/styles.xml",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    auto summary = featherdoc::table_style_definition_summary{};
    summary.style = summarize_style_node(style);
    if (summary.style.kind != featherdoc::style_kind::table) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "style id '" + std::string{style_id} +
                           "' is not a table style",
                       std::string{styles_xml_entry});
        return std::nullopt;
    }

    summary.whole_table = detail::read_table_style_region(style, true);
    summary.first_row = detail::read_table_style_conditional_region(style, "firstRow");
    summary.last_row = detail::read_table_style_conditional_region(style, "lastRow");
    summary.first_column =
        detail::read_table_style_conditional_region(style, "firstCol");
    summary.last_column = detail::read_table_style_conditional_region(style, "lastCol");
    summary.banded_rows =
        detail::read_table_style_conditional_region(style, "band1Horz");
    summary.banded_columns =
        detail::read_table_style_conditional_region(style, "band1Vert");
    summary.second_banded_rows =
        detail::read_table_style_conditional_region(style, "band2Horz");
    summary.second_banded_columns =
        detail::read_table_style_conditional_region(style, "band2Vert");

    this->last_error_info.clear();
    return summary;
}

table_style_region_audit_report
Document::audit_table_style_regions(std::optional<std::string_view> style_id) {
    auto report = table_style_region_audit_report{};
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before auditing table style regions",
            std::string{styles_xml_entry});
        return report;
    }

    if (style_id.has_value() && style_id->empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when auditing table style regions",
            std::string{styles_xml_entry});
        return report;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return report;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return report;
    }

    auto add_issue = [&](const style_summary &style, std::string region,
                         std::size_t property_count) {
        auto issue = table_style_region_audit_issue{};
        issue.style_id = style.style_id;
        issue.style_name = style.name;
        issue.region = std::move(region);
        issue.issue_type = "empty_region";
        issue.property_count = property_count;
        issue.suggestion = "remove this empty table style region or add typed "
                           "visual properties";
        report.issues.push_back(std::move(issue));
    };

    auto audit_region = [&](const style_summary &style,
                            std::string_view region_name,
                            pugi::xml_node region_node, bool whole_table,
                            bool declared) {
        if (!declared) {
            return;
        }
        ++report.region_count;
        const auto region = detail::read_table_style_region(region_node, whole_table);
        const auto property_count =
            region.has_value() ? detail::table_style_region_property_count(*region)
                               : 0U;
        if (property_count == 0U) {
            add_issue(style, std::string{region_name}, property_count);
        }
    };

    auto audit_style = [&](pugi::xml_node style) {
        const auto summary = summarize_style_node(style);
        if (summary.kind != style_kind::table) {
            return;
        }
        ++report.table_style_count;
        audit_region(summary, "whole_table", style, true,
                     detail::table_style_whole_region_declared(style));

        auto audit_conditional = [&](std::string_view region_name,
                                     std::string_view xml_region) {
            const auto conditional =
                detail::find_table_style_conditional_node(style, xml_region);
            audit_region(summary, region_name, conditional, false,
                         conditional != pugi::xml_node{});
        };
        audit_conditional("first_row", "firstRow");
        audit_conditional("last_row", "lastRow");
        audit_conditional("first_column", "firstCol");
        audit_conditional("last_column", "lastCol");
        audit_conditional("banded_rows", "band1Horz");
        audit_conditional("banded_columns", "band1Vert");
        audit_conditional("second_banded_rows", "band2Horz");
        audit_conditional("second_banded_columns", "band2Vert");
    };

    if (style_id.has_value()) {
        const auto style = find_style_node(styles_root, *style_id);
        if (style == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{*style_id} +
                               "' was not found in word/styles.xml",
                           std::string{styles_xml_entry});
            return report;
        }
        const auto summary = summarize_style_node(style);
        if (summary.kind != style_kind::table) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{*style_id} +
                               "' is not a table style",
                           std::string{styles_xml_entry});
            return report;
        }
        audit_style(style);
    } else {
        for (auto style = styles_root.child("w:style");
             style != pugi::xml_node{}; style = style.next_sibling("w:style")) {
            audit_style(style);
        }
    }

    this->last_error_info.clear();
    return report;
}

table_style_inheritance_audit_report Document::audit_table_style_inheritance(
    std::optional<std::string_view> style_id) {
    auto report = table_style_inheritance_audit_report{};
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before auditing table "
                       "style inheritance",
                       std::string{styles_xml_entry});
        return report;
    }

    if (style_id.has_value() && style_id->empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "style id must not be empty when auditing table style inheritance",
            std::string{styles_xml_entry});
        return report;
    }

    if (const auto error = this->ensure_styles_loaded()) {
        return report;
    }

    const auto styles_root = this->styles.child("w:styles");
    if (styles_root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::styles_xml_parse_failed,
                       "word/styles.xml does not contain a w:styles root",
                       std::string{styles_xml_entry});
        return report;
    }

    auto add_issue =
        [&](const style_summary &style, std::string based_on_style_id,
            std::string based_on_style_kind, std::string issue_type,
            const std::vector<std::string> &chain, std::string suggestion) {
            auto issue = table_style_inheritance_audit_issue{};
            issue.style_id = style.style_id;
            issue.style_name = style.name;
            issue.based_on_style_id = std::move(based_on_style_id);
            issue.based_on_style_kind = std::move(based_on_style_kind);
            issue.issue_type = std::move(issue_type);
            issue.inheritance_chain = chain;
            issue.suggestion = std::move(suggestion);
            report.issues.push_back(std::move(issue));
        };

    auto audit_style = [&](pugi::xml_node style) {
        const auto root_summary = summarize_style_node(style);
        if (root_summary.kind != style_kind::table) {
            return;
        }
        ++report.table_style_count;

        auto visited = std::unordered_set<std::string>{};
        auto chain = std::vector<std::string>{};
        auto current_style = style;
        auto current_summary = root_summary;
        while (current_style != pugi::xml_node{}) {
            const auto current_id = current_summary.style_id;
            if (!visited.insert(current_id).second) {
                chain.push_back(current_id);
                add_issue(root_summary, current_id, "table",
                          "inheritance_cycle", chain,
                          "break the table style based_on cycle before "
                          "resolving inheritance");
                return;
            }
            chain.push_back(current_id);

            if (!current_summary.based_on.has_value() ||
                current_summary.based_on->empty()) {
                return;
            }

            const auto parent_style =
                find_style_node(styles_root, *current_summary.based_on);
            if (parent_style == pugi::xml_node{}) {
                add_issue(
                    root_summary, *current_summary.based_on, {},
                    "missing_based_on", chain,
                    "create the referenced table style or clear based_on");
                return;
            }

            const auto parent_summary = summarize_style_node(parent_style);
            if (parent_summary.kind != style_kind::table) {
                add_issue(root_summary, parent_summary.style_id,
                          parent_summary.type_name, "based_on_not_table", chain,
                          "rebase this table style to another table style");
                return;
            }

            current_style = parent_style;
            current_summary = parent_summary;
        }
    };

    if (style_id.has_value()) {
        const auto style = find_style_node(styles_root, *style_id);
        if (style == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{*style_id} +
                               "' was not found in word/styles.xml",
                           std::string{styles_xml_entry});
            return report;
        }
        const auto summary = summarize_style_node(style);
        if (summary.kind != style_kind::table) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "style id '" + std::string{*style_id} +
                               "' is not a table style",
                           std::string{styles_xml_entry});
            return report;
        }
        audit_style(style);
    } else {
        for (auto style = styles_root.child("w:style");
             style != pugi::xml_node{}; style = style.next_sibling("w:style")) {
            audit_style(style);
        }
    }

    this->last_error_info.clear();
    return report;
}
