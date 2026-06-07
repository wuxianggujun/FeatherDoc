#include "featherdoc_cli_template_schema_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_template_part_selection.hpp"
#include "featherdoc_cli_template_schema_model.hpp"
#include "featherdoc_cli_template_schema_ops.hpp"
#include "featherdoc_cli_template_schema_options_parse.hpp"
#include "featherdoc_cli_template_schema_patch_ops.hpp"
#include "featherdoc_cli_template_schema_patch_parse.hpp"
#include "featherdoc_cli_template_schema_parse.hpp"
#include "featherdoc_cli_template_validation_options_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

enum class section_part_family {
    header,
    footer,
};

void write_json_bookmark_summary(std::ostream &stream,
                                 const featherdoc::bookmark_summary &bookmark) {
    stream << "{\"bookmark_name\":";
    write_json_string(stream, bookmark.bookmark_name);
    stream << ",\"occurrence_count\":" << bookmark.occurrence_count
           << ",\"kind\":";
    write_json_string(stream, bookmark_kind_name(bookmark.kind));
    stream << ",\"is_duplicate\":" << json_bool(bookmark.is_duplicate()) << '}';
}

void print_bookmark_summary(std::ostream &stream,
                            const featherdoc::bookmark_summary &bookmark) {
    stream << "name=" << bookmark.bookmark_name
           << " occurrences=" << bookmark.occurrence_count
           << " kind=" << bookmark_kind_name(bookmark.kind)
           << " duplicate=" << yes_no(bookmark.is_duplicate());
}
auto bookmark_kind_to_template_slot_kind(featherdoc::bookmark_kind kind)
    -> std::optional<featherdoc::template_slot_kind> {
    switch (kind) {
    case featherdoc::bookmark_kind::text:
        return featherdoc::template_slot_kind::text;
    case featherdoc::bookmark_kind::block:
        return featherdoc::template_slot_kind::block;
    case featherdoc::bookmark_kind::table_rows:
        return featherdoc::template_slot_kind::table_rows;
    case featherdoc::bookmark_kind::block_range:
    case featherdoc::bookmark_kind::malformed:
    case featherdoc::bookmark_kind::mixed:
        return std::nullopt;
    }

    return std::nullopt;
}

auto content_control_kind_to_template_slot_kind(featherdoc::content_control_kind kind)
    -> featherdoc::template_slot_kind {
    switch (kind) {
    case featherdoc::content_control_kind::run:
        return featherdoc::template_slot_kind::text;
    case featherdoc::content_control_kind::table_row:
        return featherdoc::template_slot_kind::table_rows;
    case featherdoc::content_control_kind::block:
    case featherdoc::content_control_kind::table_cell:
    case featherdoc::content_control_kind::unknown:
        return featherdoc::template_slot_kind::block;
    }

    return featherdoc::template_slot_kind::block;
}

auto to_template_schema_part_kind(validation_part_family family)
    -> featherdoc::template_schema_part_kind {
    switch (family) {
    case validation_part_family::body:
        return featherdoc::template_schema_part_kind::body;
    case validation_part_family::header:
        return featherdoc::template_schema_part_kind::header;
    case validation_part_family::footer:
        return featherdoc::template_schema_part_kind::footer;
    case validation_part_family::section_header:
        return featherdoc::template_schema_part_kind::section_header;
    case validation_part_family::section_footer:
        return featherdoc::template_schema_part_kind::section_footer;
    }

    return featherdoc::template_schema_part_kind::body;
}

auto validation_part_name(featherdoc::template_schema_part_kind part) -> const char * {
    switch (part) {
    case featherdoc::template_schema_part_kind::body:
        return "body";
    case featherdoc::template_schema_part_kind::header:
        return "header";
    case featherdoc::template_schema_part_kind::footer:
        return "footer";
    case featherdoc::template_schema_part_kind::section_header:
        return "section-header";
    case featherdoc::template_schema_part_kind::section_footer:
        return "section-footer";
    }

    return "body";
}

auto append_validate_template_schema_file_targets(
    const path_type &schema_path,
    std::vector<validate_template_schema_target_options> &targets,
    std::string &error_message) -> bool {
    exported_template_schema_result schema_file;
    if (!read_template_schema_file(schema_path, schema_file, error_message)) {
        return false;
    }

    for (const auto &schema_target : schema_file.targets) {
        validate_template_schema_target_options target{};
        target.part = schema_target.part;
        target.part_index = schema_target.part_index;
        target.section_index = schema_target.section_index;
        target.requirements = schema_target.requirements;
        if (schema_target.reference_kind.has_value()) {
            target.reference_kind = *schema_target.reference_kind;
            target.has_kind = true;
        }
        targets.push_back(std::move(target));
    }

    return true;
}

void print_string_list(std::ostream &stream, std::string_view label,
                       const std::vector<std::string> &values) {
    stream << label << ": ";
    if (values.empty()) {
        stream << "none\n";
        return;
    }

    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0U) {
            stream << ", ";
        }
        stream << values[index];
    }
    stream << '\n';
}

void write_json_bookmark_summaries(
    std::ostream &stream,
    const std::vector<featherdoc::bookmark_summary> &bookmarks) {
    stream << '[';
    for (std::size_t index = 0; index < bookmarks.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_bookmark_summary(stream, bookmarks[index]);
    }
    stream << ']';
}

void write_json_template_kind_mismatches(
    std::ostream &stream,
    const std::vector<featherdoc::template_kind_mismatch> &mismatches) {
    stream << '[';
    for (std::size_t index = 0; index < mismatches.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        const auto &mismatch = mismatches[index];
        stream << "{\"bookmark_name\":";
        write_json_string(stream, mismatch.bookmark_name);
        stream << ",\"expected_kind\":";
        write_json_string(stream, template_slot_kind_name(mismatch.expected_kind));
        stream << ",\"actual_kind\":";
        write_json_string(stream, bookmark_kind_name(mismatch.actual_kind));
        stream << ",\"occurrence_count\":" << mismatch.occurrence_count << '}';
    }
    stream << ']';
}

void write_json_template_occurrence_mismatches(
    std::ostream &stream,
    const std::vector<featherdoc::template_occurrence_mismatch> &mismatches) {
    stream << '[';
    for (std::size_t index = 0; index < mismatches.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        const auto &mismatch = mismatches[index];
        stream << "{\"bookmark_name\":";
        write_json_string(stream, mismatch.bookmark_name);
        stream << ",\"actual_occurrences\":" << mismatch.actual_occurrences
               << ",\"min_occurrences\":" << mismatch.min_occurrences
               << ",\"max_occurrences\":";
        if (mismatch.max_occurrences.has_value()) {
            stream << *mismatch.max_occurrences;
        } else {
            stream << "null";
        }
        stream << '}';
    }
    stream << ']';
}

void print_bookmark_summary_list(std::ostream &stream, std::string_view label,
                                 const std::vector<featherdoc::bookmark_summary> &bookmarks) {
    stream << label << ": ";
    if (bookmarks.empty()) {
        stream << "none\n";
        return;
    }

    for (std::size_t index = 0; index < bookmarks.size(); ++index) {
        if (index != 0U) {
            stream << "; ";
        }
        print_bookmark_summary(stream, bookmarks[index]);
    }
    stream << '\n';
}

void print_template_kind_mismatch_list(
    std::ostream &stream, std::string_view label,
    const std::vector<featherdoc::template_kind_mismatch> &mismatches) {
    stream << label << ": ";
    if (mismatches.empty()) {
        stream << "none\n";
        return;
    }

    for (std::size_t index = 0; index < mismatches.size(); ++index) {
        if (index != 0U) {
            stream << "; ";
        }

        const auto &mismatch = mismatches[index];
        stream << mismatch.bookmark_name
               << " expected=" << template_slot_kind_name(mismatch.expected_kind)
               << " actual=" << bookmark_kind_name(mismatch.actual_kind)
               << " occurrences=" << mismatch.occurrence_count;
    }
    stream << '\n';
}

void print_template_occurrence_mismatch_list(
    std::ostream &stream, std::string_view label,
    const std::vector<featherdoc::template_occurrence_mismatch> &mismatches) {
    stream << label << ": ";
    if (mismatches.empty()) {
        stream << "none\n";
        return;
    }

    for (std::size_t index = 0; index < mismatches.size(); ++index) {
        if (index != 0U) {
            stream << "; ";
        }

        const auto &mismatch = mismatches[index];
        stream << mismatch.bookmark_name
               << " actual=" << mismatch.actual_occurrences
               << " min=" << mismatch.min_occurrences
               << " max=";
        if (mismatch.max_occurrences.has_value()) {
            stream << *mismatch.max_occurrences;
        } else {
            stream << "unbounded";
        }
    }
    stream << '\n';
}


void print_template_validation_result(
    const selected_template_part &selected,
    const featherdoc::template_validation_result &result, bool json_output) {
    const auto entry_name = std::string(selected.part.entry_name());
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, validation_part_name(selected.family));
        if (selected.part_index.has_value()) {
            std::cout << ",\"part_index\":" << *selected.part_index;
        }
        if (selected.section_index.has_value()) {
            std::cout << ",\"section\":" << *selected.section_index;
        }
        if (selected.reference_kind.has_value()) {
            std::cout << ",\"kind\":";
            write_json_string(std::cout,
                              featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, entry_name);
        std::cout << ",\"passed\":" << json_bool(static_cast<bool>(result))
                  << ",\"missing_required\":";
        write_json_strings(std::cout, result.missing_required);
        std::cout << ",\"duplicate_bookmarks\":";
        write_json_strings(std::cout, result.duplicate_bookmarks);
        std::cout << ",\"malformed_placeholders\":";
        write_json_strings(std::cout, result.malformed_placeholders);
        std::cout << ",\"unexpected_bookmarks\":";
        write_json_bookmark_summaries(std::cout, result.unexpected_bookmarks);
        std::cout << ",\"kind_mismatches\":";
        write_json_template_kind_mismatches(std::cout, result.kind_mismatches);
        std::cout << ",\"occurrence_mismatches\":";
        write_json_template_occurrence_mismatches(
            std::cout, result.occurrence_mismatches);
        std::cout << "}\n";
        return;
    }

    std::cout << "part: " << validation_part_name(selected.family) << '\n';
    if (selected.part_index.has_value()) {
        std::cout << "part_index: " << *selected.part_index << '\n';
    }
    if (selected.section_index.has_value()) {
        std::cout << "section: " << *selected.section_index << '\n';
    }
    if (selected.reference_kind.has_value()) {
        std::cout << "kind: "
                  << featherdoc::to_xml_reference_type(*selected.reference_kind) << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "passed: " << yes_no(static_cast<bool>(result)) << '\n';
    print_string_list(std::cout, "missing_required", result.missing_required);
    print_string_list(std::cout, "duplicate_bookmarks", result.duplicate_bookmarks);
    print_string_list(std::cout, "malformed_placeholders",
                      result.malformed_placeholders);
    print_bookmark_summary_list(std::cout, "unexpected_bookmarks",
                                result.unexpected_bookmarks);
    print_template_kind_mismatch_list(std::cout, "kind_mismatches",
                                      result.kind_mismatches);
    print_template_occurrence_mismatch_list(std::cout, "occurrence_mismatches",
                                            result.occurrence_mismatches);
}

void write_json_template_schema_part_result(
    std::ostream &stream,
    const featherdoc::template_schema_part_validation_result &part_result) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(part_result.target.part));
    if (part_result.target.part_index.has_value()) {
        stream << ",\"part_index\":" << *part_result.target.part_index;
    }
    if (part_result.target.section_index.has_value()) {
        stream << ",\"section\":" << *part_result.target.section_index;
    }
    if (part_result.target.part ==
            featherdoc::template_schema_part_kind::section_header ||
        part_result.target.part ==
            featherdoc::template_schema_part_kind::section_footer) {
        stream << ",\"kind\":";
        write_json_string(
            stream, featherdoc::to_xml_reference_type(part_result.target.reference_kind));
    }
    stream << ",\"available\":" << json_bool(part_result.available)
           << ",\"entry_name\":";
    write_json_string(stream, part_result.entry_name);
    stream << ",\"passed\":" << json_bool(static_cast<bool>(part_result))
           << ",\"missing_required\":";
    write_json_strings(stream, part_result.validation.missing_required);
    stream << ",\"duplicate_bookmarks\":";
    write_json_strings(stream, part_result.validation.duplicate_bookmarks);
    stream << ",\"malformed_placeholders\":";
    write_json_strings(stream, part_result.validation.malformed_placeholders);
    stream << ",\"unexpected_bookmarks\":";
    write_json_bookmark_summaries(stream, part_result.validation.unexpected_bookmarks);
    stream << ",\"kind_mismatches\":";
    write_json_template_kind_mismatches(stream, part_result.validation.kind_mismatches);
    stream << ",\"occurrence_mismatches\":";
    write_json_template_occurrence_mismatches(
        stream, part_result.validation.occurrence_mismatches);
    stream << '}';
}

void print_template_schema_part_result(
    std::ostream &stream,
    const featherdoc::template_schema_part_validation_result &part_result) {
    stream << "part: " << validation_part_name(part_result.target.part) << '\n';
    if (part_result.target.part_index.has_value()) {
        stream << "part_index: " << *part_result.target.part_index << '\n';
    }
    if (part_result.target.section_index.has_value()) {
        stream << "section: " << *part_result.target.section_index << '\n';
    }
    if (part_result.target.part ==
            featherdoc::template_schema_part_kind::section_header ||
        part_result.target.part ==
            featherdoc::template_schema_part_kind::section_footer) {
        stream << "kind: "
               << featherdoc::to_xml_reference_type(part_result.target.reference_kind)
               << '\n';
    }
    stream << "available: " << yes_no(part_result.available) << '\n';
    stream << "entry_name: " << part_result.entry_name << '\n';
    stream << "passed: " << yes_no(static_cast<bool>(part_result)) << '\n';
    print_string_list(stream, "missing_required",
                      part_result.validation.missing_required);
    print_string_list(stream, "duplicate_bookmarks",
                      part_result.validation.duplicate_bookmarks);
    print_string_list(stream, "malformed_placeholders",
                      part_result.validation.malformed_placeholders);
    print_bookmark_summary_list(stream, "unexpected_bookmarks",
                                part_result.validation.unexpected_bookmarks);
    print_template_kind_mismatch_list(stream, "kind_mismatches",
                                      part_result.validation.kind_mismatches);
    print_template_occurrence_mismatch_list(
        stream, "occurrence_mismatches",
        part_result.validation.occurrence_mismatches);
}

void print_template_schema_validation_result(
    const featherdoc::template_schema_validation_result &result, bool json_output) {
    if (json_output) {
        std::cout << "{\"passed\":" << json_bool(static_cast<bool>(result))
                  << ",\"part_results\":[";
        for (std::size_t index = 0U; index < result.part_results.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_schema_part_result(std::cout,
                                                   result.part_results[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "passed: " << yes_no(static_cast<bool>(result)) << '\n';
    std::cout << "part_result_count: " << result.part_results.size() << '\n';
    for (std::size_t index = 0U; index < result.part_results.size(); ++index) {
        std::cout << '\n';
        std::cout << "part_result[" << index << "]\n";
        print_template_schema_part_result(std::cout, result.part_results[index]);
    }
}

void write_json_exported_template_schema_requirement(
    std::ostream &stream, const featherdoc::template_slot_requirement &requirement) {
    stream << "{\"" << template_slot_source_json_name(requirement.source) << "\":";
    write_json_string(stream, requirement.bookmark_name);
    stream << ",\"kind\":";
    write_json_string(stream, template_slot_kind_name(requirement.kind));
    if (requirement.min_occurrences.has_value() &&
        requirement.max_occurrences.has_value() &&
        *requirement.min_occurrences == *requirement.max_occurrences) {
        stream << ",\"count\":" << *requirement.min_occurrences;
    } else {
        if (!requirement.required) {
            stream << ",\"required\":false";
        }
        if (requirement.min_occurrences.has_value()) {
            stream << ",\"min\":" << *requirement.min_occurrences;
        }
        if (requirement.max_occurrences.has_value()) {
            stream << ",\"max\":" << *requirement.max_occurrences;
        }
    }
    stream << '}';
}

struct previewed_template_schema_patch_summary {
    std::size_t left_slot_count = 0U;
    std::optional<path_type> output_patch_path;
    std::optional<path_type> review_json_path;
    std::optional<std::size_t> right_slot_count;
    std::optional<std::size_t> upsert_slot_count;
    std::optional<std::size_t> remove_target_count;
    std::optional<std::size_t> remove_slot_count;
    std::optional<std::size_t> rename_slot_count;
    std::optional<std::size_t> update_slot_count;
    featherdoc::template_schema_patch_summary applied{};
};

auto to_template_schema_part_selector(
    const template_schema_patch_target_selector &selector)
    -> featherdoc::template_schema_part_selector {
    featherdoc::template_schema_part_selector target{};
    target.part = to_template_schema_part_kind(selector.part);
    target.part_index = selector.part_index;
    target.section_index = selector.section_index;
    target.reference_kind = selector.reference_kind.value_or(
        featherdoc::section_reference_kind::default_reference);
    return target;
}

auto to_template_schema_part_selector(const exported_template_schema_target &target)
    -> featherdoc::template_schema_part_selector {
    featherdoc::template_schema_part_selector selector{};
    selector.part = to_template_schema_part_kind(target.part);
    selector.part_index = target.part_index;
    selector.section_index = target.section_index;
    selector.reference_kind = target.reference_kind.value_or(
        featherdoc::section_reference_kind::default_reference);
    return selector;
}

auto to_template_schema(const exported_template_schema_result &result)
    -> featherdoc::template_schema {
    featherdoc::template_schema schema{};
    schema.entries.reserve(result.slot_count());
    for (const auto &target : result.targets) {
        const auto selector = to_template_schema_part_selector(target);
        for (const auto &requirement : target.requirements) {
            schema.entries.push_back({selector, requirement});
        }
    }
    return schema;
}

auto to_template_schema_patch(const template_schema_patch_document &document)
    -> featherdoc::template_schema_patch {
    featherdoc::template_schema_patch patch{};

    for (const auto &target : document.upsert_targets) {
        const auto selector = to_template_schema_part_selector(target);
        for (const auto &requirement : target.requirements) {
            patch.upsert_slots.push_back({selector, requirement});
        }
    }

    patch.remove_targets.reserve(document.remove_targets.size());
    for (const auto &target : document.remove_targets) {
        patch.remove_targets.push_back(to_template_schema_part_selector(target));
    }

    patch.remove_slots.reserve(document.remove_slots.size());
    for (const auto &remove_slot : document.remove_slots) {
        featherdoc::template_schema_slot_selector slot{};
        slot.target = to_template_schema_part_selector(remove_slot.target);
        slot.source = remove_slot.source;
        slot.bookmark_name = remove_slot.bookmark_name;
        patch.remove_slots.push_back(std::move(slot));
    }

    patch.rename_slots.reserve(document.rename_slots.size());
    for (const auto &rename_slot : document.rename_slots) {
        featherdoc::template_schema_slot_rename operation{};
        operation.slot.target = to_template_schema_part_selector(rename_slot.target);
        operation.slot.source = rename_slot.source;
        operation.slot.bookmark_name = rename_slot.bookmark_name;
        operation.new_bookmark_name = rename_slot.new_bookmark_name;
        patch.rename_slots.push_back(std::move(operation));
    }

    patch.update_slots.reserve(document.update_slots.size());
    for (const auto &update_slot : document.update_slots) {
        featherdoc::template_schema_slot_patch_update operation{};
        operation.slot.target = to_template_schema_part_selector(update_slot.target);
        operation.slot.source = update_slot.source;
        operation.slot.bookmark_name = update_slot.bookmark_name;
        operation.update = update_slot.update;
        patch.update_slots.push_back(std::move(operation));
    }

    return patch;
}

void write_json_exported_template_schema_target(
    std::ostream &stream, const exported_template_schema_target &target) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(target.part));
    if (target.part_index.has_value()) {
        stream << ",\"index\":" << *target.part_index;
    }
    if (target.section_index.has_value()) {
        stream << ",\"section\":" << *target.section_index;
    }
    if (target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*target.reference_kind));
    }
    if (target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *target.resolved_from_section_index;
    }
    if (target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"slots\":[";
    for (std::size_t slot_index = 0U; slot_index < target.requirements.size();
         ++slot_index) {
        if (slot_index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_requirement(stream,
                                                        target.requirements[slot_index]);
    }
    stream << "]}";
}

void write_json_exported_template_schema(std::ostream &stream,
                                         const exported_template_schema_result &result) {
    stream << "{\"targets\":[";
    for (std::size_t target_index = 0U; target_index < result.targets.size();
         ++target_index) {
        if (target_index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.targets[target_index]);
    }
    stream << "]}\n";
}

void write_json_template_schema_patch_selector(
    std::ostream &stream, const template_schema_patch_target_selector &selector) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(selector.part));
    if (selector.part_index.has_value()) {
        stream << ",\"index\":" << *selector.part_index;
    }
    if (selector.section_index.has_value()) {
        stream << ",\"section\":" << *selector.section_index;
    }
    if (selector.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*selector.reference_kind));
    }
    if (selector.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *selector.resolved_from_section_index;
    }
    if (selector.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << '}';
}

void write_json_template_schema_patch_remove_slot(
    std::ostream &stream, const template_schema_patch_remove_slot &operation) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(operation.target.part));
    if (operation.target.part_index.has_value()) {
        stream << ",\"index\":" << *operation.target.part_index;
    }
    if (operation.target.section_index.has_value()) {
        stream << ",\"section\":" << *operation.target.section_index;
    }
    if (operation.target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(
            stream,
            featherdoc::to_xml_reference_type(*operation.target.reference_kind));
    }
    if (operation.target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *operation.target.resolved_from_section_index;
    }
    if (operation.target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    stream << '}';
}

void write_json_template_schema_patch_rename_slot(
    std::ostream &stream, const template_schema_patch_rename_slot &operation) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(operation.target.part));
    if (operation.target.part_index.has_value()) {
        stream << ",\"index\":" << *operation.target.part_index;
    }
    if (operation.target.section_index.has_value()) {
        stream << ",\"section\":" << *operation.target.section_index;
    }
    if (operation.target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(
            stream,
            featherdoc::to_xml_reference_type(*operation.target.reference_kind));
    }
    if (operation.target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *operation.target.resolved_from_section_index;
    }
    if (operation.target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    stream << ",\"" << template_slot_source_new_json_name(operation.source) << "\":";
    write_json_string(stream, operation.new_bookmark_name);
    stream << '}';
}

void write_json_template_schema_patch_update_slot(
    std::ostream &stream, const template_schema_patch_update_slot &operation) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(operation.target.part));
    if (operation.target.part_index.has_value()) {
        stream << ",\"index\":" << *operation.target.part_index;
    }
    if (operation.target.section_index.has_value()) {
        stream << ",\"section\":" << *operation.target.section_index;
    }
    if (operation.target.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(
            stream,
            featherdoc::to_xml_reference_type(*operation.target.reference_kind));
    }
    if (operation.target.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *operation.target.resolved_from_section_index;
    }
    if (operation.target.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"" << template_slot_source_json_name(operation.source) << "\":";
    write_json_string(stream, operation.bookmark_name);
    if (operation.update.kind.has_value()) {
        stream << ",\"slot_kind\":";
        write_json_string(stream, template_slot_kind_name(*operation.update.kind));
    }
    if (operation.update.required.has_value()) {
        stream << ",\"required\":" << json_bool(*operation.update.required);
    }
    if (operation.update.min_occurrences.has_value()) {
        stream << ",\"min_occurrences\":" << *operation.update.min_occurrences;
    }
    if (operation.update.max_occurrences.has_value()) {
        stream << ",\"max_occurrences\":" << *operation.update.max_occurrences;
    }
    if (operation.update.clear_min_occurrences) {
        stream << ",\"clear_min_occurrences\":true";
    }
    if (operation.update.clear_max_occurrences) {
        stream << ",\"clear_max_occurrences\":true";
    }
    stream << '}';
}

void write_json_template_schema_patch_document(
    std::ostream &stream, const template_schema_patch_document &patch) {
    bool wrote_member = false;
    stream << '{';

    const auto write_separator = [&]() {
        if (wrote_member) {
            stream << ',';
        }
        wrote_member = true;
    };

    if (!patch.remove_targets.empty()) {
        write_separator();
        stream << "\"remove_targets\":[";
        for (std::size_t index = 0U; index < patch.remove_targets.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_selector(stream,
                                                      patch.remove_targets[index]);
        }
        stream << ']';
    }

    if (!patch.remove_slots.empty()) {
        write_separator();
        stream << "\"remove_slots\":[";
        for (std::size_t index = 0U; index < patch.remove_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_remove_slot(stream,
                                                         patch.remove_slots[index]);
        }
        stream << ']';
    }

    if (!patch.rename_slots.empty()) {
        write_separator();
        stream << "\"rename_slots\":[";
        for (std::size_t index = 0U; index < patch.rename_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_rename_slot(stream,
                                                         patch.rename_slots[index]);
        }
        stream << ']';
    }

    if (!patch.update_slots.empty()) {
        write_separator();
        stream << "\"update_slots\":[";
        for (std::size_t index = 0U; index < patch.update_slots.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_template_schema_patch_update_slot(stream,
                                                         patch.update_slots[index]);
        }
        stream << ']';
    }

    if (!patch.upsert_targets.empty()) {
        write_separator();
        stream << "\"upsert_targets\":[";
        for (std::size_t index = 0U; index < patch.upsert_targets.size(); ++index) {
            if (index != 0U) {
                stream << ',';
            }
            write_json_exported_template_schema_target(stream,
                                                       patch.upsert_targets[index]);
        }
        stream << ']';
    }

    stream << "}\n";
}

void write_json_template_schema_lint_issue(std::ostream &stream,
                                           const template_schema_lint_issue &issue) {
    stream << "{\"issue\":";
    write_json_string(stream, template_schema_lint_issue_name(issue.kind));
    stream << ",\"target_index\":" << issue.target_index << ",\"target\":";
    write_json_template_schema_patch_selector(stream, issue.target);
    if (issue.previous_target_index.has_value()) {
        stream << ",\"previous_target_index\":" << *issue.previous_target_index;
    }
    if (issue.slot_index.has_value()) {
        stream << ",\"slot_index\":" << *issue.slot_index;
    }
    if (issue.previous_slot_index.has_value()) {
        stream << ",\"previous_slot_index\":" << *issue.previous_slot_index;
    }
    if (!issue.bookmark_name.empty()) {
        stream << ",\"bookmark\":";
        write_json_string(stream, issue.bookmark_name);
    }
    if (!issue.previous_bookmark_name.empty()) {
        stream << ",\"previous_bookmark\":";
        write_json_string(stream, issue.previous_bookmark_name);
    }
    if (!issue.entry_name.empty()) {
        stream << ",\"entry_name\":";
        write_json_string(stream, issue.entry_name);
    }
    stream << '}';
}

void write_json_exported_template_schema_skipped_bookmark(
    std::ostream &stream,
    const exported_template_schema_skipped_bookmark &bookmark) {
    stream << "{\"part\":";
    write_json_string(stream, validation_part_name(bookmark.part));
    if (bookmark.part_index.has_value()) {
        stream << ",\"part_index\":" << *bookmark.part_index;
    }
    stream << ",\"entry_name\":";
    write_json_string(stream, bookmark.entry_name);
    if (bookmark.section_index.has_value()) {
        stream << ",\"section\":" << *bookmark.section_index;
    }
    if (bookmark.reference_kind.has_value()) {
        stream << ",\"kind\":";
        write_json_string(stream,
                          featherdoc::to_xml_reference_type(*bookmark.reference_kind));
    }
    if (bookmark.resolved_from_section_index.has_value()) {
        stream << ",\"resolved_from_section\":"
               << *bookmark.resolved_from_section_index;
    }
    if (bookmark.linked_to_previous) {
        stream << ",\"linked_to_previous\":true";
    }
    stream << ",\"bookmark_name\":";
    write_json_string(stream, bookmark.bookmark.bookmark_name);
    stream << ",\"kind\":";
    write_json_string(stream, bookmark_kind_name(bookmark.bookmark.kind));
    stream << ",\"occurrence_count\":" << bookmark.bookmark.occurrence_count
           << ",\"reason\":";
    write_json_string(stream, bookmark.reason);
    stream << '}';
}

void print_exported_template_schema_summary(
    const exported_template_schema_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"export-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"skipped_count\":" << result.skipped_bookmarks.size()
                  << ",\"skipped_bookmarks\":[";
        for (std::size_t index = 0U; index < result.skipped_bookmarks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_skipped_bookmark(
                std::cout, result.skipped_bookmarks[index]);
        }
        std::cout << "]}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "skipped_count: " << result.skipped_bookmarks.size() << '\n';
    if (result.skipped_bookmarks.empty()) {
        std::cout << "skipped_bookmarks: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.skipped_bookmarks.size(); ++index) {
        const auto &bookmark = result.skipped_bookmarks[index];
        std::cout << "skipped_bookmarks[" << index << "]: part="
                  << validation_part_name(bookmark.part);
        if (bookmark.part_index.has_value()) {
            std::cout << " part_index=" << *bookmark.part_index;
        }
        if (bookmark.section_index.has_value()) {
            std::cout << " section=" << *bookmark.section_index;
        }
        if (bookmark.reference_kind.has_value()) {
            std::cout << " kind="
                      << featherdoc::to_xml_reference_type(*bookmark.reference_kind);
        }
        if (bookmark.resolved_from_section_index.has_value()) {
            std::cout << " resolved_from_section="
                      << *bookmark.resolved_from_section_index;
        }
        if (bookmark.linked_to_previous) {
            std::cout << " linked_to_previous=yes";
        }
        std::cout << " entry=" << bookmark.entry_name
                  << " bookmark=" << bookmark.bookmark.bookmark_name
                  << " kind=" << bookmark_kind_name(bookmark.bookmark.kind)
                  << " occurrences=" << bookmark.bookmark.occurrence_count
                  << " reason=" << bookmark.reason << '\n';
    }
}

void print_exported_template_schema_requirement(
    std::ostream &stream, const featherdoc::template_slot_requirement &requirement) {
    stream << template_slot_source_text_name(requirement.source) << '='
           << requirement.bookmark_name << " kind="
           << template_slot_kind_name(requirement.kind);
    if (requirement.min_occurrences.has_value() &&
        requirement.max_occurrences.has_value() &&
        *requirement.min_occurrences == *requirement.max_occurrences) {
        stream << " count=" << *requirement.min_occurrences;
        return;
    }

    stream << " required=" << yes_no(requirement.required);
    if (requirement.min_occurrences.has_value()) {
        stream << " min=" << *requirement.min_occurrences;
    }
    if (requirement.max_occurrences.has_value()) {
        stream << " max=" << *requirement.max_occurrences;
    }
}

void print_exported_template_schema_target(
    std::ostream &stream, const exported_template_schema_target &target) {
    stream << "part: " << validation_part_name(target.part) << '\n';
    if (target.part_index.has_value()) {
        stream << "part_index: " << *target.part_index << '\n';
    }
    if (target.section_index.has_value()) {
        stream << "section: " << *target.section_index << '\n';
    }
    if (target.reference_kind.has_value()) {
        stream << "kind: "
               << featherdoc::to_xml_reference_type(*target.reference_kind) << '\n';
    }
    if (target.resolved_from_section_index.has_value()) {
        stream << "resolved_from_section: " << *target.resolved_from_section_index
               << '\n';
    }
    stream << "linked_to_previous: " << yes_no(target.linked_to_previous) << '\n';
    stream << "slot_count: " << target.requirements.size() << '\n';
    for (std::size_t index = 0U; index < target.requirements.size(); ++index) {
        stream << "slot[" << index << "]: ";
        print_exported_template_schema_requirement(stream, target.requirements[index]);
        stream << '\n';
    }
}

void print_normalized_template_schema_summary(
    const exported_template_schema_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"normalize-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count() << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n';
}

void print_template_schema_patch_selector_summary(
    std::ostream &stream, const template_schema_patch_target_selector &selector) {
    stream << "part=" << validation_part_name(selector.part);
    if (selector.part_index.has_value()) {
        stream << " index=" << *selector.part_index;
    }
    if (selector.section_index.has_value()) {
        stream << " section=" << *selector.section_index;
    }
    if (selector.reference_kind.has_value()) {
        stream << " kind="
               << featherdoc::to_xml_reference_type(*selector.reference_kind);
    }
    if (selector.resolved_from_section_index.has_value()) {
        stream << " resolved_from_section="
               << *selector.resolved_from_section_index;
    }
    if (selector.linked_to_previous) {
        stream << " linked_to_previous=yes";
    }
}

void print_linted_template_schema_result(const template_schema_lint_result &result,
                                         bool json_output) {
    const auto duplicate_target_count =
        result.issue_count(template_schema_lint_issue_kind::duplicate_target_identity);
    const auto duplicate_slot_count =
        result.issue_count(template_schema_lint_issue_kind::duplicate_slot_name);
    const auto target_order_issue_count =
        result.issue_count(template_schema_lint_issue_kind::target_order);
    const auto slot_order_issue_count =
        result.issue_count(template_schema_lint_issue_kind::slot_order);
    const auto entry_name_issue_count =
        result.issue_count(template_schema_lint_issue_kind::entry_name_present);

    if (json_output) {
        std::cout << "{\"command\":\"lint-template-schema\",\"ok\":true,"
                  << "\"clean\":" << json_bool(result.clean())
                  << ",\"target_count\":" << result.target_count
                  << ",\"slot_count\":" << result.slot_count
                  << ",\"issue_count\":" << result.issues.size()
                  << ",\"duplicate_target_count\":" << duplicate_target_count
                  << ",\"duplicate_slot_count\":" << duplicate_slot_count
                  << ",\"target_order_issue_count\":" << target_order_issue_count
                  << ",\"slot_order_issue_count\":" << slot_order_issue_count
                  << ",\"entry_name_issue_count\":" << entry_name_issue_count
                  << ",\"issues\":[";
        for (std::size_t index = 0U; index < result.issues.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_template_schema_lint_issue(std::cout, result.issues[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "clean: " << yes_no(result.clean()) << '\n'
              << "target_count: " << result.target_count << '\n'
              << "slot_count: " << result.slot_count << '\n'
              << "issue_count: " << result.issues.size() << '\n'
              << "duplicate_target_count: " << duplicate_target_count << '\n'
              << "duplicate_slot_count: " << duplicate_slot_count << '\n'
              << "target_order_issue_count: " << target_order_issue_count << '\n'
              << "slot_order_issue_count: " << slot_order_issue_count << '\n'
              << "entry_name_issue_count: " << entry_name_issue_count << '\n';
    if (result.issues.empty()) {
        std::cout << "issues: none\n";
        return;
    }

    for (std::size_t index = 0U; index < result.issues.size(); ++index) {
        const auto &issue = result.issues[index];
        std::cout << "issue[" << index << "]: issue="
                  << template_schema_lint_issue_name(issue.kind)
                  << " target_index=" << issue.target_index << ' ';
        print_template_schema_patch_selector_summary(std::cout, issue.target);
        if (issue.previous_target_index.has_value()) {
            std::cout << " previous_target_index=" << *issue.previous_target_index;
        }
        if (issue.slot_index.has_value()) {
            std::cout << " slot_index=" << *issue.slot_index;
        }
        if (issue.previous_slot_index.has_value()) {
            std::cout << " previous_slot_index=" << *issue.previous_slot_index;
        }
        if (!issue.bookmark_name.empty()) {
            std::cout << " bookmark=" << issue.bookmark_name;
        }
        if (!issue.previous_bookmark_name.empty()) {
            std::cout << " previous_bookmark=" << issue.previous_bookmark_name;
        }
        if (!issue.entry_name.empty()) {
            std::cout << " entry_name=" << issue.entry_name;
        }
        std::cout << '\n';
    }
}

void print_repaired_template_schema_summary(
    const exported_template_schema_result &result,
    const repaired_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"repair-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"input_target_count\":" << summary.input_target_count
                  << ",\"input_slot_count\":" << summary.input_slot_count
                  << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"merged_duplicate_target_count\":"
                  << summary.merged_duplicate_target_count
                  << ",\"deduplicated_target_count\":"
                  << summary.deduplicated_target_count
                  << ",\"deduplicated_slot_count\":"
                  << summary.deduplicated_slot_count
                  << ",\"stripped_entry_name_count\":"
                  << summary.stripped_entry_name_count
                  << ",\"replaced_slot_count\":"
                  << summary.replaced_slot_count
                  << ",\"changed\":" << json_bool(summary.changed) << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "input_target_count: " << summary.input_target_count << '\n'
              << "input_slot_count: " << summary.input_slot_count << '\n'
              << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "merged_duplicate_target_count: "
              << summary.merged_duplicate_target_count << '\n'
              << "deduplicated_target_count: "
              << summary.deduplicated_target_count << '\n'
              << "deduplicated_slot_count: "
              << summary.deduplicated_slot_count << '\n'
              << "stripped_entry_name_count: "
              << summary.stripped_entry_name_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n'
              << "changed: " << yes_no(summary.changed) << '\n';
}

void print_merged_template_schema_summary(
    const exported_template_schema_result &result,
    const merged_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"merge-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"input_count\":" << summary.input_count
                  << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"updated_target_count\":" << summary.updated_target_count
                  << ",\"replaced_slot_count\":" << summary.replaced_slot_count
                  << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "input_count: " << summary.input_count << '\n'
              << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "updated_target_count: " << summary.updated_target_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n';
}

void print_patched_template_schema_summary(
    const exported_template_schema_result &result,
    const patched_template_schema_summary &summary,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"patch-template-schema\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"target_count\":" << result.targets.size()
                  << ",\"slot_count\":" << result.slot_count()
                  << ",\"upsert_target_count\":" << summary.upsert_target_count
                  << ",\"remove_target_count\":" << summary.remove_target_count
                  << ",\"remove_slot_count\":" << summary.remove_slot_count
                  << ",\"rename_slot_count\":" << summary.rename_slot_count
                  << ",\"update_slot_count\":" << summary.update_slot_count
                  << ",\"updated_target_count\":" << summary.updated_target_count
                  << ",\"replaced_slot_count\":" << summary.replaced_slot_count
                  << ",\"applied_remove_target_count\":"
                  << summary.applied_remove_target_count
                  << ",\"applied_remove_slot_count\":"
                  << summary.applied_remove_slot_count
                  << ",\"applied_rename_slot_count\":"
                  << summary.applied_rename_slot_count
                  << ",\"applied_update_slot_count\":"
                  << summary.applied_update_slot_count
                  << ",\"pruned_empty_target_count\":"
                  << summary.pruned_empty_target_count << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    std::cout << "target_count: " << result.targets.size() << '\n'
              << "slot_count: " << result.slot_count() << '\n'
              << "upsert_target_count: " << summary.upsert_target_count << '\n'
              << "remove_target_count: " << summary.remove_target_count << '\n'
              << "remove_slot_count: " << summary.remove_slot_count << '\n'
              << "rename_slot_count: " << summary.rename_slot_count << '\n'
              << "update_slot_count: " << summary.update_slot_count << '\n'
              << "updated_target_count: " << summary.updated_target_count << '\n'
              << "replaced_slot_count: " << summary.replaced_slot_count << '\n'
              << "applied_remove_target_count: "
              << summary.applied_remove_target_count << '\n'
              << "applied_remove_slot_count: "
              << summary.applied_remove_slot_count << '\n'
              << "applied_rename_slot_count: "
              << summary.applied_rename_slot_count << '\n'
              << "applied_update_slot_count: "
              << summary.applied_update_slot_count << '\n'
              << "pruned_empty_target_count: "
              << summary.pruned_empty_target_count << '\n';
}

void print_previewed_template_schema_patch_summary(
    const previewed_template_schema_patch_summary &summary, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"preview-template-schema-patch\",\"ok\":true";
        if (summary.output_patch_path.has_value()) {
            std::cout << ",\"output_patch_path\":";
            write_json_string(std::cout, summary.output_patch_path->string());
        }
        if (summary.review_json_path.has_value()) {
            std::cout << ",\"review_json_path\":";
            write_json_string(std::cout, summary.review_json_path->string());
        }
        std::cout << ",\"left_slot_count\":" << summary.left_slot_count;
        if (summary.right_slot_count.has_value()) {
            std::cout << ",\"right_slot_count\":" << *summary.right_slot_count;
        }
        if (summary.upsert_slot_count.has_value()) {
            std::cout << ",\"upsert_slot_count\":" << *summary.upsert_slot_count;
        }
        if (summary.remove_target_count.has_value()) {
            std::cout << ",\"remove_target_count\":"
                      << *summary.remove_target_count;
        }
        if (summary.remove_slot_count.has_value()) {
            std::cout << ",\"remove_slot_count\":" << *summary.remove_slot_count;
        }
        if (summary.rename_slot_count.has_value()) {
            std::cout << ",\"rename_slot_count\":" << *summary.rename_slot_count;
        }
        if (summary.update_slot_count.has_value()) {
            std::cout << ",\"update_slot_count\":" << *summary.update_slot_count;
        }
        std::cout << ",\"removed_targets\":" << summary.applied.removed_targets
                  << ",\"removed_slots\":" << summary.applied.removed_slots
                  << ",\"renamed_slots\":" << summary.applied.renamed_slots
                  << ",\"inserted_slots\":" << summary.applied.inserted_slots
                  << ",\"replaced_slots\":" << summary.applied.replaced_slots
                  << ",\"changed\":" << json_bool(summary.applied.changed())
                  << "}\n";
        return;
    }

    if (summary.output_patch_path.has_value()) {
        std::cout << "output_patch_path: " << summary.output_patch_path->string()
                  << '\n';
    }
    if (summary.review_json_path.has_value()) {
        std::cout << "review_json_path: " << summary.review_json_path->string()
                  << '\n';
    }
    std::cout << "left_slot_count: " << summary.left_slot_count << '\n';
    if (summary.right_slot_count.has_value()) {
        std::cout << "right_slot_count: " << *summary.right_slot_count << '\n';
    }
    if (summary.upsert_slot_count.has_value()) {
        std::cout << "upsert_slot_count: " << *summary.upsert_slot_count << '\n';
    }
    if (summary.remove_target_count.has_value()) {
        std::cout << "remove_target_count: " << *summary.remove_target_count << '\n';
    }
    if (summary.remove_slot_count.has_value()) {
        std::cout << "remove_slot_count: " << *summary.remove_slot_count << '\n';
    }
    if (summary.rename_slot_count.has_value()) {
        std::cout << "rename_slot_count: " << *summary.rename_slot_count << '\n';
    }
    if (summary.update_slot_count.has_value()) {
        std::cout << "update_slot_count: " << *summary.update_slot_count << '\n';
    }
    std::cout << "removed_targets: " << summary.applied.removed_targets << '\n'
              << "removed_slots: " << summary.applied.removed_slots << '\n'
              << "renamed_slots: " << summary.applied.renamed_slots << '\n'
              << "inserted_slots: " << summary.applied.inserted_slots << '\n'
              << "replaced_slots: " << summary.applied.replaced_slots << '\n'
              << "changed: " << yes_no(summary.applied.changed()) << '\n';
}

void print_built_template_schema_patch_summary(
    const built_template_schema_patch_summary &summary,
    const std::optional<path_type> &output_path,
    const std::optional<path_type> &review_json_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"build-template-schema-patch\",\"ok\":true";
        if (output_path.has_value()) {
            std::cout << ",\"output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        if (review_json_path.has_value()) {
            std::cout << ",\"review_json_path\":";
            write_json_string(std::cout, review_json_path->string());
        }
        std::cout << ",\"added_target_count\":" << summary.added_target_count
                  << ",\"removed_target_count\":" << summary.removed_target_count
                  << ",\"changed_target_count\":" << summary.changed_target_count
                  << ",\"generated_remove_target_count\":"
                  << summary.generated_remove_target_count
                  << ",\"generated_remove_slot_count\":"
                  << summary.generated_remove_slot_count
                  << ",\"generated_rename_slot_count\":"
                  << summary.generated_rename_slot_count
                  << ",\"generated_update_slot_count\":"
                  << summary.generated_update_slot_count
                  << ",\"generated_upsert_target_count\":"
                  << summary.generated_upsert_target_count
                  << ",\"empty_patch\":" << json_bool(summary.empty_patch()) << "}\n";
        return;
    }

    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    }
    if (review_json_path.has_value()) {
        std::cout << "review_json_path: " << review_json_path->string() << '\n';
    }
    std::cout << "added_target_count: " << summary.added_target_count << '\n'
              << "removed_target_count: " << summary.removed_target_count << '\n'
              << "changed_target_count: " << summary.changed_target_count << '\n'
              << "generated_remove_target_count: "
              << summary.generated_remove_target_count << '\n'
              << "generated_remove_slot_count: "
              << summary.generated_remove_slot_count << '\n'
              << "generated_rename_slot_count: "
              << summary.generated_rename_slot_count << '\n'
              << "generated_update_slot_count: "
              << summary.generated_update_slot_count << '\n'
              << "generated_upsert_target_count: "
              << summary.generated_upsert_target_count << '\n'
              << "empty_patch: " << yes_no(summary.empty_patch()) << '\n';
}

void write_json_template_schema_diff_result(
    std::ostream &stream, const template_schema_diff_result &result) {
    stream << "{\"equal\":" << json_bool(result.equal())
           << ",\"added_target_count\":" << result.added_targets.size()
           << ",\"removed_target_count\":" << result.removed_targets.size()
           << ",\"changed_target_count\":" << result.changed_targets.size()
           << ",\"added_targets\":[";
    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.added_targets[index]);
    }
    stream << "],\"removed_targets\":[";
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_exported_template_schema_target(stream, result.removed_targets[index]);
    }
    stream << "],\"changed_targets\":[";
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"left\":";
        write_json_exported_template_schema_target(stream, result.changed_targets[index].left);
        stream << ",\"right\":";
        write_json_exported_template_schema_target(stream, result.changed_targets[index].right);
        stream << '}';
    }
    stream << "]}\n";
}

void print_template_schema_diff_result(const template_schema_diff_result &result,
                                       bool json_output) {
    if (json_output) {
        write_json_template_schema_diff_result(std::cout, result);
        return;
    }

    std::cout << "equal: " << yes_no(result.equal()) << '\n'
              << "added_target_count: " << result.added_targets.size() << '\n'
              << "removed_target_count: " << result.removed_targets.size() << '\n'
              << "changed_target_count: " << result.changed_targets.size() << '\n';

    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        std::cout << '\n' << "added_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.added_targets[index]);
    }
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        std::cout << '\n' << "removed_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.removed_targets[index]);
    }
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        std::cout << '\n' << "changed_target[" << index << "].left\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].left);
        std::cout << '\n' << "changed_target[" << index << "].right\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].right);
    }
}

void print_checked_template_schema_result(
    const path_type &schema_path, const template_schema_diff_result &result,
    const std::optional<path_type> &output_path, bool json_output) {
    if (json_output) {
        std::cout << "{\"command\":\"check-template-schema\",\"matches\":"
                  << json_bool(result.equal()) << ",\"schema_file\":";
        write_json_string(std::cout, schema_path.string());
        if (output_path.has_value()) {
            std::cout << ",\"generated_output_path\":";
            write_json_string(std::cout, output_path->string());
        }
        std::cout << ",\"added_target_count\":" << result.added_targets.size()
                  << ",\"removed_target_count\":" << result.removed_targets.size()
                  << ",\"changed_target_count\":" << result.changed_targets.size()
                  << ",\"added_targets\":[";
        for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_target(std::cout,
                                                       result.added_targets[index]);
        }
        std::cout << "],\"removed_targets\":[";
        for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_exported_template_schema_target(
                std::cout, result.removed_targets[index]);
        }
        std::cout << "],\"changed_targets\":[";
        for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            std::cout << "{\"left\":";
            write_json_exported_template_schema_target(
                std::cout, result.changed_targets[index].left);
            std::cout << ",\"right\":";
            write_json_exported_template_schema_target(
                std::cout, result.changed_targets[index].right);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "schema_file: " << schema_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "generated_output_path: " << output_path->string() << '\n';
    }
    std::cout << "matches: " << yes_no(result.equal()) << '\n'
              << "added_target_count: " << result.added_targets.size() << '\n'
              << "removed_target_count: " << result.removed_targets.size() << '\n'
              << "changed_target_count: " << result.changed_targets.size() << '\n';

    for (std::size_t index = 0U; index < result.added_targets.size(); ++index) {
        std::cout << '\n' << "added_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.added_targets[index]);
    }
    for (std::size_t index = 0U; index < result.removed_targets.size(); ++index) {
        std::cout << '\n' << "removed_target[" << index << "]\n";
        print_exported_template_schema_target(std::cout, result.removed_targets[index]);
    }
    for (std::size_t index = 0U; index < result.changed_targets.size(); ++index) {
        std::cout << '\n' << "changed_target[" << index << "].baseline\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].left);
        std::cout << '\n' << "changed_target[" << index << "].generated\n";
        print_exported_template_schema_target(std::cout,
                                              result.changed_targets[index].right);
    }
}

auto append_exported_template_schema_target(
    featherdoc::Document &doc, validation_part_family part_family,
    std::optional<std::size_t> part_index,
    std::optional<std::size_t> section_index,
    std::optional<featherdoc::section_reference_kind> reference_kind,
    std::optional<std::size_t> resolved_from_section_index,
    bool linked_to_previous,
    featherdoc::TemplatePart part,
    exported_template_schema_result &result, std::string_view command,
    bool json_output) -> bool {
    if (!part) {
        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = "target template part is not available";
        return report_operation_failure(command, "inspect",
                                        "failed to inspect template part",
                                        error_info, json_output);
    }

    const auto entry_name = std::string(part.entry_name());
    const auto bookmarks = part.list_bookmarks();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    exported_template_schema_target target{};
    target.part = part_family;
    target.part_index = part_index;
    target.section_index = section_index;
    target.reference_kind = reference_kind;
    target.resolved_from_section_index = resolved_from_section_index;
    target.linked_to_previous = linked_to_previous;
    target.entry_name = entry_name;

    for (const auto &bookmark : bookmarks) {
        const auto slot_kind = bookmark_kind_to_template_slot_kind(bookmark.kind);
        if (!slot_kind.has_value()) {
            result.skipped_bookmarks.push_back(
                {part_family,
                 part_index,
                 section_index,
                 reference_kind,
                 resolved_from_section_index,
                 linked_to_previous,
                 entry_name,
                 bookmark,
                 "bookmark kind cannot be represented as a template slot"});
            continue;
        }

        featherdoc::template_slot_requirement requirement{};
        requirement.bookmark_name = bookmark.bookmark_name;
        requirement.kind = *slot_kind;
        if (bookmark.occurrence_count > 1U) {
            requirement.min_occurrences = bookmark.occurrence_count;
            requirement.max_occurrences = bookmark.occurrence_count;
        }
        target.requirements.push_back(std::move(requirement));
    }

    const auto content_controls = part.list_content_controls();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    std::vector<featherdoc::template_slot_requirement> content_control_requirements;
    std::unordered_map<std::string, std::size_t> content_control_counts;
    for (const auto &content_control : content_controls) {
        auto source = featherdoc::template_slot_source_kind::content_control_tag;
        std::optional<std::string> selector_value = content_control.tag;
        if (!selector_value.has_value()) {
            source = featherdoc::template_slot_source_kind::content_control_alias;
            selector_value = content_control.alias;
        }
        if (!selector_value.has_value()) {
            continue;
        }

        const auto key = std::string{template_slot_source_json_name(source)} +
                         ":" + *selector_value;
        auto [count_it, inserted] = content_control_counts.emplace(key, 1U);
        if (!inserted) {
            ++count_it->second;
            continue;
        }

        featherdoc::template_slot_requirement requirement{};
        requirement.bookmark_name = *selector_value;
        requirement.kind = content_control_kind_to_template_slot_kind(content_control.kind);
        requirement.source = source;
        content_control_requirements.push_back(std::move(requirement));
    }
    for (auto &requirement : content_control_requirements) {
        const auto key = std::string{template_slot_source_json_name(requirement.source)} +
                         ":" + requirement.bookmark_name;
        if (const auto count_it = content_control_counts.find(key);
            count_it != content_control_counts.end() && count_it->second > 1U) {
            requirement.min_occurrences = count_it->second;
            requirement.max_occurrences = count_it->second;
        }
        target.requirements.push_back(std::move(requirement));
    }

    if (!target.requirements.empty()) {
        result.targets.push_back(std::move(target));
    }

    return true;
}

auto write_exported_template_schema_file(
    const path_type &output_path, const exported_template_schema_result &result,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open schema output path: " + output_path.string();
        return false;
    }

    write_json_exported_template_schema(stream, result);
    if (!stream.good()) {
        error_message = "failed to write schema output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_template_schema_patch_file(const path_type &output_path,
                                      const template_schema_patch_document &patch,
                                      std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message = "failed to open patch output path: " + output_path.string();
        return false;
    }

    write_json_template_schema_patch_document(stream, patch);
    if (!stream.good()) {
        error_message = "failed to write patch output path: " + output_path.string();
        return false;
    }

    return true;
}

auto write_template_schema_patch_review_file(
    const path_type &output_path,
    const featherdoc::template_schema_patch_review_summary &summary,
    std::string &error_message) -> bool {
    std::ofstream stream(output_path, std::ios::binary | std::ios::trunc);
    if (!stream.good()) {
        error_message =
            "failed to open schema patch review output path: " + output_path.string();
        return false;
    }

    featherdoc::write_template_schema_patch_review_json(stream, summary);
    stream << '\n';
    if (!stream.good()) {
        error_message =
            "failed to write schema patch review output path: " + output_path.string();
        return false;
    }

    return true;
}


auto append_exported_section_targets(featherdoc::Document &doc,
                                     exported_template_schema_result &result,
                                     std::string_view command, bool json_output)
    -> bool {
    const auto sections = doc.inspect_sections();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    for (const auto &section : sections.sections) {
        const auto append_if_present =
            [&](validation_part_family part_family,
                featherdoc::section_reference_kind reference_kind,
                bool present) -> bool {
            if (!present) {
                return true;
            }

            const auto template_part =
                part_family == validation_part_family::section_header
                    ? doc.section_header_template(section.index, reference_kind)
                    : doc.section_footer_template(section.index, reference_kind);
            return append_exported_template_schema_target(
                doc, part_family, std::nullopt, section.index, reference_kind,
                std::nullopt, false, template_part, result, command, json_output);
        };

        if (!append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::default_reference,
                               section.header.has_default) ||
            !append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::first_page,
                               section.header.has_first) ||
            !append_if_present(validation_part_family::section_header,
                               featherdoc::section_reference_kind::even_page,
                               section.header.has_even) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::default_reference,
                               section.footer.has_default) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::first_page,
                               section.footer.has_first) ||
            !append_if_present(validation_part_family::section_footer,
                               featherdoc::section_reference_kind::even_page,
                               section.footer.has_even)) {
            return false;
        }
    }

    return true;
}

auto find_related_template_part_by_entry_name(
    featherdoc::Document &doc, section_part_family family, std::string_view entry_name,
    std::optional<std::size_t> &part_index, featherdoc::TemplatePart &part) -> bool {
    const auto total =
        family == section_part_family::header ? doc.header_count() : doc.footer_count();
    for (std::size_t index = 0U; index < total; ++index) {
        auto current =
            family == section_part_family::header ? doc.header_template(index)
                                                  : doc.footer_template(index);
        if (current && current.entry_name() == entry_name) {
            part_index = index;
            part = current;
            return true;
        }
    }

    return false;
}

auto append_exported_resolved_section_targets(featherdoc::Document &doc,
                                              exported_template_schema_result &result,
                                              std::string_view command,
                                              bool json_output) -> bool {
    const auto sections = doc.inspect_sections();
    if (const auto &error_info = doc.last_error(); error_info.code) {
        return report_document_error(command, "inspect", error_info, json_output);
    }

    for (const auto &section : sections.sections) {
        const auto append_if_resolved =
            [&](validation_part_family part_family, section_part_family part_group,
                featherdoc::section_reference_kind reference_kind,
                const std::optional<std::string> &resolved_entry_name,
                const std::optional<std::size_t> &resolved_section_index) -> bool {
            if (!resolved_entry_name.has_value() || !resolved_section_index.has_value()) {
                return true;
            }

            std::optional<std::size_t> part_index;
            featherdoc::TemplatePart part;
            if (!find_related_template_part_by_entry_name(doc, part_group,
                                                          *resolved_entry_name,
                                                          part_index, part)) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "failed to resolve related part by entry name during schema export";
                error_info.entry_name = *resolved_entry_name;
                return report_operation_failure(command, "inspect",
                                                "failed to resolve related part",
                                                error_info, json_output);
            }

            return append_exported_template_schema_target(
                doc, part_family, std::nullopt, section.index, reference_kind,
                *resolved_section_index, *resolved_section_index != section.index, part,
                result, command, json_output);
        };

        if (!append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::default_reference,
                                section.header.resolved_default_entry_name,
                                section.header.resolved_default_section_index) ||
            !append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::first_page,
                                section.header.resolved_first_entry_name,
                                section.header.resolved_first_section_index) ||
            !append_if_resolved(validation_part_family::section_header,
                                section_part_family::header,
                                featherdoc::section_reference_kind::even_page,
                                section.header.resolved_even_entry_name,
                                section.header.resolved_even_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::default_reference,
                                section.footer.resolved_default_entry_name,
                                section.footer.resolved_default_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::first_page,
                                section.footer.resolved_first_entry_name,
                                section.footer.resolved_first_section_index) ||
            !append_if_resolved(validation_part_family::section_footer,
                                section_part_family::footer,
                                featherdoc::section_reference_kind::even_page,
                                section.footer.resolved_even_entry_name,
                                section.footer.resolved_even_section_index)) {
            return false;
        }
    }

    return true;
}

auto build_exported_template_schema(featherdoc::Document &doc,
                                    bool section_targets,
                                    bool resolved_section_targets,
                                    exported_template_schema_result &result,
                                    std::string_view command, bool json_output)
    -> bool {
    result = {};
    if (!append_exported_template_schema_target(
            doc, validation_part_family::body, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, false, doc.body_template(), result, command,
            json_output)) {
        return false;
    }

    if (resolved_section_targets) {
        if (!append_exported_resolved_section_targets(doc, result, command, json_output)) {
            return false;
        }
    } else if (section_targets) {
        if (!append_exported_section_targets(doc, result, command, json_output)) {
            return false;
        }
    } else {
        for (std::size_t index = 0U; index < doc.header_count(); ++index) {
            if (!append_exported_template_schema_target(
                    doc, validation_part_family::header, index, std::nullopt,
                    std::nullopt, std::nullopt, false, doc.header_template(index),
                    result, command, json_output)) {
                return false;
            }
        }

        for (std::size_t index = 0U; index < doc.footer_count(); ++index) {
            if (!append_exported_template_schema_target(
                    doc, validation_part_family::footer, index, std::nullopt,
                    std::nullopt, std::nullopt, false, doc.footer_template(index),
                    result, command, json_output)) {
                return false;
            }
        }
    }

    if (!result.targets.empty()) {
        return true;
    }

    featherdoc::document_error_info error_info{};
    error_info.code = std::make_error_code(std::errc::invalid_argument);
    error_info.detail = "document does not contain any exportable template bookmarks";
    return report_operation_failure(command, "inspect",
                                    "failed to export template schema", error_info,
                                    json_output);
}


} // namespace

auto is_template_schema_command(std::string_view command) -> bool {
    return command == "validate-template" ||
           command == "validate-template-schema" ||
           command == "export-template-schema" ||
           command == "normalize-template-schema" ||
           command == "lint-template-schema" ||
           command == "repair-template-schema" ||
           command == "merge-template-schema" ||
           command == "patch-template-schema" ||
           command == "preview-template-schema-patch" ||
           command == "build-template-schema-patch" ||
           command == "diff-template-schema" ||
           command == "check-template-schema";
}

auto run_template_schema_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {    if (command == "validate-template") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "validate-template expects an input path and validation options",
                json_output);
            return 2;
        }

        validate_template_options options;
        std::string error_message;
        if (!parse_validate_template_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_template_part(doc, options.part, options.part_index,
                                  options.section_index, options.reference_kind,
                                  selected, error_message)) {
            report_operation_failure(command, "inspect", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        const auto result = selected.part.validate_template(options.requirements);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, options.json_output);
            return 1;
        }

        print_template_validation_result(selected, result, options.json_output);
        return 0;
    }

    if (command == "validate-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "validate-template-schema expects an input path and schema validation options",
                json_output);
            return 2;
        }

        validate_template_schema_options options;
        std::string error_message;
        if (!parse_validate_template_schema_options(arguments, 2U, options,
                                                    error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        for (const auto &schema_file : options.schema_files) {
            if (!append_validate_template_schema_file_targets(
                    schema_file, options.targets, error_message)) {
                print_parse_error(command, error_message, json_output);
                return 2;
            }
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::template_schema schema;
        std::size_t entry_count = 0U;
        for (const auto &target : options.targets) {
            entry_count += target.requirements.size();
        }
        schema.entries.reserve(entry_count);

        for (const auto &target : options.targets) {
            featherdoc::template_schema_part_selector selector{};
            selector.part = to_template_schema_part_kind(target.part);
            selector.part_index = target.part_index;
            selector.section_index = target.section_index;
            selector.reference_kind = target.reference_kind;

            for (const auto &requirement : target.requirements) {
                schema.entries.push_back({selector, requirement});
            }
        }

        const auto result = doc.validate_template_schema(schema);
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info, options.json_output);
            return 1;
        }

        print_template_schema_validation_result(result, options.json_output);
        return 0;
    }

    if (command == "export-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "export-template-schema expects an input path",
                              json_output);
            return 2;
        }

        export_template_schema_options options;
        std::string error_message;
        if (!parse_export_template_schema_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        exported_template_schema_result result;
        if (!build_exported_template_schema(
                doc, options.section_targets, options.resolved_section_targets, result,
                command, options.json_output)) {
            return 1;
        }

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write template schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_exported_template_schema_summary(result, options.output_path,
                                               options.json_output);
        return 0;
    }

    if (command == "normalize-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "normalize-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        normalize_template_schema_options options;
        std::string error_message;
        if (!parse_normalize_template_schema_options(arguments, 2U, options,
                                                     error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write normalized schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_normalized_template_schema_summary(result, options.output_path,
                                                 options.json_output);
        return 0;
    }

    if (command == "lint-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "lint-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        lint_template_schema_options options;
        std::string error_message;
        if (!parse_lint_template_schema_options(arguments, 2U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        const auto lint = lint_template_schema(result);
        print_linted_template_schema_result(lint, options.json_output);
        if (!lint.clean()) {
            return 1;
        }
        return 0;
    }

    if (command == "repair-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U || arguments[1].starts_with("--")) {
            print_parse_error(command, "repair-template-schema expects a schema path",
                              json_output);
            return 2;
        }

        repair_template_schema_options options;
        std::string error_message;
        if (!parse_repair_template_schema_options(arguments, 2U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result input;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), input,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result result;
        repaired_template_schema_summary summary{};
        repair_template_schema_result(input, result, summary);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write repaired schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_repaired_template_schema_summary(result, summary, options.output_path,
                                               options.json_output);
        return 0;
    }

    if (command == "merge-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "merge-template-schema expects at least two schema paths",
                              json_output);
            return 2;
        }

        merge_template_schema_options options;
        std::string error_message;
        if (!parse_merge_template_schema_options(arguments, 1U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        merged_template_schema_summary summary;
        for (const auto &schema_path : options.schema_paths) {
            exported_template_schema_result input;
            if (!read_template_schema_file(schema_path, input, error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            merge_template_schema_result(result, input, summary);
        }
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write merged schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_merged_template_schema_summary(result, summary, options.output_path,
                                             options.json_output);
        return 0;
    }

    if (command == "patch-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              "patch-template-schema expects a schema path and "
                              "--patch-file <path>",
                              json_output);
            return 2;
        }

        patch_template_schema_options options;
        std::string error_message;
        if (!parse_patch_template_schema_options(arguments, 2U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), result,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        template_schema_patch_document patch;
        if (!read_template_schema_patch_file(*options.patch_path, patch,
                                             error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        patched_template_schema_summary summary;
        apply_template_schema_patch(result, patch, summary);
        normalize_template_schema_result(result);

        if (!options.output_path.has_value()) {
            write_json_exported_template_schema(std::cout, result);
            return 0;
        }

        if (!write_exported_template_schema_file(*options.output_path, result,
                                                 error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write patched schema output",
                                     error_info, options.json_output);
            return 1;
        }

        print_patched_template_schema_summary(result, summary, options.output_path,
                                              options.json_output);
        return 0;
    }

    if (command == "preview-template-schema-patch") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U || arguments[1].starts_with("--")) {
            print_parse_error(command,
                              "preview-template-schema-patch expects a schema path "
                              "and --patch-file <path> or <right-schema.json>",
                              json_output);
            return 2;
        }

        preview_template_schema_patch_options options;
        std::string error_message;
        if (!parse_preview_template_schema_patch_options(arguments, 2U, options,
                                                         error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left_result;
        if (!read_template_schema_file(path_type(std::string(arguments[1])),
                                       left_result, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(left_result);
        if (has_template_schema_resolved_target_metadata(left_result)) {
            print_parse_error(command,
                              "preview-template-schema-patch does not support "
                              "schemas with resolved-section target metadata",
                              options.json_output);
            return 2;
        }

        previewed_template_schema_patch_summary summary{};
        summary.left_slot_count = left_result.slot_count();
        summary.review_json_path = options.review_json_path;
        const auto left_schema = to_template_schema(left_result);

        if (options.patch_path.has_value()) {
            template_schema_patch_document patch_document;
            if (!read_template_schema_patch_file(*options.patch_path, patch_document,
                                                 error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            if (has_template_schema_resolved_target_metadata(patch_document)) {
                print_parse_error(command,
                                  "preview-template-schema-patch does not support "
                                  "patches with resolved-section target metadata",
                                  options.json_output);
                return 2;
            }

            const auto patch = to_template_schema_patch(patch_document);
            summary.upsert_slot_count = patch.upsert_slots.size();
            summary.remove_target_count = patch.remove_targets.size();
            summary.remove_slot_count = patch.remove_slots.size();
            summary.rename_slot_count = patch.rename_slots.size();
            summary.update_slot_count = patch.update_slots.size();
            summary.applied =
                featherdoc::preview_template_schema_patch(left_schema, patch);
            if (options.review_json_path.has_value()) {
                const auto review =
                    featherdoc::make_template_schema_patch_review_summary(
                        left_schema, patch);
                if (!write_template_schema_patch_review_file(
                        *options.review_json_path, review, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(
                        command, "review-json",
                        "failed to write schema patch review output", error_info,
                        options.json_output);
                    return 1;
                }
            }
            if (options.output_patch_path.has_value()) {
                if (!write_template_schema_patch_file(*options.output_patch_path,
                                                      patch_document, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(command, "output-patch",
                                             "failed to write previewed patch output",
                                             error_info, options.json_output);
                    return 1;
                }
            }
        } else {
            exported_template_schema_result right_result;
            if (!read_template_schema_file(*options.right_schema_path, right_result,
                                           error_message)) {
                print_parse_error(command, error_message, options.json_output);
                return 2;
            }
            normalize_template_schema_result(right_result);
            if (has_template_schema_resolved_target_metadata(right_result)) {
                print_parse_error(command,
                                  "preview-template-schema-patch does not support "
                                  "schemas with resolved-section target metadata",
                                  options.json_output);
                return 2;
            }

            summary.right_slot_count = right_result.slot_count();
            const auto diff = diff_template_schema_results(left_result, right_result);
            built_template_schema_patch_summary built_summary{};
            const auto patch_document =
                build_template_schema_patch_document(diff, built_summary);
            const auto patch = to_template_schema_patch(patch_document);
            summary.upsert_slot_count = patch.upsert_slots.size();
            summary.remove_target_count = patch.remove_targets.size();
            summary.remove_slot_count = patch.remove_slots.size();
            summary.rename_slot_count = patch.rename_slots.size();
            summary.update_slot_count = patch.update_slots.size();
            summary.applied =
                featherdoc::preview_template_schema_patch(left_schema, patch);
            if (options.review_json_path.has_value()) {
                auto review = featherdoc::make_template_schema_patch_review_summary(
                    left_schema, patch);
                review.generated_slot_count = right_result.slot_count();
                if (!write_template_schema_patch_review_file(
                        *options.review_json_path, review, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(
                        command, "review-json",
                        "failed to write schema patch review output", error_info,
                        options.json_output);
                    return 1;
                }
            }
            if (options.output_patch_path.has_value()) {
                if (!write_template_schema_patch_file(*options.output_patch_path,
                                                      patch_document, error_message)) {
                    featherdoc::document_error_info error_info{};
                    error_info.code = std::make_error_code(std::errc::io_error);
                    error_info.detail = std::move(error_message);
                    report_operation_failure(command, "output-patch",
                                             "failed to write previewed generated patch output",
                                             error_info, options.json_output);
                    return 1;
                }
            }
        }

        summary.output_patch_path = options.output_patch_path;
        print_previewed_template_schema_patch_summary(summary, options.json_output);
        return 0;
    }

    if (command == "build-template-schema-patch") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U || arguments[1].starts_with("--") ||
            arguments[2].starts_with("--")) {
            print_parse_error(
                command,
                "build-template-schema-patch expects left and right schema paths",
                json_output);
            return 2;
        }

        build_template_schema_patch_options options;
        std::string error_message;
        if (!parse_build_template_schema_patch_options(arguments, 3U, options,
                                                       error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), left,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result right;
        if (!read_template_schema_file(path_type(std::string(arguments[2])), right,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        normalize_template_schema_result(left);
        normalize_template_schema_result(right);
        const auto diff = diff_template_schema_results(left, right);
        built_template_schema_patch_summary summary{};
        const auto patch = build_template_schema_patch_document(diff, summary);

        if (options.review_json_path.has_value()) {
            const auto left_schema = to_template_schema(left);
            const auto patch_model = to_template_schema_patch(patch);
            auto review = featherdoc::make_template_schema_patch_review_summary(
                left_schema, patch_model);
            review.generated_slot_count = right.slot_count();
            if (!write_template_schema_patch_review_file(
                    *options.review_json_path, review, error_message)) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::io_error);
                error_info.detail = std::move(error_message);
                report_operation_failure(
                    command, "review-json",
                    "failed to write schema patch review output", error_info,
                    options.json_output);
                return 1;
            }
        }

        if (!options.output_path.has_value()) {
            write_json_template_schema_patch_document(std::cout, patch);
            return 0;
        }

        if (!write_template_schema_patch_file(*options.output_path, patch,
                                              error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write generated patch output",
                                     error_info, options.json_output);
            return 1;
        }

        print_built_template_schema_patch_summary(
            summary, options.output_path, options.review_json_path,
            options.json_output);
        return 0;
    }

    if (command == "diff-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "diff-template-schema expects left and right schema paths",
                              json_output);
            return 2;
        }

        diff_template_schema_options options;
        std::string error_message;
        if (!parse_diff_template_schema_options(arguments, 3U, options,
                                                error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result left;
        if (!read_template_schema_file(path_type(std::string(arguments[1])), left,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        exported_template_schema_result right;
        if (!read_template_schema_file(path_type(std::string(arguments[2])), right,
                                       error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }

        normalize_template_schema_result(left);
        normalize_template_schema_result(right);
        const auto result = diff_template_schema_results(left, right);
        print_template_schema_diff_result(result, options.json_output);
        if (options.fail_on_diff && !result.equal()) {
            return 1;
        }
        return 0;
    }

    if (command == "check-template-schema") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "check-template-schema expects an input path and --schema-file <path>",
                json_output);
            return 2;
        }

        check_template_schema_options options;
        std::string error_message;
        if (!parse_check_template_schema_options(arguments, 2U, options,
                                                 error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        exported_template_schema_result baseline;
        if (!read_template_schema_file(*options.schema_path, baseline, error_message)) {
            print_parse_error(command, error_message, options.json_output);
            return 2;
        }
        normalize_template_schema_result(baseline);

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        exported_template_schema_result generated;
        if (!build_exported_template_schema(
                doc, options.section_targets, options.resolved_section_targets,
                generated, command, options.json_output)) {
            return 1;
        }
        normalize_template_schema_result(generated);

        if (options.output_path.has_value() &&
            !write_exported_template_schema_file(*options.output_path, generated,
                                                error_message)) {
            featherdoc::document_error_info error_info{};
            error_info.code = std::make_error_code(std::errc::io_error);
            error_info.detail = std::move(error_message);
            report_operation_failure(command, "output",
                                     "failed to write generated schema output",
                                     error_info, options.json_output);
            return 1;
        }

        const auto result = diff_template_schema_results(baseline, generated);
        print_checked_template_schema_result(*options.schema_path, result,
                                             options.output_path,
                                             options.json_output);
        if (!result.equal()) {
            return 1;
        }
        return 0;
    }

    return 2;
}

} // namespace featherdoc_cli
