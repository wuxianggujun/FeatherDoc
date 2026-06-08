std::vector<featherdoc::table_inspection_summary> Document::inspect_tables() {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inspecting tables",
                       std::string{document_xml_entry});
        return {};
    }

    auto summaries = std::vector<featherdoc::table_inspection_summary>{};
    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        summaries.push_back(summarize_table_handle(table_handle, table_index));
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::table_inspection_summary>
Document::inspect_table(std::size_t table_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inspecting tables",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next();
         ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    auto summary = summarize_table_handle(table_handle, table_index);
    this->last_error_info.clear();
    return summary;
}

std::vector<featherdoc::body_block_inspection_summary>
Document::inspect_body_blocks() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting body blocks",
            std::string{document_xml_entry});
        return {};
    }

    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::document_xml_parse_failed,
            "word/document.xml does not contain a w:document/w:body root",
            std::string{document_xml_entry});
        return {};
    }

    auto summaries = std::vector<featherdoc::body_block_inspection_summary>{};
    auto paragraph_index = std::size_t{0U};
    auto table_index = std::size_t{0U};
    auto block_index = std::size_t{0U};
    auto section_index = std::size_t{0U};
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:p") {
            summaries.push_back(featherdoc::body_block_inspection_summary{
                featherdoc::body_block_kind::paragraph, block_index++,
                paragraph_index++, section_index});
            if (child.child("w:pPr").child("w:sectPr") != pugi::xml_node{}) {
                ++section_index;
            }
        } else if (child_name == "w:tbl") {
            summaries.push_back(featherdoc::body_block_inspection_summary{
                featherdoc::body_block_kind::table, block_index++,
                table_index++, section_index});
        }
    }

    this->last_error_info.clear();
    return summaries;
}

table_style_look_consistency_report
Document::check_table_style_look_consistency() {
    auto report = table_style_look_consistency_report{};
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before checking table style look",
            std::string{document_xml_entry});
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

    auto add_issue = [&](std::size_t table_index, const std::string &style_id,
                         std::string issue_type, std::string region,
                         std::string required_flag,
                         std::optional<bool> actual_value,
                         std::string suggestion) {
        auto issue = table_style_look_consistency_issue{};
        issue.table_index = table_index;
        issue.style_id = style_id;
        issue.issue_type = std::move(issue_type);
        issue.region = std::move(region);
        issue.required_style_look_flag = std::move(required_flag);
        issue.actual_value = actual_value;
        issue.expected_value = true;
        issue.suggestion = std::move(suggestion);
        report.issues.push_back(std::move(issue));
    };

    auto required_flag_value = [](const table_style_look &style_look,
                                  std::string_view flag_name) -> bool {
        if (flag_name == "first_row") {
            return style_look.first_row;
        }
        if (flag_name == "last_row") {
            return style_look.last_row;
        }
        if (flag_name == "first_column") {
            return style_look.first_column;
        }
        if (flag_name == "last_column") {
            return style_look.last_column;
        }
        if (flag_name == "banded_rows") {
            return style_look.banded_rows;
        }
        if (flag_name == "banded_columns") {
            return style_look.banded_columns;
        }
        return false;
    };

    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        ++report.table_count;
        const auto style_id = table_handle.style_id();
        if (!style_id.has_value() || style_id->empty()) {
            continue;
        }

        const auto style = find_style_node(styles_root, *style_id);
        if (style == pugi::xml_node{}) {
            add_issue(table_index, *style_id, "missing_style", {}, {},
                      std::nullopt,
                      "create table style '" + *style_id +
                          "' or change this table to an existing table style");
            continue;
        }

        const auto style_summary = summarize_style_node(style);
        if (style_summary.kind != style_kind::table) {
            add_issue(table_index, *style_id, "style_not_table", {}, {},
                      std::nullopt,
                      "replace style id '" + *style_id +
                          "' with a table style before relying on tblLook");
            continue;
        }

        const auto style_look = table_handle.style_look();
        auto check_region = [&](std::string_view xml_region,
                                std::string region_name,
                                std::string flag_name) {
            if (detail::find_table_style_conditional_node(style, xml_region) ==
                pugi::xml_node{}) {
                return;
            }

            auto actual_value = std::optional<bool>{};
            if (style_look.has_value()) {
                actual_value = required_flag_value(*style_look, flag_name);
            }
            if (actual_value.value_or(false)) {
                return;
            }

            const auto issue_type = actual_value.has_value()
                                        ? "style_look_disabled"
                                        : "style_look_missing";
            add_issue(table_index, *style_id, issue_type,
                      std::move(region_name), std::move(flag_name),
                      actual_value,
                      "enable the required tblLook flag for this table");
        };

        check_region("firstRow", "first_row", "first_row");
        check_region("lastRow", "last_row", "last_row");
        check_region("firstCol", "first_column", "first_column");
        check_region("lastCol", "last_column", "last_column");
        check_region("band1Horz", "banded_rows", "banded_rows");
        check_region("band2Horz", "second_banded_rows", "banded_rows");
        check_region("band1Vert", "banded_columns", "banded_columns");
        check_region("band2Vert", "second_banded_columns", "banded_columns");
    }

    this->last_error_info.clear();
    return report;
}

table_style_look_repair_report Document::repair_table_style_look_consistency() {
    auto repair = table_style_look_repair_report{};
    repair.before = this->check_table_style_look_consistency();
    if (this->last_error_info.code) {
        return repair;
    }

    auto required_flags_by_table =
        std::unordered_map<std::size_t, std::vector<std::string>>{};
    for (const auto &issue : repair.before.issues) {
        if ((issue.issue_type != "style_look_missing" &&
             issue.issue_type != "style_look_disabled") ||
            issue.required_style_look_flag.empty()) {
            continue;
        }
        required_flags_by_table[issue.table_index].push_back(
            issue.required_style_look_flag);
    }

    auto enable_required_flag = [](table_style_look &style_look,
                                   std::string_view flag_name) -> bool {
        auto changed = false;
        auto enable_flag = [&changed](bool &value) {
            if (!value) {
                value = true;
                changed = true;
            }
        };

        if (flag_name == "first_row") {
            enable_flag(style_look.first_row);
        } else if (flag_name == "last_row") {
            enable_flag(style_look.last_row);
        } else if (flag_name == "first_column") {
            enable_flag(style_look.first_column);
        } else if (flag_name == "last_column") {
            enable_flag(style_look.last_column);
        } else if (flag_name == "banded_rows") {
            enable_flag(style_look.banded_rows);
        } else if (flag_name == "banded_columns") {
            enable_flag(style_look.banded_columns);
        }

        return changed;
    };

    auto table_handle = this->tables();
    for (std::size_t table_index = 0U; table_handle.has_next();
         ++table_index, table_handle.next()) {
        const auto required_flags = required_flags_by_table.find(table_index);
        if (required_flags == required_flags_by_table.end()) {
            continue;
        }

        const auto existing_style_look = table_handle.style_look();
        auto style_look = existing_style_look.value_or(table_style_look{});
        auto changed = !existing_style_look.has_value();
        for (const auto &flag_name : required_flags->second) {
            changed = enable_required_flag(style_look, flag_name) || changed;
        }
        if (!changed) {
            continue;
        }
        if (!table_handle.set_style_look(style_look)) {
            set_last_error(this->last_error_info,
                           document_errc::document_xml_parse_failed,
                           "failed to update table style look flags",
                           std::string{document_xml_entry});
            return repair;
        }
        ++repair.changed_table_count;
    }

    repair.after = this->check_table_style_look_consistency();
    if (this->last_error_info.code) {
        return repair;
    }

    this->last_error_info.clear();
    return repair;
}

table_style_quality_audit_report Document::audit_table_style_quality() {
    auto report = table_style_quality_audit_report{};
    report.region_audit = this->audit_table_style_regions();
    if (this->last_error_info.code) {
        return report;
    }

    report.inheritance_audit = this->audit_table_style_inheritance();
    if (this->last_error_info.code) {
        return report;
    }

    report.style_look = this->check_table_style_look_consistency();
    if (this->last_error_info.code) {
        return report;
    }

    this->last_error_info.clear();
    return report;
}

table_style_quality_fix_plan Document::plan_table_style_quality_fixes() {
    auto plan = table_style_quality_fix_plan{};
    plan.audit = this->audit_table_style_quality();
    if (this->last_error_info.code) {
        return plan;
    }

    for (const auto &issue : plan.audit.region_audit.issues) {
        auto item = table_style_quality_fix_item{};
        item.source = "region_audit";
        item.issue_type = issue.issue_type;
        item.style_id = issue.style_id;
        item.style_name = issue.style_name;
        item.region = issue.region;
        item.action = "edit_table_style_region";
        item.automatic = false;
        item.suggestion = issue.suggestion;
        plan.items.push_back(std::move(item));
    }

    for (const auto &issue : plan.audit.inheritance_audit.issues) {
        auto item = table_style_quality_fix_item{};
        item.source = "inheritance_audit";
        item.issue_type = issue.issue_type;
        item.style_id = issue.style_id;
        item.style_name = issue.style_name;
        item.action = issue.issue_type == "inheritance_cycle"
                          ? "break_based_on_cycle"
                          : (issue.issue_type == "based_on_not_table"
                                 ? "rebase_table_style"
                                 : "create_or_clear_based_on");
        item.automatic = false;
        item.suggestion = issue.suggestion;
        plan.items.push_back(std::move(item));
    }

    for (const auto &issue : plan.audit.style_look.issues) {
        auto item = table_style_quality_fix_item{};
        item.source = "style_look";
        item.issue_type = issue.issue_type;
        item.table_index = issue.table_index;
        item.style_id = issue.style_id;
        item.region = issue.region;
        item.automatic = (issue.issue_type == "style_look_missing" ||
                          issue.issue_type == "style_look_disabled") &&
                         !issue.required_style_look_flag.empty();
        item.action = item.automatic ? "repair_table_style_look"
                                     : "fix_table_style_reference";
        item.command =
            item.automatic ? "repair-table-style-look --plan-only" : "";
        item.suggestion = issue.suggestion;
        plan.items.push_back(std::move(item));
    }

    this->last_error_info.clear();
    return plan;
}

std::vector<featherdoc::table_cell_inspection_summary>
Document::inspect_table_cells(std::size_t table_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting table cells",
            std::string{document_xml_entry});
        return {};
    }

    auto table_handle = this->tables();
    for (std::size_t current_index = 0U;
         current_index < table_index && table_handle.has_next();
         ++current_index) {
        table_handle.next();
    }

    if (!table_handle.has_next()) {
        this->last_error_info.clear();
        return {};
    }

    auto summaries = collect_table_cell_summaries(table_handle);
    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::table_cell_inspection_summary>
Document::inspect_table_cell(std::size_t table_index, std::size_t row_index,
                             std::size_t cell_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting table cells",
            std::string{document_xml_entry});
        return std::nullopt;
    }

    const auto cells = this->inspect_table_cells(table_index);
    for (const auto &cell : cells) {
        if (cell.row_index == row_index && cell.cell_index == cell_index) {
            this->last_error_info.clear();
            return cell;
        }
    }

    this->last_error_info.clear();
    return std::nullopt;
}

std::optional<featherdoc::table_cell_inspection_summary>
Document::inspect_table_cell_by_grid_column(std::size_t table_index,
                                            std::size_t row_index,
                                            std::size_t grid_column) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting table cells",
            std::string{document_xml_entry});
        return std::nullopt;
    }

    const auto cells = this->inspect_table_cells(table_index);
    for (const auto &cell : cells) {
        if (cell.row_index == row_index && grid_column >= cell.column_index &&
            grid_column - cell.column_index < cell.column_span) {
            this->last_error_info.clear();
            return cell;
        }
    }

    this->last_error_info.clear();
    return std::nullopt;
}

std::vector<featherdoc::paragraph_inspection_summary>
Document::inspect_paragraphs() {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting paragraphs",
            std::string{document_xml_entry});
        return {};
    }

    auto summaries = std::vector<featherdoc::paragraph_inspection_summary>{};
    auto paragraph_handle = this->paragraphs();
    auto numbering_lookup_resolved = false;
    auto numbering_lookup_ready = false;
    for (std::size_t paragraph_index = 0U; paragraph_handle.has_next();
         ++paragraph_index, paragraph_handle.next()) {
        auto summary =
            summarize_paragraph_node(paragraph_handle.current, paragraph_index);
        if (summary.numbering.has_value() &&
            summary.numbering->num_id.has_value()) {
            if (!numbering_lookup_resolved) {
                numbering_lookup_ready =
                    !this->ensure_numbering_loaded() &&
                    this->numbering.child("w:numbering") != pugi::xml_node{};
                numbering_lookup_resolved = true;
            }

            if (numbering_lookup_ready) {
                const auto lookup =
                    this->find_numbering_instance(*summary.numbering->num_id);
                if (lookup.has_value()) {
                    summary.numbering->definition_id = lookup->definition_id;
                    if (!lookup->definition_name.empty()) {
                        summary.numbering->definition_name =
                            lookup->definition_name;
                    }
                }
            }
        }

        summaries.push_back(std::move(summary));
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::paragraph_inspection_summary>
Document::inspect_paragraph(std::size_t paragraph_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting paragraphs",
            std::string{document_xml_entry});
        return std::nullopt;
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    auto summary =
        summarize_paragraph_node(paragraph_handle.current, paragraph_index);
    if (summary.numbering.has_value() &&
        summary.numbering->num_id.has_value()) {
        const auto numbering_lookup_ready =
            !this->ensure_numbering_loaded() &&
            this->numbering.child("w:numbering") != pugi::xml_node{};
        if (numbering_lookup_ready) {
            const auto lookup =
                this->find_numbering_instance(*summary.numbering->num_id);
            if (lookup.has_value()) {
                summary.numbering->definition_id = lookup->definition_id;
                if (!lookup->definition_name.empty()) {
                    summary.numbering->definition_name =
                        lookup->definition_name;
                }
            }
        }
    }

    this->last_error_info.clear();
    return summary;
}

std::vector<featherdoc::run_inspection_summary>
Document::inspect_paragraph_runs(std::size_t paragraph_index) {
    if (!this->is_open()) {
        set_last_error(
            this->last_error_info, document_errc::document_not_open,
            "call open() or create_empty() before inspecting paragraph runs",
            std::string{document_xml_entry});
        return {};
    }

    auto paragraph_handle = this->paragraphs();
    for (std::size_t current_index = 0U;
         current_index < paragraph_index && paragraph_handle.has_next();
         ++current_index) {
        paragraph_handle.next();
    }

    if (!paragraph_handle.has_next()) {
        this->last_error_info.clear();
        return {};
    }

    auto summaries = std::vector<featherdoc::run_inspection_summary>{};
    auto run = paragraph_handle.runs();
    for (std::size_t run_index = 0U; run.has_next(); ++run_index, run.next()) {
        summaries.push_back(summarize_run_handle(run, run_index));
    }

    this->last_error_info.clear();
    return summaries;
}

std::optional<featherdoc::run_inspection_summary>
Document::inspect_paragraph_run(std::size_t paragraph_index,
                                std::size_t run_index) {
    const auto runs = this->inspect_paragraph_runs(paragraph_index);
    if (run_index >= runs.size()) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    return runs[run_index];
}
