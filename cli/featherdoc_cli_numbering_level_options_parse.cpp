#include "featherdoc_cli_numbering_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

auto parse_numbering_level_definition(
    std::string_view text, featherdoc::numbering_level_definition &definition,
    std::string &error_message) -> bool {
    const auto first_separator = text.find(':');
    const auto second_separator =
        first_separator == std::string_view::npos
            ? std::string_view::npos
            : text.find(':', first_separator + 1U);
    const auto third_separator =
        second_separator == std::string_view::npos
            ? std::string_view::npos
            : text.find(':', second_separator + 1U);
    if (first_separator == std::string_view::npos ||
        second_separator == std::string_view::npos ||
        third_separator == std::string_view::npos) {
        error_message =
            "invalid --numbering-level value: expected "
            "<level>:<kind>:<start>:<text-pattern>";
        return false;
    }

    const auto level_text = text.substr(0U, first_separator);
    const auto kind_text =
        text.substr(first_separator + 1U, second_separator - first_separator - 1U);
    const auto start_text =
        text.substr(second_separator + 1U, third_separator - second_separator - 1U);
    const auto text_pattern = text.substr(third_separator + 1U);
    if (text_pattern.empty()) {
        error_message =
            "invalid --numbering-level value: text pattern must not be empty";
        return false;
    }

    std::uint32_t level = 0U;
    if (!parse_uint32(level_text, level)) {
        error_message = "invalid numbering level: " + std::string(level_text);
        return false;
    }

    featherdoc::list_kind kind{};
    if (!parse_list_kind(kind_text, kind)) {
        error_message = "invalid numbering kind: " + std::string(kind_text);
        return false;
    }

    std::uint32_t start = 0U;
    if (!parse_uint32(start_text, start)) {
        error_message = "invalid numbering start: " + std::string(start_text);
        return false;
    }

    definition.kind = kind;
    definition.start = start;
    definition.level = level;
    definition.text_pattern = std::string(text_pattern);
    return true;
}

auto parse_paragraph_style_numbering_link(
    std::string_view text, featherdoc::paragraph_style_numbering_link &style_link,
    std::string &error_message) -> bool {
    const auto separator = text.find(':');
    if (separator == std::string_view::npos) {
        error_message =
            "invalid --style-link value: expected <style-id>:<level>";
        return false;
    }

    const auto style_id = text.substr(0U, separator);
    const auto level_text = text.substr(separator + 1U);
    if (style_id.empty()) {
        error_message = "invalid --style-link value: style id must not be empty";
        return false;
    }
    if (level_text.empty()) {
        error_message = "invalid --style-link value: level must not be empty";
        return false;
    }

    std::uint32_t level = 0U;
    if (!parse_uint32(level_text, level)) {
        error_message = "invalid style link level: " + std::string(level_text);
        return false;
    }

    style_link.style_id = std::string(style_id);
    style_link.level = level;
    return true;
}

} // namespace featherdoc_cli
