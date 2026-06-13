auto decode_numbering_format_kind(std::string_view format_name) -> featherdoc::list_kind {
    return format_name == "bullet" ? featherdoc::list_kind::bullet
                                   : featherdoc::list_kind::decimal;
}

auto sort_numbering_level_overrides(
    const std::vector<featherdoc::numbering_level_override_summary> &level_overrides)
    -> std::vector<featherdoc::numbering_level_override_summary> {
    auto sorted = level_overrides;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto &left, const auto &right) { return left.level < right.level; });
    return sorted;
}

auto summarize_numbering_level_node(
    pugi::xml_node level_node, std::optional<std::uint32_t> fallback_level = std::nullopt)
    -> std::optional<featherdoc::numbering_level_definition> {
    auto level = parse_u32_attribute_value(level_node.attribute("w:ilvl").value());
    if (!level.has_value()) {
        level = fallback_level;
    }
    if (!level.has_value()) {
        return std::nullopt;
    }

    auto definition = featherdoc::numbering_level_definition{};
    definition.level = *level;
    definition.kind = decode_numbering_format_kind(
        level_node.child("w:numFmt").attribute("w:val").value());
    definition.start = parse_u32_attribute_value(
                           level_node.child("w:start").attribute("w:val").value())
                           .value_or(1U);
    definition.text_pattern = level_node.child("w:lvlText").attribute("w:val").value();
    return definition;
}

auto summarize_numbering_level_override_node(pugi::xml_node level_override_node)
    -> std::optional<featherdoc::numbering_level_override_summary> {
    const auto level =
        parse_u32_attribute_value(level_override_node.attribute("w:ilvl").value());
    if (!level.has_value()) {
        return std::nullopt;
    }

    auto summary = featherdoc::numbering_level_override_summary{};
    summary.level = *level;
    summary.start_override = parse_u32_attribute_value(
        level_override_node.child("w:startOverride").attribute("w:val").value());
    if (const auto level_node = level_override_node.child("w:lvl");
        level_node != pugi::xml_node{}) {
        if (const auto level_definition =
                summarize_numbering_level_node(level_node, *level);
            level_definition.has_value()) {
            summary.level_definition = *level_definition;
        }
    }

    return summary;
}

auto summarize_numbering_instance_node(pugi::xml_node numbering_instance)
    -> std::optional<featherdoc::numbering_instance_summary> {
    const auto num_id =
        parse_u32_attribute_value(numbering_instance.attribute("w:numId").value());
    if (!num_id.has_value()) {
        return std::nullopt;
    }

    auto instance = featherdoc::numbering_instance_summary{};
    instance.instance_id = *num_id;
    for (auto level_override = numbering_instance.child("w:lvlOverride");
         level_override != pugi::xml_node{};
         level_override = level_override.next_sibling("w:lvlOverride")) {
        if (const auto override_summary =
                summarize_numbering_level_override_node(level_override);
            override_summary.has_value()) {
            instance.level_overrides.push_back(*override_summary);
        }
    }
    instance.level_overrides = sort_numbering_level_overrides(instance.level_overrides);
    return instance;
}

auto summarize_numbering_instance_lookup_node(pugi::xml_node numbering_root,
                                              pugi::xml_node numbering_instance)
    -> std::optional<featherdoc::numbering_instance_lookup_summary> {
    const auto abstract_num_id = parse_u32_attribute_value(
        numbering_instance.child("w:abstractNumId").attribute("w:val").value());
    if (!abstract_num_id.has_value()) {
        return std::nullopt;
    }

    const auto abstract_num = find_abstract_numbering_by_id(numbering_root, *abstract_num_id);
    if (abstract_num == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto instance = summarize_numbering_instance_node(numbering_instance);
    if (!instance.has_value()) {
        return std::nullopt;
    }

    auto summary = featherdoc::numbering_instance_lookup_summary{};
    summary.definition_id = *abstract_num_id;
    summary.definition_name = abstract_num.child("w:name").attribute("w:val").value();
    summary.instance = *instance;
    return summary;
}

auto collect_numbering_instance_summaries(pugi::xml_node numbering_root,
                                          std::uint32_t abstract_num_id)
    -> std::vector<featherdoc::numbering_instance_summary> {
    auto instances = std::vector<featherdoc::numbering_instance_summary>{};
    const auto abstract_num_id_text = std::to_string(abstract_num_id);
    for (auto num = numbering_root.child("w:num"); num != pugi::xml_node{};
         num = num.next_sibling("w:num")) {
        if (std::string_view{num.child("w:abstractNumId").attribute("w:val").value()} !=
            abstract_num_id_text) {
            continue;
        }

        const auto num_id = parse_u32_attribute_value(num.attribute("w:numId").value());
        if (!num_id.has_value()) {
            continue;
        }

        if (const auto instance = summarize_numbering_instance_node(num);
            instance.has_value()) {
            instances.push_back(*instance);
        }
    }

    std::sort(instances.begin(), instances.end(),
              [](const auto &left, const auto &right) {
                  return left.instance_id < right.instance_id;
              });
    return instances;
}

auto collect_numbering_instance_ids(
    const std::vector<featherdoc::numbering_instance_summary> &instances)
    -> std::vector<std::uint32_t> {
    auto instance_ids = std::vector<std::uint32_t>{};
    instance_ids.reserve(instances.size());
    for (const auto &instance : instances) {
        instance_ids.push_back(instance.instance_id);
    }
    return instance_ids;
}

auto summarize_numbering_definition_node(pugi::xml_node numbering_root,
                                         pugi::xml_node abstract_num)
    -> std::optional<featherdoc::numbering_definition_summary> {
    const auto definition_id =
        parse_u32_attribute_value(abstract_num.attribute("w:abstractNumId").value());
    if (!definition_id.has_value()) {
        return std::nullopt;
    }

    auto summary = featherdoc::numbering_definition_summary{};
    summary.definition_id = *definition_id;
    summary.name = abstract_num.child("w:name").attribute("w:val").value();
    for (auto level_node = abstract_num.child("w:lvl"); level_node != pugi::xml_node{};
         level_node = level_node.next_sibling("w:lvl")) {
        if (const auto level = summarize_numbering_level_node(level_node);
            level.has_value()) {
            summary.levels.push_back(*level);
        }
    }
    summary.levels = sort_numbering_levels(summary.levels);
    summary.instances = collect_numbering_instance_summaries(numbering_root, *definition_id);
    summary.instance_ids = collect_numbering_instance_ids(summary.instances);
    return summary;
}

auto validate_numbering_definition(const featherdoc::numbering_definition &definition,
                                   std::string &detail) -> bool {
    if (definition.name.empty()) {
        detail = "numbering definition name must not be empty";
        return false;
    }

    if (definition.name == managed_bullet_list_name ||
        definition.name == managed_decimal_list_name) {
        detail = "numbering definition name is reserved for FeatherDoc managed lists";
        return false;
    }

    if (definition.levels.empty()) {
        detail = "numbering definition must contain at least one level";
        return false;
    }

    std::unordered_set<std::uint32_t> seen_levels;
    for (const auto &level_definition : definition.levels) {
        if (level_definition.start == 0U) {
            detail = "numbering level start must be greater than 0";
            return false;
        }

        if (level_definition.level > max_list_level) {
            detail = "numbering level must be in the range [0, 8]";
            return false;
        }

        if (level_definition.text_pattern.empty()) {
            detail = "numbering level text_pattern must not be empty";
            return false;
        }

        if (!seen_levels.insert(level_definition.level).second) {
            detail = "numbering definition levels must be unique";
            return false;
        }
    }

    detail.clear();
    return true;
}

auto validate_numbering_catalog_definition(
    const featherdoc::numbering_catalog_definition &catalog_definition,
    std::string &detail) -> bool {
    if (catalog_definition.definition.name.empty()) {
        detail = "numbering catalog definition name must not be empty";
        return false;
    }

    if (catalog_definition.definition.levels.empty()) {
        detail = "numbering catalog definition must contain at least one level";
        return false;
    }

    std::unordered_set<std::uint32_t> seen_levels;
    for (const auto &level_definition : catalog_definition.definition.levels) {
        if (level_definition.start == 0U) {
            detail = "numbering catalog level start must be greater than 0";
            return false;
        }

        if (level_definition.level > max_list_level) {
            detail = "numbering catalog level must be in the range [0, 8]";
            return false;
        }

        if (level_definition.text_pattern.empty()) {
            detail = "numbering catalog level text_pattern must not be empty";
            return false;
        }

        if (!seen_levels.insert(level_definition.level).second) {
            detail = "numbering catalog definition levels must be unique";
            return false;
        }
    }

    for (const auto &instance : catalog_definition.instances) {
        std::unordered_set<std::uint32_t> seen_override_levels;
        for (const auto &level_override : instance.level_overrides) {
            if (level_override.level > max_list_level) {
                detail = "numbering catalog override level must be in the range [0, 8]";
                return false;
            }
            if (!seen_override_levels.insert(level_override.level).second) {
                detail = "numbering catalog override levels must be unique per instance";
                return false;
            }
            if (level_override.start_override.has_value() &&
                *level_override.start_override == 0U) {
                detail = "numbering catalog start override must be greater than 0";
                return false;
            }
            if (level_override.level_definition.has_value()) {
                const auto &definition = *level_override.level_definition;
                if (definition.start == 0U || definition.level > max_list_level ||
                    definition.text_pattern.empty()) {
                    detail = "numbering catalog override level definition is invalid";
                    return false;
                }
            }
        }
    }

    detail.clear();
    return true;
}
