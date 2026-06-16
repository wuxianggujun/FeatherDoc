#include "document_image_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>

#include <zip.h>

namespace featherdoc::detail {
namespace {
constexpr auto empty_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)"};

auto initialize_xml_document(pugi::xml_document &xml_document,
                             std::string_view xml_text) -> bool {
    xml_document.reset();
    return static_cast<bool>(
        xml_document.load_buffer(xml_text.data(), xml_text.size()));
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

void collect_drawing_image_references(pugi::xml_node node,
                                      pugi::xml_node relationships_root,
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

} // namespace

auto initialize_empty_relationships_document(pugi::xml_document &xml_document) -> bool {
    return initialize_xml_document(xml_document, empty_relationships_xml);
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    std::error_code code, std::string detail,
                    std::string entry_name,
                    std::optional<std::ptrdiff_t> xml_offset)
    -> std::error_code {
    error_info.code = code;
    error_info.detail = std::move(detail);
    error_info.entry_name = std::move(entry_name);
    error_info.xml_offset = xml_offset;
    return code;
}

auto set_last_error(featherdoc::document_error_info &error_info,
                    featherdoc::document_errc code, std::string detail,
                    std::string entry_name,
                    std::optional<std::ptrdiff_t> xml_offset)
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

void ensure_attribute_value(pugi::xml_node node, const char *name,
                            std::string_view value) {
    if (node == pugi::xml_node{}) {
        return;
    }

    auto attribute = node.attribute(name);
    if (attribute == pugi::xml_attribute{}) {
        attribute = node.append_attribute(name);
    }
    attribute.set_value(std::string{value}.c_str());
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

auto read_zip_entry_binary(zip_t *archive, std::string_view entry_name,
                           std::string &content) -> zip_entry_binary_read_status {
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

void collect_drawing_image_references(
    pugi::xml_node node, pugi::xml_node relationships_root,
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
    const std::vector<featherdoc::drawing_image_info> &images,
    std::size_t inline_index) -> std::optional<std::size_t> {
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

void collect_relationship_image_targets(
    const pugi::xml_document &source_relationships,
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

auto relationships_reference_image_entry(
    const pugi::xml_document &source_relationships,
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

} // namespace featherdoc::detail
