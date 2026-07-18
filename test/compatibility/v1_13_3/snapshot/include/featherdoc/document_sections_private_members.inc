    [[nodiscard]] std::error_code ensure_even_and_odd_headers_enabled();
    [[nodiscard]] std::optional<bool> inspect_even_and_odd_headers_enabled();
    [[nodiscard]] pugi::xml_node
    section_properties(std::size_t section_index) const;
    [[nodiscard]] pugi::xml_node
    ensure_section_properties(std::size_t section_index);
    [[nodiscard]] bool
    ensure_title_page_enabled(pugi::xml_node section_properties);
    Paragraph &related_part_paragraphs_by_relationship_id(
        std::string_view relationship_id,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name);
    Paragraph &section_related_part_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *reference_name, const char *part_root_name);
    [[nodiscard]] xml_part_state *section_related_part_state(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *reference_name);
    Paragraph &ensure_section_related_part_paragraphs(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name, const char *reference_name,
        const char *relationship_type, const char *content_type);
    Paragraph &assign_section_related_part_paragraphs(
        std::size_t section_index, std::size_t part_index,
        featherdoc::section_reference_kind reference_kind,
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name, const char *reference_name);
    [[nodiscard]] bool remove_section_related_part_reference(
        std::size_t section_index,
        featherdoc::section_reference_kind reference_kind,
        const char *reference_name);
    [[nodiscard]] std::error_code ensure_removal_prerequisites_loaded();
    void cleanup_first_page_section_markers();
    [[nodiscard]] std::error_code cleanup_even_and_odd_headers_setting();
    [[nodiscard]] bool
    remove_related_part(std::size_t part_index,
                        std::vector<std::unique_ptr<xml_part_state>> &parts,
                        const char *reference_name);
    [[nodiscard]] bool
    move_related_part(std::size_t source_index, std::size_t target_index,
                      std::vector<std::unique_ptr<xml_part_state>> &parts,
                      const char *relationship_type);
    [[nodiscard]] bool
    copy_section_related_part_references(std::size_t source_section_index,
                                         std::size_t target_section_index,
                                         const char *reference_name);
    Paragraph &ensure_related_part_paragraphs(
        std::vector<std::unique_ptr<xml_part_state>> &parts,
        const char *part_root_name, const char *reference_name,
        const char *relationship_type, const char *content_type);
    [[nodiscard]] xml_part_state *
    find_related_part_state(std::string_view entry_name);
    [[nodiscard]] const xml_part_state *
    find_related_part_state(std::string_view entry_name) const;
