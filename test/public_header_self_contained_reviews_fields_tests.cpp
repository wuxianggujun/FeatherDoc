#include <featherdoc/reviews_fields.hpp>

void featherdoc_public_header_self_contained_reviews_fields() {
    featherdoc::hyperlink_summary hyperlink{};
    featherdoc::field_summary field{};
    featherdoc::field_state_options field_state{};
    featherdoc::table_of_contents_field_options toc{};
    featherdoc::reference_field_options reference{};
    featherdoc::review_note_summary review_note{};
    featherdoc::revision_summary revision{};
    featherdoc::revision_text_range_options revision_range{};
    featherdoc::text_range_preview preview{};

    const auto omml_text = featherdoc::make_omml_text("x");

    (void)hyperlink;
    (void)field;
    (void)field_state;
    (void)toc;
    (void)reference;
    (void)review_note;
    (void)revision;
    (void)revision_range;
    (void)preview;
    (void)omml_text;
}
