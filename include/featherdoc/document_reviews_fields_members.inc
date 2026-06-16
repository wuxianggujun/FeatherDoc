    [[nodiscard]] std::size_t
    replace_bookmark_text(const std::string &bookmark_name,
                          const std::string &replacement);
    [[nodiscard]] std::size_t replace_bookmark_text(const char *bookmark_name,
                                                    const char *replacement);
    [[nodiscard]] bookmark_fill_result
    fill_bookmarks(std::span<const bookmark_text_binding> bindings);
    [[nodiscard]] bookmark_fill_result
    fill_bookmarks(std::initializer_list<bookmark_text_binding> bindings);
    [[nodiscard]] std::vector<bookmark_summary> list_bookmarks() const;
    [[nodiscard]] std::optional<bookmark_summary>
    find_bookmark(std::string_view bookmark_name) const;
    [[nodiscard]] std::vector<content_control_summary>
    list_content_controls() const;
    [[nodiscard]] std::optional<featherdoc::document_semantic_diff_result>
    compare_semantic(
        const Document &other,
        featherdoc::document_semantic_diff_options options = {}) const;
    [[nodiscard]] std::optional<custom_xml_data_binding_sync_result>
    sync_content_controls_from_custom_xml();
    [[nodiscard]] std::vector<content_control_summary>
    find_content_controls_by_tag(std::string_view tag) const;
    [[nodiscard]] std::vector<content_control_summary>
    find_content_controls_by_alias(std::string_view alias) const;
    [[nodiscard]] std::vector<featherdoc::review_note_summary>
    list_footnotes() const;
    [[nodiscard]] std::size_t append_footnote(std::string_view reference_text,
                                              std::string_view note_text);
    [[nodiscard]] bool replace_footnote(std::size_t note_index,
                                        std::string_view note_text);
    [[nodiscard]] bool remove_footnote(std::size_t note_index);
    [[nodiscard]] std::vector<featherdoc::review_note_summary>
    list_endnotes() const;
    [[nodiscard]] std::size_t append_endnote(std::string_view reference_text,
                                             std::string_view note_text);
    [[nodiscard]] bool replace_endnote(std::size_t note_index,
                                       std::string_view note_text);
    [[nodiscard]] bool remove_endnote(std::size_t note_index);
    [[nodiscard]] std::vector<featherdoc::review_note_summary>
    list_comments() const;
    [[nodiscard]] std::size_t append_comment(std::string_view selected_text,
                                             std::string_view comment_text,
                                             std::string_view author = {},
                                             std::string_view initials = {},
                                             std::string_view date = {});
    [[nodiscard]] std::size_t append_paragraph_text_comment(
        std::size_t paragraph_index, std::size_t text_offset,
        std::size_t text_length, std::string_view comment_text,
        std::string_view author = {}, std::string_view initials = {},
        std::string_view date = {});
    [[nodiscard]] std::size_t append_text_range_comment(
        std::size_t start_paragraph_index, std::size_t start_text_offset,
        std::size_t end_paragraph_index, std::size_t end_text_offset,
        std::string_view comment_text, std::string_view author = {},
        std::string_view initials = {}, std::string_view date = {});
    [[nodiscard]] std::size_t append_comment_reply(
        std::size_t parent_comment_index, std::string_view comment_text,
        std::string_view author = {}, std::string_view initials = {},
        std::string_view date = {});
    [[nodiscard]] bool
    set_comment_metadata(std::size_t comment_index,
                         const featherdoc::comment_metadata_update &metadata);
    [[nodiscard]] bool set_paragraph_text_comment_range(
        std::size_t comment_index, std::size_t paragraph_index,
        std::size_t text_offset, std::size_t text_length);
    [[nodiscard]] bool set_text_range_comment_range(
        std::size_t comment_index, std::size_t start_paragraph_index,
        std::size_t start_text_offset, std::size_t end_paragraph_index,
        std::size_t end_text_offset);
    [[nodiscard]] bool set_comment_resolved(std::size_t comment_index,
                                            bool resolved);
    [[nodiscard]] bool replace_comment(std::size_t comment_index,
                                       std::string_view comment_text);
    [[nodiscard]] bool remove_comment(std::size_t comment_index);
    [[nodiscard]] std::vector<featherdoc::revision_summary>
    list_revisions() const;
    [[nodiscard]] std::size_t
    append_insertion_revision(std::string_view text,
                              std::string_view author = {},
                              std::string_view date = {});
    [[nodiscard]] std::size_t
    append_deletion_revision(std::string_view text,
                             std::string_view author = {},
                             std::string_view date = {});
    [[nodiscard]] bool insert_run_revision_after(std::size_t paragraph_index,
                                                 std::size_t run_index,
                                                 std::string_view text,
                                                 std::string_view author = {},
                                                 std::string_view date = {});
    [[nodiscard]] bool delete_run_revision(std::size_t paragraph_index,
                                           std::size_t run_index,
                                           std::string_view author = {},
                                           std::string_view date = {});
    [[nodiscard]] bool replace_run_revision(std::size_t paragraph_index,
                                            std::size_t run_index,
                                            std::string_view text,
                                            std::string_view author = {},
                                            std::string_view date = {});
    [[nodiscard]] bool insert_paragraph_text_revision(
        std::size_t paragraph_index, std::size_t text_offset,
        std::string_view text, std::string_view author = {},
        std::string_view date = {});
    [[nodiscard]] bool delete_paragraph_text_revision(
        std::size_t paragraph_index, std::size_t text_offset,
        std::size_t text_length, std::string_view author = {},
        std::string_view date = {});
    [[nodiscard]] bool delete_paragraph_text_revision(
        std::size_t paragraph_index, std::size_t text_offset,
        std::size_t text_length,
        const featherdoc::revision_text_range_options &options);
    [[nodiscard]] bool replace_paragraph_text_revision(
        std::size_t paragraph_index, std::size_t text_offset,
        std::size_t text_length, std::string_view text,
        std::string_view author = {}, std::string_view date = {});
    [[nodiscard]] bool replace_paragraph_text_revision(
        std::size_t paragraph_index, std::size_t text_offset,
        std::size_t text_length, std::string_view text,
        const featherdoc::revision_text_range_options &options);
    [[nodiscard]] bool insert_text_range_revision(std::size_t paragraph_index,
                                                  std::size_t text_offset,
                                                  std::string_view text,
                                                  std::string_view author = {},
                                                  std::string_view date = {});
    [[nodiscard]] bool delete_text_range_revision(
        std::size_t start_paragraph_index, std::size_t start_text_offset,
        std::size_t end_paragraph_index, std::size_t end_text_offset,
        std::string_view author = {}, std::string_view date = {});
    [[nodiscard]] bool delete_text_range_revision(
        std::size_t start_paragraph_index, std::size_t start_text_offset,
        std::size_t end_paragraph_index, std::size_t end_text_offset,
        const featherdoc::revision_text_range_options &options);
    [[nodiscard]] bool replace_text_range_revision(
        std::size_t start_paragraph_index, std::size_t start_text_offset,
        std::size_t end_paragraph_index, std::size_t end_text_offset,
        std::string_view text, std::string_view author = {},
        std::string_view date = {});
    [[nodiscard]] bool replace_text_range_revision(
        std::size_t start_paragraph_index, std::size_t start_text_offset,
        std::size_t end_paragraph_index, std::size_t end_text_offset,
        std::string_view text,
        const featherdoc::revision_text_range_options &options);
    [[nodiscard]] std::optional<featherdoc::text_range_preview>
    preview_text_range(std::size_t start_paragraph_index,
                       std::size_t start_text_offset,
                       std::size_t end_paragraph_index,
                       std::size_t end_text_offset) const;
    [[nodiscard]] std::vector<featherdoc::text_range_preview>
    find_text_ranges(std::string_view text) const;
    [[nodiscard]] bool
    set_revision_metadata(std::size_t revision_index,
                          const featherdoc::revision_metadata_update &metadata);
    [[nodiscard]] bool accept_revision(std::size_t revision_index);
    [[nodiscard]] bool reject_revision(std::size_t revision_index);
    [[nodiscard]] std::size_t accept_all_revisions();
    [[nodiscard]] std::size_t reject_all_revisions();
    [[nodiscard]] std::vector<featherdoc::hyperlink_summary>
    list_hyperlinks() const;
    [[nodiscard]] std::size_t append_hyperlink(std::string_view text,
                                               std::string_view target);
    [[nodiscard]] bool replace_hyperlink(std::size_t hyperlink_index,
                                         std::string_view text,
                                         std::string_view target);
    [[nodiscard]] bool remove_hyperlink(std::size_t hyperlink_index);
    [[nodiscard]] std::size_t
    replace_content_control_text_by_tag(std::string_view tag,
                                        std::string_view replacement);
    [[nodiscard]] std::size_t
    replace_content_control_text_by_alias(std::string_view alias,
                                          std::string_view replacement);
    [[nodiscard]] std::size_t set_content_control_form_state_by_tag(
        std::string_view tag,
        const featherdoc::content_control_form_state_options &options);
    [[nodiscard]] std::size_t set_content_control_form_state_by_alias(
        std::string_view alias,
        const featherdoc::content_control_form_state_options &options);
    [[nodiscard]] std::size_t replace_content_control_with_paragraphs_by_tag(
        std::string_view tag, const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_content_control_with_paragraphs_by_alias(
        std::string_view alias, const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_content_control_with_table_rows_by_tag(
        std::string_view tag,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_content_control_with_table_rows_by_alias(
        std::string_view alias,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_content_control_with_table_by_tag(
        std::string_view tag,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_content_control_with_table_by_alias(
        std::string_view alias,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_content_control_with_image_by_tag(
        std::string_view tag, const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_content_control_with_image_by_tag(
        std::string_view tag, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] std::size_t replace_content_control_with_image_by_alias(
        std::string_view alias, const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_content_control_with_image_by_alias(
        std::string_view alias, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px);
