#include "featherdoc.hpp"
#include "image_helpers.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace {
constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
constexpr auto content_types_xml_entry = std::string_view{"[Content_Types].xml"};
constexpr auto office_document_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships"};
constexpr auto image_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"};
constexpr auto wordprocessing_drawing_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"};
constexpr auto drawingml_main_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/drawingml/2006/main"};
constexpr auto drawingml_picture_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/drawingml/2006/picture"};
constexpr auto drawingml_picture_uri = std::string_view{
    "http://schemas.openxmlformats.org/drawingml/2006/picture"};
constexpr auto empty_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"}; 

auto initialize_xml_document(pugi::xml_document &xml_document, std::string_view xml_text)
    -> bool {
    xml_document.reset();
    return static_cast<bool>(
        xml_document.load_buffer(xml_text.data(), xml_text.size()));
}

auto initialize_empty_relationships_document(pugi::xml_document &xml_document) -> bool {
    return initialize_xml_document(xml_document, empty_relationships_xml);
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = xml_offset;
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail = {},
                    std::string entry_name = {},
                    std::optional<std::ptrdiff_t> xml_offset = std::nullopt)
    -> std::error_code {
    return set_last_error(error_info, featherdoc::make_error_code(code),
                          std::move(detail), std::move(entry_name), xml_offset);
}

auto normalize_word_part_entry(std::string_view target) -> std::string {
    std::string normalized_target{target};
    std::replace(normalized_target.begin(), normalized_target.end(), '\\', '/');

    if (!normalized_target.empty() && normalized_target.front() == '/') {
        return std::filesystem::path{normalized_target.substr(1)}
            .lexically_normal()
            .generic_string();
    }

    return (std::filesystem::path{"word"} / std::filesystem::path{normalized_target})
        .lexically_normal()
        .generic_string();
}

auto make_document_relationship_target(std::string_view entry_name) -> std::string {
    const auto normalized_entry = std::filesystem::path{std::string{entry_name}}
                                      .lexically_normal();
    const auto relative_target =
        normalized_entry.lexically_relative(std::filesystem::path{"word"});
    if (!relative_target.empty()) {
        return relative_target.generic_string();
    }

    return normalized_entry.filename().generic_string();
}

void ensure_attribute_value(pugi::xml_node node, const char *name, std::string_view value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(name);
    }
    attribute.set_value(std::string{value}.c_str());
}

auto parse_u32_attribute_value(const char *text) -> std::optional<std::uint32_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoul(text, &end, 10);
    if (end == text || *end != '\0' ||
        value > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(value);
}

void collect_max_drawing_object_id(pugi::xml_node node, std::uint32_t &max_id) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "wp:docPr" || child_name == "pic:cNvPr") {
            if (const auto parsed_id = parse_u32_attribute_value(child.attribute("id").value())) {
                max_id = std::max(max_id, *parsed_id);
            }
        }

        collect_max_drawing_object_id(child, max_id);
    }
}
} // namespace

namespace featherdoc {

bool Document::append_inline_image_part(std::string image_data, std::string extension,
                                        std::string content_type,
                                        std::string display_name,
                                        std::uint32_t width_px,
                                        std::uint32_t height_px) {
    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    return this->append_inline_image_part(body, {}, std::move(image_data),
                                          std::move(extension), std::move(content_type),
                                          std::move(display_name), width_px, height_px);
}

bool Document::append_inline_image_part(pugi::xml_node parent, pugi::xml_node insert_before,
                                        std::string image_data, std::string extension,
                                        std::string content_type,
                                        std::string display_name,
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
                       "image width and height must both be greater than zero");
        return false;
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    auto document_root = this->document.child("w:document");
    if (document_root == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain a valid image insertion parent",
                       std::string{document_xml_entry});
        return false;
    }

    if (this->document_relationships.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(this->document_relationships)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{document_relationships_xml_entry});
        return false;
    }

    auto relationships = this->document_relationships.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "word/_rels/document.xml.rels does not contain a Relationships root",
                       std::string{document_relationships_xml_entry});
        return false;
    }

    auto types = this->content_types.child("Types");
    if (types == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::content_types_xml_parse_failed,
                       "[Content_Types].xml does not contain a Types root",
                       std::string{content_types_xml_entry});
        return false;
    }

    auto relationships_namespace = document_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = document_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    std::unordered_set<std::string> used_relationship_ids;
    std::unordered_set<std::string> used_part_entries;
    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto id = relationship.attribute("Id").value();
        if (*id != '\0') {
            used_relationship_ids.emplace(id);
        }

        if (std::string_view{relationship.attribute("Type").value()} ==
            image_relationship_type) {
            const auto target = std::string_view{relationship.attribute("Target").value()};
            if (!target.empty()) {
                used_part_entries.insert(normalize_word_part_entry(target));
            }
        }
    }

    for (const auto &part : this->image_parts) {
        used_part_entries.insert(part.entry_name);
    }

    std::string relationship_id;
    for (std::size_t next_index = 1U; relationship_id.empty(); ++next_index) {
        auto candidate = "rId" + std::to_string(next_index);
        if (!used_relationship_ids.contains(candidate)) {
            relationship_id = std::move(candidate);
        }
    }

    std::string entry_name;
    for (std::size_t next_index = 1U; entry_name.empty(); ++next_index) {
        auto candidate = std::string{"word/media/image"} +
                         std::to_string(next_index) + "." + extension;
        if (!used_part_entries.contains(candidate)) {
            entry_name = std::move(candidate);
        }
    }

    auto default_node = pugi::xml_node{};
    for (auto existing_default = types.child("Default");
         existing_default != pugi::xml_node{};
         existing_default = existing_default.next_sibling("Default")) {
        if (std::string_view{existing_default.attribute("Extension").value()} ==
            extension) {
            default_node = existing_default;
            break;
        }
    }
    if (default_node == pugi::xml_node{}) {
        default_node = types.append_child("Default");
    }
    if (default_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append an image content type default",
                       std::string{content_types_xml_entry});
        return false;
    }
    ensure_attribute_value(default_node, "Extension", extension);
    ensure_attribute_value(default_node, "ContentType", content_type);

    auto relationship = relationships.append_child("Relationship");
    if (relationship == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append an image relationship",
                       std::string{document_relationships_xml_entry});
        return false;
    }
    ensure_attribute_value(relationship, "Id", relationship_id);
    ensure_attribute_value(relationship, "Type", image_relationship_type);
    ensure_attribute_value(relationship, "Target",
                           make_document_relationship_target(entry_name));

    std::uint32_t max_drawing_id = 0U;
    collect_max_drawing_object_id(document_root, max_drawing_id);
    const auto drawing_id = max_drawing_id + 1U;
    const auto drawing_id_text = std::to_string(drawing_id);
    const auto width_emu_text =
        std::to_string(featherdoc::detail::pixels_to_emu(width_px));
    const auto height_emu_text =
        std::to_string(featherdoc::detail::pixels_to_emu(height_px));
    if (display_name.empty()) {
        display_name = "image-" + drawing_id_text + "." + extension;
    }

    auto image_paragraph = featherdoc::detail::insert_paragraph_node(parent, insert_before);
    if (image_paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to append an image paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    auto run = image_paragraph.append_child("w:r");
    auto drawing = run.append_child("w:drawing");
    auto inline_node = drawing.append_child("wp:inline");
    if (run == pugi::xml_node{} || drawing == pugi::xml_node{} ||
        inline_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to build inline image XML",
                       std::string{document_xml_entry});
        return false;
    }

    ensure_attribute_value(drawing, "xmlns:r", office_document_relationships_namespace_uri);
    ensure_attribute_value(drawing, "xmlns:wp", wordprocessing_drawing_namespace_uri);
    ensure_attribute_value(drawing, "xmlns:a", drawingml_main_namespace_uri);
    ensure_attribute_value(drawing, "xmlns:pic", drawingml_picture_namespace_uri);
    ensure_attribute_value(inline_node, "distT", "0");
    ensure_attribute_value(inline_node, "distB", "0");
    ensure_attribute_value(inline_node, "distL", "0");
    ensure_attribute_value(inline_node, "distR", "0");

    auto extent = inline_node.append_child("wp:extent");
    ensure_attribute_value(extent, "cx", width_emu_text);
    ensure_attribute_value(extent, "cy", height_emu_text);

    auto effect_extent = inline_node.append_child("wp:effectExtent");
    ensure_attribute_value(effect_extent, "l", "0");
    ensure_attribute_value(effect_extent, "t", "0");
    ensure_attribute_value(effect_extent, "r", "0");
    ensure_attribute_value(effect_extent, "b", "0");

    auto doc_properties = inline_node.append_child("wp:docPr");
    ensure_attribute_value(doc_properties, "id", drawing_id_text);
    ensure_attribute_value(doc_properties, "name", display_name);

    auto graphic_frame_properties = inline_node.append_child("wp:cNvGraphicFramePr");
    auto frame_locks = graphic_frame_properties.append_child("a:graphicFrameLocks");
    ensure_attribute_value(frame_locks, "noChangeAspect", "1");

    auto graphic = inline_node.append_child("a:graphic");
    auto graphic_data = graphic.append_child("a:graphicData");
    ensure_attribute_value(graphic_data, "uri", drawingml_picture_uri);

    auto picture = graphic_data.append_child("pic:pic");
    auto non_visual_picture = picture.append_child("pic:nvPicPr");
    auto non_visual_properties = non_visual_picture.append_child("pic:cNvPr");
    ensure_attribute_value(non_visual_properties, "id", drawing_id_text);
    ensure_attribute_value(non_visual_properties, "name", display_name);
    non_visual_picture.append_child("pic:cNvPicPr");

    auto blip_fill = picture.append_child("pic:blipFill");
    auto blip = blip_fill.append_child("a:blip");
    ensure_attribute_value(blip, "r:embed", relationship_id);
    auto stretch = blip_fill.append_child("a:stretch");
    stretch.append_child("a:fillRect");

    auto shape_properties = picture.append_child("pic:spPr");
    auto transform = shape_properties.append_child("a:xfrm");
    auto offset = transform.append_child("a:off");
    ensure_attribute_value(offset, "x", "0");
    ensure_attribute_value(offset, "y", "0");
    auto extents = transform.append_child("a:ext");
    ensure_attribute_value(extents, "cx", width_emu_text);
    ensure_attribute_value(extents, "cy", height_emu_text);
    auto geometry = shape_properties.append_child("a:prstGeom");
    ensure_attribute_value(geometry, "prst", "rect");
    geometry.append_child("a:avLst");

    this->image_parts.push_back({std::move(relationship_id), std::move(entry_name),
                                 std::move(content_type), std::move(image_data)});
    this->has_document_relationships_part = true;
    this->document_relationships_dirty = true;
    this->content_types_dirty = true;
    this->last_error_info.clear();
    return true;
}

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

} // namespace featherdoc
