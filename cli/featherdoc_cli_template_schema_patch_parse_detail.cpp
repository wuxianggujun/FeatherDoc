#include "featherdoc_cli_template_schema_patch_parse_detail.hpp"

#include "featherdoc_cli_json_parse.hpp"
#include "featherdoc_cli_validation_part.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace featherdoc_cli {
namespace detail {

void rewrite_template_schema_patch_error(std::string &error_message) {
    constexpr std::string_view schema_prefix = "JSON schema ";
    constexpr std::string_view patch_prefix = "JSON schema patch ";
    std::size_t position = 0U;
    while ((position = error_message.find(schema_prefix, position)) !=
           std::string::npos) {
        error_message.replace(position, schema_prefix.size(), patch_prefix);
        position += patch_prefix.size();
    }
}

auto finalize_template_schema_patch_selector(
    const std::optional<std::string> &part_value,
    const std::optional<std::size_t> &index_value,
    const std::optional<std::size_t> &part_index_value,
    const std::optional<std::size_t> &section_value,
    const std::optional<featherdoc::section_reference_kind> &kind_value,
    const std::optional<std::size_t> &resolved_from_section_value,
    const std::optional<bool> &linked_to_previous_value,
    template_schema_patch_target_selector &selector, std::string &error_message)
    -> bool {
    if (!part_value.has_value()) {
        error_message = "JSON schema patch selector must contain 'part'";
        return false;
    }

    selector = {};
    if (!parse_validation_part(*part_value, selector.part)) {
        error_message =
            "JSON schema patch member 'part' must be one of "
            "'body', 'header', 'footer', 'section-header', or 'section-footer'";
        return false;
    }

    std::optional<std::size_t> resolved_part_index;
    if (!resolve_json_patch_index_member(index_value, part_index_value, "index",
                                         "part_index", resolved_part_index,
                                         error_message)) {
        return false;
    }

    selector.part_index = resolved_part_index;
    selector.section_index = section_value;
    selector.reference_kind = kind_value;
    selector.resolved_from_section_index = resolved_from_section_value;
    selector.linked_to_previous = linked_to_previous_value.value_or(false);

    return validate_template_part_selection(selector.part, selector.part_index,
                                            selector.section_index,
                                            selector.reference_kind.has_value(),
                                            "schema patch selection",
                                            error_message);
}

} // namespace detail
} // namespace featherdoc_cli
