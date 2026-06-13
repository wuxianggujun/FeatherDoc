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

    auto run = pugi::xml_node{};
    if (std::string_view{parent.name()} == "w:r" &&
        insert_before == pugi::xml_node{}) {
        run = parent;
    } else {
        auto image_paragraph = featherdoc::detail::insert_paragraph_node(parent, insert_before);
        if (image_paragraph == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to append an image paragraph",
                           std::string{xml_entry_name});
            return false;
        }

        run = image_paragraph.append_child("w:r");
    }

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
        ensure_attribute_value(drawing_container, "relativeHeight",
                               std::to_string(floating_options->z_order));
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

