#pragma once

#include <featherdoc.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

struct simple_document_mutation_options {
    std::optional<std::filesystem::path> output_path;
    bool json_output = false;
};

struct hyperlink_mutation_options : simple_document_mutation_options {
    std::string text;
    std::string target;
    bool has_text = false;
    bool has_target = false;
};

struct omml_mutation_options : simple_document_mutation_options {
    std::string xml;
    bool has_xml = false;
};

struct review_note_mutation_options : simple_document_mutation_options {
    std::string reference_text;
    std::string note_text;
    bool has_reference_text = false;
    bool has_note_text = false;
};

struct comment_mutation_options : simple_document_mutation_options {
    std::string selected_text;
    std::string comment_text;
    std::string author;
    std::string initials;
    std::string date;
    bool has_selected_text = false;
    bool has_comment_text = false;
    bool has_author = false;
    bool has_initials = false;
    bool has_date = false;
};

struct comment_metadata_mutation_options : simple_document_mutation_options {
    featherdoc::comment_metadata_update metadata;
};

struct revision_authoring_options : simple_document_mutation_options {
    std::string text;
    std::string author;
    std::string date;
    std::string expected_text;
    bool has_text = false;
    bool has_author = false;
    bool has_date = false;
    bool has_expected_text = false;
};

struct revision_metadata_mutation_options : simple_document_mutation_options {
    featherdoc::revision_metadata_update metadata;
};

[[nodiscard]] auto parse_simple_document_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    simple_document_mutation_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_revision_authoring_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    revision_authoring_options &options, std::string &error_message,
    bool require_text = true,
    std::string_view text_forbidden_error =
        "delete-run-revision does not accept --text",
    bool allow_expected_text = false,
    std::string_view expected_text_forbidden_error =
        "this revision command does not accept --expected-text") -> bool;

[[nodiscard]] auto parse_revision_metadata_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    revision_metadata_mutation_options &options, std::string &error_message)
    -> bool;

[[nodiscard]] auto parse_hyperlink_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    hyperlink_mutation_options &options, bool require_text_and_target,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_omml_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    omml_mutation_options &options, bool require_xml,
    std::string &error_message) -> bool;

[[nodiscard]] auto parse_review_note_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    review_note_mutation_options &options, bool require_reference_text,
    bool require_note_text, std::string &error_message) -> bool;

[[nodiscard]] auto parse_comment_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    comment_mutation_options &options, bool require_selected_text,
    bool require_comment_text, std::string &error_message) -> bool;

[[nodiscard]] auto parse_comment_metadata_mutation_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    comment_metadata_mutation_options &options, std::string &error_message)
    -> bool;

} // namespace featherdoc_cli
