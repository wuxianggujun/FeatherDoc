#include "featherdoc_cli_domain_parse.hpp"

#include "featherdoc_cli_json_parse.hpp"

#include <string>

namespace featherdoc_cli {

auto parse_page_orientation(std::string_view text,
                            featherdoc::page_orientation &orientation) -> bool {
    if (text == "portrait") {
        orientation = featherdoc::page_orientation::portrait;
        return true;
    }
    if (text == "landscape") {
        orientation = featherdoc::page_orientation::landscape;
        return true;
    }

    return false;
}

auto parse_list_kind(std::string_view text, featherdoc::list_kind &kind)
    -> bool {
    if (text == "bullet") {
        kind = featherdoc::list_kind::bullet;
        return true;
    }
    if (text == "decimal") {
        kind = featherdoc::list_kind::decimal;
        return true;
    }

    return false;
}

auto parse_reference_kind(std::string_view text,
                          featherdoc::section_reference_kind &reference_kind)
    -> bool {
    if (text == "default") {
        reference_kind = featherdoc::section_reference_kind::default_reference;
        return true;
    }
    if (text == "first") {
        reference_kind = featherdoc::section_reference_kind::first_page;
        return true;
    }
    if (text == "even") {
        reference_kind = featherdoc::section_reference_kind::even_page;
        return true;
    }

    return false;
}

auto parse_json_patch_reference_kind_value(
    std::string_view text, std::size_t &index,
    featherdoc::section_reference_kind &reference_kind,
    std::string &error_message) -> bool {
    skip_json_patch_whitespace(text, index);
    std::string token;
    if (!parse_json_patch_string(text, index, token, error_message)) {
        return false;
    }

    if (!parse_reference_kind(token, reference_kind)) {
        error_message =
            "JSON patch member 'kind' must be 'default', 'first', or 'even'";
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
