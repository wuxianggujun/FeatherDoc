enum class numbering_mutation_status {
    success,
    identifier_space_exhausted,
    allocation_failed,
};

struct numbering_identifier_result final {
    numbering_mutation_status status{
        numbering_mutation_status::allocation_failed};
    std::optional<std::uint32_t> identifier;

    [[nodiscard]] auto succeeded() const noexcept -> bool {
        return status == numbering_mutation_status::success &&
               identifier.has_value();
    }
};

struct numbering_definition_instance_plan final {
    std::uint32_t abstract_num_id{0U};
    std::optional<std::uint32_t> reserved_abstract_num_id;
    std::optional<std::uint32_t> existing_num_id;
    std::optional<std::uint32_t> reserved_num_id;
};

struct numbering_definition_instance_plan_result final {
    numbering_mutation_status status{
        numbering_mutation_status::allocation_failed};
    std::optional<numbering_definition_instance_plan> plan;

    [[nodiscard]] auto succeeded() const noexcept -> bool {
        return status == numbering_mutation_status::success && plan.has_value();
    }
};

struct numbering_identifier_batch_result final {
    numbering_mutation_status status{
        numbering_mutation_status::allocation_failed};
    std::vector<std::uint32_t> identifiers;

    [[nodiscard]] auto succeeded() const noexcept -> bool {
        return status == numbering_mutation_status::success;
    }
};

struct numbering_catalog_definition_plan final {
    std::optional<std::uint32_t> reserved_abstract_num_id;
    std::vector<std::uint32_t> reserved_num_ids;
};

struct numbering_catalog_import_plan final {
    std::vector<numbering_catalog_definition_plan> definitions;
};

enum class numbering_catalog_exhausted_identifier_kind {
    none,
    abstract_numbering,
    numbering_instance,
};

struct numbering_catalog_import_plan_result final {
    numbering_mutation_status status{
        numbering_mutation_status::allocation_failed};
    numbering_catalog_exhausted_identifier_kind exhausted_identifier_kind{
        numbering_catalog_exhausted_identifier_kind::none};
    std::optional<numbering_catalog_import_plan> plan;

    [[nodiscard]] auto succeeded() const noexcept -> bool {
        return status == numbering_mutation_status::success && plan.has_value();
    }
};

void set_numbering_mutation_error(featherdoc::document_error_info &error_info,
                                  std::string_view numbering_entry_name,
                                  numbering_mutation_status status,
                                  std::string_view exhausted_detail,
                                  std::string_view allocation_detail) {
    if (status == numbering_mutation_status::identifier_space_exhausted) {
        set_last_error(
            error_info, featherdoc::document_errc::identifier_space_exhausted,
            std::string{exhausted_detail}, std::string{numbering_entry_name});
        return;
    }

    set_last_error(
        error_info, std::make_error_code(std::errc::not_enough_memory),
        std::string{allocation_detail}, std::string{numbering_entry_name});
}

auto reserve_next_numbering_identifier(pugi::xml_node numbering_root,
                                       const char *child_name,
                                       const char *attribute_name)
    -> numbering_identifier_result {
    const auto identifier = featherdoc::detail::increment_identifier(
        max_numbering_id(numbering_root, child_name, attribute_name));
    if (!identifier.has_value()) {
        return {numbering_mutation_status::identifier_space_exhausted,
                std::nullopt};
    }
    return {numbering_mutation_status::success, *identifier};
}

auto reserve_numbering_identifier_batch(pugi::xml_node numbering_root,
                                        const char *child_name,
                                        const char *attribute_name,
                                        std::size_t requested_count)
    -> numbering_identifier_batch_result {
    const auto maximum =
        max_numbering_id(numbering_root, child_name, attribute_name);
    const auto available_count = static_cast<std::size_t>(
        std::numeric_limits<std::uint32_t>::max() - maximum);
    if (requested_count > available_count) {
        return {numbering_mutation_status::identifier_space_exhausted, {}};
    }

    auto identifiers = std::vector<std::uint32_t>{};
    identifiers.reserve(requested_count);
    for (std::size_t index = 0U; index < requested_count; ++index) {
        identifiers.push_back(maximum + 1U + static_cast<std::uint32_t>(index));
    }
    return {numbering_mutation_status::success, std::move(identifiers)};
}

auto plan_numbering_abstract_definition(pugi::xml_node numbering_root,
                                        std::string_view definition_name)
    -> numbering_definition_instance_plan_result {
    auto plan = numbering_definition_instance_plan{};
    const auto abstract_num =
        find_abstract_numbering_by_name(numbering_root, definition_name);
    if (abstract_num != pugi::xml_node{}) {
        if (const auto abstract_num_id = parse_u32_attribute_value(
                abstract_num.attribute("w:abstractNumId").value())) {
            plan.abstract_num_id = *abstract_num_id;
            return {numbering_mutation_status::success, std::move(plan)};
        }
    }

    const auto abstract_num_id = reserve_next_numbering_identifier(
        numbering_root, "w:abstractNum", "w:abstractNumId");
    if (!abstract_num_id.succeeded()) {
        return {abstract_num_id.status, std::nullopt};
    }
    plan.abstract_num_id = *abstract_num_id.identifier;
    plan.reserved_abstract_num_id = *abstract_num_id.identifier;
    return {numbering_mutation_status::success, std::move(plan)};
}

auto plan_numbering_instance_for_abstract(pugi::xml_node numbering_root,
                                          std::uint32_t abstract_num_id,
                                          bool force_new_instance = false)
    -> numbering_definition_instance_plan_result {
    auto plan = numbering_definition_instance_plan{};
    plan.abstract_num_id = abstract_num_id;

    if (!force_new_instance) {
        const auto numbering_instance = find_numbering_instance_for_abstract(
            numbering_root, abstract_num_id);
        if (numbering_instance != pugi::xml_node{}) {
            const auto num_id = parse_u32_attribute_value(
                numbering_instance.attribute("w:numId").value());
            if (num_id.has_value()) {
                plan.existing_num_id = *num_id;
                return {numbering_mutation_status::success, std::move(plan)};
            }
        }
    }

    const auto num_id =
        reserve_next_numbering_identifier(numbering_root, "w:num", "w:numId");
    if (!num_id.succeeded()) {
        return {num_id.status, std::nullopt};
    }
    plan.reserved_num_id = *num_id.identifier;
    return {numbering_mutation_status::success, std::move(plan)};
}

auto plan_numbering_definition_instance(pugi::xml_node numbering_root,
                                        std::string_view definition_name,
                                        bool force_new_instance = false)
    -> numbering_definition_instance_plan_result {
    auto abstract_plan =
        plan_numbering_abstract_definition(numbering_root, definition_name);
    if (!abstract_plan.succeeded()) {
        return abstract_plan;
    }
    auto plan = std::move(*abstract_plan.plan);

    if (!force_new_instance && !plan.reserved_abstract_num_id.has_value()) {
        const auto numbering_instance = find_numbering_instance_for_abstract(
            numbering_root, plan.abstract_num_id);
        if (numbering_instance != pugi::xml_node{}) {
            const auto num_id = parse_u32_attribute_value(
                numbering_instance.attribute("w:numId").value());
            if (num_id.has_value()) {
                plan.existing_num_id = *num_id;
                return {numbering_mutation_status::success, std::move(plan)};
            }
        }
    }

    const auto num_id =
        reserve_next_numbering_identifier(numbering_root, "w:num", "w:numId");
    if (!num_id.succeeded()) {
        return {num_id.status, std::nullopt};
    }
    plan.reserved_num_id = *num_id.identifier;
    return {numbering_mutation_status::success, std::move(plan)};
}

auto plan_numbering_catalog_import(pugi::xml_node numbering_root,
                                   const featherdoc::numbering_catalog &catalog)
    -> numbering_catalog_import_plan_result {
    std::size_t required_abstract_num_ids = 0U;
    std::size_t required_num_ids = 0U;
    constexpr auto maximum_identifier_count =
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());

    for (const auto &catalog_definition : catalog.definitions) {
        const auto abstract_num = find_abstract_numbering_by_name(
            numbering_root, catalog_definition.definition.name);
        if (abstract_num == pugi::xml_node{} ||
            !parse_u32_attribute_value(
                 abstract_num.attribute("w:abstractNumId").value())
                 .has_value()) {
            if (required_abstract_num_ids == maximum_identifier_count) {
                return {numbering_mutation_status::identifier_space_exhausted,
                        numbering_catalog_exhausted_identifier_kind::
                            abstract_numbering,
                        std::nullopt};
            }
            ++required_abstract_num_ids;
        }

        if (catalog_definition.instances.size() >
            maximum_identifier_count - required_num_ids) {
            return {
                numbering_mutation_status::identifier_space_exhausted,
                numbering_catalog_exhausted_identifier_kind::numbering_instance,
                std::nullopt};
        }
        required_num_ids += catalog_definition.instances.size();
    }

    auto abstract_num_ids = reserve_numbering_identifier_batch(
        numbering_root, "w:abstractNum", "w:abstractNumId",
        required_abstract_num_ids);
    if (!abstract_num_ids.succeeded()) {
        return {abstract_num_ids.status,
                numbering_catalog_exhausted_identifier_kind::abstract_numbering,
                std::nullopt};
    }

    auto num_ids = reserve_numbering_identifier_batch(
        numbering_root, "w:num", "w:numId", required_num_ids);
    if (!num_ids.succeeded()) {
        return {num_ids.status,
                numbering_catalog_exhausted_identifier_kind::numbering_instance,
                std::nullopt};
    }

    auto plan = numbering_catalog_import_plan{};
    plan.definitions.reserve(catalog.definitions.size());
    std::size_t abstract_num_id_index = 0U;
    std::size_t num_id_index = 0U;
    for (const auto &catalog_definition : catalog.definitions) {
        auto definition_plan = numbering_catalog_definition_plan{};
        const auto abstract_num = find_abstract_numbering_by_name(
            numbering_root, catalog_definition.definition.name);
        if (abstract_num == pugi::xml_node{} ||
            !parse_u32_attribute_value(
                 abstract_num.attribute("w:abstractNumId").value())
                 .has_value()) {
            definition_plan.reserved_abstract_num_id =
                abstract_num_ids.identifiers[abstract_num_id_index++];
        }

        const auto instance_count = catalog_definition.instances.size();
        definition_plan.reserved_num_ids.reserve(instance_count);
        for (std::size_t instance_index = 0U; instance_index < instance_count;
             ++instance_index) {
            definition_plan.reserved_num_ids.push_back(
                num_ids.identifiers[num_id_index++]);
        }
        plan.definitions.push_back(std::move(definition_plan));
    }

    return {numbering_mutation_status::success,
            numbering_catalog_exhausted_identifier_kind::none, std::move(plan)};
}

auto ensure_managed_abstract_numbering(
    pugi::xml_node numbering_root, featherdoc::list_kind kind,
    std::optional<std::uint32_t> reserved_abstract_num_id = std::nullopt)
    -> numbering_identifier_result {
    const auto managed_name = list_name_for(kind);

    auto abstract_num =
        find_abstract_numbering_by_name(numbering_root, managed_name);
    if (abstract_num == pugi::xml_node{}) {
        auto abstract_num_id = reserved_abstract_num_id;
        if (!abstract_num_id.has_value()) {
            const auto reservation = reserve_next_numbering_identifier(
                numbering_root, "w:abstractNum", "w:abstractNumId");
            if (!reservation.succeeded()) {
                return reservation;
            }
            abstract_num_id = *reservation.identifier;
        }
        abstract_num = featherdoc::detail::checked_append_xml_element(
            numbering_root, "w:abstractNum");
        if (abstract_num == pugi::xml_node{}) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }

        if (!ensure_attribute_value(abstract_num, "w:abstractNumId",
                                    std::to_string(*abstract_num_id))) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }

        auto multi_level_type = featherdoc::detail::checked_append_xml_element(
            abstract_num, "w:multiLevelType");
        auto name_node = featherdoc::detail::checked_append_xml_element(
            abstract_num, "w:name");
        if (multi_level_type == pugi::xml_node{} ||
            name_node == pugi::xml_node{}) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }

        if (!ensure_attribute_value(multi_level_type, "w:val", "multilevel") ||
            !ensure_attribute_value(name_node, "w:val", managed_name)) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }

        for (std::uint32_t level = 0U; level <= max_list_level; ++level) {
            if (!append_level_definition(abstract_num, kind, level)) {
                return {numbering_mutation_status::allocation_failed,
                        std::nullopt};
            }
        }
    }

    const auto identifier = parse_u32_attribute_value(
        abstract_num.attribute("w:abstractNumId").value());
    if (!identifier.has_value()) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }
    return {numbering_mutation_status::success, *identifier};
}

auto ensure_custom_abstract_numbering(
    pugi::xml_node numbering_root,
    const featherdoc::numbering_definition &definition,
    std::optional<std::uint32_t> reserved_abstract_num_id = std::nullopt)
    -> numbering_identifier_result {
    auto abstract_num =
        find_abstract_numbering_by_name(numbering_root, definition.name);
    auto abstract_num_id = std::optional<std::uint32_t>{};
    if (abstract_num == pugi::xml_node{}) {
        abstract_num_id = reserved_abstract_num_id;
        if (!abstract_num_id.has_value()) {
            const auto reservation = reserve_next_numbering_identifier(
                numbering_root, "w:abstractNum", "w:abstractNumId");
            if (!reservation.succeeded()) {
                return reservation;
            }
            abstract_num_id = *reservation.identifier;
        }
        abstract_num = featherdoc::detail::checked_append_xml_element(
            numbering_root, "w:abstractNum");
        if (abstract_num == pugi::xml_node{}) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }
    } else {
        abstract_num_id = parse_u32_attribute_value(
            abstract_num.attribute("w:abstractNumId").value());
        if (!abstract_num_id.has_value()) {
            abstract_num_id = reserved_abstract_num_id;
            if (!abstract_num_id.has_value()) {
                const auto reservation = reserve_next_numbering_identifier(
                    numbering_root, "w:abstractNum", "w:abstractNumId");
                if (!reservation.succeeded()) {
                    return reservation;
                }
                abstract_num_id = *reservation.identifier;
            }
        }
    }

    if (!abstract_num_id.has_value()) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    if (!ensure_attribute_value(abstract_num, "w:abstractNumId",
                                std::to_string(*abstract_num_id))) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    auto multi_level_type = abstract_num.child("w:multiLevelType");
    if (multi_level_type == pugi::xml_node{}) {
        multi_level_type = featherdoc::detail::checked_append_xml_element(
            abstract_num, "w:multiLevelType");
    }

    auto name_node = abstract_num.child("w:name");
    if (name_node == pugi::xml_node{}) {
        name_node = featherdoc::detail::checked_append_xml_element(abstract_num,
                                                                   "w:name");
    }

    if (multi_level_type == pugi::xml_node{} || name_node == pugi::xml_node{}) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    if (!ensure_attribute_value(multi_level_type, "w:val", "multilevel") ||
        !ensure_attribute_value(name_node, "w:val", definition.name)) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    if (!remove_named_children(abstract_num, "w:lvl")) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }
    for (const auto &level_definition :
         sort_numbering_levels(definition.levels)) {
        if (!append_custom_level_definition(abstract_num, level_definition)) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }
    }

    return {numbering_mutation_status::success, *abstract_num_id};
}

auto append_numbering_instance_for_abstract(
    pugi::xml_node numbering_root, std::uint32_t abstract_num_id,
    std::optional<std::uint32_t> restart_level = std::nullopt,
    std::optional<std::uint32_t> reserved_num_id = std::nullopt)
    -> numbering_identifier_result {
    auto num_id = reserved_num_id;
    if (!num_id.has_value()) {
        const auto reservation = reserve_next_numbering_identifier(
            numbering_root, "w:num", "w:numId");
        if (!reservation.succeeded()) {
            return reservation;
        }
        num_id = *reservation.identifier;
    }
    auto numbering_instance =
        featherdoc::detail::checked_append_xml_element(numbering_root, "w:num");
    if (numbering_instance == pugi::xml_node{}) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    if (!ensure_attribute_value(numbering_instance, "w:numId",
                                std::to_string(*num_id))) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    auto abstract_num_id_node = featherdoc::detail::checked_append_xml_element(
        numbering_instance, "w:abstractNumId");
    if (abstract_num_id_node == pugi::xml_node{}) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }
    if (!ensure_attribute_value(abstract_num_id_node, "w:val",
                                std::to_string(abstract_num_id))) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    if (restart_level.has_value()) {
        auto level_override = featherdoc::detail::checked_append_xml_element(
            numbering_instance, "w:lvlOverride");
        if (level_override == pugi::xml_node{}) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }
        if (!ensure_attribute_value(level_override, "w:ilvl",
                                    std::to_string(*restart_level))) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }

        auto start_override = featherdoc::detail::checked_append_xml_element(
            level_override, "w:startOverride");
        if (start_override == pugi::xml_node{}) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }
        if (!ensure_attribute_value(start_override, "w:val", "1")) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }
    }

    return {numbering_mutation_status::success, *num_id};
}

auto append_numbering_level_override(
    pugi::xml_node numbering_instance,
    const featherdoc::numbering_level_override_summary &override_summary)
    -> bool {
    if (override_summary.level > max_list_level) {
        return false;
    }

    auto level_override = featherdoc::detail::checked_append_xml_element(
        numbering_instance, "w:lvlOverride");
    if (level_override == pugi::xml_node{}) {
        return false;
    }
    if (!ensure_attribute_value(level_override, "w:ilvl",
                                std::to_string(override_summary.level))) {
        return false;
    }

    if (override_summary.start_override.has_value()) {
        auto start_override = featherdoc::detail::checked_append_xml_element(
            level_override, "w:startOverride");
        if (start_override == pugi::xml_node{}) {
            return false;
        }
        if (!ensure_attribute_value(
                start_override, "w:val",
                std::to_string(*override_summary.start_override))) {
            return false;
        }
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
    const std::vector<featherdoc::numbering_level_override_summary>
        &level_overrides,
    std::optional<std::uint32_t> reserved_num_id = std::nullopt)
    -> numbering_identifier_result {
    auto num_id = reserved_num_id;
    if (!num_id.has_value()) {
        const auto reservation = reserve_next_numbering_identifier(
            numbering_root, "w:num", "w:numId");
        if (!reservation.succeeded()) {
            return reservation;
        }
        num_id = *reservation.identifier;
    }
    auto numbering_instance =
        featherdoc::detail::checked_append_xml_element(numbering_root, "w:num");
    if (numbering_instance == pugi::xml_node{}) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    if (!ensure_attribute_value(numbering_instance, "w:numId",
                                std::to_string(*num_id))) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    auto abstract_num_id_node = featherdoc::detail::checked_append_xml_element(
        numbering_instance, "w:abstractNumId");
    if (abstract_num_id_node == pugi::xml_node{}) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }
    if (!ensure_attribute_value(abstract_num_id_node, "w:val",
                                std::to_string(abstract_num_id))) {
        return {numbering_mutation_status::allocation_failed, std::nullopt};
    }

    for (const auto &level_override :
         sort_numbering_level_overrides(level_overrides)) {
        if (!append_numbering_level_override(numbering_instance,
                                             level_override)) {
            return {numbering_mutation_status::allocation_failed, std::nullopt};
        }
    }

    return {numbering_mutation_status::success, *num_id};
}

auto ensure_numbering_instance_for_abstract(
    pugi::xml_node numbering_root, std::uint32_t abstract_num_id,
    std::optional<std::uint32_t> restart_level = std::nullopt,
    bool force_new_instance = false,
    std::optional<std::uint32_t> reserved_num_id = std::nullopt)
    -> numbering_identifier_result {
    if (!force_new_instance) {
        const auto numbering_instance = find_numbering_instance_for_abstract(
            numbering_root, abstract_num_id);
        if (numbering_instance != pugi::xml_node{}) {
            if (const auto num_id = parse_u32_attribute_value(
                    numbering_instance.attribute("w:numId").value());
                num_id.has_value()) {
                return {numbering_mutation_status::success, *num_id};
            }
        }
    }

    return append_numbering_instance_for_abstract(
        numbering_root, abstract_num_id, restart_level, reserved_num_id);
}

auto managed_list_kind_for_num_id(pugi::xml_node numbering_root,
                                  std::uint32_t num_id)
    -> std::optional<featherdoc::list_kind> {
    const auto numbering_instance =
        find_numbering_instance_by_id(numbering_root, num_id);
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
        if (parse_u32_attribute_value(
                abstract_num.attribute("w:abstractNumId").value()) !=
            abstract_num_id) {
            continue;
        }

        const auto name = std::string_view{
            abstract_num.child("w:name").attribute("w:val").value()};
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

auto previous_managed_num_id_for_kind(pugi::xml_node paragraph,
                                      pugi::xml_node numbering_root,
                                      featherdoc::list_kind kind)
    -> std::optional<std::uint32_t> {
    if (paragraph == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto previous_paragraph = paragraph.previous_sibling("w:p");
    if (previous_paragraph == pugi::xml_node{}) {
        return std::nullopt;
    }

    const auto num_id =
        parse_u32_attribute_value(previous_paragraph.child("w:pPr")
                                      .child("w:numPr")
                                      .child("w:numId")
                                      .attribute("w:val")
                                      .value());
    if (!num_id.has_value()) {
        return std::nullopt;
    }

    const auto previous_kind =
        managed_list_kind_for_num_id(numbering_root, *num_id);
    if (!previous_kind.has_value() || *previous_kind != kind) {
        return std::nullopt;
    }

    return num_id;
}

auto commit_managed_numbering_instance(
    pugi::xml_node numbering_root, featherdoc::list_kind kind,
    const numbering_definition_instance_plan &plan,
    std::optional<std::uint32_t> restart_level = std::nullopt,
    bool force_new_instance = false) -> numbering_identifier_result {
    const auto abstract_num_id = ensure_managed_abstract_numbering(
        numbering_root, kind, plan.reserved_abstract_num_id);
    if (!abstract_num_id.succeeded()) {
        return abstract_num_id;
    }

    if (plan.existing_num_id.has_value() && !force_new_instance) {
        return {numbering_mutation_status::success, *plan.existing_num_id};
    }

    return ensure_numbering_instance_for_abstract(
        numbering_root, *abstract_num_id.identifier, restart_level,
        force_new_instance, plan.reserved_num_id);
}

auto apply_numbering_to_paragraph_properties(
    pugi::xml_node paragraph_properties, std::uint32_t level,
    std::uint32_t num_id) -> bool {
    if (paragraph_properties == pugi::xml_node{} ||
        !remove_named_children(paragraph_properties, "w:numPr")) {
        return false;
    }

    auto numbering_properties =
        append_numbering_properties_in_schema_order(paragraph_properties);
    if (numbering_properties == pugi::xml_node{}) {
        return false;
    }

    auto level_node = featherdoc::detail::checked_append_xml_element(
        numbering_properties, "w:ilvl");
    auto num_id_node = featherdoc::detail::checked_append_xml_element(
        numbering_properties, "w:numId");
    if (level_node == pugi::xml_node{} || num_id_node == pugi::xml_node{}) {
        return false;
    }

    return ensure_attribute_value(level_node, "w:val", std::to_string(level)) &&
           ensure_attribute_value(num_id_node, "w:val",
                                  std::to_string(num_id));
}

auto apply_numbering_to_style(pugi::xml_node style, std::uint32_t level,
                              std::uint32_t num_id) -> bool {
    bool cleared = false;
    if (!clear_direct_numbering_properties(style, cleared)) {
        return false;
    }
    const auto paragraph_properties =
        ensure_style_paragraph_properties_node(style);
    return apply_numbering_to_paragraph_properties(paragraph_properties, level,
                                                   num_id);
}
