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
    pugi::xml_node drawing_container;
    pugi::xml_node drawing_object;
};

void collect_drawing_image_reference(
    pugi::xml_node drawing_container, pugi::xml_node drawing_node,
    featherdoc::drawing_image_placement placement, pugi::xml_node relationships_root,
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
         height_emu.has_value() ? emu_to_pixels(*height_emu) : 0U, drawing_container,
         drawing_node});
}

void collect_drawing_image_references(pugi::xml_node node, pugi::xml_node relationships_root,
                                      std::vector<drawing_image_reference_state> &references) {
    for (auto child = node.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} == "w:drawing") {
            collect_drawing_image_reference(
                child, child.child("wp:inline"),
                featherdoc::drawing_image_placement::inline_object, relationships_root,
                references);
            collect_drawing_image_reference(
                child, child.child("wp:anchor"),
                featherdoc::drawing_image_placement::anchored_object, relationships_root,
                references);
        }

        collect_drawing_image_references(child, relationships_root, references);
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

    std::vector<drawing_image_reference_state> references;
    collect_drawing_image_references(root, relationships_document->child("Relationships"),
                                     references);

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
                          reference.height_px});
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
                          drawing_image.height_px});
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

bool TemplatePart::append_image(const std::filesystem::path &image_path) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(*this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    if (this->entry_name_storage == document_xml_entry) {
        return this->owner->append_inline_image_part(std::move(image_info.data),
                                                     std::move(image_info.extension),
                                                     std::move(image_info.content_type),
                                                     image_path.filename().string(),
                                                     image_info.width_px,
                                                     image_info.height_px);
    }

    auto *part = this->owner->find_related_part_state(this->entry_name_storage);
    if (part == nullptr) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       this->entry_name_storage);
        return false;
    }

    return this->owner->append_inline_image_part(
        part->xml, this->entry_name_storage, part->relationships,
        part->relationships_entry_name, part->has_relationships_part,
        part->relationships_dirty, part->xml.document_element(), {},
        std::move(image_info.data), std::move(image_info.extension),
        std::move(image_info.content_type), image_path.filename().string(),
        image_info.width_px, image_info.height_px);
}

bool TemplatePart::append_image(const std::filesystem::path &image_path,
                                std::uint32_t width_px,
                                std::uint32_t height_px) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    if (width_px == 0U || height_px == 0U) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(*this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    if (this->entry_name_storage == document_xml_entry) {
        return this->owner->append_inline_image_part(std::move(image_info.data),
                                                     std::move(image_info.extension),
                                                     std::move(image_info.content_type),
                                                     image_path.filename().string(),
                                                     width_px, height_px);
    }

    auto *part = this->owner->find_related_part_state(this->entry_name_storage);
    if (part == nullptr) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       this->entry_name_storage);
        return false;
    }

    return this->owner->append_inline_image_part(
        part->xml, this->entry_name_storage, part->relationships,
        part->relationships_entry_name, part->has_relationships_part,
        part->relationships_dirty, part->xml.document_element(), {},
        std::move(image_info.data), std::move(image_info.extension),
        std::move(image_info.content_type), image_path.filename().string(), width_px,
        height_px);
}

bool TemplatePart::append_floating_image(
    const std::filesystem::path &image_path,
    featherdoc::floating_image_options options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(*this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    if (this->entry_name_storage == document_xml_entry) {
        return this->owner->append_floating_image_part(
            std::move(image_info.data), std::move(image_info.extension),
            std::move(image_info.content_type), image_path.filename().string(),
            image_info.width_px, image_info.height_px, std::move(options));
    }

    auto *part = this->owner->find_related_part_state(this->entry_name_storage);
    if (part == nullptr) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       this->entry_name_storage);
        return false;
    }

    return this->owner->append_floating_image_part(
        part->xml, this->entry_name_storage, part->relationships,
        part->relationships_entry_name, part->has_relationships_part,
        part->relationships_dirty, part->xml.document_element(), {},
        std::move(image_info.data), std::move(image_info.extension),
        std::move(image_info.content_type), image_path.filename().string(),
        image_info.width_px, image_info.height_px, std::move(options));
}

bool TemplatePart::append_floating_image(
    const std::filesystem::path &image_path, std::uint32_t width_px,
    std::uint32_t height_px, featherdoc::floating_image_options options) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    if (width_px == 0U || height_px == 0U) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "image width and height must both be greater than zero",
                       image_path.string());
        return false;
    }

    featherdoc::detail::image_file_info image_info;
    auto error_code = featherdoc::document_errc::success;
    std::string detail;
    if (!featherdoc::detail::load_image_file(image_path, image_info, error_code, detail)) {
        set_last_error(*this->last_error_info, error_code, std::move(detail),
                       image_path.string());
        return false;
    }

    if (this->entry_name_storage == document_xml_entry) {
        return this->owner->append_floating_image_part(
            std::move(image_info.data), std::move(image_info.extension),
            std::move(image_info.content_type), image_path.filename().string(), width_px,
            height_px, std::move(options));
    }

    auto *part = this->owner->find_related_part_state(this->entry_name_storage);
    if (part == nullptr) {
        set_last_error(*this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "template part is not attached to this document",
                       this->entry_name_storage);
        return false;
    }

    return this->owner->append_floating_image_part(
        part->xml, this->entry_name_storage, part->relationships,
        part->relationships_entry_name, part->has_relationships_part,
        part->relationships_dirty, part->xml.document_element(), {},
        std::move(image_info.data), std::move(image_info.extension),
        std::move(image_info.content_type), image_path.filename().string(), width_px,
        height_px, std::move(options));
}

std::vector<drawing_image_info> TemplatePart::drawing_images() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return {};
    }

    return this->owner->drawing_images_in_part(this->entry_name_storage);
}

bool TemplatePart::extract_drawing_image(
    std::size_t image_index, const std::filesystem::path &output_path) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->extract_drawing_image_from_part(this->entry_name_storage,
                                                        image_index, output_path);
}

bool TemplatePart::remove_drawing_image(std::size_t image_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->remove_drawing_image_in_part(this->entry_name_storage,
                                                     image_index);
}

bool TemplatePart::replace_drawing_image(std::size_t image_index,
                                         const std::filesystem::path &image_path) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->replace_drawing_image_in_part(this->entry_name_storage,
                                                      image_index, image_path);
}

std::vector<inline_image_info> TemplatePart::inline_images() const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return {};
    }

    return this->owner->inline_images_in_part(this->entry_name_storage);
}

bool TemplatePart::extract_inline_image(
    std::size_t image_index, const std::filesystem::path &output_path) const {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->extract_inline_image_from_part(this->entry_name_storage,
                                                       image_index, output_path);
}

bool TemplatePart::remove_inline_image(std::size_t image_index) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->remove_inline_image_in_part(this->entry_name_storage,
                                                    image_index);
}

bool TemplatePart::replace_inline_image(std::size_t image_index,
                                        const std::filesystem::path &image_path) {
    if (this->xml_document == nullptr || this->last_error_info == nullptr ||
        this->owner == nullptr) {
        if (this->last_error_info != nullptr) {
            set_last_error(*this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "template part is not available", this->entry_name_storage);
        }
        return false;
    }

    return this->owner->replace_inline_image_in_part(this->entry_name_storage,
                                                     image_index, image_path);
}

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

bool Document::append_drawing_image_part(
    pugi::xml_document &xml_document, std::string_view xml_entry_name,
    pugi::xml_document &relationships_document, std::string_view relationships_entry_name,
    bool &has_relationships_part, bool &relationships_dirty, pugi::xml_node parent,
    pugi::xml_node insert_before, std::string image_data, std::string extension,
    std::string content_type, std::string display_name, std::uint32_t width_px,
    std::uint32_t height_px,
    std::optional<featherdoc::floating_image_options> floating_options) {
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

    if (floating_options.has_value() && floating_options->crop.has_value() &&
        !is_valid_floating_crop(*floating_options->crop)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "floating image crop must stay within 0..1000 per-mille "
                       "per edge and keep a visible image area");
        return false;
    }

    if (const auto error = this->ensure_content_types_loaded()) {
        return false;
    }

    auto part_root = xml_document.document_element();
    if (part_root == pugi::xml_node{} || parent == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::string{xml_entry_name} +
                           " does not contain a valid image insertion parent",
                       std::string{xml_entry_name});
        return false;
    }

    if (relationships_document.child("Relationships") == pugi::xml_node{} &&
        !initialize_empty_relationships_document(relationships_document)) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       "failed to initialize default relationships XML",
                       std::string{relationships_entry_name});
        return false;
    }

    auto relationships = relationships_document.child("Relationships");
    if (relationships == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::relationships_xml_parse_failed,
                       std::string{relationships_entry_name} +
                           " does not contain a Relationships root",
                       std::string{relationships_entry_name});
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

    auto relationships_namespace = part_root.attribute("xmlns:r");
    if (relationships_namespace == pugi::xml_attribute{}) {
        relationships_namespace = part_root.append_attribute("xmlns:r");
    }
    relationships_namespace.set_value(
        office_document_relationships_namespace_uri.data());

    std::unordered_set<std::string> used_relationship_ids;
    std::unordered_set<std::string> used_part_entries;
    const auto collect_relationship_image_targets =
        [&](const pugi::xml_document &source_relationships) {
            auto source_root = source_relationships.child("Relationships");
            for (auto relationship = source_root.child("Relationship");
                 relationship != pugi::xml_node{};
                 relationship = relationship.next_sibling("Relationship")) {
                if (std::string_view{relationship.attribute("Type").value()} !=
                    image_relationship_type) {
                    continue;
                }

                const auto target =
                    std::string_view{relationship.attribute("Target").value()};
                if (!target.empty()) {
                    used_part_entries.insert(normalize_word_part_entry(target));
                }
            }
        };

    for (auto relationship = relationships.child("Relationship");
         relationship != pugi::xml_node{};
         relationship = relationship.next_sibling("Relationship")) {
        const auto id = relationship.attribute("Id").value();
        if (*id != '\0') {
            used_relationship_ids.emplace(id);
        }
    }

    collect_relationship_image_targets(this->document_relationships);
    for (const auto &part : this->header_parts) {
        collect_relationship_image_targets(part->relationships);
    }
    for (const auto &part : this->footer_parts) {
        collect_relationship_image_targets(part->relationships);
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
                       std::string{relationships_entry_name});
        return false;
    }
    ensure_attribute_value(relationship, "Id", relationship_id);
    ensure_attribute_value(relationship, "Type", image_relationship_type);
    ensure_attribute_value(relationship, "Target",
                           make_part_relationship_target(xml_entry_name, entry_name));

    const auto drawing_id = this->next_drawing_object_id();
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
                       std::string{xml_entry_name});
        return false;
    }

    auto run = image_paragraph.append_child("w:r");
    auto drawing = run.append_child("w:drawing");
    const auto drawing_node_name =
        floating_options.has_value() ? "wp:anchor" : "wp:inline";
    auto drawing_container = drawing.append_child(drawing_node_name);
    if (run == pugi::xml_node{} || drawing == pugi::xml_node{} ||
        drawing_container == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to build drawing image XML",
                       std::string{xml_entry_name});
        return false;
    }

    ensure_attribute_value(drawing, "xmlns:r", office_document_relationships_namespace_uri);
    ensure_attribute_value(drawing, "xmlns:wp", wordprocessing_drawing_namespace_uri);
    ensure_attribute_value(drawing, "xmlns:a", drawingml_main_namespace_uri);
    ensure_attribute_value(drawing, "xmlns:pic", drawingml_picture_namespace_uri);
    std::string wrap_distance_top_text{"0"};
    std::string wrap_distance_bottom_text{"0"};
    std::string wrap_distance_left_text{"0"};
    std::string wrap_distance_right_text{"0"};

    if (floating_options.has_value()) {
        wrap_distance_top_text = std::to_string(
            featherdoc::detail::pixels_to_emu(
                floating_options->wrap_distance_top_px));
        wrap_distance_bottom_text = std::to_string(
            featherdoc::detail::pixels_to_emu(
                floating_options->wrap_distance_bottom_px));
        wrap_distance_left_text = std::to_string(
            featherdoc::detail::pixels_to_emu(
                floating_options->wrap_distance_left_px));
        wrap_distance_right_text = std::to_string(
            featherdoc::detail::pixels_to_emu(
                floating_options->wrap_distance_right_px));
    }
    ensure_attribute_value(drawing_container, "distT", wrap_distance_top_text);
    ensure_attribute_value(drawing_container, "distB", wrap_distance_bottom_text);
    ensure_attribute_value(drawing_container, "distL", wrap_distance_left_text);
    ensure_attribute_value(drawing_container, "distR", wrap_distance_right_text);

    if (floating_options.has_value()) {
        ensure_attribute_value(drawing_container, "simplePos", "0");
        ensure_attribute_value(drawing_container, "relativeHeight", "0");
        ensure_attribute_value(drawing_container, "behindDoc",
                               floating_options->behind_text ? "1" : "0");
        ensure_attribute_value(drawing_container, "locked", "0");
        ensure_attribute_value(drawing_container, "layoutInCell", "1");
        ensure_attribute_value(drawing_container, "allowOverlap",
                               floating_options->allow_overlap ? "1" : "0");

        auto simple_position = drawing_container.append_child("wp:simplePos");
        ensure_attribute_value(simple_position, "x", "0");
        ensure_attribute_value(simple_position, "y", "0");

        auto position_horizontal = drawing_container.append_child("wp:positionH");
        ensure_attribute_value(position_horizontal, "relativeFrom",
                               to_xml_reference(
                                   floating_options->horizontal_reference));
        auto horizontal_offset = position_horizontal.append_child("wp:posOffset");
        const auto horizontal_offset_emu =
            std::to_string(signed_pixels_to_emu(
                floating_options->horizontal_offset_px));
        horizontal_offset.text().set(horizontal_offset_emu.c_str());

        auto position_vertical = drawing_container.append_child("wp:positionV");
        ensure_attribute_value(position_vertical, "relativeFrom",
                               to_xml_reference(
                                   floating_options->vertical_reference));
        auto vertical_offset = position_vertical.append_child("wp:posOffset");
        const auto vertical_offset_emu =
            std::to_string(signed_pixels_to_emu(
                floating_options->vertical_offset_px));
        vertical_offset.text().set(vertical_offset_emu.c_str());
    }

    auto extent = drawing_container.append_child("wp:extent");
    ensure_attribute_value(extent, "cx", width_emu_text);
    ensure_attribute_value(extent, "cy", height_emu_text);

    auto effect_extent = drawing_container.append_child("wp:effectExtent");
    ensure_attribute_value(effect_extent, "l", "0");
    ensure_attribute_value(effect_extent, "t", "0");
    ensure_attribute_value(effect_extent, "r", "0");
    ensure_attribute_value(effect_extent, "b", "0");

    if (floating_options.has_value()) {
        append_wrap_mode_node(drawing_container, floating_options->wrap_mode);
    }

    auto doc_properties = drawing_container.append_child("wp:docPr");
    ensure_attribute_value(doc_properties, "id", drawing_id_text);
    ensure_attribute_value(doc_properties, "name", display_name);

    auto graphic_frame_properties = drawing_container.append_child("wp:cNvGraphicFramePr");
    auto frame_locks = graphic_frame_properties.append_child("a:graphicFrameLocks");
    ensure_attribute_value(frame_locks, "noChangeAspect", "1");

    auto graphic = drawing_container.append_child("a:graphic");
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
    if (floating_options.has_value() && floating_options->crop.has_value()) {
        append_crop_node(blip_fill, *floating_options->crop);
    }
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

    this->image_parts.push_back({std::string{xml_entry_name}, std::move(entry_name),
                                 std::move(content_type), std::move(image_data)});
    has_relationships_part = true;
    relationships_dirty = true;
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

} // namespace featherdoc
