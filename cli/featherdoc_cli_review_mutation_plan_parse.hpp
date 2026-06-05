#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

enum class review_mutation_plan_operation_kind {
    append_paragraph_text_comment,
    append_comment_reply,
    replace_comment,
    remove_comment,
    set_comment_resolved,
    set_comment_metadata,
    insert_paragraph_text_revision,
    delete_paragraph_text_revision,
    replace_paragraph_text_revision,
    append_text_range_comment,
    insert_text_range_revision,
    delete_text_range_revision,
    replace_text_range_revision,
};

struct review_mutation_plan_operation {
    review_mutation_plan_operation_kind kind =
        review_mutation_plan_operation_kind::delete_paragraph_text_revision;
    std::size_t paragraph_index = 0U;
    std::size_t text_offset = 0U;
    std::size_t text_length = 0U;
    std::size_t start_paragraph_index = 0U;
    std::size_t start_text_offset = 0U;
    std::size_t end_paragraph_index = 0U;
    std::size_t end_text_offset = 0U;
    std::size_t comment_index = 0U;
    bool resolved = false;
    std::string text;
    std::optional<std::string> expected_text;
    std::optional<std::string> expected_comment_text;
    std::optional<bool> expected_resolved;
    std::optional<std::size_t> expected_parent_index;
    std::optional<std::string> author;
    std::optional<std::string> initials;
    std::optional<std::string> date;
    bool clear_author = false;
    bool clear_initials = false;
    bool clear_date = false;
};

struct review_mutation_plan_build_request_operation {
    review_mutation_plan_operation_kind kind =
        review_mutation_plan_operation_kind::replace_text_range_revision;
    std::string find_text;
    std::size_t occurrence = 0U;
    std::optional<std::string> before_text;
    std::optional<std::string> after_text;
    bool require_unique = false;
    bool insert_after_match = false;
    std::string text;
    std::optional<std::string> author;
    std::optional<std::string> initials;
    std::optional<std::string> date;
};

[[nodiscard]] auto review_mutation_plan_operation_kind_name(
    review_mutation_plan_operation_kind kind) -> std::string_view;
[[nodiscard]] auto parse_review_mutation_plan_operation_kind(
    std::string_view content, std::size_t &index,
    review_mutation_plan_operation_kind &kind, std::string &error_message)
    -> bool;
[[nodiscard]] auto read_review_mutation_plan_file(
    const std::filesystem::path &plan_path,
    std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool;
[[nodiscard]] auto read_review_mutation_plan_build_request_file(
    const std::filesystem::path &plan_path,
    std::vector<review_mutation_plan_build_request_operation> &operations,
    std::string &error_message) -> bool;
void write_json_review_mutation_plan_document(
    std::ostream &stream,
    const std::vector<review_mutation_plan_operation> &operations);
[[nodiscard]] auto write_review_mutation_plan_file(
    const std::filesystem::path &plan_path,
    const std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool;

} // namespace featherdoc_cli
