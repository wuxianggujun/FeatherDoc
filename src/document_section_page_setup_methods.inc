std::optional<section_page_setup> Document::get_section_page_setup(
    std::size_t section_index) const {
    this->last_error_info.clear();

    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before inspecting section page setup");
        return std::nullopt;
    }

    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    if (section_index >= this->section_count()) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for page setup inspection",
                       std::string{document_xml_entry});
        return std::nullopt;
    }

    const auto section_properties = this->section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    const auto page_size = section_properties.child("w:pgSz");
    const auto page_margins = section_properties.child("w:pgMar");
    if (page_size == pugi::xml_node{} || page_margins == pugi::xml_node{}) {
        this->last_error_info.clear();
        return std::nullopt;
    }

    auto require_uint32_attribute =
        [this](pugi::xml_node node, const char *attribute_name, std::uint32_t &value)
        -> bool {
        const auto status = parse_xml_uint32_attribute(node, attribute_name, value);
        if (status == xml_uint_attribute_status::ok) {
            return true;
        }

        std::string detail = "invalid section page setup attribute ";
        detail += node.name();
        detail += '/';
        detail += attribute_name;
        if (status == xml_uint_attribute_status::missing) {
            detail += " (missing)";
        } else {
            detail += " (expected unsigned integer)";
        }

        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::move(detail), std::string{document_xml_entry});
        return false;
    };

    featherdoc::section_page_setup result;
    if (!require_uint32_attribute(page_size, "w:w", result.width_twips) ||
        !require_uint32_attribute(page_size, "w:h", result.height_twips) ||
        !require_uint32_attribute(page_margins, "w:top", result.margins.top_twips) ||
        !require_uint32_attribute(page_margins, "w:bottom",
                                  result.margins.bottom_twips) ||
        !require_uint32_attribute(page_margins, "w:left", result.margins.left_twips) ||
        !require_uint32_attribute(page_margins, "w:right",
                                  result.margins.right_twips) ||
        !require_uint32_attribute(page_margins, "w:header",
                                  result.margins.header_twips) ||
        !require_uint32_attribute(page_margins, "w:footer",
                                  result.margins.footer_twips)) {
        return std::nullopt;
    }

    const auto orientation =
        std::string_view{page_size.attribute("w:orient").value()};
    if (orientation.empty() || orientation == "portrait") {
        result.orientation = featherdoc::page_orientation::portrait;
    } else if (orientation == "landscape") {
        result.orientation = featherdoc::page_orientation::landscape;
    } else {
        std::string detail = "unsupported section page orientation: ";
        detail += orientation;
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       std::move(detail), std::string{document_xml_entry});
        return std::nullopt;
    }

    const auto page_numbering = section_properties.child("w:pgNumType");
    if (page_numbering != pugi::xml_node{}) {
        std::uint32_t page_number_start = 0U;
        switch (parse_xml_uint32_attribute(page_numbering, "w:start",
                                           page_number_start)) {
        case xml_uint_attribute_status::ok:
            result.page_number_start = page_number_start;
            break;
        case xml_uint_attribute_status::missing:
            break;
        case xml_uint_attribute_status::invalid:
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::invalid_argument),
                           "invalid section page setup attribute w:pgNumType/w:start "
                           "(expected unsigned integer)",
                           std::string{document_xml_entry});
            return std::nullopt;
        }
    }

    this->last_error_info.clear();
    return result;
}

bool Document::set_section_page_setup(std::size_t section_index,
                                      const section_page_setup &setup) {
    this->last_error_info.clear();

    if (setup.width_twips == 0U || setup.height_twips == 0U) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::invalid_argument),
                       "section page width and height must be greater than zero",
                       std::string{document_xml_entry});
        return false;
    }

    auto section_properties = this->ensure_section_properties(section_index);
    if (section_properties == pugi::xml_node{}) {
        return false;
    }

    auto page_size = ensure_section_property_node(section_properties, "w:pgSz");
    if (page_size == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pgSz for section page setup",
                       std::string{document_xml_entry});
        return false;
    }

    auto page_margins = ensure_section_property_node(section_properties, "w:pgMar");
    if (page_margins == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:pgMar for section page setup",
                       std::string{document_xml_entry});
        return false;
    }

    if (!ensure_xml_uint32_attribute(page_size, "w:w", setup.width_twips) ||
        !ensure_xml_uint32_attribute(page_size, "w:h", setup.height_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:top", setup.margins.top_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:bottom",
                                     setup.margins.bottom_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:left",
                                     setup.margins.left_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:right",
                                     setup.margins.right_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:header",
                                     setup.margins.header_twips) ||
        !ensure_xml_uint32_attribute(page_margins, "w:footer",
                                     setup.margins.footer_twips)) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to write section page setup attributes",
                       std::string{document_xml_entry});
        return false;
    }

    if (setup.orientation == featherdoc::page_orientation::landscape) {
        auto orientation_attribute = page_size.attribute("w:orient");
        if (orientation_attribute == pugi::xml_attribute{}) {
            orientation_attribute = page_size.append_attribute("w:orient");
        }
        if (orientation_attribute == pugi::xml_attribute{} ||
            !orientation_attribute.set_value("landscape")) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to write w:pgSz/w:orient for section page setup",
                           std::string{document_xml_entry});
            return false;
        }
    } else {
        page_size.remove_attribute("w:orient");
    }

    auto page_numbering = section_properties.child("w:pgNumType");
    if (setup.page_number_start.has_value()) {
        if (page_numbering == pugi::xml_node{}) {
            page_numbering = ensure_section_property_node(section_properties, "w:pgNumType");
        }
        if (page_numbering == pugi::xml_node{} ||
            !ensure_xml_uint32_attribute(page_numbering, "w:start",
                                         *setup.page_number_start)) {
            set_last_error(this->last_error_info,
                           std::make_error_code(std::errc::not_enough_memory),
                           "failed to write w:pgNumType/w:start for section page setup",
                           std::string{document_xml_entry});
            return false;
        }
    } else if (page_numbering != pugi::xml_node{}) {
        page_numbering.remove_attribute("w:start");
        remove_empty_node(page_numbering);
    }

    this->last_error_info.clear();
    return true;
}

pugi::xml_node Document::section_properties(std::size_t section_index) const {
    const auto body = this->document.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        return {};
    }

    std::size_t current_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        const auto paragraph_section_properties = child.child("w:pPr").child("w:sectPr");
        if (paragraph_section_properties == pugi::xml_node{}) {
            continue;
        }

        if (current_index == section_index) {
            return paragraph_section_properties;
        }
        ++current_index;
    }

    if (current_index == section_index) {
        return body.child("w:sectPr");
    }

    return {};
}

pugi::xml_node Document::ensure_section_properties(std::size_t section_index) {
    if (!this->is_open()) {
        set_last_error(this->last_error_info, document_errc::document_not_open,
                       "call open() or create_empty() before editing section properties");
        return {};
    }

    auto document_root = this->document.child("w:document");
    auto body = document_root.child("w:body");
    if (document_root == pugi::xml_node{} || body == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "document.xml does not contain the expected w:document/w:body structure",
                       std::string{document_xml_entry});
        return {};
    }

    const auto count = this->section_count();
    if (section_index >= count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "section index is out of range for section editing",
                       std::string{document_xml_entry});
        return {};
    }

    if (const auto existing = this->section_properties(section_index);
        existing != pugi::xml_node{}) {
        return existing;
    }

    if (section_index + 1U != count) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "the target section does not expose an editable w:sectPr node",
                       std::string{document_xml_entry});
        return {};
    }

    auto section_properties = body.child("w:sectPr");
    if (section_properties != pugi::xml_node{}) {
        return section_properties;
    }

    section_properties = body.append_child("w:sectPr");
    if (section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to create w:sectPr for section editing",
                       std::string{document_xml_entry});
        return {};
    }

    return section_properties;
}
