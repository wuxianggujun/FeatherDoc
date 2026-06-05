#include "featherdoc_cli_template_slot_parse.hpp"

#include "featherdoc_cli_parse.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc_cli {

auto parse_template_slot_kind(std::string_view text,
                              featherdoc::template_slot_kind &kind) -> bool {
    if (text == "text") {
        kind = featherdoc::template_slot_kind::text;
        return true;
    }
    if (text == "table_rows" || text == "table-rows") {
        kind = featherdoc::template_slot_kind::table_rows;
        return true;
    }
    if (text == "table") {
        kind = featherdoc::template_slot_kind::table;
        return true;
    }
    if (text == "image") {
        kind = featherdoc::template_slot_kind::image;
        return true;
    }
    if (text == "floating_image" || text == "floating-image") {
        kind = featherdoc::template_slot_kind::floating_image;
        return true;
    }
    if (text == "block") {
        kind = featherdoc::template_slot_kind::block;
        return true;
    }

    return false;
}

auto parse_template_slot_requirement(
    std::string_view text, featherdoc::template_slot_requirement &requirement,
    std::string &error_message) -> bool {
    const auto first_separator = text.find(':');
    if (first_separator == std::string_view::npos || first_separator == 0U) {
        error_message =
            "invalid --slot value: expected <bookmark>:<kind>"
            "[:required|optional][:count=<n>|min=<n>|max=<n>...]";
        return false;
    }

    std::vector<std::string_view> segments;
    std::size_t segment_start = 0U;
    while (segment_start <= text.size()) {
        const auto separator = text.find(':', segment_start);
        if (separator == std::string_view::npos) {
            segments.push_back(text.substr(segment_start));
            break;
        }

        segments.push_back(text.substr(segment_start, separator - segment_start));
        segment_start = separator + 1U;
    }

    if (segments.size() < 2U || segments[0].empty()) {
        error_message =
            "invalid --slot value: expected <bookmark>:<kind>"
            "[:required|optional][:count=<n>|min=<n>|max=<n>...]";
        return false;
    }

    auto slot_name = segments[0];
    auto source = featherdoc::template_slot_source_kind::bookmark;
    const auto parse_prefixed_slot =
        [&](std::string_view prefix,
            featherdoc::template_slot_source_kind parsed_source) {
            if (slot_name.starts_with(prefix)) {
                slot_name.remove_prefix(prefix.size());
                source = parsed_source;
                return true;
            }
            return false;
        };
    (void)(parse_prefixed_slot(
               "content_control_tag=",
               featherdoc::template_slot_source_kind::content_control_tag) ||
           parse_prefixed_slot(
               "content-control-tag=",
               featherdoc::template_slot_source_kind::content_control_tag) ||
           parse_prefixed_slot(
               "cc-tag=",
               featherdoc::template_slot_source_kind::content_control_tag) ||
           parse_prefixed_slot(
               "content_control_alias=",
               featherdoc::template_slot_source_kind::content_control_alias) ||
           parse_prefixed_slot(
               "content-control-alias=",
               featherdoc::template_slot_source_kind::content_control_alias) ||
           parse_prefixed_slot(
               "cc-alias=",
               featherdoc::template_slot_source_kind::content_control_alias));
    if (slot_name.empty()) {
        error_message = "invalid --slot value: slot name must not be empty";
        return false;
    }

    const auto kind_text = segments[1];
    if (kind_text.empty()) {
        error_message =
            "invalid --slot value: expected <bookmark>:<kind>"
            "[:required|optional][:count=<n>|min=<n>|max=<n>...]";
        return false;
    }

    featherdoc::template_slot_kind kind;
    if (!parse_template_slot_kind(kind_text, kind)) {
        error_message = "invalid --slot kind: " + std::string(kind_text);
        return false;
    }

    bool required = true;
    bool requirement_seen = false;
    bool count_seen = false;
    bool min_seen = false;
    bool max_seen = false;
    std::optional<std::size_t> min_occurrences;
    std::optional<std::size_t> max_occurrences;
    for (std::size_t index = 2U; index < segments.size(); ++index) {
        const auto segment = segments[index];
        if (segment.empty()) {
            error_message = "invalid --slot segment: value must not be empty";
            return false;
        }

        if (segment == "required" || segment == "optional") {
            if (requirement_seen) {
                error_message = "duplicate --slot requirement segment: " +
                                std::string(segment);
                return false;
            }

            required = segment == "required";
            requirement_seen = true;
            continue;
        }

        const auto equals = segment.find('=');
        if (equals == std::string_view::npos || equals == 0U ||
            equals + 1U >= segment.size()) {
            error_message = "invalid --slot segment: " + std::string(segment);
            return false;
        }

        const auto key = segment.substr(0U, equals);
        const auto value_text = segment.substr(equals + 1U);
        std::size_t value = 0U;
        if (!parse_index(value_text, value)) {
            error_message = "invalid --slot occurrence value: " +
                            std::string(value_text);
            return false;
        }

        if (key == "count") {
            if (count_seen || min_seen || max_seen) {
                error_message = "conflicting --slot occurrence segment: " +
                                std::string(segment);
                return false;
            }

            min_occurrences = value;
            max_occurrences = value;
            count_seen = true;
            continue;
        }

        if (key == "min") {
            if (count_seen || min_seen) {
                error_message = "duplicate --slot min occurrence segment";
                return false;
            }

            min_occurrences = value;
            min_seen = true;
            continue;
        }

        if (key == "max") {
            if (count_seen || max_seen) {
                error_message = "duplicate --slot max occurrence segment";
                return false;
            }

            max_occurrences = value;
            max_seen = true;
            continue;
        }

        error_message = "invalid --slot segment: " + std::string(segment);
        return false;
    }

    if (min_occurrences.has_value() && max_occurrences.has_value() &&
        *max_occurrences < *min_occurrences) {
        error_message =
            "invalid --slot occurrence range: max must be greater than or equal to min";
        return false;
    }

    requirement.bookmark_name = std::string(slot_name);
    requirement.kind = kind;
    requirement.required = required;
    requirement.min_occurrences = min_occurrences;
    requirement.max_occurrences = max_occurrences;
    requirement.source = source;
    return true;
}

} // namespace featherdoc_cli
