#pragma once

#include "featherdoc.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <vector>

#include <pugixml.hpp>

struct zip_t;

namespace featherdoc::detail {

inline constexpr auto document_xml_entry = std::string_view{"word/document.xml"};
inline constexpr auto document_relationships_xml_entry =
    std::string_view{"word/_rels/document.xml.rels"};
inline constexpr auto content_types_xml_entry = std::string_view{"[Content_Types].xml"};
inline constexpr auto office_document_relationships_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships"};
inline constexpr auto image_relationship_type = std::string_view{
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/image"};
inline constexpr auto wordprocessing_drawing_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing"};
inline constexpr auto drawingml_main_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/drawingml/2006/main"};
inline constexpr auto drawingml_picture_namespace_uri = std::string_view{
    "http://schemas.openxmlformats.org/drawingml/2006/picture"};
inline constexpr auto drawingml_picture_uri = std::string_view{
    "http://schemas.openxmlformats.org/drawingml/2006/picture"};

enum class zip_entry_binary_read_status {
    missing = 0,
    read_failed,
    ok,
};

struct drawing_image_reference_state final {
    std::size_t index{};
    featherdoc::drawing_image_placement placement{
        featherdoc::drawing_image_placement::inline_object};
    std::string relationship_id;
    std::string entry_name;
    std::string display_name;
    std::uint32_t width_px{};
    std::uint32_t height_px{};
    std::optional<featherdoc::floating_image_options> floating_options;
    std::optional<std::size_t> body_block_index;
    std::optional<std::size_t> paragraph_index;
    pugi::xml_node drawing_container;
    pugi::xml_node drawing_object;
};

[[nodiscard]] auto initialize_empty_relationships_document(
    pugi::xml_document &xml_document) -> bool;
auto set_last_error(
    featherdoc::document_error_info &error_info, std::error_code code,
    std::string detail = {}, std::string entry_name = {},
    std::optional<std::ptrdiff_t> xml_offset = std::nullopt) -> std::error_code;
auto set_last_error(
    featherdoc::document_error_info &error_info, featherdoc::document_errc code,
    std::string detail = {}, std::string entry_name = {},
    std::optional<std::ptrdiff_t> xml_offset = std::nullopt) -> std::error_code;
[[nodiscard]] auto normalize_word_part_entry(std::string_view target)
    -> std::string;
[[nodiscard]] auto make_part_relationship_target(
    std::string_view source_entry_name, std::string_view target_entry_name)
    -> std::string;
void ensure_attribute_value(pugi::xml_node node, const char *name,
                            std::string_view value);
[[nodiscard]] auto image_content_type_for_extension(std::string_view extension)
    -> std::string;
[[nodiscard]] auto image_extension_from_entry_name(std::string_view entry_name)
    -> std::string;
[[nodiscard]] auto read_zip_entry_binary(
    zip_t *archive, std::string_view entry_name, std::string &content)
    -> zip_entry_binary_read_status;
void collect_drawing_image_references(
    pugi::xml_node node, pugi::xml_node relationships_root,
    std::vector<drawing_image_reference_state> &references);
void collect_body_drawing_image_references(
    pugi::xml_node body, pugi::xml_node relationships_root,
    std::vector<drawing_image_reference_state> &references);
void collect_max_drawing_object_id(pugi::xml_node node, std::uint32_t &max_id);
[[nodiscard]] auto drawing_index_for_inline_image(
    const std::vector<featherdoc::drawing_image_info> &images,
    std::size_t inline_index) -> std::optional<std::size_t>;
[[nodiscard]] auto find_image_relationship_node(
    pugi::xml_node relationships_root, std::string_view relationship_id)
    -> pugi::xml_node;
void collect_relationship_image_targets(
    const pugi::xml_document &source_relationships,
    std::unordered_set<std::string> &used_part_entries);
[[nodiscard]] auto relationships_reference_image_entry(
    const pugi::xml_document &source_relationships, std::string_view entry_name)
    -> bool;
[[nodiscard]] auto signed_pixels_to_emu(std::int32_t pixels) -> std::int64_t;
[[nodiscard]] auto to_xml_reference(
    featherdoc::floating_image_horizontal_reference reference)
    -> std::string_view;
[[nodiscard]] auto to_xml_reference(
    featherdoc::floating_image_vertical_reference reference)
    -> std::string_view;
void append_wrap_mode_node(
    pugi::xml_node drawing_container,
    featherdoc::floating_image_wrap_mode wrap_mode);
[[nodiscard]] auto is_valid_floating_crop(
    const featherdoc::floating_image_crop &crop) -> bool;
void append_crop_node(pugi::xml_node blip_fill,
                      const featherdoc::floating_image_crop &crop);

} // namespace featherdoc::detail
