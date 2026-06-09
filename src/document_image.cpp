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

#include "document_image_access_methods.inc"

#include "document_image_append_methods.inc"

} // namespace featherdoc
