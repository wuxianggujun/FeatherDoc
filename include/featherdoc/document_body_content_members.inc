    [[nodiscard]] std::vector<featherdoc::table_inspection_summary>
    inspect_tables();
    [[nodiscard]] std::optional<featherdoc::table_inspection_summary>
    inspect_table(std::size_t table_index);
    [[nodiscard]] featherdoc::table_style_look_consistency_report
    check_table_style_look_consistency();
    [[nodiscard]] featherdoc::table_style_look_repair_report
    repair_table_style_look_consistency();
    [[nodiscard]] featherdoc::table_style_quality_audit_report
    audit_table_style_quality();
    [[nodiscard]] featherdoc::table_style_quality_fix_plan
    plan_table_style_quality_fixes();
    [[nodiscard]] std::vector<featherdoc::body_block_inspection_summary>
    inspect_body_blocks();
    [[nodiscard]] std::vector<featherdoc::table_cell_inspection_summary>
    inspect_table_cells(std::size_t table_index);
    [[nodiscard]] std::optional<featherdoc::table_cell_inspection_summary>
    inspect_table_cell(std::size_t table_index, std::size_t row_index,
                       std::size_t cell_index);
    [[nodiscard]] std::optional<featherdoc::table_cell_inspection_summary>
    inspect_table_cell_by_grid_column(std::size_t table_index,
                                      std::size_t row_index,
                                      std::size_t grid_column);
    [[nodiscard]] std::vector<featherdoc::paragraph_inspection_summary>
    inspect_paragraphs();
    [[nodiscard]] std::optional<featherdoc::paragraph_inspection_summary>
    inspect_paragraph(std::size_t paragraph_index);
    [[nodiscard]] std::vector<featherdoc::run_inspection_summary>
    inspect_paragraph_runs(std::size_t paragraph_index);
    [[nodiscard]] std::optional<featherdoc::run_inspection_summary>
    inspect_paragraph_run(std::size_t paragraph_index, std::size_t run_index);
    [[nodiscard]] std::vector<featherdoc::omml_summary> list_omml() const;
    [[nodiscard]] bool append_omml(std::string_view omml_xml);
    [[nodiscard]] bool replace_omml(std::size_t omml_index,
                                    std::string_view omml_xml);
    [[nodiscard]] bool remove_omml(std::size_t omml_index);

    Paragraph &paragraphs();
    Table &tables();
    Table append_table(std::size_t row_count = 1U,
                       std::size_t column_count = 1U);
