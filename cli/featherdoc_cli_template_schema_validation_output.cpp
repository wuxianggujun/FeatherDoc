#include "featherdoc_cli_template_schema_validation_output.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {


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

} // namespace featherdoc_cli
