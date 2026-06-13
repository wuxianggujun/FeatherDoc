    struct image_part_state {
        std::string owner_entry_name;
        std::string entry_name;
        std::string content_type;
        std::string data;
    };

    [[nodiscard]] std::uint32_t next_drawing_object_id() const;
    [[nodiscard]] bool
    append_inline_image_part(std::string image_data, std::string extension,
                             std::string content_type, std::string display_name,
                             std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] bool
    append_inline_image_part(pugi::xml_node parent,
                             pugi::xml_node insert_before,
                             std::string image_data, std::string extension,
                             std::string content_type, std::string display_name,
                             std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] bool append_inline_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document,
        std::string_view relationships_entry_name, bool &has_relationships_part,
        bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px);
    [[nodiscard]] bool append_drawing_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document,
        std::string_view relationships_entry_name, bool &has_relationships_part,
        bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px,
        std::optional<featherdoc::floating_image_options> floating_options);
    [[nodiscard]] bool append_floating_image_part(
        std::string image_data, std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px, featherdoc::floating_image_options options);
    [[nodiscard]] bool append_floating_image_part(
        pugi::xml_node parent, pugi::xml_node insert_before,
        std::string image_data, std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px, featherdoc::floating_image_options options);
    [[nodiscard]] bool append_floating_image_part(
        pugi::xml_document &xml_document, std::string_view xml_entry_name,
        pugi::xml_document &relationships_document,
        std::string_view relationships_entry_name, bool &has_relationships_part,
        bool &relationships_dirty, pugi::xml_node parent,
        pugi::xml_node insert_before, std::string image_data,
        std::string extension, std::string content_type,
        std::string display_name, std::uint32_t width_px,
        std::uint32_t height_px, featherdoc::floating_image_options options);
    [[nodiscard]] std::vector<drawing_image_info>
    drawing_images_in_part(std::string_view entry_name) const;
    [[nodiscard]] bool extract_drawing_image_from_part(
        std::string_view entry_name, std::size_t image_index,
        const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_drawing_image_in_part(std::string_view entry_name,
                                                    std::size_t image_index);
    [[nodiscard]] bool
    replace_drawing_image_in_part(std::string_view entry_name,
                                  std::size_t image_index,
                                  const std::filesystem::path &image_path);
    [[nodiscard]] std::vector<inline_image_info>
    inline_images_in_part(std::string_view entry_name) const;
    [[nodiscard]] bool extract_inline_image_from_part(
        std::string_view entry_name, std::size_t image_index,
        const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_inline_image_in_part(std::string_view entry_name,
                                                   std::size_t image_index);
    [[nodiscard]] bool
    replace_inline_image_in_part(std::string_view entry_name,
                                 std::size_t image_index,
                                 const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_bookmark_with_image_in_part(
        pugi::xml_document &xml_document, std::string_view entry_name,
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions);
    [[nodiscard]] std::size_t replace_content_control_with_image_in_part(
        pugi::xml_document &xml_document, std::string_view entry_name,
        std::string_view value, bool match_tag,
        const std::filesystem::path &image_path,
        std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions);
    [[nodiscard]] std::size_t replace_bookmark_with_floating_image_in_part(
        pugi::xml_document &xml_document, std::string_view entry_name,
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::optional<std::pair<std::uint32_t, std::uint32_t>> dimensions,
        featherdoc::floating_image_options options);
