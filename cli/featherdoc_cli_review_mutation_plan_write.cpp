#include "featherdoc_cli_review_mutation_plan_parse.hpp"

#include "featherdoc_cli_json.hpp"

#include <fstream>
#include <ostream>
#include <string>

namespace featherdoc_cli {
namespace {

[[nodiscard]] auto write_json_bool(bool value) noexcept -> const char * {
    return value ? "true" : "false";
}

} // namespace

void write_json_review_mutation_plan_operation(
    std::ostream &stream, const review_mutation_plan_operation &operation) {
    stream << "{\"kind\":";
    write_json_string(stream, review_mutation_plan_operation_kind_name(operation.kind));
    switch (operation.kind) {
    case review_mutation_plan_operation_kind::append_comment_reply:
    case review_mutation_plan_operation_kind::replace_comment:
    case review_mutation_plan_operation_kind::remove_comment:
        stream << ",\"comment_index\":" << operation.comment_index;
        break;
    case review_mutation_plan_operation_kind::set_comment_resolved:
        stream << ",\"comment_index\":" << operation.comment_index
               << ",\"resolved\":" << write_json_bool(operation.resolved);
        break;
    case review_mutation_plan_operation_kind::set_comment_metadata:
        stream << ",\"comment_index\":" << operation.comment_index;
        break;
    case review_mutation_plan_operation_kind::append_paragraph_text_comment:
    case review_mutation_plan_operation_kind::insert_paragraph_text_revision:
    case review_mutation_plan_operation_kind::delete_paragraph_text_revision:
    case review_mutation_plan_operation_kind::replace_paragraph_text_revision:
        stream << ",\"paragraph_index\":" << operation.paragraph_index
               << ",\"text_offset\":" << operation.text_offset;
        if (operation.kind !=
            review_mutation_plan_operation_kind::insert_paragraph_text_revision) {
            stream << ",\"text_length\":" << operation.text_length;
        }
        break;
    case review_mutation_plan_operation_kind::append_text_range_comment:
    case review_mutation_plan_operation_kind::insert_text_range_revision:
    case review_mutation_plan_operation_kind::delete_text_range_revision:
    case review_mutation_plan_operation_kind::replace_text_range_revision:
        stream << ",\"start_paragraph_index\":"
               << operation.start_paragraph_index
               << ",\"start_text_offset\":" << operation.start_text_offset;
        if (operation.kind !=
            review_mutation_plan_operation_kind::insert_text_range_revision) {
            stream << ",\"end_paragraph_index\":"
                   << operation.end_paragraph_index
                   << ",\"end_text_offset\":" << operation.end_text_offset;
        }
        break;
    }
    if (operation.kind ==
            review_mutation_plan_operation_kind::append_paragraph_text_comment ||
        operation.kind ==
            review_mutation_plan_operation_kind::append_comment_reply ||
        operation.kind ==
            review_mutation_plan_operation_kind::replace_comment ||
        operation.kind ==
            review_mutation_plan_operation_kind::append_text_range_comment) {
        stream << ",\"comment_text\":";
        write_json_string(stream, operation.text);
    } else if (operation.kind ==
            review_mutation_plan_operation_kind::insert_paragraph_text_revision ||
        operation.kind ==
            review_mutation_plan_operation_kind::replace_paragraph_text_revision ||
        operation.kind ==
            review_mutation_plan_operation_kind::insert_text_range_revision ||
        operation.kind ==
            review_mutation_plan_operation_kind::replace_text_range_revision) {
        stream << ",\"text\":";
        write_json_string(stream, operation.text);
    }
    if (operation.expected_text.has_value()) {
        stream << ",\"expected_text\":";
        write_json_string(stream, *operation.expected_text);
    }
    if (operation.expected_comment_text.has_value()) {
        stream << ",\"expected_comment_text\":";
        write_json_string(stream, *operation.expected_comment_text);
    }
    if (operation.expected_resolved.has_value()) {
        stream << ",\"expected_resolved\":"
               << write_json_bool(*operation.expected_resolved);
    }
    if (operation.expected_parent_index.has_value()) {
        stream << ",\"expected_parent_index\":"
               << *operation.expected_parent_index;
    }
    if (operation.author.has_value()) {
        stream << ",\"author\":";
        write_json_string(stream, *operation.author);
    }
    if (operation.initials.has_value()) {
        stream << ",\"initials\":";
        write_json_string(stream, *operation.initials);
    }
    if (operation.date.has_value()) {
        stream << ",\"date\":";
        write_json_string(stream, *operation.date);
    }
    if (operation.clear_author) {
        stream << ",\"clear_author\":true";
    }
    if (operation.clear_initials) {
        stream << ",\"clear_initials\":true";
    }
    if (operation.clear_date) {
        stream << ",\"clear_date\":true";
    }
    stream << '}';
}

void write_json_review_mutation_plan_document(
    std::ostream &stream,
    const std::vector<review_mutation_plan_operation> &operations) {
    stream << "{\"operations\":[";
    for (std::size_t index = 0U; index < operations.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_review_mutation_plan_operation(stream, operations[index]);
    }
    stream << "]}";
}

auto write_review_mutation_plan_file(
    const std::filesystem::path &plan_path,
    const std::vector<review_mutation_plan_operation> &operations,
    std::string &error_message) -> bool {
    std::ofstream stream(plan_path, std::ios::binary);
    if (!stream.good()) {
        error_message = "failed to write review mutation plan file: " +
                        plan_path.string();
        return false;
    }

    write_json_review_mutation_plan_document(stream, operations);
    stream << '\n';
    if (!stream.good()) {
        error_message = "failed to write review mutation plan file: " +
                        plan_path.string();
        return false;
    }
    return true;
}

} // namespace featherdoc_cli
