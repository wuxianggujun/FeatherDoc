    [[nodiscard]] bool set_paragraph_list(Paragraph paragraph,
                                          featherdoc::list_kind kind,
                                          std::uint32_t level = 0U);
    [[nodiscard]] bool restart_paragraph_list(Paragraph paragraph,
                                              featherdoc::list_kind kind,
                                              std::uint32_t level = 0U);
    [[nodiscard]] bool clear_paragraph_list(Paragraph paragraph);
    [[nodiscard]] std::vector<featherdoc::numbering_definition_summary>
    list_numbering_definitions();
    [[nodiscard]] std::optional<featherdoc::numbering_definition_summary>
    find_numbering_definition(std::uint32_t definition_id);
    [[nodiscard]] std::optional<featherdoc::numbering_instance_lookup_summary>
    find_numbering_instance(std::uint32_t instance_id);
    [[nodiscard]] featherdoc::numbering_catalog export_numbering_catalog();
    [[nodiscard]] featherdoc::numbering_catalog_import_summary
    import_numbering_catalog(const featherdoc::numbering_catalog &catalog);
    [[nodiscard]] std::optional<std::uint32_t> ensure_numbering_definition(
        const featherdoc::numbering_definition &definition);
    [[nodiscard]] bool
    set_paragraph_numbering(Paragraph paragraph,
                            std::uint32_t numbering_definition_id,
                            std::uint32_t level = 0U);
    [[nodiscard]] std::optional<std::string> default_run_font_family();
    [[nodiscard]] std::optional<std::string>
    default_run_east_asia_font_family();
    [[nodiscard]] std::optional<std::string> default_run_language();
    [[nodiscard]] std::optional<std::string> default_run_east_asia_language();
    [[nodiscard]] std::optional<std::string> default_run_bidi_language();
    [[nodiscard]] std::optional<bool> default_run_rtl();
    [[nodiscard]] std::optional<bool> default_paragraph_bidi();
    [[nodiscard]] bool
    set_default_run_font_family(std::string_view font_family);
    [[nodiscard]] bool
    set_default_run_east_asia_font_family(std::string_view font_family);
    [[nodiscard]] bool set_default_run_language(std::string_view language);
    [[nodiscard]] bool
    set_default_run_east_asia_language(std::string_view language);
    [[nodiscard]] bool set_default_run_bidi_language(std::string_view language);
    [[nodiscard]] bool set_default_run_rtl(bool enabled = true);
    [[nodiscard]] bool set_default_paragraph_bidi(bool enabled = true);
    [[nodiscard]] bool clear_default_run_font_family();
    [[nodiscard]] bool clear_default_run_east_asia_font_family();
    [[nodiscard]] bool clear_default_run_primary_language();
    [[nodiscard]] bool clear_default_run_language();
    [[nodiscard]] bool clear_default_run_east_asia_language();
    [[nodiscard]] bool clear_default_run_bidi_language();
    [[nodiscard]] bool clear_default_run_rtl();
    [[nodiscard]] bool clear_default_paragraph_bidi();
    [[nodiscard]] std::vector<featherdoc::style_summary> list_styles();
    [[nodiscard]] std::optional<featherdoc::style_summary>
    find_style(std::string_view style_id);
    [[nodiscard]] std::optional<featherdoc::resolved_style_properties_summary>
    resolve_style_properties(std::string_view style_id);
    [[nodiscard]] std::optional<featherdoc::style_usage_summary>
    find_style_usage(std::string_view style_id);
    [[nodiscard]] std::optional<featherdoc::style_usage_report>
    list_style_usage();
    [[nodiscard]] std::optional<featherdoc::style_refactor_plan>
    plan_style_refactor(
        const std::vector<featherdoc::style_refactor_request> &requests);
    [[nodiscard]] std::optional<featherdoc::style_refactor_plan>
    suggest_style_merges();
    [[nodiscard]] std::optional<featherdoc::style_refactor_apply_result>
    apply_style_refactor(
        const std::vector<featherdoc::style_refactor_request> &requests);
    [[nodiscard]] std::optional<featherdoc::style_refactor_restore_result>
    plan_style_refactor_restore(
        const std::vector<featherdoc::style_refactor_rollback_entry>
            &rollback_entries);
    [[nodiscard]] std::optional<featherdoc::style_refactor_restore_result>
    restore_style_refactor(
        const std::vector<featherdoc::style_refactor_rollback_entry>
            &rollback_entries);
    [[nodiscard]] bool rename_style(std::string_view old_style_id,
                                    std::string_view new_style_id);
    [[nodiscard]] bool merge_style(std::string_view source_style_id,
                                   std::string_view target_style_id);
    [[nodiscard]] std::optional<featherdoc::style_prune_plan>
    plan_prune_unused_styles();
    [[nodiscard]] std::optional<featherdoc::style_prune_summary>
    prune_unused_styles();
    [[nodiscard]] bool ensure_paragraph_style(
        std::string_view style_id,
        const featherdoc::paragraph_style_definition &definition);
    [[nodiscard]] bool ensure_character_style(
        std::string_view style_id,
        const featherdoc::character_style_definition &definition);
    [[nodiscard]] bool
    ensure_table_style(std::string_view style_id,
                       const featherdoc::table_style_definition &definition);
    [[nodiscard]] std::optional<featherdoc::table_style_definition_summary>
    find_table_style_definition(std::string_view style_id);
    [[nodiscard]] featherdoc::table_style_region_audit_report
    audit_table_style_regions(
        std::optional<std::string_view> style_id = std::nullopt);
    [[nodiscard]] featherdoc::table_style_inheritance_audit_report
    audit_table_style_inheritance(
        std::optional<std::string_view> style_id = std::nullopt);
    [[nodiscard]] std::optional<std::string>
    style_run_text_color(std::string_view style_id);
    [[nodiscard]] std::optional<bool> style_run_bold(std::string_view style_id);
    [[nodiscard]] std::optional<bool>
    style_run_italic(std::string_view style_id);
    [[nodiscard]] std::optional<bool>
    style_run_strikethrough(std::string_view style_id);
    [[nodiscard]] std::optional<bool>
    style_run_underline(std::string_view style_id);
    [[nodiscard]] std::optional<double>
    style_run_font_size_points(std::string_view style_id);
    [[nodiscard]] std::optional<std::string>
    style_run_font_family(std::string_view style_id);
    [[nodiscard]] std::optional<std::string>
    style_run_east_asia_font_family(std::string_view style_id);
    [[nodiscard]] std::optional<std::string>
    style_run_language(std::string_view style_id);
    [[nodiscard]] std::optional<std::string>
    style_run_east_asia_language(std::string_view style_id);
    [[nodiscard]] std::optional<std::string>
    style_run_bidi_language(std::string_view style_id);
    [[nodiscard]] std::optional<bool> style_run_rtl(std::string_view style_id);
    [[nodiscard]] std::optional<bool>
    style_paragraph_bidi(std::string_view style_id);
    [[nodiscard]] std::optional<std::string>
    paragraph_style_next_style(std::string_view style_id);
    [[nodiscard]] std::optional<std::uint32_t>
    paragraph_style_outline_level(std::string_view style_id);
    [[nodiscard]] bool
    materialize_style_run_properties(std::string_view style_id);
    [[nodiscard]] bool
    rebase_character_style_based_on(std::string_view style_id,
                                    std::string_view based_on);
    [[nodiscard]] bool set_character_style_based_on(std::string_view style_id,
                                                    std::string_view based_on);
    [[nodiscard]] bool
    clear_character_style_based_on(std::string_view style_id);
    [[nodiscard]] bool
    rebase_paragraph_style_based_on(std::string_view style_id,
                                    std::string_view based_on);
    [[nodiscard]] bool set_paragraph_style_based_on(std::string_view style_id,
                                                    std::string_view based_on);
    [[nodiscard]] bool
    clear_paragraph_style_based_on(std::string_view style_id);
    [[nodiscard]] bool
    set_paragraph_style_next_style(std::string_view style_id,
                                   std::string_view next_style);
    [[nodiscard]] bool
    clear_paragraph_style_next_style(std::string_view style_id);
    [[nodiscard]] bool
    set_paragraph_style_outline_level(std::string_view style_id,
                                      std::uint32_t outline_level);
    [[nodiscard]] bool
    clear_paragraph_style_outline_level(std::string_view style_id);
    [[nodiscard]] bool set_style_run_text_color(std::string_view style_id,
                                                std::string_view text_color);
    [[nodiscard]] bool set_style_run_bold(std::string_view style_id,
                                          bool enabled = true);
    [[nodiscard]] bool set_style_run_italic(std::string_view style_id,
                                            bool enabled = true);
    [[nodiscard]] bool set_style_run_strikethrough(std::string_view style_id,
                                                   bool enabled = true);
    [[nodiscard]] bool set_style_run_underline(std::string_view style_id,
                                               bool enabled = true);
    [[nodiscard]] bool set_style_run_superscript(std::string_view style_id,
                                                 bool enabled = true);
    [[nodiscard]] bool set_style_run_subscript(std::string_view style_id,
                                               bool enabled = true);
    [[nodiscard]] bool set_style_run_font_size_points(std::string_view style_id,
                                                      double font_size_points);
    [[nodiscard]] bool set_style_run_font_family(std::string_view style_id,
                                                 std::string_view font_family);
    [[nodiscard]] bool
    set_style_run_east_asia_font_family(std::string_view style_id,
                                        std::string_view font_family);
    [[nodiscard]] bool set_style_run_language(std::string_view style_id,
                                              std::string_view language);
    [[nodiscard]] bool
    set_style_run_east_asia_language(std::string_view style_id,
                                     std::string_view language);
    [[nodiscard]] bool set_style_run_bidi_language(std::string_view style_id,
                                                   std::string_view language);
    [[nodiscard]] bool set_style_run_rtl(std::string_view style_id,
                                         bool enabled = true);
    [[nodiscard]] bool set_style_paragraph_bidi(std::string_view style_id,
                                                bool enabled = true);
    [[nodiscard]] bool clear_style_run_text_color(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_bold(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_italic(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_strikethrough(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_underline(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_superscript(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_subscript(std::string_view style_id);
    [[nodiscard]] bool
    clear_style_run_font_size_points(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_font_family(std::string_view style_id);
    [[nodiscard]] bool
    clear_style_run_east_asia_font_family(std::string_view style_id);
    [[nodiscard]] bool
    clear_style_run_primary_language(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_language(std::string_view style_id);
    [[nodiscard]] bool
    clear_style_run_east_asia_language(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_bidi_language(std::string_view style_id);
    [[nodiscard]] bool clear_style_run_rtl(std::string_view style_id);
    [[nodiscard]] bool clear_style_paragraph_bidi(std::string_view style_id);
    [[nodiscard]] std::optional<std::uint32_t> ensure_style_linked_numbering(
        const featherdoc::numbering_definition &definition,
        const std::vector<featherdoc::paragraph_style_numbering_link>
            &style_links);
    [[nodiscard]] bool
    set_paragraph_style_numbering(std::string_view style_id,
                                  std::uint32_t numbering_definition_id,
                                  std::uint32_t level = 0U);
    [[nodiscard]] bool
    clear_paragraph_style_numbering(std::string_view style_id);
    [[nodiscard]] bool set_paragraph_style(Paragraph paragraph,
                                           std::string_view style_id);
    [[nodiscard]] bool clear_paragraph_style(Paragraph paragraph);
    [[nodiscard]] bool set_run_style(Run run, std::string_view style_id);
    [[nodiscard]] bool clear_run_style(Run run);
