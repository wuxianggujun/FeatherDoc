#include "document_image_template_part_methods.inc"

bool Document::append_inline_image_part(std::string image_data, std::string extension,
                                        std::string content_type,
                                        std::string display_name,
                                        std::uint32_t width_px,
                                        std::uint32_t height_px) {
    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    return this->append_drawing_image_part(
        this->document, document_xml_entry, this->document_relationships,
        document_relationships_xml_entry, this->has_document_relationships_part,
        this->document_relationships_dirty, body, {}, std::move(image_data),
        std::move(extension), std::move(content_type), std::move(display_name), width_px,
        height_px, std::nullopt);
}

bool Document::append_inline_image_part(pugi::xml_node parent, pugi::xml_node insert_before,
                                        std::string image_data, std::string extension,
                                        std::string content_type,
                                        std::string display_name,
                                        std::uint32_t width_px,
                                        std::uint32_t height_px) {
    return this->append_drawing_image_part(
        this->document, document_xml_entry, this->document_relationships,
        document_relationships_xml_entry, this->has_document_relationships_part,
        this->document_relationships_dirty, parent, insert_before, std::move(image_data),
        std::move(extension), std::move(content_type), std::move(display_name), width_px,
        height_px, std::nullopt);
}

bool Document::append_inline_image_part(
    pugi::xml_document &xml_document, std::string_view xml_entry_name,
    pugi::xml_document &relationships_document, std::string_view relationships_entry_name,
    bool &has_relationships_part, bool &relationships_dirty, pugi::xml_node parent,
    pugi::xml_node insert_before, std::string image_data, std::string extension,
    std::string content_type, std::string display_name, std::uint32_t width_px,
    std::uint32_t height_px) {
    return this->append_drawing_image_part(
        xml_document, xml_entry_name, relationships_document, relationships_entry_name,
        has_relationships_part, relationships_dirty, parent, insert_before,
        std::move(image_data), std::move(extension), std::move(content_type),
        std::move(display_name), width_px, height_px, std::nullopt);
}

bool Document::append_floating_image_part(std::string image_data, std::string extension,
                                          std::string content_type,
                                          std::string display_name,
                                          std::uint32_t width_px,
                                          std::uint32_t height_px,
                                          featherdoc::floating_image_options options) {
    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    return this->append_drawing_image_part(
        this->document, document_xml_entry, this->document_relationships,
        document_relationships_xml_entry, this->has_document_relationships_part,
        this->document_relationships_dirty, body, {}, std::move(image_data),
        std::move(extension), std::move(content_type), std::move(display_name), width_px,
        height_px, std::move(options));
}

bool Document::append_floating_image_part(
    pugi::xml_node parent, pugi::xml_node insert_before, std::string image_data,
    std::string extension, std::string content_type, std::string display_name,
    std::uint32_t width_px, std::uint32_t height_px,
    featherdoc::floating_image_options options) {
    return this->append_drawing_image_part(
        this->document, document_xml_entry, this->document_relationships,
        document_relationships_xml_entry, this->has_document_relationships_part,
        this->document_relationships_dirty, parent, insert_before, std::move(image_data),
        std::move(extension), std::move(content_type), std::move(display_name), width_px,
        height_px, std::move(options));
}

bool Document::append_floating_image_part(
    pugi::xml_document &xml_document, std::string_view xml_entry_name,
    pugi::xml_document &relationships_document, std::string_view relationships_entry_name,
    bool &has_relationships_part, bool &relationships_dirty, pugi::xml_node parent,
    pugi::xml_node insert_before, std::string image_data, std::string extension,
    std::string content_type, std::string display_name, std::uint32_t width_px,
    std::uint32_t height_px, featherdoc::floating_image_options options) {
    return this->append_drawing_image_part(
        xml_document, xml_entry_name, relationships_document, relationships_entry_name,
        has_relationships_part, relationships_dirty, parent, insert_before,
        std::move(image_data), std::move(extension), std::move(content_type),
        std::move(display_name), width_px, height_px, std::move(options));
}

#include "document_image_append_drawing_part_methods.inc"

bool Document::append_image(const std::filesystem::path &image_path) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending an image");
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    return this->append_inline_image_part(std::move(image_info.data),
                                          std::move(image_info.extension),
                                          std::move(image_info.content_type),
                                          image_path.filename().string(),
                                          image_info.width_px, image_info.height_px);
}

bool Document::append_image(const std::filesystem::path &image_path,
                            std::uint32_t width_px,
                            std::uint32_t height_px) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending an image");
        return false;
    }

    if (width_px == 0U || height_px == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    return this->append_inline_image_part(std::move(image_info.data),
                                          std::move(image_info.extension),
                                          std::move(image_info.content_type),
                                          image_path.filename().string(),
                                          width_px, height_px);
}

bool Document::append_floating_image(
    const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a floating image");
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    return this->append_floating_image_part(std::move(image_info.data),
                                            std::move(image_info.extension),
                                            std::move(image_info.content_type),
                                            image_path.filename().string(),
                                            image_info.width_px,
                                            image_info.height_px,
                                            std::move(options));
}

bool Document::append_floating_image(
    const std::filesystem::path &image_path, std::uint32_t width_px,
    std::uint32_t height_px, featherdoc::floating_image_options options) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a floating image");
        return false;
    }

    if (width_px == 0U || height_px == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    return this->append_floating_image_part(std::move(image_info.data),
                                            std::move(image_info.extension),
                                            std::move(image_info.content_type),
                                            image_path.filename().string(),
                                            width_px, height_px,
                                            std::move(options));
}

