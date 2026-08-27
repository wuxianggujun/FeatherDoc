namespace {

struct section_related_part_mutation_work final {
    pugi::xml_document document;
    pugi::xml_document relationships;
    pugi::xml_document content_types;
    pugi::xml_document settings;
    bool document_changed{false};
    bool relationships_changed{false};
    bool content_types_changed{false};
    bool settings_changed{false};
};

// Lazy package-part loading is an implementation detail of the mutation. If a
// later allocation fails, restore the exact pre-call DOM and loaded/attached
// flags so a failed edit cannot affect a subsequent save.
class lazy_xml_document_load_rollback final {
  public:
    lazy_xml_document_load_rollback(bool enabled, pugi::xml_document &document,
                                    bool &loaded, bool *has_part = nullptr,
                                    std::string *entry_name = nullptr)
        : document_(&document), loaded_(&loaded), has_part_(has_part),
          entry_name_(entry_name), old_loaded_(loaded),
          old_has_part_(has_part != nullptr ? *has_part : false) {
        if (!enabled || loaded) {
            return;
        }
        if (entry_name_ != nullptr) {
            old_entry_name_ = *entry_name_;
        }
        this->backup_ = std::move(document);
        this->active_ = true;
    }

    lazy_xml_document_load_rollback(
        const lazy_xml_document_load_rollback &) = delete;
    auto operator=(const lazy_xml_document_load_rollback &)
        -> lazy_xml_document_load_rollback & = delete;

    ~lazy_xml_document_load_rollback() {
        if (!this->active_) {
            return;
        }
        this->document_->reset();
        *this->document_ = std::move(this->backup_);
        *this->loaded_ = this->old_loaded_;
        if (this->has_part_ != nullptr) {
            *this->has_part_ = this->old_has_part_;
        }
        if (this->entry_name_ != nullptr) {
            *this->entry_name_ = std::move(this->old_entry_name_);
        }
    }

    void commit() noexcept { this->active_ = false; }

  private:
    pugi::xml_document *document_;
    bool *loaded_;
    bool *has_part_;
    std::string *entry_name_;
    pugi::xml_document backup_;
    std::string old_entry_name_;
    bool old_loaded_;
    bool old_has_part_;
    bool active_{false};
};

auto checked_set_section_attribute(pugi::xml_node node, const char *name,
                                   std::string_view value, bool &changed)
    -> bool {
    if (std::string_view{node.attribute(name).value()} == value) {
        return true;
    }
    if (!featherdoc::detail::checked_set_xml_attribute_value(node, name,
                                                              value)) {
        return false;
    }
    changed = true;
    return true;
}

auto checked_append_section_reference(pugi::xml_node section_properties,
                                      const char *reference_name)
    -> pugi::xml_node {
    const auto reference_name_view = std::string_view{reference_name};
    auto insertion_anchor = pugi::xml_node{};
    for (auto child = section_properties.first_child();
         child != pugi::xml_node{}; child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (reference_name_view == "w:headerReference") {
            if (child_name != "w:headerReference") {
                insertion_anchor = child;
                break;
            }
            continue;
        }
        if (child_name != "w:headerReference" &&
            child_name != "w:footerReference") {
            insertion_anchor = child;
            break;
        }
    }

    return insertion_anchor == pugi::xml_node{}
               ? featherdoc::detail::checked_append_xml_element(
                     section_properties, reference_name)
               : featherdoc::detail::checked_insert_xml_element_before(
                     section_properties, reference_name, insertion_anchor);
}

auto checked_enable_section_title_page(pugi::xml_node section_properties,
                                       bool &changed) -> bool {
    auto title_page = section_properties.child("w:titlePg");
    if (title_page == pugi::xml_node{}) {
        auto insertion_anchor = pugi::xml_node{};
        for (auto child = section_properties.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            const auto child_name = std::string_view{child.name()};
            if (child_name == "w:textDirection" || child_name == "w:bidi" ||
                child_name == "w:rtlGutter" || child_name == "w:docGrid" ||
                child_name == "w:printerSettings" ||
                child_name == "w:sectPrChange") {
                insertion_anchor = child;
                break;
            }
        }
        title_page = insertion_anchor == pugi::xml_node{}
                         ? featherdoc::detail::checked_append_xml_element(
                               section_properties, "w:titlePg")
                         : featherdoc::detail::checked_insert_xml_element_before(
                               section_properties, "w:titlePg",
                               insertion_anchor);
        if (title_page == pugi::xml_node{}) {
            return false;
        }
        changed = true;
    }
    return checked_set_section_attribute(title_page, "w:val", "1", changed);
}

auto checked_enable_even_and_odd_headers(pugi::xml_document &settings,
                                         bool &changed) -> bool {
    auto settings_root = settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        return false;
    }
    auto marker = settings_root.child("w:evenAndOddHeaders");
    if (marker == pugi::xml_node{}) {
        marker = featherdoc::detail::checked_append_xml_element(
            settings_root, "w:evenAndOddHeaders");
        if (marker == pugi::xml_node{}) {
            return false;
        }
        changed = true;
    }
    return checked_set_section_attribute(marker, "w:val", "1", changed);
}

auto checked_ensure_section_properties(pugi::xml_node body,
                                       std::size_t section_index,
                                       std::size_t section_count,
                                       bool &changed) -> pugi::xml_node {
    auto section_properties =
        featherdoc::detail::section_properties_at(body, section_index);
    if (section_properties != pugi::xml_node{}) {
        return section_properties;
    }
    if (section_index + 1U != section_count) {
        return {};
    }
    section_properties = featherdoc::detail::checked_append_xml_element(
        body, "w:sectPr");
    if (section_properties != pugi::xml_node{}) {
        changed = true;
    }
    return section_properties;
}

auto checked_prepare_relationships_root(pugi::xml_document &relationships,
                                        bool &changed) -> pugi::xml_node {
    auto root =
        featherdoc::detail::package_relationships_root(relationships);
    if (root != pugi::xml_node{}) {
        return root;
    }
    const auto initialization =
        featherdoc::detail::initialize_empty_relationships_document(
            relationships);
    if (!initialization) {
        return {};
    }
    changed = true;
    return featherdoc::detail::package_relationships_root(relationships);
}

auto checked_find_content_type_override(pugi::xml_node types,
                                        std::string_view part_name)
    -> pugi::xml_node {
    for (auto override_node =
             featherdoc::detail::first_package_content_type_override(types);
         override_node != pugi::xml_node{};
         override_node =
             featherdoc::detail::next_package_content_type_override(
                 override_node)) {
        if (featherdoc::detail::content_type_part_names_equivalent(
                override_node.attribute("PartName").value(), part_name)) {
            return override_node;
        }
    }
    return {};
}

} // namespace

Paragraph &Document::ensure_related_part_paragraphs(
    std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *part_root_name, const char *reference_name,
    const char *relationship_type, const char *content_type) {
    this->detached_paragraph.set_parent(detail::tracked_xml_node{});

    const auto count = this->section_count();
    const auto last_section_index = count == 0U ? 0U : count - 1U;
    return this->ensure_section_related_part_paragraphs(
        last_section_index,
        featherdoc::section_reference_kind::default_reference, parts,
        part_root_name, reference_name, relationship_type, content_type);
}

Paragraph &Document::ensure_section_related_part_paragraphs(
    std::size_t section_index,
    featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *part_root_name, const char *reference_name,
    const char *relationship_type, const char *content_type) try {
    this->detached_paragraph.set_parent(detail::tracked_xml_node{});
    this->ensure_xml_handle_lifetime();

    if (!relationships_document_allows_mutation(
            this->document_relationships, this->has_document_relationships_part,
            document_relationships_xml_entry, this->last_error_info)) {
        return this->detached_paragraph;
    }
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing section "
                       "header/footer parts");
        return this->detached_paragraph;
    }

    const auto count = this->section_count();
    if (section_index >= count) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for section editing",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    const auto xml_reference_type =
        featherdoc::to_xml_reference_type(reference_kind);
    const auto live_section_properties = this->section_properties(section_index);
    const auto live_reference =
        find_section_reference(live_section_properties, reference_name,
                               xml_reference_type);
    xml_part_state *existing_part = nullptr;
    if (live_reference != pugi::xml_node{}) {
        const auto relationship_id =
            std::string_view{live_reference.attribute("r:id").value()};
        for (const auto &candidate : parts) {
            if (!relationship_id.empty() &&
                related_part_has_relationship_id(*candidate,
                                                 relationship_id)) {
                existing_part = candidate.get();
                break;
            }
        }
    }

    const bool first_page =
        reference_kind == featherdoc::section_reference_kind::first_page;
    const bool even_page =
        reference_kind == featherdoc::section_reference_kind::even_page;
    if (existing_part != nullptr && !first_page && !even_page) {
        existing_part->paragraph.set_parent(
            this->tracked_node(existing_part->xml.child(part_root_name)));
        this->last_error_info.clear();
        return existing_part->paragraph;
    }

    const bool create_part = existing_part == nullptr;
    auto *settings_entry =
        even_page
            ? &this->extension_state().singleton_part_entries.settings
            : nullptr;
    auto content_types_load = lazy_xml_document_load_rollback{
        create_part || even_page, this->content_types,
        this->content_types_loaded};
    auto settings_load = lazy_xml_document_load_rollback{
        even_page, this->settings, this->settings_loaded,
        &this->has_settings_part, settings_entry};
    if ((create_part || even_page) && this->ensure_content_types_loaded()) {
        return this->detached_paragraph;
    }

    std::string settings_entry_name;
    if (even_page) {
        if (this->ignored_singleton_relationship_type(
                settings_relationship_type)) {
            set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                "cannot attach settings while its source relationship is "
                "invalid",
                std::string{document_relationships_xml_entry});
            return this->detached_paragraph;
        }
        if (this->ensure_settings_loaded()) {
            return this->detached_paragraph;
        }
        settings_entry_name = *settings_entry;
    }

    auto work = section_related_part_mutation_work{};
    if (featherdoc::detail::checked_clone_xml_document(this->document,
                                                       work.document) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }

    if (even_page) {
        const auto relationship_target =
            make_document_relationship_target(settings_entry_name);
        const auto override_part_name =
            make_override_part_name(settings_entry_name);
        const auto options =
            featherdoc::detail::singleton_part_attachment_options{
                settings_relationship_type,
                relationship_target,
                override_part_name,
                settings_content_type,
                document_relationships_xml_entry,
                content_types_xml_entry,
                document_errc::relationships_xml_parse_failed,
                document_errc::content_types_xml_parse_failed};
        auto attachment =
            featherdoc::detail::singleton_part_attachment_work{};
        if (featherdoc::detail::prepare_checked_singleton_part_attachment(
                this->document_relationships, this->content_types, options,
                this->last_error_info, attachment)) {
            return this->detached_paragraph;
        }
        work.relationships = std::move(attachment.relationships);
        work.content_types = std::move(attachment.content_types);
        work.relationships_changed = attachment.relationships_changed;
        work.content_types_changed = attachment.content_types_changed;

        if (featherdoc::detail::checked_clone_xml_document(this->settings,
                                                           work.settings) !=
            featherdoc::detail::xml_document_clone_status::success) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info, settings_entry_name);
            return this->detached_paragraph;
        }
        if (!checked_enable_even_and_odd_headers(work.settings,
                                                 work.settings_changed)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, settings_entry_name);
            return this->detached_paragraph;
        }
    } else if (create_part) {
        if (featherdoc::detail::checked_clone_xml_document(
                this->document_relationships, work.relationships) !=
            featherdoc::detail::xml_document_clone_status::success) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info, document_relationships_xml_entry);
            return this->detached_paragraph;
        }
        if (featherdoc::detail::checked_clone_xml_document(
                this->content_types, work.content_types) !=
            featherdoc::detail::xml_document_clone_status::success) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info, content_types_xml_entry);
            return this->detached_paragraph;
        }
    }

    auto document_root = work.document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::invalid_package_structure,
                       "document.xml does not contain the expected "
                       "w:document/w:body structure",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }
    auto section_properties = checked_ensure_section_properties(
        body, section_index, count, work.document_changed);
    if (section_properties == pugi::xml_node{}) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }
    if (first_page && !checked_enable_section_title_page(
                          section_properties, work.document_changed)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }

    const auto publish_document_update = [&]() -> bool {
        const auto result = publish_checked_section_document_update(
            this->document, work.document, section_index,
            work.document_changed);
        if (result == section_document_publish_result::failure) {
            return false;
        }
        if (result ==
            section_document_publish_result::requires_full_document_publish) {
            const auto published_body =
                this->document.child("w:document").child("w:body");
            this->xml_handle_lifetime->retire_subtree(published_body);
            static_assert(
                std::is_nothrow_move_assignable_v<pugi::xml_document>);
            this->document = std::move(work.document);
            this->rebind_internal_xml_handles();
        }
        return true;
    };

    if (existing_part != nullptr) {
        if (!publish_document_update()) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, document_xml_entry);
            return this->detached_paragraph;
        }
        if (work.relationships_changed) {
            this->document_relationships = std::move(work.relationships);
            this->document_relationships_dirty = true;
        }
        if (work.content_types_changed) {
            this->content_types = std::move(work.content_types);
            this->content_types_dirty = true;
        }
        if (work.settings_changed) {
            this->settings = std::move(work.settings);
            this->settings_dirty = true;
        }
        if (even_page) {
            if (!this->has_settings_part) {
                this->settings_dirty = true;
            }
            this->has_settings_part = true;
            this->has_document_relationships_part = true;
        }
        existing_part->paragraph.set_parent(
            this->tracked_node(existing_part->xml.child(part_root_name)));
        content_types_load.commit();
        settings_load.commit();
        this->last_error_info.clear();
        return existing_part->paragraph;
    }

    auto relationships = checked_prepare_relationships_root(
        work.relationships, work.relationships_changed);
    auto types =
        featherdoc::detail::package_content_types_root(work.content_types);
    if (relationships == pugi::xml_node{} || types == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::invalid_package_structure,
                       "header/footer package metadata does not contain the "
                       "expected Relationships/Types roots",
                       relationships == pugi::xml_node{}
                           ? std::string{document_relationships_xml_entry}
                           : std::string{content_types_xml_entry});
        return this->detached_paragraph;
    }

    if (!checked_set_section_attribute(
            document_root, "xmlns:r",
            office_document_relationships_namespace_uri,
            work.document_changed)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }

    std::unordered_set<std::string> used_relationship_ids;
    for (auto relationship =
             featherdoc::detail::first_package_relationship(relationships);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        const auto id = relationship.attribute("Id").value();
        if (*id != '\0') {
            used_relationship_ids.emplace(id);
        }
    }
    std::string relationship_id;
    for (std::size_t relationship_index = 1U; relationship_id.empty();
         ++relationship_index) {
        auto candidate = "rId" + std::to_string(relationship_index);
        if (!used_relationship_ids.contains(candidate)) {
            relationship_id = std::move(candidate);
        }
        if (relationship_index ==
                (std::numeric_limits<std::size_t>::max)() &&
            relationship_id.empty()) {
            set_last_error(this->last_error_info,
                           document_errc::identifier_space_exhausted,
                           "document relationship identifier space is "
                           "exhausted",
                           std::string{document_relationships_xml_entry});
            return this->detached_paragraph;
        }
    }

    const auto part_prefix =
        std::string_view{part_root_name} == "w:hdr" ? "header" : "footer";
    std::set<std::string> used_part_identities;
    const auto remember_used_entry = [&](std::string_view entry_name) {
        const auto identity =
            featherdoc::detail::package_part_name_identity(entry_name);
        if (identity.has_value()) {
            used_part_identities.insert(*identity);
        }
    };
    const auto remember_used_part = [&](const auto &part) {
        remember_used_entry(part->entry_name);
        remember_used_entry(part->relationships_entry_name);
    };
    for (const auto &part : this->header_parts) {
        remember_used_part(part);
    }
    for (const auto &part : this->footer_parts) {
        remember_used_part(part);
    }

    std::string entry_name;
    for (std::size_t part_index = 1U; entry_name.empty(); ++part_index) {
        auto candidate = std::string{"word/"} + part_prefix +
                         std::to_string(part_index) + ".xml";
        const auto candidate_relationships =
            make_part_relationships_entry(candidate);
        const auto candidate_identity =
            featherdoc::detail::package_part_name_identity(candidate);
        const auto candidate_relationships_identity =
            featherdoc::detail::package_part_name_identity(
                candidate_relationships);
        const auto identity_is_available =
            [&](const auto &identity, std::string_view candidate_name) {
                return identity.has_value() &&
                       !used_part_identities.contains(*identity) &&
                       !featherdoc::detail::
                           package_part_identity_has_derivation_conflict(
                               used_part_identities, *identity) &&
                       !this->source_package_part_name_conflicts(
                           candidate_name);
            };
        if (candidate_identity.has_value() &&
            identity_is_available(candidate_identity, candidate) &&
            identity_is_available(candidate_relationships_identity,
                                  candidate_relationships)) {
            entry_name = std::move(candidate);
        }
        if (part_index == (std::numeric_limits<std::size_t>::max)() &&
            entry_name.empty()) {
            set_last_error(this->last_error_info,
                           document_errc::identifier_space_exhausted,
                           "header/footer part identifier space is exhausted",
                           std::string{document_xml_entry});
            return this->detached_paragraph;
        }
    }

    auto part = std::make_unique<xml_part_state>();
    part->relationship_id = relationship_id;
    part->entry_name = entry_name;
    part->relationships_entry_name = make_part_relationships_entry(entry_name);
    const auto empty_part_xml = std::string_view{part_root_name} == "w:hdr"
                                    ? empty_header_xml
                                    : empty_footer_xml;
    const auto parse_result =
        part->xml.load_buffer(empty_part_xml.data(), empty_part_xml.size());
    if (!parse_result || part->xml.child(part_root_name) == pugi::xml_node{}) {
        (void)featherdoc::detail::set_xml_parse_failure(
            this->last_error_info, parse_result,
            document_errc::related_part_parse_failed, entry_name);
        return this->detached_paragraph;
    }

    auto relationship =
        featherdoc::detail::append_package_relationship(relationships);
    const auto relationship_target =
        make_document_relationship_target(entry_name);
    if (relationship == pugi::xml_node{} ||
        !featherdoc::detail::checked_set_xml_attribute_value(
            relationship, "Id", relationship_id) ||
        !featherdoc::detail::checked_set_xml_attribute_value(
            relationship, "Type", relationship_type) ||
        !featherdoc::detail::checked_set_xml_attribute_value(
            relationship, "Target", relationship_target)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_relationships_xml_entry);
        return this->detached_paragraph;
    }
    work.relationships_changed = true;

    const auto override_part_name = make_override_part_name(entry_name);
    auto override_node =
        checked_find_content_type_override(types, override_part_name);
    if (override_node == pugi::xml_node{}) {
        override_node =
            featherdoc::detail::append_package_content_type_override(types);
    }
    if (override_node == pugi::xml_node{} ||
        !featherdoc::detail::checked_set_xml_attribute_value(
            override_node, "PartName", override_part_name) ||
        !featherdoc::detail::checked_set_xml_attribute_value(
            override_node, "ContentType", content_type)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, content_types_xml_entry);
        return this->detached_paragraph;
    }
    work.content_types_changed = true;

    auto reference = find_section_reference(section_properties, reference_name,
                                            xml_reference_type);
    if (reference == pugi::xml_node{}) {
        reference =
            checked_append_section_reference(section_properties, reference_name);
        work.document_changed = true;
    }
    if (reference == pugi::xml_node{} ||
        !checked_set_section_attribute(reference, "w:type",
                                       xml_reference_type,
                                       work.document_changed) ||
        !checked_set_section_attribute(reference, "r:id", relationship_id,
                                       work.document_changed)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }

    parts.reserve(parts.size() + 1U);
    part->paragraph.set_parent(
        this->tracked_node(part->xml.child(part_root_name)));
    auto *created_part = part.get();

    if (!publish_document_update()) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }
    this->document_relationships = std::move(work.relationships);
    this->content_types = std::move(work.content_types);
    if (even_page && work.settings_changed) {
        this->settings = std::move(work.settings);
    }
    parts.push_back(std::move(part));

    this->has_document_relationships_part = true;
    this->document_relationships_dirty = true;
    this->content_types_dirty = true;
    if (even_page) {
        if (!this->has_settings_part || work.settings_changed) {
            this->settings_dirty = true;
        }
        this->has_settings_part = true;
    }
    created_part->paragraph.set_parent(
        this->tracked_node(created_part->xml.child(part_root_name)));
    content_types_load.commit();
    settings_load.commit();
    this->last_error_info.clear();
    return created_part->paragraph;
} catch (...) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return this->detached_paragraph;
}

Paragraph &Document::assign_section_related_part_paragraphs(
    std::size_t section_index, std::size_t part_index,
    featherdoc::section_reference_kind reference_kind,
    std::vector<std::unique_ptr<xml_part_state>> &parts,
    const char *part_root_name, const char *reference_name) try {
    this->detached_paragraph.set_parent(detail::tracked_xml_node{});
    this->ensure_xml_handle_lifetime();

    if (!relationships_document_allows_mutation(
            this->document_relationships, this->has_document_relationships_part,
            document_relationships_xml_entry, this->last_error_info)) {
        return this->detached_paragraph;
    }
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before assigning section "
                       "header/footer parts");
        return this->detached_paragraph;
    }
    if (part_index >= parts.size()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "header/footer part index is out of range for section assignment",
            std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    auto &part = *parts[part_index];
    if (part.relationship_id.empty()) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::invalid_argument),
            "target header/footer part does not have a relationship id",
            part.entry_name);
        return this->detached_paragraph;
    }

    const auto count = this->section_count();
    if (section_index >= count) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for section assignment",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }

    const bool first_page =
        reference_kind == featherdoc::section_reference_kind::first_page;
    const bool even_page =
        reference_kind == featherdoc::section_reference_kind::even_page;
    auto *settings_entry =
        even_page
            ? &this->extension_state().singleton_part_entries.settings
            : nullptr;
    auto content_types_load = lazy_xml_document_load_rollback{
        even_page, this->content_types, this->content_types_loaded};
    auto settings_load = lazy_xml_document_load_rollback{
        even_page, this->settings, this->settings_loaded,
        &this->has_settings_part, settings_entry};
    std::string settings_entry_name;
    if (even_page) {
        if (this->ignored_singleton_relationship_type(
                settings_relationship_type)) {
            set_last_error(
                this->last_error_info, document_errc::invalid_package_structure,
                "cannot attach settings while its source relationship is "
                "invalid",
                std::string{document_relationships_xml_entry});
            return this->detached_paragraph;
        }
        if (this->ensure_content_types_loaded() ||
            this->ensure_settings_loaded()) {
            return this->detached_paragraph;
        }
        settings_entry_name = *settings_entry;
    }

    auto work = section_related_part_mutation_work{};
    if (featherdoc::detail::checked_clone_xml_document(this->document,
                                                       work.document) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }

    if (even_page) {
        const auto relationship_target =
            make_document_relationship_target(settings_entry_name);
        const auto override_part_name =
            make_override_part_name(settings_entry_name);
        const auto options =
            featherdoc::detail::singleton_part_attachment_options{
                settings_relationship_type,
                relationship_target,
                override_part_name,
                settings_content_type,
                document_relationships_xml_entry,
                content_types_xml_entry,
                document_errc::relationships_xml_parse_failed,
                document_errc::content_types_xml_parse_failed};
        auto attachment =
            featherdoc::detail::singleton_part_attachment_work{};
        if (featherdoc::detail::prepare_checked_singleton_part_attachment(
                this->document_relationships, this->content_types, options,
                this->last_error_info, attachment)) {
            return this->detached_paragraph;
        }
        work.relationships = std::move(attachment.relationships);
        work.content_types = std::move(attachment.content_types);
        work.relationships_changed = attachment.relationships_changed;
        work.content_types_changed = attachment.content_types_changed;

        if (featherdoc::detail::checked_clone_xml_document(this->settings,
                                                           work.settings) !=
            featherdoc::detail::xml_document_clone_status::success) {
            (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
                this->last_error_info, settings_entry_name);
            return this->detached_paragraph;
        }
        if (!checked_enable_even_and_odd_headers(work.settings,
                                                 work.settings_changed)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, settings_entry_name);
            return this->detached_paragraph;
        }
    }

    auto document_root = work.document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       document_errc::invalid_package_structure,
                       "document.xml does not contain the expected "
                       "w:document/w:body structure",
                       std::string{document_xml_entry});
        return this->detached_paragraph;
    }
    auto section_properties = checked_ensure_section_properties(
        body, section_index, count, work.document_changed);
    if (section_properties == pugi::xml_node{}) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }
    if (first_page && !checked_enable_section_title_page(
                          section_properties, work.document_changed)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }
    if (!checked_set_section_attribute(
            document_root, "xmlns:r",
            office_document_relationships_namespace_uri,
            work.document_changed)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }

    const auto xml_reference_type =
        featherdoc::to_xml_reference_type(reference_kind);
    auto reference = find_section_reference(section_properties, reference_name,
                                            xml_reference_type);
    if (reference == pugi::xml_node{}) {
        reference =
            checked_append_section_reference(section_properties, reference_name);
        work.document_changed = true;
    }
    if (reference == pugi::xml_node{} ||
        !checked_set_section_attribute(reference, "w:type",
                                       xml_reference_type,
                                       work.document_changed) ||
        !checked_set_section_attribute(reference, "r:id",
                                       part.relationship_id,
                                       work.document_changed)) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }

    const auto document_publish_result =
        publish_checked_section_document_update(
            this->document, work.document, section_index,
            work.document_changed);
    if (document_publish_result == section_document_publish_result::failure) {
        (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
            this->last_error_info, document_xml_entry);
        return this->detached_paragraph;
    }
    if (document_publish_result ==
        section_document_publish_result::requires_full_document_publish) {
        const auto published_body =
            this->document.child("w:document").child("w:body");
        this->xml_handle_lifetime->retire_subtree(published_body);
        static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
        this->document = std::move(work.document);
        this->rebind_internal_xml_handles();
    }
    if (work.relationships_changed) {
        this->document_relationships = std::move(work.relationships);
        this->document_relationships_dirty = true;
    }
    if (work.content_types_changed) {
        this->content_types = std::move(work.content_types);
        this->content_types_dirty = true;
    }
    if (work.settings_changed) {
        this->settings = std::move(work.settings);
        this->settings_dirty = true;
    }
    if (even_page) {
        if (!this->has_settings_part) {
            this->settings_dirty = true;
        }
        this->has_settings_part = true;
        this->has_document_relationships_part = true;
    }
    part.paragraph.set_parent(
        this->tracked_node(part.xml.child(part_root_name)));
    content_types_load.commit();
    settings_load.commit();
    this->last_error_info.clear();
    return part.paragraph;
} catch (...) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return this->detached_paragraph;
}
