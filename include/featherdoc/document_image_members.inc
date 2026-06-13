    [[nodiscard]] std::vector<drawing_image_info> drawing_images() const;
    [[nodiscard]] bool
    extract_drawing_image(std::size_t image_index,
                          const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_drawing_image(std::size_t image_index);
    [[nodiscard]] bool
    replace_drawing_image(std::size_t image_index,
                          const std::filesystem::path &image_path);
    [[nodiscard]] std::vector<inline_image_info> inline_images() const;
    [[nodiscard]] bool
    extract_inline_image(std::size_t image_index,
                         const std::filesystem::path &output_path) const;
    [[nodiscard]] bool remove_inline_image(std::size_t image_index);
    [[nodiscard]] bool
    replace_inline_image(std::size_t image_index,
                         const std::filesystem::path &image_path);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path);
    [[nodiscard]] bool append_image(const std::filesystem::path &image_path,
                                    std::uint32_t width_px,
                                    std::uint32_t height_px);
    [[nodiscard]] bool
    append_floating_image(const std::filesystem::path &image_path,
                          featherdoc::floating_image_options options = {});
    [[nodiscard]] bool
    append_floating_image(const std::filesystem::path &image_path,
                          std::uint32_t width_px, std::uint32_t height_px,
                          featherdoc::floating_image_options options = {});
