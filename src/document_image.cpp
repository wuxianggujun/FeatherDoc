#include "featherdoc.hpp"
#include "image_helpers.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include <zip.h>

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

auto make_part_relationship_target(std::string_view source_entry_name,
                                   std::string_view target_entry_name) -> std::string {
    const auto normalized_source =
        std::filesystem::path{std::string{source_entry_name}}.lexically_normal();
    const auto normalized_target =
        std::filesystem::path{std::string{target_entry_name}}.lexically_normal();
    const auto relative_target =
        normalized_target.lexically_relative(normalized_source.parent_path());
    if (!relative_target.empty()) {
        return relative_target.generic_string();
    }

    return normalized_target.filename().generic_string();
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

auto parse_u64_attribute_value(const char *text) -> std::optional<std::uint64_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoull(text, &end, 10);
    if (end == text || *end != '\0') {
        return std::nullopt;
    }

    return static_cast<std::uint64_t>(value);
}

auto parse_i64_attribute_value(const char *text) -> std::optional<std::int64_t> {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    char *end = nullptr;
    const auto value = std::strtoll(text, &end, 10);
    if (end == text || *end != '\0') {
        return std::nullopt;
    }

    return static_cast<std::int64_t>(value);
}

auto to_lower_ascii(std::string text) -> std::string {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return text;
}

auto image_content_type_for_extension(std::string_view extension) -> std::string {
    if (extension == "png") {
        return "image/png";
    }
    if (extension == "jpg" || extension == "jpeg") {
        return "image/jpeg";
    }
    if (extension == "gif") {
        return "image/gif";
    }
    if (extension == "bmp") {
        return "image/bmp";
    }
    if (extension == "svg") {
        return "image/svg+xml";
    }
    if (extension == "webp") {
        return "image/webp";
    }
    if (extension == "tif" || extension == "tiff") {
        return "image/tiff";
    }

    return {};
}

auto image_extension_from_entry_name(std::string_view entry_name) -> std::string {
    auto extension = std::filesystem::path{std::string{entry_name}}.extension().string();
    if (!extension.empty() && extension.front() == '.') {
        extension.erase(extension.begin());
    }
    return to_lower_ascii(std::move(extension));
}

auto emu_to_pixels(std::uint64_t emu) -> std::uint32_t {
    constexpr std::uint64_t emu_per_pixel = 9525U;
    const auto rounded_pixels = (emu + (emu_per_pixel / 2U)) / emu_per_pixel;
    return static_cast<std::uint32_t>(
        std::min<std::uint64_t>(rounded_pixels,
                                std::numeric_limits<std::uint32_t>::max()));
}

auto signed_emu_to_pixels(std::int64_t emu) -> std::int32_t {
    constexpr std::int64_t emu_per_pixel = 9525;
    const auto absolute_emu = emu < 0 ? -emu : emu;
    const auto rounded_pixels = (absolute_emu + (emu_per_pixel / 2)) / emu_per_pixel;
    const auto signed_pixels = emu < 0 ? -rounded_pixels : rounded_pixels;
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(
        signed_pixels, std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()));
}

enum class zip_entry_binary_read_status {
    missing = 0,
    read_failed,
    ok,
};

auto read_zip_entry_binary(zip_t *archive, std::string_view entry_name, std::string &content)
    -> zip_entry_binary_read_status {
    content.clear();

    if (zip_entry_open(archive, entry_name.data()) != 0) {
        return zip_entry_binary_read_status::missing;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0U;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(archive);
    if (read_result < 0 || close_result != 0 || buffer == nullptr) {
        std::free(buffer);
        return zip_entry_binary_read_status::read_failed;
    }

    const auto *buffer_begin = static_cast<const char *>(buffer);
    content.assign(buffer_begin, buffer_begin + buffer_size);
    std::free(buffer);
    return zip_entry_binary_read_status::ok;
}

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

auto parse_horizontal_reference(
    std::string_view text,
    featherdoc::floating_image_horizontal_reference &reference) -> bool {
    if (text == "page") {
        reference = featherdoc::floating_image_horizontal_reference::page;
        return true;
    }
    if (text == "margin") {
        reference = featherdoc::floating_image_horizontal_reference::margin;
        return true;
    }
    if (text == "column") {
        reference = featherdoc::floating_image_horizontal_reference::column;
        return true;
    }
    if (text == "character") {
        reference = featherdoc::floating_image_horizontal_reference::character;
        return true;
    }

    return false;
}

auto parse_vertical_reference(
    std::string_view text,
    featherdoc::floating_image_vertical_reference &reference) -> bool {
    if (text == "page") {
        reference = featherdoc::floating_image_vertical_reference::page;
        return true;
    }
    if (text == "margin") {
        reference = featherdoc::floating_image_vertical_reference::margin;
        return true;
    }
    if (text == "paragraph") {
        reference = featherdoc::floating_image_vertical_reference::paragraph;
        return true;
    }
    if (text == "line") {
        reference = featherdoc::floating_image_vertical_reference::line;
        return true;
    }

    return false;
}

auto parse_wrap_mode(pugi::xml_node drawing_node)
    -> featherdoc::floating_image_wrap_mode {
    if (drawing_node.child("wp:wrapSquare") != pugi::xml_node{}) {
        return featherdoc::floating_image_wrap_mode::square;
    }
    if (drawing_node.child("wp:wrapTopAndBottom") != pugi::xml_node{}) {
        return featherdoc::floating_image_wrap_mode::top_bottom;
    }

    return featherdoc::floating_image_wrap_mode::none;
}

auto parse_crop_per_mille_attribute(const char *text) -> std::uint32_t {
    const auto parsed = parse_u32_attribute_value(text);
    if (!parsed.has_value()) {
        return 0U;
    }

    return std::min<std::uint32_t>((*parsed + 50U) / 100U, 1000U);
}

auto parse_floating_crop(pugi::xml_node drawing_node)
    -> std::optional<featherdoc::floating_image_crop> {
    const auto src_rect =
        drawing_node.child("a:graphic")
            .child("a:graphicData")
            .child("pic:pic")
            .child("pic:blipFill")
            .child("a:srcRect");
    if (src_rect == pugi::xml_node{}) {
        return std::nullopt;
    }

    return featherdoc::floating_image_crop{
        parse_crop_per_mille_attribute(src_rect.attribute("l").value()),
        parse_crop_per_mille_attribute(src_rect.attribute("t").value()),
        parse_crop_per_mille_attribute(src_rect.attribute("r").value()),
        parse_crop_per_mille_attribute(src_rect.attribute("b").value()),
    };
}

auto parse_floating_options(pugi::xml_node drawing_node)
    -> std::optional<featherdoc::floating_image_options> {
    if (drawing_node == pugi::xml_node{} ||
        std::string_view{drawing_node.name()} != "wp:anchor") {
        return std::nullopt;
    }

    featherdoc::floating_image_options options;
    options.behind_text =
        std::string_view{drawing_node.attribute("behindDoc").value()} == "1";
    options.allow_overlap =
        std::string_view{drawing_node.attribute("allowOverlap").value()} != "0";
    if (const auto relative_height =
            parse_u32_attribute_value(drawing_node.attribute("relativeHeight").value())) {
        options.z_order = *relative_height;
    }
    options.wrap_mode = parse_wrap_mode(drawing_node);

    if (const auto distance =
            parse_u64_attribute_value(drawing_node.attribute("distL").value())) {
        options.wrap_distance_left_px = emu_to_pixels(*distance);
    }
    if (const auto distance =
            parse_u64_attribute_value(drawing_node.attribute("distR").value())) {
        options.wrap_distance_right_px = emu_to_pixels(*distance);
    }
    if (const auto distance =
            parse_u64_attribute_value(drawing_node.attribute("distT").value())) {
        options.wrap_distance_top_px = emu_to_pixels(*distance);
    }
    if (const auto distance =
            parse_u64_attribute_value(drawing_node.attribute("distB").value())) {
        options.wrap_distance_bottom_px = emu_to_pixels(*distance);
    }

    const auto position_horizontal = drawing_node.child("wp:positionH");
    parse_horizontal_reference(position_horizontal.attribute("relativeFrom").value(),
                               options.horizontal_reference);
    if (const auto horizontal_offset = parse_i64_attribute_value(
            position_horizontal.child("wp:posOffset").child_value())) {
        options.horizontal_offset_px = signed_emu_to_pixels(*horizontal_offset);
    }

    const auto position_vertical = drawing_node.child("wp:positionV");
    parse_vertical_reference(position_vertical.attribute("relativeFrom").value(),
                             options.vertical_reference);
    if (const auto vertical_offset = parse_i64_attribute_value(
            position_vertical.child("wp:posOffset").child_value())) {
        options.vertical_offset_px = signed_emu_to_pixels(*vertical_offset);
    }

    options.crop = parse_floating_crop(drawing_node);
    return options;
}

void collect_drawing_image_reference(
    pugi::xml_node drawing_container, pugi::xml_node drawing_node,
    featherdoc::drawing_image_placement placement, pugi::xml_node relationships_root,
    std::optional<std::size_t> body_block_index,
    std::optional<std::size_t> paragraph_index,
    std::vector<drawing_image_reference_state> &references) {
    if (drawing_node == pugi::xml_node{} || relationships_root == pugi::xml_node{}) {
        return;
    }

    const auto blip =
        drawing_node.child("a:graphic")
            .child("a:graphicData")
            .child("pic:pic")
            .child("pic:blipFill")
            .child("a:blip");
    const auto relationship_id = std::string_view{blip.attribute("r:embed").value()};
    if (relationship_id.empty()) {
        return;
    }

    auto relationship_node = pugi::xml_node{};
    for (auto relationship = relationships_root.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Id").value()} != relationship_id ||
            std::string_view{relationship.attribute("Type").value()} !=
                image_relationship_type) {
            continue;
        }

        relationship_node = relationship;
        break;
    }

    const auto target_entry = relationship_node == pugi::xml_node{}
        ? std::string{}
        : normalize_word_part_entry(relationship_node.attribute("Target").value());
    if (target_entry.empty()) {
        return;
    }

    auto display_name =
        std::string{drawing_node.child("wp:docPr").attribute("name").value()};
    if (display_name.empty()) {
        display_name =
            drawing_node.child("a:graphic")
                .child("a:graphicData")
                .child("pic:pic")
                .child("pic:nvPicPr")
                .child("pic:cNvPr")
                .attribute("name")
                .value();
    }

    const auto width_emu =
        parse_u64_attribute_value(drawing_node.child("wp:extent").attribute("cx").value());
    const auto height_emu =
        parse_u64_attribute_value(drawing_node.child("wp:extent").attribute("cy").value());

    references.push_back(
        {references.size(), placement, std::string{relationship_id}, target_entry,
         std::move(display_name), width_emu.has_value() ? emu_to_pixels(*width_emu) : 0U,
         height_emu.has_value() ? emu_to_pixels(*height_emu) : 0U,
         parse_floating_options(drawing_node), body_block_index, paragraph_index,
         drawing_container, drawing_node});
}

void collect_drawing_image_references(pugi::xml_node node, pugi::xml_node relationships_root,
                                      std::optional<std::size_t> body_block_index,
                                      std::optional<std::size_t> paragraph_index,
                                      std::vector<drawing_image_reference_state> &references) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:drawing") {
            collect_drawing_image_reference(
                child, child.child("wp:inline"),
                featherdoc::drawing_image_placement::inline_object, relationships_root,
                body_block_index, paragraph_index, references);
            collect_drawing_image_reference(
                child, child.child("wp:anchor"),
                featherdoc::drawing_image_placement::anchored_object, relationships_root,
                body_block_index, paragraph_index, references);
        }

        collect_drawing_image_references(child, relationships_root, body_block_index,
                                         paragraph_index, references);
    }
}

void collect_drawing_image_references(pugi::xml_node node,
                                      pugi::xml_node relationships_root,
                                      std::vector<drawing_image_reference_state> &references) {
    collect_drawing_image_references(node, relationships_root, std::nullopt,
                                     std::nullopt, references);
}

void collect_body_drawing_image_references(
    pugi::xml_node body, pugi::xml_node relationships_root,
    std::vector<drawing_image_reference_state> &references) {
    auto body_block_index = std::size_t{0U};
    auto paragraph_index = std::size_t{0U};
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:p") {
            collect_drawing_image_references(child, relationships_root,
                                             body_block_index, paragraph_index,
                                             references);
            ++body_block_index;
            ++paragraph_index;
        } else if (child_name == "w:tbl") {
            collect_drawing_image_references(child, relationships_root,
                                             body_block_index, std::nullopt,
                                             references);
            ++body_block_index;
        } else if (child_name == "w:sdt") {
            collect_drawing_image_references(child, relationships_root,
                                             body_block_index, std::nullopt,
                                             references);
            ++body_block_index;
        }
    }
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

auto drawing_index_for_inline_image(
    const std::vector<featherdoc::drawing_image_info> &images, std::size_t inline_index)
    -> std::optional<std::size_t> {
    std::size_t current_inline_index = 0U;
    for (const auto &image : images) {
        if (image.placement != featherdoc::drawing_image_placement::inline_object) {
            continue;
        }

        if (current_inline_index == inline_index) {
            return image.index;
        }

        ++current_inline_index;
    }

    return std::nullopt;
}

auto find_image_relationship_node(pugi::xml_node relationships_root,
                                  std::string_view relationship_id) -> pugi::xml_node {
    for (auto relationship = relationships_root.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Id").value()} == relationship_id &&
            std::string_view{relationship.attribute("Type").value()} ==
                image_relationship_type) {
            return relationship;
        }
    }

    return {};
}

void collect_relationship_image_targets(const pugi::xml_document &source_relationships,
                                       std::unordered_set<std::string> &used_part_entries) {
    const auto source_root = source_relationships.child("Relationships");
    for (auto relationship = source_root.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            image_relationship_type) {
            continue;
        }

        const auto target = std::string_view{relationship.attribute("Target").value()};
        if (!target.empty()) {
            used_part_entries.insert(normalize_word_part_entry(target));
        }
    }
}

auto relationships_reference_image_entry(const pugi::xml_document &source_relationships,
                                         std::string_view entry_name) -> bool {
    const auto source_root = source_relationships.child("Relationships");
    for (auto relationship = source_root.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        if (std::string_view{relationship.attribute("Type").value()} !=
            image_relationship_type) {
            continue;
        }

        if (normalize_word_part_entry(relationship.attribute("Target").value()) ==
            entry_name) {
            return true;
        }
    }

    return false;
}

auto signed_pixels_to_emu(std::int32_t pixels) -> std::int64_t {
    return static_cast<std::int64_t>(pixels) * 9525;
}

auto to_xml_reference(
    featherdoc::floating_image_horizontal_reference reference) -> std::string_view {
    switch (reference) {
    case featherdoc::floating_image_horizontal_reference::page:
        return "page";
    case featherdoc::floating_image_horizontal_reference::margin:
        return "margin";
    case featherdoc::floating_image_horizontal_reference::column:
        return "column";
    case featherdoc::floating_image_horizontal_reference::character:
        return "character";
    }

    return "column";
}

auto to_xml_reference(
    featherdoc::floating_image_vertical_reference reference) -> std::string_view {
    switch (reference) {
    case featherdoc::floating_image_vertical_reference::page:
        return "page";
    case featherdoc::floating_image_vertical_reference::margin:
        return "margin";
    case featherdoc::floating_image_vertical_reference::paragraph:
        return "paragraph";
    case featherdoc::floating_image_vertical_reference::line:
        return "line";
    }

    return "paragraph";
}

void append_wrap_mode_node(
    pugi::xml_node drawing_container,
    featherdoc::floating_image_wrap_mode wrap_mode) {
    switch (wrap_mode) {
    case featherdoc::floating_image_wrap_mode::none:
        drawing_container.append_child("wp:wrapNone");
        return;
    case featherdoc::floating_image_wrap_mode::square: {
        auto wrap_square = drawing_container.append_child("wp:wrapSquare");
        ensure_attribute_value(wrap_square, "wrapText", "bothSides");
        return;
    }
    case featherdoc::floating_image_wrap_mode::top_bottom:
        drawing_container.append_child("wp:wrapTopAndBottom");
        return;
    }

    drawing_container.append_child("wp:wrapNone");
}

auto is_valid_floating_crop(
    const featherdoc::floating_image_crop &crop) -> bool {
    constexpr std::uint32_t max_crop_per_mille = 1000U;
    if (crop.left_per_mille > max_crop_per_mille ||
        crop.top_per_mille > max_crop_per_mille ||
        crop.right_per_mille > max_crop_per_mille ||
        crop.bottom_per_mille > max_crop_per_mille) {
        return false;
    }

    const auto horizontal_crop =
        static_cast<std::uint64_t>(crop.left_per_mille) + crop.right_per_mille;
    const auto vertical_crop =
        static_cast<std::uint64_t>(crop.top_per_mille) + crop.bottom_per_mille;
    return horizontal_crop < max_crop_per_mille &&
           vertical_crop < max_crop_per_mille;
}

void append_crop_node(
    pugi::xml_node blip_fill,
    const featherdoc::floating_image_crop &crop) {
    auto src_rect = blip_fill.append_child("a:srcRect");
    ensure_attribute_value(src_rect, "l",
                           std::to_string(crop.left_per_mille * 100U));
    ensure_attribute_value(src_rect, "t",
                           std::to_string(crop.top_per_mille * 100U));
    ensure_attribute_value(src_rect, "r",
                           std::to_string(crop.right_per_mille * 100U));
    ensure_attribute_value(src_rect, "b",
                           std::to_string(crop.bottom_per_mille * 100U));
}
} // namespace

namespace featherdoc {

std::uint32_t Document::next_drawing_object_id() const {
    std::uint32_t max_drawing_id = 0U;

    if (const auto document_root = this->document.document_element();
        document_root != pugi::xml_node{}) {
        collect_max_drawing_object_id(document_root, max_drawing_id);
    }

    for (const auto &part : this->header_parts) {
        if (const auto root = part->xml.document_element(); root != pugi::xml_node{}) {
            collect_max_drawing_object_id(root, max_drawing_id);
        }
    }

    for (const auto &part : this->footer_parts) {
        if (const auto root = part->xml.document_element(); root != pugi::xml_node{}) {
            collect_max_drawing_object_id(root, max_drawing_id);
        }
    }

    return max_drawing_id + 1U;
}

std::vector<drawing_image_info> Document::drawing_images_in_part(
    std::string_view entry_name) const {
    std::vector<drawing_image_info> images;

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before reading drawing images");
        return images;
    }

    pugi::xml_node root;
    const pugi::xml_document *relationships_document = nullptr;
    if (entry_name == document_xml_entry) {
        root = this->document.child("w:document").child("w:body");
        relationships_document = &this->document_relationships;
    } else if (const auto *part = this->find_related_part_state(entry_name);
               part != nullptr) {
        root = part->xml.document_element();
        relationships_document = &part->relationships;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return images;
    }

    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::string{entry_name} +
                           " does not contain a valid drawing image root",
                       std::string{entry_name});
        return images;
    }

    const auto relationships_root = relationships_document->child("Relationships");
    std::vector<drawing_image_reference_state> references;
    if (entry_name == document_xml_entry) {
        collect_body_drawing_image_references(root, relationships_root, references);
    } else {
        collect_drawing_image_references(root, relationships_root, references);
    }

    images.reserve(references.size());
    for (const auto &reference : references) {
        auto content_type = std::string{};
        for (auto part = this->image_parts.rbegin(); part != this->image_parts.rend(); ++part) {
            if (part->entry_name == reference.entry_name) {
                content_type = part->content_type;
                break;
            }
        }

        if (content_type.empty()) {
            content_type =
                image_content_type_for_extension(image_extension_from_entry_name(
                    reference.entry_name));
        }

        images.push_back({reference.index,
                          reference.placement,
                          reference.relationship_id,
                          reference.entry_name,
                          reference.display_name,
                          std::move(content_type),
                          reference.width_px,
                          reference.height_px,
                          reference.floating_options,
                          reference.body_block_index,
                          reference.paragraph_index});
    }

    this->last_error_info.clear();
    return images;
}

std::vector<drawing_image_info> Document::drawing_images() const {
    return this->drawing_images_in_part(document_xml_entry);
}

std::vector<inline_image_info> Document::inline_images_in_part(
    std::string_view entry_name) const {
    const auto drawing_images = this->drawing_images_in_part(entry_name);
    if (this->last_error_info) {
        return {};
    }

    std::vector<inline_image_info> images;
    images.reserve(drawing_images.size());
    for (const auto &drawing_image : drawing_images) {
        if (drawing_image.placement != drawing_image_placement::inline_object) {
            continue;
        }

        images.push_back({images.size(),
                          drawing_image.relationship_id,
                          drawing_image.entry_name,
                          drawing_image.display_name,
                          drawing_image.content_type,
                          drawing_image.width_px,
                          drawing_image.height_px,
                          drawing_image.body_block_index,
                          drawing_image.paragraph_index});
    }

    this->last_error_info.clear();
    return images;
}

std::vector<inline_image_info> Document::inline_images() const {
    return this->inline_images_in_part(document_xml_entry);
}

bool Document::extract_drawing_image_from_part(
    std::string_view entry_name, std::size_t image_index,
    const std::filesystem::path &output_path) const {
    if (output_path.empty()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "output path must not be empty when extracting a drawing image");
        return false;
    }

    const auto images = this->drawing_images_in_part(entry_name);
    if (this->last_error_info) {
        return false;
    }

    if (image_index >= images.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::result_out_of_range),
                       "drawing image index is out of range",
                       std::to_string(image_index));
        return false;
    }

    std::string image_data;
    for (auto part = this->image_parts.rbegin(); part != this->image_parts.rend(); ++part) {
        if (part->entry_name == images[image_index].entry_name) {
            image_data = part->data;
            break;
        }
    }

    if (image_data.empty()) {
        if (!this->has_source_archive || this->document_path.empty()) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::no_such_file_or_directory),
                           "the requested drawing image does not have readable archive data",
                           images[image_index].entry_name);
            return false;
        }

        int zip_error = 0;
        zip_t *archive = zip_openwitherror(this->document_path.string().c_str(),
                                           ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                           &zip_error);
        if (archive == nullptr) {
            set_last_error(this->last_error_info, document_errc::source_archive_open_failed,
                           "failed to reopen source archive '" +
                               this->document_path.string() + "' for drawing image extraction",
                           this->document_path.string());
            return false;
        }

        const auto read_status =
            read_zip_entry_binary(archive, images[image_index].entry_name, image_data);
        zip_close(archive);

        if (read_status == zip_entry_binary_read_status::missing) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::no_such_file_or_directory),
                           "the requested drawing image entry is missing from the source archive",
                           images[image_index].entry_name);
            return false;
        }
        if (read_status == zip_entry_binary_read_status::read_failed) {
            set_last_error(this->last_error_info, document_errc::source_entry_open_failed,
                           "failed to read the requested drawing image entry from the source archive",
                           images[image_index].entry_name);
            return false;
        }
    }

    if (const auto parent_path = output_path.parent_path(); !parent_path.empty()) {
        std::error_code filesystem_error;
        std::filesystem::create_directories(parent_path, filesystem_error);
        if (filesystem_error) {
            set_last_error(this->last_error_info, filesystem_error,
                           "failed to create output directory '" +
                               parent_path.string() +
                               "' for drawing image extraction",
                           output_path.string());
            return false;
        }
    }

    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to open output file for drawing image extraction",
                       output_path.string());
        return false;
    }

    stream.write(image_data.data(), static_cast<std::streamsize>(image_data.size()));
    if (!stream.good()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed while writing extracted drawing image data",
                       output_path.string());
        return false;
    }

    this->last_error_info.clear();
    return true;
}

bool Document::extract_drawing_image(
    std::size_t image_index, const std::filesystem::path &output_path) const {
    return this->extract_drawing_image_from_part(document_xml_entry, image_index,
                                                 output_path);
}

bool Document::extract_inline_image_from_part(
    std::string_view entry_name, std::size_t image_index,
    const std::filesystem::path &output_path) const {
    const auto images = this->drawing_images_in_part(entry_name);
    if (this->last_error_info) {
        return false;
    }

    const auto drawing_index = drawing_index_for_inline_image(images, image_index);
    if (!drawing_index.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::result_out_of_range),
                       "inline image index is out of range",
                       std::to_string(image_index));
        return false;
    }

    return this->extract_drawing_image_from_part(entry_name, *drawing_index, output_path);
}

bool Document::extract_inline_image(std::size_t image_index,
                                    const std::filesystem::path &output_path) const {
    return this->extract_inline_image_from_part(document_xml_entry, image_index,
                                                output_path);
}

bool Document::replace_drawing_image_in_part(std::string_view entry_name,
                                             std::size_t image_index,
                                             const std::filesystem::path &image_path) {
    const auto images = this->drawing_images_in_part(entry_name);
    if (this->last_error_info) {
        return false;
    }

    if (image_index >= images.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::result_out_of_range),
                       "drawing image index is out of range",
                       std::to_string(image_index));
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

    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    pugi::xml_document *relationships_document = nullptr;
    std::string_view relationships_entry_name;
    bool *relationships_dirty = nullptr;
    if (entry_name == document_xml_entry) {
        relationships_document = &this->document_relationships;
        relationships_entry_name = document_relationships_xml_entry;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        relationships_document = &part->relationships;
        relationships_entry_name = part->relationships_entry_name;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return false;
    }

    auto relationships = relationships_document->child("Relationships");
    if (relationships == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       std::string{relationships_entry_name} +
                           " does not contain a Relationships root",
                       std::string{relationships_entry_name});
        return false;
    }

    auto relationship_node =
        find_image_relationship_node(relationships, images[image_index].relationship_id);

    if (relationship_node == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "the requested drawing image relationship could not be resolved",
                       images[image_index].relationship_id);
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

    std::unordered_set<std::string> used_part_entries;
    collect_relationship_image_targets(this->document_relationships, used_part_entries);
    for (const auto &part : this->header_parts) {
        collect_relationship_image_targets(part->relationships, used_part_entries);
    }
    for (const auto &part : this->footer_parts) {
        collect_relationship_image_targets(part->relationships, used_part_entries);
    }
    for (const auto &part : this->image_parts) {
        used_part_entries.insert(part.entry_name);
    }

    std::string replacement_entry_name;
    for (std::size_t next_index = 1U; replacement_entry_name.empty(); ++next_index) {
        auto candidate = std::string{"word/media/image"} +
                         std::to_string(next_index) + "." + image_info.extension;
        if (!used_part_entries.contains(candidate)) {
            replacement_entry_name = std::move(candidate);
        }
    }

    auto default_node = pugi::xml_node{};
    for (auto existing_default = types.child("Default");
         existing_default != pugi::xml_node{};
         existing_default = existing_default.next_sibling("Default")) {
        if (std::string_view{existing_default.attribute("Extension").value()} ==
            image_info.extension) {
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
    ensure_attribute_value(default_node, "Extension", image_info.extension);
    ensure_attribute_value(default_node, "ContentType", image_info.content_type);

    const auto previous_entry_name = images[image_index].entry_name;
    ensure_attribute_value(relationship_node, "Target",
                           make_part_relationship_target(entry_name,
                                                         replacement_entry_name));

    this->image_parts.push_back({std::string{entry_name},
                                 std::move(replacement_entry_name),
                                 std::move(image_info.content_type),
                                 std::move(image_info.data)});
    this->removed_archive_entries.erase(this->image_parts.back().entry_name);
    *relationships_dirty = true;
    this->content_types_dirty = true;

    const auto has_image_relationship_reference =
        [&](std::string_view target_entry_name) {
            if (relationships_reference_image_entry(this->document_relationships,
                                                    target_entry_name)) {
                return true;
            }
            for (const auto &part : this->header_parts) {
                if (relationships_reference_image_entry(part->relationships,
                                                        target_entry_name)) {
                    return true;
                }
            }
            for (const auto &part : this->footer_parts) {
                if (relationships_reference_image_entry(part->relationships,
                                                        target_entry_name)) {
                    return true;
                }
            }

            return false;
        };

    if (!has_image_relationship_reference(previous_entry_name)) {
        this->image_parts.erase(
            std::remove_if(this->image_parts.begin(), this->image_parts.end(),
                           [&](const image_part_state &part) {
                               return part.entry_name == previous_entry_name;
                           }),
            this->image_parts.end());
        if (this->has_source_archive) {
            this->removed_archive_entries.insert(previous_entry_name);
        }
    } else {
        this->removed_archive_entries.erase(previous_entry_name);
    }

    this->last_error_info.clear();
    return true;
}

bool Document::remove_drawing_image_in_part(std::string_view entry_name,
                                            std::size_t image_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing drawing images");
        return false;
    }

    pugi::xml_node root;
    pugi::xml_document *relationships_document = nullptr;
    bool *relationships_dirty = nullptr;
    if (entry_name == document_xml_entry) {
        root = this->document.child("w:document").child("w:body");
        relationships_document = &this->document_relationships;
        relationships_dirty = &this->document_relationships_dirty;
    } else if (auto *part = this->find_related_part_state(entry_name); part != nullptr) {
        root = part->xml.document_element();
        relationships_document = &part->relationships;
        relationships_dirty = &part->relationships_dirty;
    } else {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       std::string{entry_name});
        return false;
    }

    if (root == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::string{entry_name} +
                           " does not contain a valid drawing image root",
                       std::string{entry_name});
        return false;
    }

    auto relationships_root =
        relationships_document != nullptr ? relationships_document->child("Relationships")
                                          : pugi::xml_node{};
    std::vector<drawing_image_reference_state> references;
    collect_drawing_image_references(root, relationships_root, references);
    if (image_index >= references.size()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::result_out_of_range),
                       "drawing image index is out of range",
                       std::to_string(image_index));
        return false;
    }

    const auto &reference = references[image_index];
    if (reference.drawing_container == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "the requested drawing image could not be resolved to an XML node",
                       std::to_string(image_index));
        return false;
    }

    const auto has_shared_relationship =
        std::any_of(references.begin(), references.end(),
                    [&](const drawing_image_reference_state &candidate) {
                        return candidate.index != reference.index &&
                               candidate.relationship_id == reference.relationship_id;
                    });

    auto drawing_parent = reference.drawing_container.parent();
    if (drawing_parent == pugi::xml_node{} ||
        !drawing_parent.remove_child(reference.drawing_container)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::io_error),
                       "failed to remove the requested drawing image from its parent node",
                       std::string{entry_name});
        return false;
    }

    if (!has_shared_relationship) {
        const auto relationship_node =
            find_image_relationship_node(relationships_root, reference.relationship_id);
        if (relationship_node != pugi::xml_node{}) {
            relationships_root.remove_child(relationship_node);
            if (relationships_dirty != nullptr) {
                *relationships_dirty = true;
            }
        }
    }

    const auto has_image_relationship_reference =
        [&](std::string_view target_entry_name) {
            if (relationships_reference_image_entry(this->document_relationships,
                                                    target_entry_name)) {
                return true;
            }
            for (const auto &part : this->header_parts) {
                if (relationships_reference_image_entry(part->relationships,
                                                        target_entry_name)) {
                    return true;
                }
            }
            for (const auto &part : this->footer_parts) {
                if (relationships_reference_image_entry(part->relationships,
                                                        target_entry_name)) {
                    return true;
                }
            }

            return false;
        };

    if (!has_image_relationship_reference(reference.entry_name)) {
        this->image_parts.erase(
            std::remove_if(this->image_parts.begin(), this->image_parts.end(),
                           [&](const image_part_state &part) {
                               return part.entry_name == reference.entry_name;
                           }),
            this->image_parts.end());
        if (this->has_source_archive) {
            this->removed_archive_entries.insert(reference.entry_name);
        }
    } else {
        this->removed_archive_entries.erase(reference.entry_name);
    }

    this->last_error_info.clear();
    return true;
}

bool Document::replace_drawing_image(std::size_t image_index,
                                     const std::filesystem::path &image_path) {
    return this->replace_drawing_image_in_part(document_xml_entry, image_index,
                                               image_path);
}

bool Document::remove_drawing_image(std::size_t image_index) {
    return this->remove_drawing_image_in_part(document_xml_entry, image_index);
}

bool Document::replace_inline_image_in_part(std::string_view entry_name,
                                            std::size_t image_index,
                                            const std::filesystem::path &image_path) {
    const auto images = this->drawing_images_in_part(entry_name);
    if (this->last_error_info) {
        return false;
    }

    const auto drawing_index = drawing_index_for_inline_image(images, image_index);
    if (!drawing_index.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::result_out_of_range),
                       "inline image index is out of range",
                       std::to_string(image_index));
        return false;
    }

    return this->replace_drawing_image_in_part(entry_name, *drawing_index, image_path);
}

bool Document::remove_inline_image_in_part(std::string_view entry_name,
                                           std::size_t image_index) {
    const auto images = this->drawing_images_in_part(entry_name);
    if (this->last_error_info) {
        return false;
    }

    const auto drawing_index = drawing_index_for_inline_image(images, image_index);
    if (!drawing_index.has_value()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::result_out_of_range),
                       "inline image index is out of range",
                       std::to_string(image_index));
        return false;
    }

    return this->remove_drawing_image_in_part(entry_name, *drawing_index);
}

bool Document::replace_inline_image(std::size_t image_index,
                                    const std::filesystem::path &image_path) {
    return this->replace_inline_image_in_part(document_xml_entry, image_index,
                                              image_path);
}

bool Document::remove_inline_image(std::size_t image_index) {
    return this->remove_inline_image_in_part(document_xml_entry, image_index);
}

#include "document_image_append_methods.inc"

} // namespace featherdoc
