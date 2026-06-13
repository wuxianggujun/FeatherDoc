auto is_table_style_command(std::string_view command) -> bool {
    return command == "set-table-style-id" ||
           command == "clear-table-style-id" ||
           command == "set-table-style-look" ||
           command == "clear-table-style-look" ||
           command == "ensure-table-style" ||
           command == "inspect-table-style" ||
           command == "audit-table-style-regions" ||
           command == "audit-table-style-inheritance" ||
           command == "audit-table-style-quality" ||
           command == "plan-table-style-quality-fixes" ||
           command == "apply-table-style-quality-fixes" ||
           command == "check-table-style-look" ||
           command == "repair-table-style-look";
}

auto run_table_style_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
    if (command == "set-table-style-id") {
        return run_set_table_style_id_command(command, arguments, doc);
    }
    if (command == "clear-table-style-id") {
        return run_clear_table_style_id_command(command, arguments, doc);
    }
    if (command == "set-table-style-look") {
        return run_set_table_style_look_command(command, arguments, doc);
    }
    if (command == "clear-table-style-look") {
        return run_clear_table_style_look_command(command, arguments, doc);
    }
    if (command == "ensure-table-style") {
        return run_ensure_table_style_command(command, arguments, doc);
    }
    if (command == "inspect-table-style") {
        return run_inspect_table_style_command(command, arguments, doc);
    }
    if (command == "audit-table-style-regions") {
        return run_audit_table_style_regions_command(command, arguments, doc);
    }
    if (command == "audit-table-style-inheritance") {
        return run_audit_table_style_inheritance_command(command, arguments, doc);
    }
    if (command == "audit-table-style-quality") {
        return run_audit_table_style_quality_command(command, arguments, doc);
    }
    if (command == "plan-table-style-quality-fixes") {
        return run_plan_table_style_quality_fixes_command(command, arguments, doc);
    }
    if (command == "apply-table-style-quality-fixes") {
        return run_apply_table_style_quality_fixes_command(command, arguments, doc);
    }
    if (command == "check-table-style-look") {
        return run_check_table_style_look_command(command, arguments, doc);
    }
    if (command == "repair-table-style-look") {
        return run_repair_table_style_look_command(command, arguments, doc);
    }

    return 2;
}
