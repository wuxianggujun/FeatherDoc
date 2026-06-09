#include "featherdoc.hpp"
#include "document_image_helpers.hpp"
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

namespace featherdoc {

using detail::append_crop_node;
using detail::append_wrap_mode_node;
using detail::collect_body_drawing_image_references;
using detail::collect_drawing_image_references;
using detail::collect_max_drawing_object_id;
using detail::collect_relationship_image_targets;
using detail::content_types_xml_entry;
using detail::document_relationships_xml_entry;
using detail::document_xml_entry;
using detail::drawing_image_reference_state;
using detail::drawing_index_for_inline_image;
using detail::drawingml_main_namespace_uri;
using detail::drawingml_picture_namespace_uri;
using detail::drawingml_picture_uri;
using detail::ensure_attribute_value;
using detail::find_image_relationship_node;
using detail::image_content_type_for_extension;
using detail::image_extension_from_entry_name;
using detail::image_relationship_type;
using detail::initialize_empty_relationships_document;
using detail::is_valid_floating_crop;
using detail::make_part_relationship_target;
using detail::normalize_word_part_entry;
using detail::office_document_relationships_namespace_uri;
using detail::read_zip_entry_binary;
using detail::relationships_reference_image_entry;
using detail::set_last_error;
using detail::signed_pixels_to_emu;
using detail::to_xml_reference;
using detail::wordprocessing_drawing_namespace_uri;
using detail::zip_entry_binary_read_status;

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
