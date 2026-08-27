std::optional<std::int64_t>
Document::reserve_review_revision_identifiers(std::size_t count) {
    try {
        review_identifier_allocator identifier_allocator{1};
        collect_revision_ids(this->document, identifier_allocator);
        for (const auto &part : this->header_parts) {
            if (part != nullptr) {
                collect_revision_ids(part->xml, identifier_allocator);
            }
        }
        for (const auto &part : this->footer_parts) {
            if (part != nullptr) {
                collect_revision_ids(part->xml, identifier_allocator);
            }
        }

        if (!this->ensure_review_notes_part(
                featherdoc::review_note_kind::footnote, false) ||
            !this->ensure_review_notes_part(
                featherdoc::review_note_kind::endnote, false) ||
            !this->ensure_comments_part(false)) {
            if (this->last_error_info.code ==
                std::make_error_code(std::errc::not_enough_memory)) {
                (void)featherdoc::detail::set_xml_document_allocation_failure(
                    this->last_error_info,
                    "review revision identifier reservation ran out of memory",
                    document_xml_entry);
            }
            return std::nullopt;
        }
        collect_revision_ids(this->footnotes, identifier_allocator);
        collect_revision_ids(this->endnotes, identifier_allocator);
        collect_revision_ids(this->comments, identifier_allocator);

        const auto first_identifier = identifier_allocator.reserve(count);
        if (!first_identifier.has_value()) {
            set_last_error(this->last_error_info,
                           document_errc::identifier_space_exhausted,
                           "review revision identifier space is exhausted",
                           std::string{document_xml_entry});
            return std::nullopt;
        }
        return first_identifier;
    } catch (...) {
        (void)featherdoc::detail::set_xml_document_allocation_failure(
            this->last_error_info,
            "review revision identifier reservation ran out of memory",
            document_xml_entry);
        return std::nullopt;
    }
}

#include "document_template_review_revision_append_methods.inc"

#include "document_template_review_revision_run_methods.inc"

#include "document_template_review_revision_paragraph_insert_methods.inc"

#include "document_template_review_revision_paragraph_delete_methods.inc"

#include "document_template_review_revision_paragraph_replace_methods.inc"

#include "document_template_review_revision_text_range_methods.inc"
