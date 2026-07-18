#include "document_image_helpers.hpp"
#include "document_archive_limit_helpers.hpp"
#include "numeric_helpers.hpp"
#include "package_path_helpers.hpp"
#include "xml_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include <zip.h>

namespace featherdoc::detail {
namespace {
auto parse_u32_attribute_value(const char *text)
    -> std::optional<std::uint32_t> {
    return featherdoc::detail::parse_integer_strict<std::uint32_t>(text);
}

auto parse_u64_attribute_value(const char *text)
    -> std::optional<std::uint64_t> {
    return featherdoc::detail::parse_integer_strict<std::uint64_t>(text);
}

auto parse_i64_attribute_value(const char *text)
    -> std::optional<std::int64_t> {
    return featherdoc::detail::parse_integer_strict<std::int64_t>(text);
}

auto to_lower_ascii(std::string text) -> std::string {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    return text;
}

auto rounded_emu_to_pixels(std::uint64_t emu) -> std::uint64_t {
    constexpr std::uint64_t emu_per_pixel = 9525U;
    const auto whole_pixels = emu / emu_per_pixel;
    const auto remaining_emu = emu % emu_per_pixel;
    return whole_pixels + (remaining_emu > emu_per_pixel / 2U ? 1U : 0U);
}

auto emu_to_pixels(std::uint64_t emu) -> std::uint32_t {
    const auto rounded_pixels = rounded_emu_to_pixels(emu);
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(
        rounded_pixels, std::numeric_limits<std::uint32_t>::max()));
}

auto signed_emu_to_pixels(std::int64_t emu) -> std::int32_t {
    const auto magnitude = emu < 0 ? static_cast<std::uint64_t>(-(emu + 1)) + 1U
                                   : static_cast<std::uint64_t>(emu);
    const auto rounded_pixels = rounded_emu_to_pixels(magnitude);
    if (emu < 0) {
        const auto minimum_magnitude =
            static_cast<std::uint64_t>(
                std::numeric_limits<std::int32_t>::max()) +
            1U;
        if (rounded_pixels >= minimum_magnitude) {
            return std::numeric_limits<std::int32_t>::min();
        }
        return -static_cast<std::int32_t>(rounded_pixels);
    }
    return static_cast<std::int32_t>(std::min<std::uint64_t>(
        rounded_pixels,
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())));
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

    const auto rounded_per_mille =
        *parsed / 100U + (*parsed % 100U >= 50U ? 1U : 0U);
    return std::min<std::uint32_t>(rounded_per_mille, 1000U);
}

auto parse_floating_crop(pugi::xml_node drawing_node)
    -> std::optional<featherdoc::floating_image_crop> {
    const auto src_rect = drawing_node.child("a:graphic")
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
    if (const auto relative_height = parse_u32_attribute_value(
            drawing_node.attribute("relativeHeight").value())) {
        options.z_order = *relative_height;
    }
    options.wrap_mode = parse_wrap_mode(drawing_node);

    if (const auto distance = parse_u64_attribute_value(
            drawing_node.attribute("distL").value())) {
        options.wrap_distance_left_px = emu_to_pixels(*distance);
    }
    if (const auto distance = parse_u64_attribute_value(
            drawing_node.attribute("distR").value())) {
        options.wrap_distance_right_px = emu_to_pixels(*distance);
    }
    if (const auto distance = parse_u64_attribute_value(
            drawing_node.attribute("distT").value())) {
        options.wrap_distance_top_px = emu_to_pixels(*distance);
    }
    if (const auto distance = parse_u64_attribute_value(
            drawing_node.attribute("distB").value())) {
        options.wrap_distance_bottom_px = emu_to_pixels(*distance);
    }

    const auto position_horizontal = drawing_node.child("wp:positionH");
    parse_horizontal_reference(
        position_horizontal.attribute("relativeFrom").value(),
        options.horizontal_reference);
    if (const auto horizontal_offset = parse_i64_attribute_value(
            position_horizontal.child("wp:posOffset").child_value())) {
        options.horizontal_offset_px = signed_emu_to_pixels(*horizontal_offset);
    }

    const auto position_vertical = drawing_node.child("wp:positionV");
    parse_vertical_reference(
        position_vertical.attribute("relativeFrom").value(),
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
    featherdoc::drawing_image_placement placement,
    pugi::xml_node relationships_root, std::string_view source_entry_name,
    std::optional<std::size_t> body_block_index,
    std::optional<std::size_t> paragraph_index,
    std::vector<drawing_image_reference_state> &references) {
    if (drawing_node == pugi::xml_node{} ||
        relationships_root == pugi::xml_node{}) {
        return;
    }

    const auto blip = drawing_node.child("a:graphic")
                          .child("a:graphicData")
                          .child("pic:pic")
                          .child("pic:blipFill")
                          .child("a:blip");
    const auto relationship_id =
        std::string_view{blip.attribute("r:embed").value()};
    if (relationship_id.empty()) {
        return;
    }

    auto relationship_node = pugi::xml_node{};
    for (auto relationship = first_package_relationship(relationships_root);
         relationship != pugi::xml_node{};
         relationship = next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Id").value()} !=
                relationship_id ||
            std::string_view{relationship.attribute("Type").value()} !=
                image_relationship_type ||
            std::string_view{relationship.attribute("TargetMode").value()} ==
                "External") {
            continue;
        }

        relationship_node = relationship;
        break;
    }

    const auto target_entry =
        relationship_node == pugi::xml_node{}
            ? std::string{}
            : resolve_package_relationship_target(
                  source_entry_name,
                  relationship_node.attribute("Target").value());
    if (target_entry.empty()) {
        return;
    }

    auto display_name =
        std::string{drawing_node.child("wp:docPr").attribute("name").value()};
    if (display_name.empty()) {
        display_name = drawing_node.child("a:graphic")
                           .child("a:graphicData")
                           .child("pic:pic")
                           .child("pic:nvPicPr")
                           .child("pic:cNvPr")
                           .attribute("name")
                           .value();
    }

    const auto width_emu = parse_u64_attribute_value(
        drawing_node.child("wp:extent").attribute("cx").value());
    const auto height_emu = parse_u64_attribute_value(
        drawing_node.child("wp:extent").attribute("cy").value());

    references.push_back(
        {references.size(), placement, std::string{relationship_id},
         target_entry, std::move(display_name),
         width_emu.has_value() ? emu_to_pixels(*width_emu) : 0U,
         height_emu.has_value() ? emu_to_pixels(*height_emu) : 0U,
         parse_floating_options(drawing_node), body_block_index,
         paragraph_index, drawing_container, drawing_node});
}

void collect_drawing_image_references(
    pugi::xml_node node, pugi::xml_node relationships_root,
    std::string_view source_entry_name,
    std::optional<std::size_t> body_block_index,
    std::optional<std::size_t> paragraph_index,
    std::vector<drawing_image_reference_state> &references) {
    for (auto current = node.first_child(); current != pugi::xml_node{};
         current = next_xml_node_preorder(node, current)) {
        if (std::string_view{current.name()} == "w:drawing") {
            collect_drawing_image_reference(
                current, current.child("wp:inline"),
                featherdoc::drawing_image_placement::inline_object,
                relationships_root, source_entry_name, body_block_index,
                paragraph_index, references);
            collect_drawing_image_reference(
                current, current.child("wp:anchor"),
                featherdoc::drawing_image_placement::anchored_object,
                relationships_root, source_entry_name, body_block_index,
                paragraph_index, references);
        }
    }
}

} // namespace

auto relationships_document_allows_mutation(
    const pugi::xml_document &relationships_document,
    bool has_relationships_part, std::string_view relationships_entry_name,
    featherdoc::document_error_info &last_error_info) -> bool {
    if (featherdoc::detail::package_relationships_document_allows_mutation(
            relationships_document, has_relationships_part)) {
        return true;
    }

    set_last_error(
        last_error_info, featherdoc::document_errc::invalid_package_structure,
        std::string{relationships_entry_name} +
            " does not contain a valid Relationships root in the package "
            "relationships namespace; relationship mutations are disabled",
        std::string{relationships_entry_name});
    return false;
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

auto make_part_relationship_target(std::string_view source_entry_name,
                                   std::string_view target_entry_name)
    -> std::string {
    return make_package_relationship_target(source_entry_name,
                                            target_entry_name);
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

auto image_content_type_for_extension(std::string_view extension)
    -> std::string {
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

auto image_extension_from_entry_name(std::string_view entry_name)
    -> std::string {
    auto extension = package_path_extension(entry_name);
    if (!extension.empty() && extension.front() == '.') {
        extension.erase(extension.begin());
    }
    return normalize_image_extension(extension);
}

auto normalize_image_extension(std::string_view extension) -> std::string {
    return to_lower_ascii(std::string{extension});
}

auto image_extensions_equivalent(std::string_view left, std::string_view right)
    -> bool {
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0U; index < left.size(); ++index) {
        const auto fold_ascii = [](unsigned char value) {
            return value >= 'A' && value <= 'Z'
                       ? static_cast<unsigned char>(value + ('a' - 'A'))
                       : value;
        };
        if (fold_ascii(static_cast<unsigned char>(left[index])) !=
            fold_ascii(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }
    return true;
}

auto read_zip_entry_binary(zip_t *archive, std::string_view entry_name,
                           std::string &content,
                           const archive_entry_catalog &catalog)
    -> zip_entry_binary_read_status {
    content.clear();

    if (open_archive_entry_by_package_name(archive, catalog, entry_name) != 0) {
        return zip_entry_binary_read_status::missing;
    }

    void *buffer = nullptr;
    size_t buffer_size = 0U;
    const auto read_result = zip_entry_read(archive, &buffer, &buffer_size);
    const auto close_result = zip_entry_close(archive);
    std::unique_ptr<void, decltype(&std::free)> buffer_guard{buffer, &std::free};
    if (read_result < 0 || close_result != 0 || buffer == nullptr) {
        return zip_entry_binary_read_status::read_failed;
    }

    const auto *buffer_begin = static_cast<const char *>(buffer);
    content.assign(buffer_begin, buffer_begin + buffer_size);
    return zip_entry_binary_read_status::ok;
}

void collect_drawing_image_references(
    pugi::xml_node node, pugi::xml_node relationships_root,
    std::string_view source_entry_name,
    std::vector<drawing_image_reference_state> &references) {
    collect_drawing_image_references(node, relationships_root,
                                     source_entry_name, std::nullopt,
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
            collect_drawing_image_references(
                child, relationships_root, document_xml_entry, body_block_index,
                paragraph_index, references);
            ++body_block_index;
            ++paragraph_index;
        } else if (child_name == "w:tbl") {
            collect_drawing_image_references(
                child, relationships_root, document_xml_entry, body_block_index,
                std::nullopt, references);
            ++body_block_index;
        } else if (child_name == "w:sdt") {
            collect_drawing_image_references(
                child, relationships_root, document_xml_entry, body_block_index,
                std::nullopt, references);
            ++body_block_index;
        }
    }
}

void collect_max_drawing_object_id(pugi::xml_node node, std::uint32_t &max_id) {
    for (auto current = node.first_child(); current != pugi::xml_node{};
         current = next_xml_node_preorder(node, current)) {
        const auto child_name = std::string_view{current.name()};
        if (child_name == "wp:docPr" || child_name == "pic:cNvPr") {
            if (const auto parsed_id = parse_u32_attribute_value(
                    current.attribute("id").value())) {
                max_id = std::max(max_id, *parsed_id);
            }
        }
    }
}

auto drawing_index_for_inline_image(
    const std::vector<featherdoc::drawing_image_info> &images,
    std::size_t inline_index) -> std::optional<std::size_t> {
    std::size_t current_inline_index = 0U;
    for (const auto &image : images) {
        if (image.placement !=
            featherdoc::drawing_image_placement::inline_object) {
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
                                  std::string_view relationship_id)
    -> pugi::xml_node {
    for (auto relationship = first_package_relationship(relationships_root);
         relationship != pugi::xml_node{};
         relationship = next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Id").value()} ==
                relationship_id &&
            std::string_view{relationship.attribute("Type").value()} ==
                image_relationship_type &&
            std::string_view{relationship.attribute("TargetMode").value()} !=
                "External") {
            return relationship;
        }
    }

    return {};
}

void collect_relationship_image_targets(
    const pugi::xml_document &source_relationships,
    std::string_view source_entry_name,
    std::set<std::string> &used_part_identities) {
    const auto source_root = package_relationships_root(source_relationships);
    for (auto relationship = first_package_relationship(source_root);
         relationship != pugi::xml_node{};
         relationship = next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} !=
                image_relationship_type ||
            std::string_view{relationship.attribute("TargetMode").value()} ==
                "External") {
            continue;
        }

        const auto resolved_target = resolve_package_relationship_target(
            source_entry_name, relationship.attribute("Target").value());
        if (const auto identity = package_part_name_identity(resolved_target)) {
            used_part_identities.insert(*identity);
        }
    }
}

auto relationships_reference_image_entry(
    const pugi::xml_document &source_relationships,
    std::string_view source_entry_name, std::string_view entry_name) -> bool {
    const auto requested_identity = package_part_name_identity(entry_name);
    if (!requested_identity.has_value()) {
        return false;
    }

    const auto source_root = package_relationships_root(source_relationships);
    for (auto relationship = first_package_relationship(source_root);
         relationship != pugi::xml_node{};
         relationship = next_package_relationship(relationship)) {
        if (std::string_view{relationship.attribute("Type").value()} !=
                image_relationship_type ||
            std::string_view{relationship.attribute("TargetMode").value()} ==
                "External") {
            continue;
        }

        const auto resolved_target = resolve_package_relationship_target(
            source_entry_name, relationship.attribute("Target").value());
        const auto resolved_identity =
            package_part_name_identity(resolved_target);
        if (resolved_identity.has_value() &&
            *resolved_identity == *requested_identity) {
            return true;
        }
    }

    return false;
}

void erase_equivalent_image_entries(std::unordered_set<std::string> &entries,
                                    std::string_view entry_name) {
    const auto identity = package_part_name_identity(entry_name);
    if (!identity.has_value()) {
        return;
    }

    std::erase_if(entries, [&](const std::string &candidate) {
        const auto candidate_identity = package_part_name_identity(candidate);
        return candidate_identity.has_value() &&
               *candidate_identity == *identity;
    });
}

auto signed_pixels_to_emu(std::int32_t pixels) -> std::int64_t {
    return static_cast<std::int64_t>(pixels) * 9525;
}

auto to_xml_reference(featherdoc::floating_image_horizontal_reference reference)
    -> std::string_view {
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

auto to_xml_reference(featherdoc::floating_image_vertical_reference reference)
    -> std::string_view {
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

void append_wrap_mode_node(pugi::xml_node drawing_container,
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

auto is_valid_floating_crop(const featherdoc::floating_image_crop &crop)
    -> bool {
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

void append_crop_node(pugi::xml_node blip_fill,
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
