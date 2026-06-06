#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc {

struct hyperlink_summary {
    std::size_t index{};
    std::string text;
    std::optional<std::string> relationship_id;
    std::optional<std::string> target;
    std::optional<std::string> anchor;
    bool external{false};
};

enum class field_kind : std::uint8_t {
    page = 0U,
    total_pages,
    table_of_contents,
    reference,
    page_reference,
    style_reference,
    document_property,
    date,
    hyperlink,
    sequence,
    index,
    index_entry,
    custom,
};

struct field_summary {
    std::size_t index{};
    featherdoc::field_kind kind{featherdoc::field_kind::custom};
    std::string instruction;
    std::string result_text;
    bool dirty{false};
    bool locked{false};
    bool complex{false};
    std::size_t depth{0U};
};

struct omml_summary {
    std::size_t index{};
    bool display{false};
    std::string text;
    std::string xml;
};

[[nodiscard]] std::string make_omml_text(std::string_view text,
                                         bool display = false);
[[nodiscard]] std::string make_omml_fraction(std::string_view numerator,
                                             std::string_view denominator,
                                             bool display = true);
[[nodiscard]] std::string make_omml_superscript(std::string_view base,
                                                std::string_view superscript,
                                                bool display = false);
[[nodiscard]] std::string make_omml_subscript(std::string_view base,
                                              std::string_view subscript,
                                              bool display = false);
[[nodiscard]] std::string make_omml_radical(std::string_view radicand,
                                            std::string_view degree = {},
                                            bool display = true);
[[nodiscard]] std::string make_omml_delimiter(std::string_view expression,
                                              std::string_view begin = "(",
                                              std::string_view end = ")",
                                              bool display = false);
[[nodiscard]] std::string make_omml_nary(std::string_view operator_character,
                                         std::string_view expression,
                                         std::string_view lower_limit = {},
                                         std::string_view upper_limit = {},
                                         bool display = true);

struct field_state_options {
    bool dirty{false};
    bool locked{false};
};

enum class complex_field_instruction_fragment_kind : std::uint8_t {
    text = 0U,
    nested_field,
};

struct complex_field_instruction_fragment {
    featherdoc::complex_field_instruction_fragment_kind kind{
        featherdoc::complex_field_instruction_fragment_kind::text};
    std::string text;
    std::string instruction;
    std::string result_text;
    featherdoc::field_state_options state{};
};

struct complex_field_options {
    featherdoc::field_state_options state{};
};

[[nodiscard]] featherdoc::complex_field_instruction_fragment
complex_field_text_fragment(std::string_view text);
[[nodiscard]] featherdoc::complex_field_instruction_fragment
complex_field_nested_fragment(std::string_view instruction,
                              std::string_view result_text = {},
                              featherdoc::field_state_options state = {});

struct table_of_contents_field_options {
    std::uint32_t min_outline_level{1U};
    std::uint32_t max_outline_level{3U};
    bool hyperlinks{true};
    bool hide_page_numbers_in_web_layout{true};
    bool use_outline_levels{true};
    featherdoc::field_state_options state{};
};

struct reference_field_options {
    bool hyperlink{true};
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct page_reference_field_options {
    bool hyperlink{true};
    bool relative_position{false};
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct style_reference_field_options {
    bool paragraph_number{false};
    bool relative_position{false};
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct document_property_field_options {
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct date_field_options {
    std::string format{"yyyy-MM-dd"};
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct hyperlink_field_options {
    std::optional<std::string> anchor;
    std::optional<std::string> tooltip;
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct sequence_field_options {
    std::string number_format{"ARABIC"};
    std::optional<std::uint32_t> restart_value;
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct caption_field_options {
    std::string number_format{"ARABIC"};
    std::optional<std::uint32_t> restart_value;
    std::string separator{": "};
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct index_field_options {
    std::optional<std::uint32_t> columns;
    bool preserve_formatting{true};
    featherdoc::field_state_options state{};
};

struct index_entry_field_options {
    std::optional<std::string> subentry;
    std::optional<std::string> bookmark_name;
    std::optional<std::string> cross_reference;
    bool bold_page_number{false};
    bool italic_page_number{false};
    featherdoc::field_state_options state{};
};

enum class review_note_kind : std::uint8_t {
    footnote = 0U,
    endnote,
    comment,
};

struct review_note_summary {
    std::size_t index{};
    featherdoc::review_note_kind kind{featherdoc::review_note_kind::footnote};
    std::string id;
    std::optional<std::string> author;
    std::optional<std::string> initials;
    std::optional<std::string> date;
    std::optional<std::string> anchor_text;
    bool resolved{false};
    std::optional<std::size_t> parent_index;
    std::optional<std::string> parent_id;
    std::string text;
};

struct comment_metadata_update {
    std::optional<std::string> author;
    std::optional<std::string> initials;
    std::optional<std::string> date;
    bool clear_author{false};
    bool clear_initials{false};
    bool clear_date{false};
};

enum class revision_kind : std::uint8_t {
    insertion = 0U,
    deletion,
    move_from,
    move_to,
    paragraph_property_change,
    run_property_change,
    unknown,
};

struct revision_summary {
    std::size_t index{};
    featherdoc::revision_kind kind{featherdoc::revision_kind::unknown};
    std::string id;
    std::optional<std::string> author;
    std::optional<std::string> date;
    std::string part_entry_name;
    std::string text;
};

struct revision_metadata_update {
    std::optional<std::string> author;
    std::optional<std::string> date;
    bool clear_author{false};
    bool clear_date{false};
};

struct revision_text_range_options {
    std::string author;
    std::string date;
    std::optional<std::string> expected_text;
};

struct text_range_preview_segment {
    std::size_t paragraph_index{};
    std::size_t text_offset{};
    std::size_t text_length{};
    std::string text;
};

struct text_range_preview {
    std::size_t start_paragraph_index{};
    std::size_t start_text_offset{};
    std::size_t end_paragraph_index{};
    std::size_t end_text_offset{};
    std::size_t text_length{};
    bool plain_text_runs_supported{true};
    std::string text;
    std::vector<featherdoc::text_range_preview_segment> segments;
};

} // namespace featherdoc
