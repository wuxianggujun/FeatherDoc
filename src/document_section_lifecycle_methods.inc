namespace {

auto checked_insert_section_paragraph_after(pugi::xml_node body,
                                            pugi::xml_node anchor)
    -> pugi::xml_node {
    if (body == pugi::xml_node{} || anchor == pugi::xml_node{} ||
        anchor.parent() != body) {
        return {};
    }
    auto paragraph = body.insert_child_after(pugi::node_element, anchor);
    if (paragraph == pugi::xml_node{}) {
        return {};
    }
    if (!paragraph.set_name("w:p")) {
        body.remove_child(paragraph);
        return {};
    }
    return paragraph;
}

auto lifecycle_title_page_insertion_anchor(pugi::xml_node section_properties)
    -> pugi::xml_node {
    for (auto child = section_properties.first_child();
         child != pugi::xml_node{}; child = child.next_sibling()) {
        const auto child_name = std::string_view{child.name()};
        if (child_name == "w:textDirection" || child_name == "w:bidi" ||
            child_name == "w:rtlGutter" || child_name == "w:docGrid" ||
            child_name == "w:printerSettings" ||
            child_name == "w:sectPrChange") {
            return child;
        }
    }
    return {};
}

auto checked_enable_lifecycle_title_page(pugi::xml_node section_properties)
    -> bool {
    if (section_properties == pugi::xml_node{}) {
        return false;
    }

    auto title_page = section_properties.child("w:titlePg");
    if (title_page == pugi::xml_node{}) {
        const auto insertion_anchor =
            lifecycle_title_page_insertion_anchor(section_properties);

        title_page =
            insertion_anchor == pugi::xml_node{}
                ? featherdoc::detail::checked_append_xml_element(
                      section_properties, "w:titlePg")
                : featherdoc::detail::checked_insert_xml_element_before(
                      section_properties, "w:titlePg", insertion_anchor);
    }

    return title_page != pugi::xml_node{} &&
           featherdoc::detail::checked_set_xml_attribute_value(
               title_page, "w:val", "1");
}

} // namespace

bool Document::append_section(bool inherit_header_footer) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before appending a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    auto document_work = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(
            this->document, document_work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }
    this->ensure_xml_handle_lifetime();

    auto work_document_root = document_work.child("w:document");
    auto work_body = work_document_root.child("w:body");
    if (work_document_root == pugi::xml_node{} ||
        work_body == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cloned document.xml does not contain the expected "
            "w:document/w:body structure",
            std::string{document_xml_entry});
        return false;
    }

    auto body_section_properties = work_body.child("w:sectPr");
    if (body_section_properties == pugi::xml_node{}) {
        body_section_properties = featherdoc::detail::checked_append_xml_element(
            work_body, "w:sectPr");
    }
    if (body_section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create body-level w:sectPr while appending a section",
                       std::string{document_xml_entry});
        return false;
    }

    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const bool body_has_first_page_header = section_has_reference_type(
        body_section_properties, "w:headerReference", first_page_reference_type);
    const bool body_has_first_page_footer = section_has_reference_type(
        body_section_properties, "w:footerReference", first_page_reference_type);
    if ((body_has_first_page_header || body_has_first_page_footer) &&
        !checked_enable_lifecycle_title_page(body_section_properties)) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "failed to enable w:titlePg in the prepared appended section",
            std::string{document_xml_entry});
        return false;
    }

    auto section_break_paragraph =
        featherdoc::detail::checked_insert_xml_element_before(
            work_body, "w:p", body_section_properties);
    if (section_break_paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert a paragraph for the new section break",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_properties = section_break_paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        paragraph_properties = featherdoc::detail::checked_append_xml_element(
            section_break_paragraph, "w:pPr");
    }
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the new section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    if (featherdoc::detail::checked_append_copy_xml_node(
            body_section_properties, paragraph_properties) !=
        featherdoc::detail::xml_document_clone_status::success) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to copy body-level w:sectPr onto the new section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    if (!inherit_header_footer) {
        clear_section_header_footer_references(body_section_properties);
    }

    // Missing references intentionally mean "linked to previous" in OOXML.
    // Keep w:titlePg and the package-level even/odd setting untouched so that
    // a detached local reference layout retains its effective inheritance.
    try {
        static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
        this->xml_handle_lifetime->retire_subtree(body);
        this->document = std::move(document_work);

        this->rebind_internal_xml_handles();
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "section append ran out of memory before it could be published",
            document_xml_entry);
        return false;
    }

    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

bool Document::insert_section(std::size_t section_index,
                              bool inherit_header_footer) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inserting a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for insertion",
                       std::string{document_xml_entry});
        return false;
    }

    if (section_index + 1U == count) {
        return this->append_section(inherit_header_footer);
    }

    auto document_work = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(
            this->document, document_work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }
    this->ensure_xml_handle_lifetime();

    auto work_document_root = document_work.child("w:document");
    auto work_body = work_document_root.child("w:body");
    if (work_document_root == pugi::xml_node{} ||
        work_body == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cloned document.xml does not contain the expected "
            "w:document/w:body structure",
            std::string{document_xml_entry});
        return false;
    }

    auto source_section_properties =
        featherdoc::detail::section_properties_at(work_body, section_index);
    if (source_section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "failed to resolve the source section break for insertion",
                       std::string{document_xml_entry});
        return false;
    }

    const auto source_break_paragraph = section_break_paragraph_for(source_section_properties);
    if (source_break_paragraph == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, std::make_error_code(std::errc::invalid_argument),
            "the target section does not end with a paragraph-level w:sectPr node",
            std::string{document_xml_entry});
        return false;
    }

    const auto first_page_reference_type =
        featherdoc::to_xml_reference_type(featherdoc::section_reference_kind::first_page);
    const bool source_has_first_page_header = section_has_reference_type(
        source_section_properties, "w:headerReference", first_page_reference_type);
    const bool source_has_first_page_footer = section_has_reference_type(
        source_section_properties, "w:footerReference", first_page_reference_type);
    if ((source_has_first_page_header || source_has_first_page_footer) &&
        !checked_enable_lifecycle_title_page(source_section_properties)) {
        set_last_error(
            this->last_error_info,
            std::make_error_code(std::errc::not_enough_memory),
            "failed to enable w:titlePg in the prepared inserted section",
            std::string{document_xml_entry});
        return false;
    }

    auto section_break_paragraph = checked_insert_section_paragraph_after(
        work_body, source_break_paragraph);
    if (section_break_paragraph == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to insert a paragraph for the new section break",
                       std::string{document_xml_entry});
        return false;
    }

    auto paragraph_properties = section_break_paragraph.child("w:pPr");
    if (paragraph_properties == pugi::xml_node{}) {
        paragraph_properties = featherdoc::detail::checked_append_xml_element(
            section_break_paragraph, "w:pPr");
    }
    if (paragraph_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pPr for the inserted section break paragraph",
                       std::string{document_xml_entry});
        return false;
    }

    if (featherdoc::detail::checked_append_copy_xml_node(
            source_section_properties, paragraph_properties) !=
        featherdoc::detail::xml_document_clone_status::success) {
        set_last_error(
            this->last_error_info, std::make_error_code(std::errc::not_enough_memory),
            "failed to copy w:sectPr onto the inserted section break paragraph",
            std::string{document_xml_entry});
        return false;
    }
    auto inserted_section_properties = paragraph_properties.child("w:sectPr");
    if (inserted_section_properties == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "prepared section break does not contain the copied w:sectPr",
            std::string{document_xml_entry});
        return false;
    }

    if (!inherit_header_footer) {
        clear_section_header_footer_references(inserted_section_properties);
    }

    // Missing references intentionally remain linked to the previous section.
    // Preserve the copied section markers and unrelated settings metadata.
    try {
        static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
        this->xml_handle_lifetime->retire_subtree(body);
        this->document = std::move(document_work);

        this->rebind_internal_xml_handles();
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "section insertion ran out of memory before it could be published",
            document_xml_entry);
        return false;
    }

    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

bool Document::remove_section(std::size_t section_index) try {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before removing a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (count <= 1U) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "cannot remove the only section in the document",
                       std::string{document_xml_entry});
        return false;
    }

    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for removal",
                       std::string{document_xml_entry});
        return false;
    }

    auto document_work = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(
            this->document, document_work) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }
    this->ensure_xml_handle_lifetime();

    auto work_document_root = document_work.child("w:document");
    auto work_body = work_document_root.child("w:body");
    if (work_document_root == pugi::xml_node{} ||
        work_body == pugi::xml_node{}) {
        set_last_error(
            this->last_error_info, document_errc::invalid_package_structure,
            "cloned document.xml does not contain the expected "
            "w:document/w:body structure",
            std::string{document_xml_entry});
        return false;
    }

    if (section_index + 1U < count) {
        auto section_properties =
            featherdoc::detail::section_properties_at(work_body,
                                                      section_index);
        if (section_properties == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the section break for removal",
                           std::string{document_xml_entry});
            return false;
        }

        auto paragraph_properties = section_properties.parent();
        auto section_break_paragraph_node = section_break_paragraph_for(section_properties);
        if (paragraph_properties == pugi::xml_node{} ||
            section_break_paragraph_node == pugi::xml_node{} ||
            !paragraph_properties.remove_child(section_properties)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the section break from the target paragraph",
                           std::string{document_xml_entry});
            return false;
        }

        remove_empty_paragraph(section_break_paragraph_node);
    } else {
        auto previous_section_properties =
            featherdoc::detail::section_properties_at(work_body,
                                                      section_index - 1U);
        auto body_section_properties = work_body.child("w:sectPr");
        if (previous_section_properties == pugi::xml_node{} ||
            body_section_properties == pugi::xml_node{}) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "failed to resolve the final section break for removal",
                           std::string{document_xml_entry});
            return false;
        }

        for (auto attribute = body_section_properties.first_attribute();
             attribute != pugi::xml_attribute{};) {
            const auto next = attribute.next_attribute();
            body_section_properties.remove_attribute(attribute);
            attribute = next;
        }
        for (auto child = body_section_properties.first_child();
             child != pugi::xml_node{};) {
            const auto next = child.next_sibling();
            body_section_properties.remove_child(child);
            child = next;
        }
        for (auto attribute = previous_section_properties.first_attribute();
             attribute != pugi::xml_attribute{};
             attribute = attribute.next_attribute()) {
            if (!featherdoc::detail::checked_append_xml_attribute(
                    body_section_properties, attribute.name(),
                    attribute.value())) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to copy the previous section attributes onto "
                    "body-level w:sectPr",
                    std::string{document_xml_entry});
                return false;
            }
        }
        for (auto child = previous_section_properties.first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            if (featherdoc::detail::checked_append_copy_xml_node(
                    child, body_section_properties) !=
                featherdoc::detail::xml_document_clone_status::success) {
                set_last_error(
                    this->last_error_info,
                    std::make_error_code(std::errc::not_enough_memory),
                    "failed to copy the previous section properties onto "
                    "body-level w:sectPr",
                    std::string{document_xml_entry});
                return false;
            }
        }

        auto previous_paragraph_properties = previous_section_properties.parent();
        auto previous_break_paragraph =
            section_break_paragraph_for(previous_section_properties);
        if (previous_paragraph_properties == pugi::xml_node{} ||
            previous_break_paragraph == pugi::xml_node{} ||
            !previous_paragraph_properties.remove_child(previous_section_properties)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to remove the previous section break while collapsing the final section",
                           std::string{document_xml_entry});
            return false;
        }

        remove_empty_paragraph(previous_break_paragraph);
    }

    // Removing a boundary must not normalize unrelated section or package
    // metadata. In particular, w:titlePg may activate an inherited first-page
    // reference and w:evenAndOddHeaders is caller-owned global state.
    try {
        static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
        this->xml_handle_lifetime->retire_subtree(body);
        this->document = std::move(document_work);

        this->rebind_internal_xml_handles();
    } catch (const std::bad_alloc &) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "section removal ran out of memory before it could be published",
            document_xml_entry);
        return false;
    }

    this->last_error_info.clear();
    return true;
} catch (const std::bad_alloc &) {
    (void)featherdoc::detail::set_xml_document_mutation_allocation_failure(
        this->last_error_info, document_xml_entry);
    return false;
}

bool Document::move_section(std::size_t source_section_index,
                            std::size_t target_section_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before moving a section");
        return false;
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return false;
    }

    const auto count = this->section_count();
    if (source_section_index >= count || target_section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for move",
                       std::string{document_xml_entry});
        return false;
    }

    if (source_section_index == target_section_index) {
        this->last_error_info.clear();
        return true;
    }

    auto reordered_document = pugi::xml_document{};
    if (featherdoc::detail::checked_clone_xml_document(
            this->document, reordered_document) !=
        featherdoc::detail::xml_document_clone_status::success) {
        (void)featherdoc::detail::set_xml_document_clone_allocation_failure(
            this->last_error_info, document_xml_entry);
        return false;
    }

    try {
        auto reordered_body =
            reordered_document.child("w:document").child("w:body");
        if (reordered_body == pugi::xml_node{}) {
            set_last_error(
                this->last_error_info,
                document_errc::invalid_package_structure,
                "cloned document.xml does not contain the expected "
                "w:document/w:body structure",
                std::string{document_xml_entry});
            return false;
        }

        auto snapshots = collect_section_snapshots(reordered_body);
        if (!snapshots.has_value() || snapshots->size() != count) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, document_xml_entry);
            return false;
        }

        auto &reordered_snapshots = *snapshots;
        if (source_section_index < target_section_index) {
            std::rotate(
                reordered_snapshots.begin() +
                    static_cast<std::ptrdiff_t>(source_section_index),
                reordered_snapshots.begin() +
                    static_cast<std::ptrdiff_t>(source_section_index + 1U),
                reordered_snapshots.begin() +
                    static_cast<std::ptrdiff_t>(target_section_index + 1U));
        } else {
            std::rotate(
                reordered_snapshots.begin() +
                    static_cast<std::ptrdiff_t>(target_section_index),
                reordered_snapshots.begin() +
                    static_cast<std::ptrdiff_t>(source_section_index),
                reordered_snapshots.begin() +
                    static_cast<std::ptrdiff_t>(source_section_index + 1U));
        }

        if (!rebuild_body_from_section_snapshots(reordered_body,
                                                 reordered_snapshots)) {
            (void)featherdoc::detail::
                set_xml_document_mutation_allocation_failure(
                    this->last_error_info, document_xml_entry);
            return false;
        }

        // Every allocation and XML mutation above targets isolated work. Once
        // the old body is retired, publishing the completed document must not
        // introduce another failure point.
        static_assert(std::is_nothrow_move_assignable_v<pugi::xml_document>);
        this->xml_handle_lifetime->retire_subtree(body);
        this->document = std::move(reordered_document);

        const auto published_body =
            this->document.child("w:document").child("w:body");
        this->paragraph.set_parent(this->tracked_node(published_body));
        this->table.set_owner(this);
        this->table.set_parent(this->tracked_node(published_body));
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "section reordering ran out of memory before it could be "
            "published",
            document_xml_entry);
        return false;
    }

    this->last_error_info.clear();
    return true;
}
