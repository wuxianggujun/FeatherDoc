auto ensure_managed_abstract_numbering(pugi::xml_node numbering_root, featherdoc::list_kind kind)
    -> std::optional<std::uint32_t> {
    const auto managed_name = list_name_for(kind);

    auto abstract_num = find_abstract_numbering_by_name(numbering_root, managed_name);
    if (abstract_num == pugi::xml_node{}) {
        const auto abstract_num_id =
            max_numbering_id(numbering_root, "w:abstractNum", "w:abstractNumId") + 1U;
        abstract_num = numbering_root.append_child("w:abstractNum");
        if (abstract_num == pugi::xml_node{}) {
            return std::nullopt;
        }

        ensure_attribute_value(abstract_num, "w:abstractNumId",
                               std::to_string(abstract_num_id));

        auto multi_level_type = abstract_num.append_child("w:multiLevelType");
        auto name_node = abstract_num.append_child("w:name");
        if (multi_level_type == pugi::xml_node{} || name_node == pugi::xml_node{}) {
            return std::nullopt;
        }

        ensure_attribute_value(multi_level_type, "w:val", "multilevel");
        ensure_attribute_value(name_node, "w:val", managed_name);

        for (std::uint32_t level = 0U; level <= max_list_level; ++level) {
            if (!append_level_definition(abstract_num, kind, level)) {
                return std::nullopt;
            }
        }
    }

    return parse_u32_attribute_value(abstract_num.attribute("w:abstractNumId").value());
}

auto ensure_custom_abstract_numbering(
    pugi::xml_node numbering_root,
    const featherdoc::numbering_definition &definition) -> std::optional<std::uint32_t> {
    auto abstract_num = find_abstract_numbering_by_name(numbering_root, definition.name);
    auto abstract_num_id = std::optional<std::uint32_t>{};
    if (abstract_num == pugi::xml_node{}) {
        abstract_num_id =
            max_numbering_id(numbering_root, "w:abstractNum", "w:abstractNumId") + 1U;
        abstract_num = numbering_root.append_child("w:abstractNum");
        if (abstract_num == pugi::xml_node{}) {
            return std::nullopt;
        }
    } else {
        abstract_num_id =
            parse_u32_attribute_value(abstract_num.attribute("w:abstractNumId").value());
        if (!abstract_num_id.has_value()) {
            abstract_num_id =
                max_numbering_id(numbering_root, "w:abstractNum", "w:abstractNumId") + 1U;
        }
    }

    ensure_attribute_value(abstract_num, "w:abstractNumId", std::to_string(*abstract_num_id));

    auto multi_level_type = abstract_num.child("w:multiLevelType");
    if (multi_level_type == pugi::xml_node{}) {
        multi_level_type = abstract_num.append_child("w:multiLevelType");
    }

    auto name_node = abstract_num.child("w:name");
    if (name_node == pugi::xml_node{}) {
        name_node = abstract_num.append_child("w:name");
    }

    if (multi_level_type == pugi::xml_node{} || name_node == pugi::xml_node{}) {
        return std::nullopt;
    }

    ensure_attribute_value(multi_level_type, "w:val", "multilevel");
    ensure_attribute_value(name_node, "w:val", definition.name);

    remove_named_children(abstract_num, "w:lvl");
    for (const auto &level_definition : sort_numbering_levels(definition.levels)) {
        if (!append_custom_level_definition(abstract_num, level_definition)) {
            return std::nullopt;
        }
    }

    return abstract_num_id;
}

auto append_numbering_instance_for_abstract(pugi::xml_node numbering_root,
                                            std::uint32_t abstract_num_id,
                                            std::optional<std::uint32_t> restart_level =
                                                std::nullopt)
    -> std::optional<std::uint32_t> {
    const auto num_id = max_numbering_id(numbering_root, "w:num", "w:numId") + 1U;
    auto numbering_instance = numbering_root.append_child("w:num");
    if (numbering_instance == pugi::xml_node{}) {
        return std::nullopt;
    }

    ensure_attribute_value(numbering_instance, "w:numId", std::to_string(num_id));

    auto abstract_num_id_node = numbering_instance.append_child("w:abstractNumId");
    if (abstract_num_id_node == pugi::xml_node{}) {
        return std::nullopt;
    }
    ensure_attribute_value(abstract_num_id_node, "w:val", std::to_string(abstract_num_id));

    if (restart_level.has_value()) {
        auto level_override = numbering_instance.append_child("w:lvlOverride");
        if (level_override == pugi::xml_node{}) {
            return std::nullopt;
        }
        ensure_attribute_value(level_override, "w:ilvl", std::to_string(*restart_level));

        auto start_override = level_override.append_child("w:startOverride");
        if (start_override == pugi::xml_node{}) {
            return std::nullopt;
        }
        ensure_attribute_value(start_override, "w:val", "1");
    }

    return num_id;
}

auto append_numbering_level_override(
    pugi::xml_node numbering_instance,
    const featherdoc::numbering_level_override_summary &override_summary) -> bool {
    if (override_summary.level > max_list_level) {
        return false;
    }

    auto level_override = numbering_instance.append_child("w:lvlOverride");
    if (level_override == pugi::xml_node{}) {
        return false;
    }
    ensure_attribute_value(level_override, "w:ilvl",
                           std::to_string(override_summary.level));

    if (override_summary.start_override.has_value()) {
        auto start_override = level_override.append_child("w:startOverride");
        if (start_override == pugi::xml_node{}) {
            return false;
        }
        ensure_attribute_value(start_override, "w:val",
                               std::to_string(*override_summary.start_override));
    }

    if (override_summary.level_definition.has_value() &&
        !append_custom_level_definition(level_override,
                                        *override_summary.level_definition)) {
        return false;
    }

    return true;
}

auto append_numbering_instance_for_abstract(
    pugi::xml_node numbering_root, std::uint32_t abstract_num_id,
    const std::vector<featherdoc::numbering_level_override_summary> &level_overrides)
    -> std::optional<std::uint32_t> {
    const auto num_id = max_numbering_id(numbering_root, "w:num", "w:numId") + 1U;
    auto numbering_instance = numbering_root.append_child("w:num");
    if (numbering_instance == pugi::xml_node{}) {
        return std::nullopt;
    }

    ensure_attribute_value(numbering_instance, "w:numId", std::to_string(num_id));

    auto abstract_num_id_node = numbering_instance.append_child("w:abstractNumId");
    if (abstract_num_id_node == pugi::xml_node{}) {
        return std::nullopt;
    }
    ensure_attribute_value(abstract_num_id_node, "w:val", std::to_string(abstract_num_id));

    for (const auto &level_override : sort_numbering_level_overrides(level_overrides)) {
        if (!append_numbering_level_override(numbering_instance, level_override)) {
            return std::nullopt;
        }
    }

    return num_id;
}

auto ensure_numbering_instance_for_abstract(
    pugi::xml_node numbering_root, std::uint32_t abstract_num_id,
    std::optional<std::uint32_t> restart_level = std::nullopt, bool force_new_instance = false)
    -> std::optional<std::uint32_t> {
    if (!force_new_instance) {
        const auto numbering_instance =
            find_numbering_instance_for_abstract(numbering_root, abstract_num_id);
        if (numbering_instance != pugi::xml_node{}) {
            if (const auto num_id =
                    parse_u32_attribute_value(numbering_instance.attribute("w:numId").value());
                num_id.has_value()) {
                return num_id;
            }
        }
    }

    return append_numbering_instance_for_abstract(numbering_root, abstract_num_id,
                                                  restart_level);
}

auto managed_list_kind_for_num_id(pugi::xml_node numbering_root, std::uint32_t num_id)
    -> std::optional<featherdoc::list_kind> {
    const auto numbering_instance = find_numbering_instance_by_id(numbering_root, num_id);
    if (numbering_instance == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto abstract_num_id = parse_u32_attribute_value(
        numbering_instance.child("w:abstractNumId").attribute("w:val").value());
    if (!abstract_num_id.has_value()) {
        return std::nullopt;
    }

    for (auto abstract_num = numbering_root.child("w:abstractNum");
         abstract_num != pugi::xml_node{};
         abstract_num = abstract_num.next_sibling("w:abstractNum")) {
        if (parse_u32_attribute_value(abstract_num.attribute("w:abstractNumId").value()) !=
            abstract_num_id) {
            continue;
        }

        const auto name =
            std::string_view{abstract_num.child("w:name").attribute("w:val").value()};
        if (name == managed_bullet_list_name) {
            return featherdoc::list_kind::bullet;
        }
        if (name == managed_decimal_list_name) {
            return featherdoc::list_kind::decimal;
        }
        return std::nullopt;
    }

    return std::nullopt;
}

auto previous_managed_num_id_for_kind(pugi::xml_node paragraph, pugi::xml_node numbering_root,
                                      featherdoc::list_kind kind)
    -> std::optional<std::uint32_t> {
    if (paragraph == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto previous_paragraph = paragraph.previous_sibling("w:p");
    if (previous_paragraph == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto num_id = parse_u32_attribute_value(previous_paragraph.child("w:pPr")
                                                      .child("w:numPr")
                                                      .child("w:numId")
                                                      .attribute("w:val")
                                                      .value());
    if (!num_id.has_value()) {
        return std::nullopt;
    }

    const auto previous_kind = managed_list_kind_for_num_id(numbering_root, *num_id);
    if (!previous_kind.has_value() || *previous_kind != kind) {
        return std::nullopt;
    }

    return num_id;
}

auto ensure_managed_numbering_instance(
    pugi::xml_node numbering_root, featherdoc::list_kind kind,
    std::optional<std::uint32_t> restart_level = std::nullopt, bool force_new_instance = false)
    -> std::optional<std::uint32_t> {
    const auto abstract_num_id = ensure_managed_abstract_numbering(numbering_root, kind);
    if (!abstract_num_id.has_value()) {
        return std::nullopt;
    }

    return ensure_numbering_instance_for_abstract(numbering_root, *abstract_num_id,
                                                  restart_level, force_new_instance);
}

auto apply_list_numbering(pugi::xml_node paragraph, pugi::xml_node numbering_root,
                          featherdoc::list_kind kind, std::uint32_t level, bool force_restart)
    -> std::optional<std::uint32_t> {
    if (!force_restart) {
        if (const auto previous_num_id =
                previous_managed_num_id_for_kind(paragraph, numbering_root, kind);
            previous_num_id.has_value()) {
            return previous_num_id;
        }
    }

    return ensure_managed_numbering_instance(
        numbering_root, kind,
        force_restart ? std::optional<std::uint32_t>{level} : std::nullopt, force_restart);
}

auto apply_numbering_to_paragraph(pugi::xml_node paragraph, std::uint32_t level,
                                  std::uint32_t num_id) -> bool {
    auto paragraph_properties = ensure_paragraph_properties_node(paragraph);
    if (paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    auto numbering_properties = paragraph_properties.child("w:numPr");
    if (numbering_properties == pugi::xml_node{}) {
        numbering_properties = paragraph_properties.append_child("w:numPr");
    }
    if (numbering_properties == pugi::xml_node{}) {
        return false;
    }

    auto level_node = numbering_properties.child("w:ilvl");
    if (level_node == pugi::xml_node{}) {
        level_node = numbering_properties.append_child("w:ilvl");
    }
    auto num_id_node = numbering_properties.child("w:numId");
    if (num_id_node == pugi::xml_node{}) {
        num_id_node = numbering_properties.append_child("w:numId");
    }
    if (level_node == pugi::xml_node{} || num_id_node == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(level_node, "w:val", std::to_string(level));
    ensure_attribute_value(num_id_node, "w:val", std::to_string(num_id));
    return true;
}

auto apply_numbering_to_style(pugi::xml_node style, std::uint32_t level, std::uint32_t num_id)
    -> bool {
    auto paragraph_properties = ensure_style_paragraph_properties_node(style);
    if (paragraph_properties == pugi::xml_node{}) {
        return false;
    }

    auto numbering_properties = paragraph_properties.child("w:numPr");
    if (numbering_properties == pugi::xml_node{}) {
        if (const auto bidi = paragraph_properties.child("w:bidi"); bidi != pugi::xml_node{}) {
            numbering_properties = paragraph_properties.insert_child_before("w:numPr", bidi);
        } else if (const auto first_child = paragraph_properties.first_child();
                   first_child != pugi::xml_node{}) {
            numbering_properties =
                paragraph_properties.insert_child_before("w:numPr", first_child);
        } else {
            numbering_properties = paragraph_properties.append_child("w:numPr");
        }
    }
    if (numbering_properties == pugi::xml_node{}) {
        return false;
    }

    auto level_node = numbering_properties.child("w:ilvl");
    if (level_node == pugi::xml_node{}) {
        level_node = numbering_properties.append_child("w:ilvl");
    }
    auto num_id_node = numbering_properties.child("w:numId");
    if (num_id_node == pugi::xml_node{}) {
        num_id_node = numbering_properties.append_child("w:numId");
    }
    if (level_node == pugi::xml_node{} || num_id_node == pugi::xml_node{}) {
        return false;
    }

    ensure_attribute_value(level_node, "w:val", std::to_string(level));
    ensure_attribute_value(num_id_node, "w:val", std::to_string(num_id));
    return true;
}
