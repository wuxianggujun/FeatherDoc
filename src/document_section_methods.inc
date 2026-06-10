#include "document_section_lifecycle_methods.inc"

#include "document_section_page_setup_methods.inc"

#include "document_section_related_part_access_methods.inc"

std::error_code Document::ensure_even_and_odd_headers_enabled() {
    if (const auto error = this->ensure_settings_part_attached()) {
        return error;
    }

    auto settings_root = this->settings.child("w:settings");
    if (settings_root == pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              document_errc::settings_xml_parse_failed,
                              "word/settings.xml does not contain the expected w:settings root",
                              std::string{settings_xml_entry});
    }

    if (ensure_on_off_node_enabled(settings_root, "w:evenAndOddHeaders") ==
        pugi::xml_node{}) {
        return set_last_error(this->last_error_info,
                              std::make_error_code(std::errc::not_enough_memory),
                              "failed to enable w:evenAndOddHeaders in word/settings.xml",
                              std::string{settings_xml_entry});
    }

    this->settings_dirty = true;
    return {};
}

bool Document::ensure_title_page_enabled(pugi::xml_node section_properties) {
    if (section_properties == pugi::xml_node{}) {
        set_last_error(this->last_error_info, std::make_error_code(std::errc::invalid_argument),
                       "cannot enable first-page header/footer without a w:sectPr node",
                       std::string{document_xml_entry});
        return false;
    }

    if (ensure_section_title_page_node(section_properties) == pugi::xml_node{}) {
        set_last_error(this->last_error_info,
                       std::make_error_code(std::errc::not_enough_memory),
                       "failed to enable w:titlePg for the target section",
                       std::string{document_xml_entry});
        return false;
    }

    return true;
}

#include "document_section_related_part_paragraph_methods.inc"

#include "document_section_related_part_methods.inc"

#include "document_section_text_replacement_methods.inc"
