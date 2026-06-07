    [[nodiscard]] template_validation_result validate_template(
        std::span<const template_slot_requirement> requirements) const;
    [[nodiscard]] template_validation_result validate_template(
        std::initializer_list<template_slot_requirement> requirements) const;
    [[nodiscard]] featherdoc::template_schema_validation_result
    validate_template_schema(
        std::span<const featherdoc::template_schema_entry> entries) const;
    [[nodiscard]] featherdoc::template_schema_validation_result
    validate_template_schema(
        std::initializer_list<featherdoc::template_schema_entry> entries) const;
    [[nodiscard]] featherdoc::template_schema_validation_result
    validate_template_schema(const featherdoc::template_schema &schema) const;
    [[nodiscard]] std::optional<featherdoc::template_schema_scan_result>
    scan_template_schema(featherdoc::template_schema_scan_options options = {});
    [[nodiscard]] std::optional<featherdoc::template_schema_patch>
    build_template_schema_patch_from_scan(
        const featherdoc::template_schema &baseline,
        featherdoc::template_schema_scan_options options = {});
    [[nodiscard]] std::optional<featherdoc::template_onboarding_result>
    onboard_template(featherdoc::template_onboarding_options options = {});
    [[nodiscard]] std::size_t replace_bookmark_with_paragraphs(
        std::string_view bookmark_name,
        const std::vector<std::string> &paragraphs);
    [[nodiscard]] std::size_t replace_bookmark_with_table_rows(
        std::string_view bookmark_name,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t replace_bookmark_with_table(
        std::string_view bookmark_name,
        const std::vector<std::vector<std::string>> &rows);
    [[nodiscard]] std::size_t
    replace_bookmark_with_image(std::string_view bookmark_name,
                                const std::filesystem::path &image_path);
    [[nodiscard]] std::size_t replace_bookmark_with_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px);
    [[nodiscard]] std::size_t replace_bookmark_with_floating_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        featherdoc::floating_image_options options = {});
    [[nodiscard]] std::size_t replace_bookmark_with_floating_image(
        std::string_view bookmark_name, const std::filesystem::path &image_path,
        std::uint32_t width_px, std::uint32_t height_px,
        featherdoc::floating_image_options options = {});
    [[nodiscard]] std::size_t
    remove_bookmark_block(std::string_view bookmark_name);
    [[nodiscard]] std::size_t
    set_bookmark_block_visibility(std::string_view bookmark_name, bool visible);
    [[nodiscard]] bookmark_block_visibility_result
    apply_bookmark_block_visibility(
        std::span<const bookmark_block_visibility_binding> bindings);
    [[nodiscard]] bookmark_block_visibility_result
    apply_bookmark_block_visibility(
        std::initializer_list<bookmark_block_visibility_binding> bindings);
