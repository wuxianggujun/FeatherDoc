#include "featherdoc_cli_review_mutation_plan.hpp"

#include "featherdoc_cli_review_mutation_plan_ranges.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

auto apply_review_mutation_plan_operation(
    featherdoc::Document &doc,
    const review_mutation_plan_operation &operation) -> bool {
    const auto author =
        operation.author.has_value() ? std::string_view(*operation.author)
                                     : std::string_view{};
    const auto date =
        operation.date.has_value() ? std::string_view(*operation.date)
                                   : std::string_view{};
    const auto initials =
        operation.initials.has_value() ? std::string_view(*operation.initials)
                                       : std::string_view{};

    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_comment_reply:
        return doc.append_comment_reply(operation.comment_index,
                                        operation.text, author, initials,
                                        date) != 0U;
    case review_mutation_plan_operation_kind::replace_comment:
        return doc.replace_comment(operation.comment_index, operation.text);
    case review_mutation_plan_operation_kind::remove_comment:
        return doc.remove_comment(operation.comment_index);
    case review_mutation_plan_operation_kind::set_comment_resolved:
        return doc.set_comment_resolved(operation.comment_index,
                                        operation.resolved);
    case review_mutation_plan_operation_kind::set_comment_metadata: {
        featherdoc::comment_metadata_update metadata;
        metadata.author = operation.author;
        metadata.initials = operation.initials;
        metadata.date = operation.date;
        metadata.clear_author = operation.clear_author;
        metadata.clear_initials = operation.clear_initials;
        metadata.clear_date = operation.clear_date;
        return doc.set_comment_metadata(operation.comment_index, metadata);
    }
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
        return doc.append_paragraph_text_comment(
                   operation.paragraph_index, operation.text_offset,
                   operation.text_length, operation.text, author, initials,
                   date) != 0U;
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
        return doc.insert_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset, operation.text,
            author, date);
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
        return doc.delete_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset,
            operation.text_length, author, date);
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        return doc.replace_paragraph_text_revision(
            operation.paragraph_index, operation.text_offset,
            operation.text_length, operation.text, author, date);
    case review_mutation_plan_operation_kind::append_text_range_comment:
        return doc.append_text_range_comment(
                   operation.start_paragraph_index, operation.start_text_offset,
                   operation.end_paragraph_index, operation.end_text_offset,
                   operation.text, author, initials, date) != 0U;
    case review_mutation_plan_operation_kind::insert_text_range_revision:
        return doc.insert_text_range_revision(operation.start_paragraph_index,
                                              operation.start_text_offset,
                                              operation.text, author, date);
    case review_mutation_plan_operation_kind::delete_text_range_revision:
        return doc.delete_text_range_revision(
            operation.start_paragraph_index, operation.start_text_offset,
            operation.end_paragraph_index, operation.end_text_offset, author,
            date);
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        return doc.replace_text_range_revision(
            operation.start_paragraph_index, operation.start_text_offset,
            operation.end_paragraph_index, operation.end_text_offset,
            operation.text, author, date);
    }

    return false;
}

} // namespace

auto apply_review_mutation_plan_operations(
    featherdoc::Document &doc,
    const std::vector<review_mutation_plan_operation> &operations,
    std::size_t &applied_count, std::string &error_message) -> bool {
    std::vector<std::size_t> operation_indices;
    if (!build_review_mutation_plan_apply_order(operations, operation_indices,
                                                error_message)) {
        return false;
    }

    applied_count = 0U;
    for (const auto operation_index : operation_indices) {
        if (!apply_review_mutation_plan_operation(doc,
                                                  operations[operation_index])) {
            const auto &error_info = doc.last_error();
            error_message = "failed to apply review mutation plan operation " +
                            std::to_string(operation_index) + ": " +
                            (!error_info.detail.empty()
                                 ? error_info.detail
                                 : error_info.code.message());
            return false;
        }
        ++applied_count;
    }

    return true;
}

} // namespace featherdoc_cli
