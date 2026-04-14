#include <charconv>
#include <featherdoc.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {
using path_type = std::filesystem::path;

struct command_options {
    bool inherit_header_footer = true;
    std::optional<path_type> output_path;
    bool json_output = false;
};

struct inspect_options {
    bool json_output = false;
};

struct inspect_styles_options {
    std::optional<std::string> style_id;
    bool usage_output = false;
    bool json_output = false;
};

struct inspect_numbering_options {
    std::optional<std::uint32_t> definition_id;
    std::optional<std::uint32_t> instance_id;
    bool json_output = false;
};

struct set_paragraph_style_numbering_options {
    std::optional<std::string> definition_name;
    std::vector<featherdoc::numbering_level_definition> levels;
    std::optional<std::uint32_t> style_level;
    std::optional<path_type> output_path;
    bool json_output = false;
};

struct clear_paragraph_style_numbering_options {
    std::optional<path_type> output_path;
    bool json_output = false;
};

struct inspect_page_setup_options {
    std::optional<std::size_t> section_index;
    bool json_output = false;
};

struct set_section_page_setup_options {
    std::optional<featherdoc::page_orientation> orientation;
    std::optional<std::uint32_t> width_twips;
    std::optional<std::uint32_t> height_twips;
    std::optional<std::uint32_t> margin_top_twips;
    std::optional<std::uint32_t> margin_bottom_twips;
    std::optional<std::uint32_t> margin_left_twips;
    std::optional<std::uint32_t> margin_right_twips;
    std::optional<std::uint32_t> margin_header_twips;
    std::optional<std::uint32_t> margin_footer_twips;
    std::optional<std::uint32_t> page_number_start;
    bool clear_page_number_start = false;
    std::optional<path_type> output_path;
    bool json_output = false;
};

struct section_text_options {
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<path_type> output_path;
    std::optional<std::string> text;
    std::optional<path_type> text_file;
    bool json_output = false;
};

enum class section_part_family {
    header,
    footer,
};

enum class validation_part_family {
    body,
    header,
    footer,
    section_header,
    section_footer,
};

struct inspect_bookmarks_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<std::string> bookmark_name;
    bool has_kind = false;
    bool json_output = false;
};

struct inspected_part_reference {
    std::size_t section_index = 0;
    std::string kind;
};

struct inspected_part_entry {
    std::string relationship_id;
    std::string entry_name;
    std::vector<inspected_part_reference> references;
    std::vector<std::string> paragraphs;
};

struct validate_template_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::vector<featherdoc::template_slot_requirement> requirements;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct append_field_options {
    validation_part_family part = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    featherdoc::section_reference_kind reference_kind =
        featherdoc::section_reference_kind::default_reference;
    std::optional<path_type> output_path;
    bool has_part = false;
    bool has_kind = false;
    bool json_output = false;
};

struct selected_template_part {
    featherdoc::TemplatePart part;
    validation_part_family family = validation_part_family::body;
    std::optional<std::size_t> part_index;
    std::optional<std::size_t> section_index;
    std::optional<featherdoc::section_reference_kind> reference_kind;
};

enum class zip_entry_read_status {
    missing,
    read_failed,
    ok,
};

void print_usage(std::ostream &stream) {
    stream
        << "Usage:\n"
        << "  featherdoc_cli inspect-sections <input.docx>\n"
        << "    [--json]\n"
        << "  featherdoc_cli inspect-styles <input.docx>"
           " [--style <style-id>] [--usage] [--json]\n"
        << "  featherdoc_cli inspect-numbering <input.docx>"
           " [--definition <id>] [--instance <num-id>] [--json]\n"
        << "  featherdoc_cli set-paragraph-style-numbering <input.docx> <style-id>"
           " --definition-name <name>"
           " --numbering-level <level>:<kind>:<start>:<text-pattern>"
           " [--numbering-level ...] [--style-level <level>]"
           " [--output <path>] [--json]\n"
        << "  featherdoc_cli clear-paragraph-style-numbering <input.docx> <style-id>"
           " [--output <path>] [--json]\n"
        << "  featherdoc_cli inspect-page-setup <input.docx>"
           " [--section <section-index>] [--json]\n"
        << "  featherdoc_cli set-section-page-setup <input.docx> <section-index>"
           " [--orientation portrait|landscape] [--width <twips>]"
           " [--height <twips>] [--margin-top <twips>]"
           " [--margin-bottom <twips>] [--margin-left <twips>]"
           " [--margin-right <twips>] [--margin-header <twips>]"
           " [--margin-footer <twips>] [--page-number-start <n> |"
           " --clear-page-number-start] [--output <path>] [--json]\n"
        << "  featherdoc_cli inspect-bookmarks <input.docx>"
           " [--part body|header|footer|section-header|section-footer]"
           " [--index <part-index>] [--section <section-index>]"
           " [--kind default|first|even] [--bookmark <name>] [--json]\n"
        << "  featherdoc_cli insert-section <input.docx> <section-index>"
           " [--no-inherit] [--output <path>] [--json]\n"
        << "  featherdoc_cli remove-section <input.docx> <section-index>"
           " [--output <path>] [--json]\n"
        << "  featherdoc_cli move-section <input.docx> <source-index>"
           " <target-index> [--output <path>] [--json]\n"
        << "  featherdoc_cli copy-section-layout <input.docx> <source-index>"
           " <target-index> [--output <path>] [--json]\n"
        << "  featherdoc_cli inspect-header-parts <input.docx> [--json]\n"
        << "  featherdoc_cli inspect-footer-parts <input.docx> [--json]\n"
        << "  featherdoc_cli assign-section-header <input.docx> <section-index>"
           " <header-index> [--kind default|first|even] [--output <path>]"
           " [--json]\n"
        << "  featherdoc_cli assign-section-footer <input.docx> <section-index>"
           " <footer-index> [--kind default|first|even] [--output <path>]"
           " [--json]\n"
        << "  featherdoc_cli remove-section-header <input.docx> <section-index>"
           " [--kind default|first|even] [--output <path>] [--json]\n"
        << "  featherdoc_cli remove-section-footer <input.docx> <section-index>"
           " [--kind default|first|even] [--output <path>] [--json]\n"
        << "  featherdoc_cli remove-header-part <input.docx> <header-index>"
           " [--output <path>] [--json]\n"
        << "  featherdoc_cli remove-footer-part <input.docx> <footer-index>"
           " [--output <path>] [--json]\n"
        << "  featherdoc_cli show-section-header <input.docx> <section-index>"
           " [--kind default|first|even] [--json]\n"
        << "  featherdoc_cli show-section-footer <input.docx> <section-index>"
           " [--kind default|first|even] [--json]\n"
        << "  featherdoc_cli set-section-header <input.docx> <section-index>"
           " (--text <text> | --text-file <path>)"
           " [--kind default|first|even] [--output <path>] [--json]\n"
        << "  featherdoc_cli set-section-footer <input.docx> <section-index>"
           " (--text <text> | --text-file <path>)"
           " [--kind default|first|even] [--output <path>] [--json]\n"
        << "  featherdoc_cli append-page-number-field <input.docx>"
           " --part body|header|footer|section-header|section-footer"
           " [--index <part-index>] [--section <section-index>]"
           " [--kind default|first|even] [--output <path>] [--json]\n"
        << "  featherdoc_cli append-total-pages-field <input.docx>"
           " --part body|header|footer|section-header|section-footer"
           " [--index <part-index>] [--section <section-index>]"
           " [--kind default|first|even] [--output <path>] [--json]\n"
        << "  featherdoc_cli validate-template <input.docx>"
           " --part body|header|footer|section-header|section-footer"
           " [--index <part-index>] [--section <section-index>]"
           " [--kind default|first|even]"
           " --slot <bookmark>:<kind>[:required|optional] [--slot ...] [--json]\n";
}

void print_error_info(const featherdoc::document_error_info &error_info) {
    const auto &error = error_info.code;
    std::cerr << (error ? error.message() : std::string{"operation failed"});
    if (!error_info.detail.empty()) {
        std::cerr << ": " << error_info.detail;
    }
    if (!error_info.entry_name.empty()) {
        std::cerr << " [entry=" << error_info.entry_name << ']';
    }
    if (error_info.xml_offset.has_value()) {
        std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
    }
    std::cerr << '\n';
}

void print_parse_error(const std::string &message) {
    std::cerr << message << '\n';
    print_usage(std::cerr);
}

auto has_json_flag(const std::vector<std::string_view> &arguments) -> bool {
    for (const auto argument : arguments) {
        if (argument == "--json") {
            return true;
        }
    }

    return false;
}

auto parse_index(std::string_view text, std::size_t &value) -> bool {
    const auto *begin = text.data();
    const auto *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

auto parse_uint32(std::string_view text, std::uint32_t &value) -> bool {
    const auto *begin = text.data();
    const auto *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

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

auto parse_list_kind(std::string_view text, featherdoc::list_kind &kind) -> bool {
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
                          featherdoc::section_reference_kind &reference_kind) -> bool {
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

auto read_zip_entry_text(zip_t *archive, std::string_view entry_name,
                         std::string &content) -> zip_entry_read_status {
    if (zip_entry_open(archive, std::string(entry_name).c_str()) < 0) {
        return zip_entry_read_status::missing;
    }

    void *buffer = nullptr;
    std::size_t size = 0;
    if (zip_entry_read(archive, &buffer, &size) < 0) {
        zip_entry_close(archive);
        return zip_entry_read_status::read_failed;
    }

    if (buffer != nullptr && size != 0U) {
        content.assign(static_cast<const char *>(buffer), size);
    } else {
        content.clear();
    }

    std::free(buffer);
    zip_entry_close(archive);
    return zip_entry_read_status::ok;
}

auto normalize_word_part_entry(std::string_view target) -> std::string {
    std::string normalized(target);
    for (auto &character : normalized) {
        if (character == '\\') {
            character = '/';
        }
    }

    if (!normalized.empty() && normalized.front() == '/') {
        normalized.erase(normalized.begin());
    }

    if (normalized.rfind("word/", 0U) == 0U) {
        return normalized;
    }

    return "word/" + normalized;
}

auto parse_options(const std::vector<std::string_view> &arguments,
                   std::size_t start_index, bool allow_no_inherit,
                   command_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (allow_no_inherit && argument == "--no-inherit") {
            options.inherit_header_footer = false;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1 >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_inspect_options(const std::vector<std::string_view> &arguments,
                           std::size_t start_index, inspect_options &options,
                           std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (arguments[index] == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(arguments[index]);
        return false;
    }

    return true;
}

auto parse_inspect_page_setup_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_page_setup_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--section") {
            if (options.section_index.has_value()) {
                error_message = "duplicate --section option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }

            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.section_index = section_index;
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_set_section_page_setup_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_section_page_setup_options &options, std::string &error_message) -> bool {
    auto parse_twips_option =
        [&](std::size_t &index, std::string_view option_name,
            std::optional<std::uint32_t> &target) -> bool {
        if (target.has_value()) {
            error_message = "duplicate " + std::string(option_name) + " option";
            return false;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after " + std::string(option_name);
            return false;
        }

        std::uint32_t value = 0U;
        if (!parse_uint32(arguments[index + 1U], value)) {
            error_message = "invalid value for " + std::string(option_name) + ": " +
                            std::string(arguments[index + 1U]);
            return false;
        }

        target = value;
        ++index;
        return true;
    };

    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--orientation") {
            if (options.orientation.has_value()) {
                error_message = "duplicate --orientation option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --orientation";
                return false;
            }

            featherdoc::page_orientation orientation{};
            if (!parse_page_orientation(arguments[index + 1U], orientation)) {
                error_message = "invalid orientation: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.orientation = orientation;
            ++index;
            continue;
        }

        if (argument == "--width") {
            if (!parse_twips_option(index, argument, options.width_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--height") {
            if (!parse_twips_option(index, argument, options.height_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-top") {
            if (!parse_twips_option(index, argument, options.margin_top_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-bottom") {
            if (!parse_twips_option(index, argument, options.margin_bottom_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-left") {
            if (!parse_twips_option(index, argument, options.margin_left_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-right") {
            if (!parse_twips_option(index, argument, options.margin_right_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-header") {
            if (!parse_twips_option(index, argument, options.margin_header_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--margin-footer") {
            if (!parse_twips_option(index, argument, options.margin_footer_twips)) {
                return false;
            }
            continue;
        }
        if (argument == "--page-number-start") {
            if (options.page_number_start.has_value() || options.clear_page_number_start) {
                error_message =
                    "duplicate page number start option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --page-number-start";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message = "invalid value for --page-number-start: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.page_number_start = value;
            ++index;
            continue;
        }
        if (argument == "--clear-page-number-start") {
            if (options.page_number_start.has_value() || options.clear_page_number_start) {
                error_message =
                    "duplicate page number start option";
                return false;
            }

            options.clear_page_number_start = true;
            continue;
        }
        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_section_text_options(const std::vector<std::string_view> &arguments,
                                std::size_t start_index, bool require_text_source,
                                bool allow_output, bool allow_json,
                                section_text_options &options,
                                std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--kind") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }

            if (!parse_reference_kind(arguments[index + 1U], options.reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            ++index;
            continue;
        }

        if (allow_output && argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            if (!allow_json) {
                error_message = "--json is not supported by this command";
                return false;
            }

            options.json_output = true;
            continue;
        }

        if (argument == "--text") {
            if (!require_text_source) {
                error_message = "--text is only supported by set-section-* commands";
                return false;
            }
            if (options.text.has_value() || options.text_file.has_value()) {
                error_message = "text source was already specified";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --text";
                return false;
            }

            options.text = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--text-file") {
            if (!require_text_source) {
                error_message =
                    "--text-file is only supported by set-section-* commands";
                return false;
            }
            if (options.text.has_value() || options.text_file.has_value()) {
                error_message = "text source was already specified";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --text-file";
                return false;
            }

            options.text_file = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (require_text_source &&
        !options.text.has_value() && !options.text_file.has_value()) {
        error_message = "expected --text <text> or --text-file <path>";
        return false;
    }

    return true;
}

auto parse_inspect_styles_options(const std::vector<std::string_view> &arguments,
                                  std::size_t start_index,
                                  inspect_styles_options &options,
                                  std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--style") {
            if (options.style_id.has_value()) {
                error_message = "duplicate --style option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style";
                return false;
            }

            options.style_id = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        if (argument == "--usage") {
            options.usage_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

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

auto parse_set_paragraph_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    set_paragraph_style_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--definition-name") {
            if (options.definition_name.has_value()) {
                error_message = "duplicate --definition-name option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --definition-name";
                return false;
            }

            options.definition_name = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--numbering-level") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --numbering-level";
                return false;
            }

            featherdoc::numbering_level_definition definition;
            if (!parse_numbering_level_definition(arguments[index + 1U], definition,
                                                  error_message)) {
                return false;
            }

            options.levels.push_back(std::move(definition));
            ++index;
            continue;
        }

        if (argument == "--style-level") {
            if (options.style_level.has_value()) {
                error_message = "duplicate --style-level option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --style-level";
                return false;
            }

            std::uint32_t value = 0U;
            if (!parse_uint32(arguments[index + 1U], value)) {
                error_message =
                    "invalid style level: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.style_level = value;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.definition_name.has_value()) {
        error_message = "missing --definition-name <name>";
        return false;
    }
    if (options.levels.empty()) {
        error_message =
            "expected at least one --numbering-level "
            "<level>:<kind>:<start>:<text-pattern>";
        return false;
    }

    return true;
}

auto parse_clear_paragraph_style_numbering_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    clear_paragraph_style_numbering_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_validation_part(std::string_view text, validation_part_family &part) -> bool {
    if (text == "body") {
        part = validation_part_family::body;
        return true;
    }
    if (text == "header") {
        part = validation_part_family::header;
        return true;
    }
    if (text == "footer") {
        part = validation_part_family::footer;
        return true;
    }
    if (text == "section-header") {
        part = validation_part_family::section_header;
        return true;
    }
    if (text == "section-footer") {
        part = validation_part_family::section_footer;
        return true;
    }

    return false;
}

auto validate_template_part_selection(validation_part_family part,
                                      const std::optional<std::size_t> &part_index,
                                      const std::optional<std::size_t> &section_index,
                                      bool has_kind,
                                      std::string_view operation_label,
                                      std::string &error_message) -> bool {
    switch (part) {
    case validation_part_family::body:
        if (part_index.has_value()) {
            error_message = "--index is only supported by header/footer " +
                            std::string(operation_label);
            return false;
        }
        if (section_index.has_value()) {
            error_message =
                "--section is only supported by section-header/section-footer";
            return false;
        }
        if (has_kind) {
            error_message =
                "--kind is only supported by section-header/section-footer";
            return false;
        }
        break;
    case validation_part_family::header:
    case validation_part_family::footer:
        if (section_index.has_value()) {
            error_message =
                "--section is only supported by section-header/section-footer";
            return false;
        }
        if (has_kind) {
            error_message =
                "--kind is only supported by section-header/section-footer";
            return false;
        }
        break;
    case validation_part_family::section_header:
    case validation_part_family::section_footer:
        if (!section_index.has_value()) {
            error_message = "section-header/section-footer " +
                            std::string(operation_label) +
                            " requires --section <index>";
            return false;
        }
        if (part_index.has_value()) {
            error_message = "--index is only supported by header/footer " +
                            std::string(operation_label);
            return false;
        }
        break;
    }

    return true;
}

auto parse_inspect_numbering_options(const std::vector<std::string_view> &arguments,
                                     std::size_t start_index,
                                     inspect_numbering_options &options,
                                     std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--definition") {
            if (options.definition_id.has_value()) {
                error_message = "duplicate --definition option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --definition";
                return false;
            }

            std::uint32_t definition_id = 0U;
            if (!parse_uint32(arguments[index + 1U], definition_id)) {
                error_message =
                    "invalid numbering definition id: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.definition_id = definition_id;
            ++index;
            continue;
        }

        if (argument == "--instance") {
            if (options.instance_id.has_value()) {
                error_message = "duplicate --instance option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --instance";
                return false;
            }

            std::uint32_t instance_id = 0U;
            if (!parse_uint32(arguments[index + 1U], instance_id)) {
                error_message =
                    "invalid numbering instance id: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.instance_id = instance_id;
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

auto parse_inspect_bookmarks_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    inspect_bookmarks_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--part") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --part";
                return false;
            }

            if (!parse_validation_part(arguments[index + 1U], options.part)) {
                error_message = "invalid template part: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            ++index;
            continue;
        }

        if (argument == "--index") {
            if (options.part_index.has_value()) {
                error_message = "duplicate --index option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --index";
                return false;
            }

            std::size_t part_index = 0U;
            if (!parse_index(arguments[index + 1U], part_index)) {
                error_message = "invalid part index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.part_index = part_index;
            ++index;
            continue;
        }

        if (argument == "--section") {
            if (options.section_index.has_value()) {
                error_message = "duplicate --section option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }

            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.section_index = section_index;
            ++index;
            continue;
        }

        if (argument == "--kind") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }

            if (!parse_reference_kind(arguments[index + 1U], options.reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_kind = true;
            ++index;
            continue;
        }

        if (argument == "--bookmark") {
            if (options.bookmark_name.has_value()) {
                error_message = "duplicate --bookmark option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --bookmark";
                return false;
            }

            options.bookmark_name = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

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

auto parse_template_slot_requirement(std::string_view text,
                                     featherdoc::template_slot_requirement &requirement,
                                     std::string &error_message) -> bool {
    const auto first_separator = text.find(':');
    if (first_separator == std::string_view::npos || first_separator == 0U) {
        error_message =
            "invalid --slot value: expected <bookmark>:<kind>[:required|optional]";
        return false;
    }

    const auto second_separator = text.find(':', first_separator + 1U);
    const auto bookmark_name = text.substr(0U, first_separator);
    const auto kind_text =
        second_separator == std::string_view::npos
            ? text.substr(first_separator + 1U)
            : text.substr(first_separator + 1U,
                          second_separator - first_separator - 1U);
    if (kind_text.empty()) {
        error_message =
            "invalid --slot value: expected <bookmark>:<kind>[:required|optional]";
        return false;
    }

    featherdoc::template_slot_kind kind;
    if (!parse_template_slot_kind(kind_text, kind)) {
        error_message = "invalid --slot kind: " + std::string(kind_text);
        return false;
    }

    bool required = true;
    if (second_separator != std::string_view::npos) {
        const auto requirement_text = text.substr(second_separator + 1U);
        if (requirement_text == "required") {
            required = true;
        } else if (requirement_text == "optional") {
            required = false;
        } else {
            error_message = "invalid --slot requirement: " +
                            std::string(requirement_text);
            return false;
        }
    }

    requirement.bookmark_name = std::string(bookmark_name);
    requirement.kind = kind;
    requirement.required = required;
    return true;
}

auto parse_validate_template_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    validate_template_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--part") {
            if (options.has_part) {
                error_message = "duplicate --part option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --part";
                return false;
            }

            if (!parse_validation_part(arguments[index + 1U], options.part)) {
                error_message = "invalid template part: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_part = true;
            ++index;
            continue;
        }

        if (argument == "--index") {
            if (options.part_index.has_value()) {
                error_message = "duplicate --index option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --index";
                return false;
            }

            std::size_t part_index = 0U;
            if (!parse_index(arguments[index + 1U], part_index)) {
                error_message = "invalid part index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.part_index = part_index;
            ++index;
            continue;
        }

        if (argument == "--section") {
            if (options.section_index.has_value()) {
                error_message = "duplicate --section option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }

            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.section_index = section_index;
            ++index;
            continue;
        }

        if (argument == "--kind") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }

            if (!parse_reference_kind(arguments[index + 1U], options.reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_kind = true;
            ++index;
            continue;
        }

        if (argument == "--slot") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --slot";
                return false;
            }

            featherdoc::template_slot_requirement requirement;
            if (!parse_template_slot_requirement(arguments[index + 1U], requirement,
                                                 error_message)) {
                return false;
            }

            options.requirements.push_back(std::move(requirement));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.has_part) {
        error_message = "missing --part <body|header|footer|section-header|section-footer>";
        return false;
    }
    if (options.requirements.empty()) {
        error_message =
            "expected at least one --slot <bookmark>:<kind>[:required|optional]";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "validation", error_message);
}

auto parse_append_field_options(const std::vector<std::string_view> &arguments,
                                std::size_t start_index,
                                append_field_options &options,
                                std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--part") {
            if (options.has_part) {
                error_message = "duplicate --part option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --part";
                return false;
            }

            if (!parse_validation_part(arguments[index + 1U], options.part)) {
                error_message = "invalid template part: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_part = true;
            ++index;
            continue;
        }

        if (argument == "--index") {
            if (options.part_index.has_value()) {
                error_message = "duplicate --index option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --index";
                return false;
            }

            std::size_t part_index = 0U;
            if (!parse_index(arguments[index + 1U], part_index)) {
                error_message = "invalid part index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.part_index = part_index;
            ++index;
            continue;
        }

        if (argument == "--section") {
            if (options.section_index.has_value()) {
                error_message = "duplicate --section option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --section";
                return false;
            }

            std::size_t section_index = 0U;
            if (!parse_index(arguments[index + 1U], section_index)) {
                error_message = "invalid section index: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.section_index = section_index;
            ++index;
            continue;
        }

        if (argument == "--kind") {
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --kind";
                return false;
            }

            if (!parse_reference_kind(arguments[index + 1U], options.reference_kind)) {
                error_message = "invalid reference kind: " +
                                std::string(arguments[index + 1U]);
                return false;
            }

            options.has_kind = true;
            ++index;
            continue;
        }

        if (argument == "--output") {
            if (options.output_path.has_value()) {
                error_message = "duplicate --output option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing path after --output";
                return false;
            }

            options.output_path = path_type(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (!options.has_part) {
        error_message = "missing --part <body|header|footer|section-header|section-footer>";
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "mutation", error_message);
}

auto has_section_header(featherdoc::Document &doc, std::size_t section_index,
                        featherdoc::section_reference_kind reference_kind)
    -> bool {
    return doc.section_header_paragraphs(section_index, reference_kind).has_next();
}

auto has_section_footer(featherdoc::Document &doc, std::size_t section_index,
                        featherdoc::section_reference_kind reference_kind)
    -> bool {
    return doc.section_footer_paragraphs(section_index, reference_kind).has_next();
}

auto yes_no(bool value) -> const char * {
    return value ? "yes" : "no";
}

auto json_bool(bool value) -> const char * {
    return value ? "true" : "false";
}

auto section_part_name(section_part_family family) -> const char * {
    return family == section_part_family::header ? "header" : "footer";
}

auto validation_part_name(validation_part_family family) -> const char * {
    switch (family) {
    case validation_part_family::body:
        return "body";
    case validation_part_family::header:
        return "header";
    case validation_part_family::footer:
        return "footer";
    case validation_part_family::section_header:
        return "section-header";
    case validation_part_family::section_footer:
        return "section-footer";
    }

    return "body";
}

auto style_kind_name(featherdoc::style_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::style_kind::paragraph:
        return "paragraph";
    case featherdoc::style_kind::character:
        return "character";
    case featherdoc::style_kind::table:
        return "table";
    case featherdoc::style_kind::numbering:
        return "numbering";
    case featherdoc::style_kind::unknown:
        return "unknown";
    }

    return "unknown";
}

auto list_kind_name(featherdoc::list_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::list_kind::bullet:
        return "bullet";
    case featherdoc::list_kind::decimal:
        return "decimal";
    }

    return "bullet";
}

auto bookmark_kind_name(featherdoc::bookmark_kind kind) -> const char * {
    switch (kind) {
    case featherdoc::bookmark_kind::text:
        return "text";
    case featherdoc::bookmark_kind::block:
        return "block";
    case featherdoc::bookmark_kind::table_rows:
        return "table_rows";
    case featherdoc::bookmark_kind::block_range:
        return "block_range";
    case featherdoc::bookmark_kind::malformed:
        return "malformed";
    case featherdoc::bookmark_kind::mixed:
        return "mixed";
    }

    return "mixed";
}

auto template_field_name(std::string_view command) -> const char * {
    return command == "append-page-number-field" ? "page_number" : "total_pages";
}

auto section_reference_name(section_part_family family) -> const char * {
    return family == section_part_family::header ? "w:headerReference"
                                                 : "w:footerReference";
}

auto section_part_paragraphs(featherdoc::Document &doc, section_part_family family,
                             std::size_t index) -> featherdoc::Paragraph & {
    if (family == section_part_family::header) {
        return doc.header_paragraphs(index);
    }

    return doc.footer_paragraphs(index);
}

auto collect_paragraph_text(featherdoc::Paragraph paragraph) -> std::string {
    std::string text;
    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
        text += run.get_text();
    }
    return text;
}

auto collect_part_lines(featherdoc::Paragraph paragraph) -> std::vector<std::string> {
    std::vector<std::string> lines;
    for (; paragraph.has_next(); paragraph.next()) {
        lines.push_back(collect_paragraph_text(paragraph));
    }
    return lines;
}

auto section_part_paragraphs(featherdoc::Document &doc, section_part_family family,
                             std::size_t section_index,
                             featherdoc::section_reference_kind reference_kind)
    -> featherdoc::Paragraph & {
    if (family == section_part_family::header) {
        return doc.section_header_paragraphs(section_index, reference_kind);
    }

    return doc.section_footer_paragraphs(section_index, reference_kind);
}

auto load_inspected_part_entries(const path_type &input_path, featherdoc::Document &doc,
                                 section_part_family family,
                                 std::vector<inspected_part_entry> &parts,
                                 std::string &error_message) -> bool {
    parts.clear();

    int zip_error = 0;
    zip_t *archive = zip_openwitherror(input_path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    if (archive == nullptr) {
        error_message = "failed to open source archive '" + input_path.string() + "'";
        return false;
    }

    auto close_archive = [&]() { zip_close(archive); };

    std::string relationships_xml_text;
    const auto relationships_status =
        read_zip_entry_text(archive, "word/_rels/document.xml.rels", relationships_xml_text);
    if (relationships_status == zip_entry_read_status::read_failed) {
        close_archive();
        error_message = "failed to read relationships entry 'word/_rels/document.xml.rels'";
        return false;
    }

    if (relationships_status == zip_entry_read_status::ok) {
        pugi::xml_document relationships_xml;
        const auto parse_result = relationships_xml.load_buffer(
            relationships_xml_text.data(), relationships_xml_text.size());
        if (!parse_result) {
            close_archive();
            error_message = "failed to parse relationships entry 'word/_rels/document.xml.rels'";
            return false;
        }

        const auto expected_type =
            family == section_part_family::header
                ? std::string_view{
                      "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
                      "header"}
                : std::string_view{
                      "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
                      "footer"};

        std::vector<std::string> seen_entries;
        const auto relationships = relationships_xml.child("Relationships");
        for (auto relationship = relationships.child("Relationship");
             relationship != pugi::xml_node{};
             relationship = relationship.next_sibling("Relationship")) {
            const auto type = std::string_view{relationship.attribute("Type").value()};
            if (type != expected_type) {
                continue;
            }

            const auto target = std::string_view{relationship.attribute("Target").value()};
            if (target.empty()) {
                continue;
            }

            const auto entry_name = normalize_word_part_entry(target);
            bool already_seen = false;
            for (const auto &seen_entry : seen_entries) {
                if (seen_entry == entry_name) {
                    already_seen = true;
                    break;
                }
            }
            if (already_seen) {
                continue;
            }

            seen_entries.push_back(entry_name);
            parts.push_back(inspected_part_entry{
                relationship.attribute("Id").value(), entry_name, {}, {}});
        }
    }

    std::string document_xml_text;
    const auto document_status =
        read_zip_entry_text(archive, "word/document.xml", document_xml_text);
    if (document_status != zip_entry_read_status::ok) {
        close_archive();
        error_message = "failed to read XML entry from archive 'word/document.xml'";
        return false;
    }

    pugi::xml_document document_xml;
    const auto document_parse_result =
        document_xml.load_buffer(document_xml_text.data(), document_xml_text.size());
    if (!document_parse_result) {
        close_archive();
        error_message = "failed to parse XML entry 'word/document.xml'";
        return false;
    }

    close_archive();

    const auto body = document_xml.child("w:document").child("w:body");
    if (body == pugi::xml_node{}) {
        error_message = "document.xml does not contain the expected w:document/w:body structure";
        return false;
    }

    const auto collect_references = [&](pugi::xml_node section_properties,
                                        std::size_t section_index) {
        for (auto reference = section_properties.child(section_reference_name(family));
             reference != pugi::xml_node{};
             reference = reference.next_sibling(section_reference_name(family))) {
            const auto relationship_id =
                std::string_view{reference.attribute("r:id").value()};
            if (relationship_id.empty()) {
                continue;
            }

            const auto kind_attribute = std::string_view{reference.attribute("w:type").value()};
            const auto kind = kind_attribute.empty() ? std::string{"default"}
                                                     : std::string{kind_attribute};

            for (auto &part : parts) {
                if (part.relationship_id == relationship_id) {
                    part.references.push_back(
                        inspected_part_reference{section_index, kind});
                    break;
                }
            }
        }
    };

    std::size_t section_index = 0U;
    for (auto child = body.first_child(); child != pugi::xml_node{};
         child = child.next_sibling()) {
        if (std::string_view{child.name()} != "w:p") {
            continue;
        }

        if (const auto section_properties = child.child("w:pPr").child("w:sectPr");
            section_properties != pugi::xml_node{}) {
            collect_references(section_properties, section_index);
            ++section_index;
        }
    }

    if (const auto final_section_properties = body.child("w:sectPr");
        final_section_properties != pugi::xml_node{}) {
        collect_references(final_section_properties, section_index);
    }

    const auto expected_count = family == section_part_family::header ? doc.header_count()
                                                                      : doc.footer_count();
    if (parts.size() != expected_count) {
        error_message = "CLI part inspection order does not match loaded document parts";
        return false;
    }

    for (std::size_t index = 0; index < parts.size(); ++index) {
        parts[index].paragraphs =
            collect_part_lines(section_part_paragraphs(doc, family, index));
    }

    return true;
}

auto json_escape(std::string_view text) -> std::string {
    std::string escaped;
    escaped.reserve(text.size());
    for (const char ch : text) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

void write_json_string(std::ostream &stream, std::string_view text) {
    stream << '"' << json_escape(text) << '"';
}

void write_json_strings(std::ostream &stream,
                        const std::vector<std::string> &values) {
    stream << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, values[index]);
    }
    stream << ']';
}

void write_json_lines(std::ostream &stream, const std::vector<std::string> &lines) {
    write_json_strings(stream, lines);
}

void write_json_section_flags(std::ostream &stream, featherdoc::Document &doc,
                              std::size_t section_index, section_part_family family) {
    const auto has_default =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::default_reference)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::default_reference);
    const auto has_first =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::first_page)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::first_page);
    const auto has_even =
        family == section_part_family::header
            ? has_section_header(doc, section_index,
                                 featherdoc::section_reference_kind::even_page)
            : has_section_footer(doc, section_index,
                                 featherdoc::section_reference_kind::even_page);

    stream << "{\"default\":" << (has_default ? "true" : "false")
           << ",\"first\":" << (has_first ? "true" : "false")
           << ",\"even\":" << (has_even ? "true" : "false") << '}';
}

void write_json_part_references(std::ostream &stream,
                                const std::vector<inspected_part_reference> &references) {
    stream << '[';
    for (std::size_t index = 0; index < references.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"section\":" << references[index].section_index << ",\"kind\":";
        write_json_string(stream, references[index].kind);
        stream << '}';
    }
    stream << ']';
}

void write_json_optional_string(std::ostream &stream,
                                const std::optional<std::string> &value) {
    if (value.has_value()) {
        write_json_string(stream, *value);
        return;
    }

    stream << "null";
}

void write_json_optional_u32(std::ostream &stream,
                             const std::optional<std::uint32_t> &value) {
    if (value.has_value()) {
        stream << *value;
        return;
    }

    stream << "null";
}

void write_json_numbering_instance_summary(
    std::ostream &stream, const featherdoc::numbering_instance_summary &instance);

void write_json_style_numbering_summary(
    std::ostream &stream,
    const featherdoc::style_summary::numbering_summary &numbering) {
    stream << "{\"num_id\":";
    write_json_optional_u32(stream, numbering.num_id);
    stream << ",\"level\":";
    write_json_optional_u32(stream, numbering.level);
    stream << ",\"definition_id\":";
    write_json_optional_u32(stream, numbering.definition_id);
    stream << ",\"definition_name\":";
    write_json_optional_string(stream, numbering.definition_name);
    stream << ",\"instance\":";
    if (numbering.instance.has_value()) {
        write_json_numbering_instance_summary(stream, *numbering.instance);
    } else {
        stream << "null";
    }
    stream << '}';
}

auto page_orientation_name(featherdoc::page_orientation orientation)
    -> std::string_view {
    switch (orientation) {
    case featherdoc::page_orientation::portrait:
        return "portrait";
    case featherdoc::page_orientation::landscape:
        return "landscape";
    }

    return "unknown";
}

void write_json_page_margins(std::ostream &stream,
                             const featherdoc::page_margins &margins) {
    stream << "{\"top_twips\":" << margins.top_twips
           << ",\"bottom_twips\":" << margins.bottom_twips
           << ",\"left_twips\":" << margins.left_twips
           << ",\"right_twips\":" << margins.right_twips
           << ",\"header_twips\":" << margins.header_twips
           << ",\"footer_twips\":" << margins.footer_twips << '}';
}

void write_json_section_page_setup(
    std::ostream &stream, const featherdoc::section_page_setup &page_setup) {
    stream << "{\"orientation\":";
    write_json_string(stream, page_orientation_name(page_setup.orientation));
    stream << ",\"width_twips\":" << page_setup.width_twips
           << ",\"height_twips\":" << page_setup.height_twips
           << ",\"margins\":";
    write_json_page_margins(stream, page_setup.margins);
    stream << ",\"page_number_start\":";
    if (page_setup.page_number_start.has_value()) {
        stream << *page_setup.page_number_start;
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_style_summary(std::ostream &stream,
                              const featherdoc::style_summary &style) {
    stream << "{\"style_id\":";
    write_json_string(stream, style.style_id);
    stream << ",\"name\":";
    write_json_string(stream, style.name);
    stream << ",\"based_on\":";
    write_json_optional_string(stream, style.based_on);
    stream << ",\"kind\":";
    write_json_string(stream, style_kind_name(style.kind));
    stream << ",\"type\":";
    write_json_string(stream, style.type_name);
    stream << ",\"numbering\":";
    if (style.numbering.has_value()) {
        write_json_style_numbering_summary(stream, *style.numbering);
    } else {
        stream << "null";
    }
    stream << ",\"is_default\":" << json_bool(style.is_default)
           << ",\"is_custom\":" << json_bool(style.is_custom)
           << ",\"is_semi_hidden\":" << json_bool(style.is_semi_hidden)
           << ",\"is_unhide_when_used\":" << json_bool(style.is_unhide_when_used)
           << ",\"is_quick_format\":" << json_bool(style.is_quick_format)
           << '}';
}

auto style_usage_part_kind_name(featherdoc::style_usage_part_kind part_kind) -> const char * {
    switch (part_kind) {
    case featherdoc::style_usage_part_kind::body:
        return "body";
    case featherdoc::style_usage_part_kind::header:
        return "header";
    case featherdoc::style_usage_part_kind::footer:
        return "footer";
    }

    return "unknown";
}

auto style_usage_hit_kind_name(featherdoc::style_usage_hit_kind hit_kind) -> const char * {
    switch (hit_kind) {
    case featherdoc::style_usage_hit_kind::paragraph:
        return "paragraph";
    case featherdoc::style_usage_hit_kind::run:
        return "run";
    case featherdoc::style_usage_hit_kind::table:
        return "table";
    }

    return "unknown";
}

void write_json_style_usage_hit_reference(
    std::ostream &stream, const featherdoc::style_usage_hit_reference &reference) {
    stream << "{\"section_index\":" << reference.section_index << ",\"reference_kind\":";
    write_json_string(stream, featherdoc::to_xml_reference_type(reference.reference_kind));
    stream << '}';
}

void write_json_style_usage_breakdown(std::ostream &stream,
                                      const featherdoc::style_usage_breakdown &usage) {
    stream << "{\"paragraph_count\":" << usage.paragraph_count
           << ",\"run_count\":" << usage.run_count
           << ",\"table_count\":" << usage.table_count
           << ",\"total_count\":" << usage.total_count() << '}';
}

void write_json_style_usage_hit(std::ostream &stream, const featherdoc::style_usage_hit &hit) {
    stream << "{\"part\":";
    write_json_string(stream, style_usage_part_kind_name(hit.part));
    stream << ",\"entry_name\":";
    write_json_string(stream, hit.entry_name);
    stream << ",\"ordinal\":" << hit.ordinal << ",\"kind\":";
    write_json_string(stream, style_usage_hit_kind_name(hit.kind));
    stream << ",\"references\":[";
    for (std::size_t index = 0; index < hit.references.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_usage_hit_reference(stream, hit.references[index]);
    }
    stream << ']';
    stream << '}';
}

void write_json_style_usage_summary(std::ostream &stream,
                                    const featherdoc::style_usage_summary &usage) {
    stream << "{\"style_id\":";
    write_json_string(stream, usage.style_id);
    stream << ",\"paragraph_count\":" << usage.paragraph_count
           << ",\"run_count\":" << usage.run_count
           << ",\"table_count\":" << usage.table_count
           << ",\"total_count\":" << usage.total_count()
           << ",\"body\":";
    write_json_style_usage_breakdown(stream, usage.body);
    stream << ",\"header\":";
    write_json_style_usage_breakdown(stream, usage.header);
    stream << ",\"footer\":";
    write_json_style_usage_breakdown(stream, usage.footer);
    stream << ",\"hits\":[";
    for (std::size_t index = 0; index < usage.hits.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_style_usage_hit(stream, usage.hits[index]);
    }
    stream << ']';
    stream << '}';
}

void print_style_numbering_inline(
    std::ostream &stream,
    const std::optional<featherdoc::style_summary::numbering_summary> &numbering) {
    if (!numbering.has_value()) {
        stream << "none";
        return;
    }

    stream << '(';
    stream << "level=";
    if (numbering->level.has_value()) {
        stream << *numbering->level;
    } else {
        stream << "unknown";
    }
    stream << ", num_id=";
    if (numbering->num_id.has_value()) {
        stream << *numbering->num_id;
    } else {
        stream << "unknown";
    }
    stream << ", definition_id=";
    if (numbering->definition_id.has_value()) {
        stream << *numbering->definition_id;
    } else {
        stream << "unknown";
    }
    stream << ", definition_name=";
    if (numbering->definition_name.has_value()) {
        stream << *numbering->definition_name;
    } else {
        stream << "none";
    }
    if (numbering->instance.has_value()) {
        stream << ", override_count="
               << numbering->instance->level_overrides.size();
    }
    stream << ')';
}

void write_json_numbering_level_definition(
    std::ostream &stream, const featherdoc::numbering_level_definition &definition) {
    stream << "{\"level\":" << definition.level << ",\"kind\":";
    write_json_string(stream, list_kind_name(definition.kind));
    stream << ",\"start\":" << definition.start << ",\"text_pattern\":";
    write_json_string(stream, definition.text_pattern);
    stream << '}';
}

void write_json_numbering_level_override_summary(
    std::ostream &stream, const featherdoc::numbering_level_override_summary &level_override) {
    stream << "{\"level\":" << level_override.level << ",\"start_override\":";
    write_json_optional_u32(stream, level_override.start_override);
    stream << ",\"level_definition\":";
    if (level_override.level_definition.has_value()) {
        write_json_numbering_level_definition(stream, *level_override.level_definition);
    } else {
        stream << "null";
    }
    stream << '}';
}

void write_json_numbering_instance_summary(
    std::ostream &stream, const featherdoc::numbering_instance_summary &instance) {
    stream << "{\"instance_id\":" << instance.instance_id << ",\"level_overrides\":[";
    for (std::size_t index = 0; index < instance.level_overrides.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_override_summary(stream,
                                                    instance.level_overrides[index]);
    }
    stream << "]}";
}

void write_json_numbering_definition_summary(
    std::ostream &stream, const featherdoc::numbering_definition_summary &definition) {
    stream << "{\"definition_id\":" << definition.definition_id << ",\"name\":";
    write_json_string(stream, definition.name);
    stream << ",\"levels\":[";
    for (std::size_t index = 0; index < definition.levels.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_level_definition(stream, definition.levels[index]);
    }
    stream << "],\"instance_ids\":[";
    for (std::size_t index = 0; index < definition.instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << definition.instance_ids[index];
    }
    stream << "],\"instances\":[";
    for (std::size_t index = 0; index < definition.instances.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_numbering_instance_summary(stream, definition.instances[index]);
    }
    stream << "]}";
}

void print_numbering_instance_ids(std::ostream &stream,
                                  const std::vector<std::uint32_t> &instance_ids) {
    if (instance_ids.empty()) {
        stream << "none";
        return;
    }

    for (std::size_t index = 0; index < instance_ids.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << instance_ids[index];
    }
}

void print_numbering_definition_summary(
    std::ostream &stream, const featherdoc::numbering_definition_summary &definition) {
    stream << "id=" << definition.definition_id << " name=" << definition.name
           << " levels=" << definition.levels.size() << " instances=";
    print_numbering_instance_ids(stream, definition.instance_ids);
}

void print_numbering_level_override_summary(
    std::ostream &stream, const featherdoc::numbering_level_override_summary &level_override) {
    stream << "level=" << level_override.level << " start_override=";
    if (level_override.start_override.has_value()) {
        stream << *level_override.start_override;
    } else {
        stream << "none";
    }

    if (level_override.level_definition.has_value()) {
        stream << " definition_kind="
               << list_kind_name(level_override.level_definition->kind)
               << " definition_start=" << level_override.level_definition->start
               << " definition_text_pattern="
               << level_override.level_definition->text_pattern;
    }
}

void print_numbering_instance_summary(
    std::ostream &stream, const featherdoc::numbering_instance_summary &instance,
    std::string_view indent = {}) {
    stream << indent << "instance[" << instance.instance_id
           << "]: override_count=" << instance.level_overrides.size() << '\n';
    for (std::size_t index = 0; index < instance.level_overrides.size(); ++index) {
        stream << indent << "  override[" << index << "]: ";
        print_numbering_level_override_summary(stream, instance.level_overrides[index]);
        stream << '\n';
    }
}

void write_json_bookmark_summary(std::ostream &stream,
                                 const featherdoc::bookmark_summary &bookmark) {
    stream << "{\"bookmark_name\":";
    write_json_string(stream, bookmark.bookmark_name);
    stream << ",\"occurrence_count\":" << bookmark.occurrence_count
           << ",\"kind\":";
    write_json_string(stream, bookmark_kind_name(bookmark.kind));
    stream << ",\"is_duplicate\":" << json_bool(bookmark.is_duplicate()) << '}';
}

void print_style_summary(std::ostream &stream,
                         const featherdoc::style_summary &style) {
    stream << "id=" << style.style_id << " name=" << style.name
           << " type=" << style.type_name
           << " kind=" << style_kind_name(style.kind)
           << " based_on=";
    if (style.based_on.has_value()) {
        stream << *style.based_on;
    } else {
        stream << "none";
    }
    stream << " default=" << yes_no(style.is_default)
           << " custom=" << yes_no(style.is_custom)
           << " semi_hidden=" << yes_no(style.is_semi_hidden)
           << " unhide_when_used=" << yes_no(style.is_unhide_when_used)
           << " quick_format=" << yes_no(style.is_quick_format)
           << " numbering=";
    print_style_numbering_inline(stream, style.numbering);
}

void print_bookmark_summary(std::ostream &stream,
                            const featherdoc::bookmark_summary &bookmark) {
    stream << "name=" << bookmark.bookmark_name
           << " occurrences=" << bookmark.occurrence_count
           << " kind=" << bookmark_kind_name(bookmark.kind)
           << " duplicate=" << yes_no(bookmark.is_duplicate());
}

void print_section_page_setup_summary(
    std::ostream &stream, const featherdoc::section_page_setup &page_setup) {
    stream << "orientation=" << page_orientation_name(page_setup.orientation)
           << " width_twips=" << page_setup.width_twips
           << " height_twips=" << page_setup.height_twips
           << " margins(top=" << page_setup.margins.top_twips
           << ", bottom=" << page_setup.margins.bottom_twips
           << ", left=" << page_setup.margins.left_twips
           << ", right=" << page_setup.margins.right_twips
           << ", header=" << page_setup.margins.header_twips
           << ", footer=" << page_setup.margins.footer_twips
           << ") page_number_start=";
    if (page_setup.page_number_start.has_value()) {
        stream << *page_setup.page_number_start;
    } else {
        stream << "none";
    }
}

void print_section_page_setup_details(
    std::ostream &stream, const featherdoc::section_page_setup &page_setup) {
    stream << "orientation: " << page_orientation_name(page_setup.orientation) << '\n';
    stream << "width_twips: " << page_setup.width_twips << '\n';
    stream << "height_twips: " << page_setup.height_twips << '\n';
    stream << "top_twips: " << page_setup.margins.top_twips << '\n';
    stream << "bottom_twips: " << page_setup.margins.bottom_twips << '\n';
    stream << "left_twips: " << page_setup.margins.left_twips << '\n';
    stream << "right_twips: " << page_setup.margins.right_twips << '\n';
    stream << "header_twips: " << page_setup.margins.header_twips << '\n';
    stream << "footer_twips: " << page_setup.margins.footer_twips << '\n';
    stream << "page_number_start: ";
    if (page_setup.page_number_start.has_value()) {
        stream << *page_setup.page_number_start;
    } else {
        stream << "none";
    }
    stream << '\n';
}

auto report_operation_failure(std::string_view command, std::string_view stage,
                              std::string_view fallback_message,
                              const featherdoc::document_error_info &error_info,
                              bool json_output) -> bool;

auto report_document_error(std::string_view command, std::string_view stage,
                           const featherdoc::document_error_info &error_info,
                           bool json_output) -> bool;

void inspect_styles(const std::vector<featherdoc::style_summary> &styles,
                    bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << styles.size() << ",\"styles\":[";
        for (std::size_t index = 0; index < styles.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_style_summary(std::cout, styles[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "styles: " << styles.size() << '\n';
    for (std::size_t index = 0; index < styles.size(); ++index) {
        std::cout << "style[" << index << "]: ";
        print_style_summary(std::cout, styles[index]);
        std::cout << '\n';
    }
}

void inspect_style(const featherdoc::style_summary &style,
                   const std::optional<featherdoc::style_usage_summary> &usage,
                   bool json_output) {
    if (json_output) {
        std::cout << "{\"style\":";
        write_json_style_summary(std::cout, style);
        if (usage.has_value()) {
            std::cout << ",\"usage\":";
            write_json_style_usage_summary(std::cout, *usage);
        }
        std::cout << "}\n";
        return;
    }

    std::cout << "style_id: " << style.style_id << '\n';
    std::cout << "name: " << style.name << '\n';
    std::cout << "type: " << style.type_name << '\n';
    std::cout << "kind: " << style_kind_name(style.kind) << '\n';
    std::cout << "based_on: ";
    if (style.based_on.has_value()) {
        std::cout << *style.based_on;
    } else {
        std::cout << "none";
    }
    std::cout << '\n';
    std::cout << "default: " << yes_no(style.is_default) << '\n';
    std::cout << "custom: " << yes_no(style.is_custom) << '\n';
    std::cout << "semi_hidden: " << yes_no(style.is_semi_hidden) << '\n';
    std::cout << "unhide_when_used: " << yes_no(style.is_unhide_when_used) << '\n';
    std::cout << "quick_format: " << yes_no(style.is_quick_format) << '\n';
    std::cout << "numbering: ";
    print_style_numbering_inline(std::cout, style.numbering);
    std::cout << '\n';
    if (style.numbering.has_value() && style.numbering->instance.has_value()) {
        std::cout << "numbering_instance_id: "
                  << style.numbering->instance->instance_id << '\n';
        std::cout << "numbering_override_count: "
                  << style.numbering->instance->level_overrides.size()
                  << '\n';
        for (std::size_t index = 0;
             index < style.numbering->instance->level_overrides.size();
             ++index) {
            std::cout << "numbering_override[" << index << "]: ";
            print_numbering_level_override_summary(
                std::cout, style.numbering->instance->level_overrides[index]);
            std::cout << '\n';
        }
    }

    if (usage.has_value()) {
        std::cout << "usage_paragraphs: " << usage->paragraph_count << '\n';
        std::cout << "usage_runs: " << usage->run_count << '\n';
        std::cout << "usage_tables: " << usage->table_count << '\n';
        std::cout << "usage_total: " << usage->total_count() << '\n';
        std::cout << "usage_body_paragraphs: " << usage->body.paragraph_count << '\n';
        std::cout << "usage_body_runs: " << usage->body.run_count << '\n';
        std::cout << "usage_body_tables: " << usage->body.table_count << '\n';
        std::cout << "usage_body_total: " << usage->body.total_count() << '\n';
        std::cout << "usage_header_paragraphs: " << usage->header.paragraph_count << '\n';
        std::cout << "usage_header_runs: " << usage->header.run_count << '\n';
        std::cout << "usage_header_tables: " << usage->header.table_count << '\n';
        std::cout << "usage_header_total: " << usage->header.total_count() << '\n';
        std::cout << "usage_footer_paragraphs: " << usage->footer.paragraph_count << '\n';
        std::cout << "usage_footer_runs: " << usage->footer.run_count << '\n';
        std::cout << "usage_footer_tables: " << usage->footer.table_count << '\n';
        std::cout << "usage_footer_total: " << usage->footer.total_count() << '\n';
        std::cout << "usage_hits: " << usage->hits.size() << '\n';
        for (std::size_t index = 0; index < usage->hits.size(); ++index) {
            const auto &hit = usage->hits[index];
            std::cout << "usage_hit[" << index << "]: part="
                      << style_usage_part_kind_name(hit.part)
                      << " entry_name=" << hit.entry_name << " ordinal=" << hit.ordinal
                      << " kind=" << style_usage_hit_kind_name(hit.kind)
                      << " references=" << hit.references.size();
            for (std::size_t reference_index = 0; reference_index < hit.references.size();
                 ++reference_index) {
                const auto &reference = hit.references[reference_index];
                std::cout << " ref[" << reference_index
                          << "]=section:" << reference.section_index
                          << ",kind:"
                          << featherdoc::to_xml_reference_type(reference.reference_kind);
            }
            std::cout << '\n';
        }
    }
}

void inspect_numbering(const std::vector<featherdoc::numbering_definition_summary> &definitions,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"count\":" << definitions.size() << ",\"definitions\":[";
        for (std::size_t index = 0; index < definitions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_numbering_definition_summary(std::cout, definitions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "definitions: " << definitions.size() << '\n';
    for (std::size_t index = 0; index < definitions.size(); ++index) {
        std::cout << "definition[" << index << "]: ";
        print_numbering_definition_summary(std::cout, definitions[index]);
        std::cout << '\n';
        for (const auto &level : definitions[index].levels) {
            std::cout << "  level[" << level.level << "]: kind="
                      << list_kind_name(level.kind) << " start=" << level.start
                      << " text_pattern=" << level.text_pattern << '\n';
        }
        for (const auto &instance : definitions[index].instances) {
            print_numbering_instance_summary(std::cout, instance, "  ");
        }
    }
}

void inspect_numbering(const featherdoc::numbering_definition_summary &definition,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"definition\":";
        write_json_numbering_definition_summary(std::cout, definition);
        std::cout << "}\n";
        return;
    }

    std::cout << "definition_id: " << definition.definition_id << '\n';
    std::cout << "name: " << definition.name << '\n';
    std::cout << "instances: ";
    print_numbering_instance_ids(std::cout, definition.instance_ids);
    std::cout << '\n';
    std::cout << "levels: " << definition.levels.size() << '\n';
    for (const auto &level : definition.levels) {
        std::cout << "level[" << level.level << "]: kind=" << list_kind_name(level.kind)
                  << " start=" << level.start
                  << " text_pattern=" << level.text_pattern << '\n';
    }
    std::cout << "instance_count: " << definition.instances.size() << '\n';
    for (const auto &instance : definition.instances) {
        print_numbering_instance_summary(std::cout, instance);
    }
}

void inspect_numbering(const featherdoc::numbering_instance_lookup_summary &instance_lookup,
                       bool json_output) {
    if (json_output) {
        std::cout << "{\"instance\":{\"definition_id\":"
                  << instance_lookup.definition_id
                  << ",\"definition_name\":";
        write_json_string(std::cout, instance_lookup.definition_name);
        std::cout << ",\"instance\":";
        write_json_numbering_instance_summary(std::cout, instance_lookup.instance);
        std::cout << "}}\n";
        return;
    }

    std::cout << "definition_id: " << instance_lookup.definition_id << '\n';
    std::cout << "name: " << instance_lookup.definition_name << '\n';
    print_numbering_instance_summary(std::cout, instance_lookup.instance);
}

void inspect_bookmarks(const selected_template_part &selected,
                       const std::vector<featherdoc::bookmark_summary> &bookmarks,
                       bool json_output) {
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
        std::cout << ",\"count\":" << bookmarks.size() << ",\"bookmarks\":[";
        for (std::size_t index = 0; index < bookmarks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_bookmark_summary(std::cout, bookmarks[index]);
        }
        std::cout << "]}\n";
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
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmarks: " << bookmarks.size() << '\n';
    for (std::size_t index = 0; index < bookmarks.size(); ++index) {
        std::cout << "bookmark[" << index << "]: ";
        print_bookmark_summary(std::cout, bookmarks[index]);
        std::cout << '\n';
    }
}

void inspect_bookmark(const selected_template_part &selected,
                      const featherdoc::bookmark_summary &bookmark,
                      bool json_output) {
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
        std::cout << ",\"bookmark\":";
        write_json_bookmark_summary(std::cout, bookmark);
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
                  << featherdoc::to_xml_reference_type(*selected.reference_kind)
                  << '\n';
    }
    std::cout << "entry_name: " << entry_name << '\n';
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
}

auto inspect_page_setup(featherdoc::Document &doc, std::size_t section_index,
                        std::string_view command, bool json_output) -> bool {
    const auto page_setup = doc.get_section_page_setup(section_index);
    if (!page_setup.has_value() && doc.last_error().code) {
        return report_document_error(command, "inspect", doc.last_error(),
                                     json_output);
    }

    if (json_output) {
        std::cout << "{\"section\":" << section_index
                  << ",\"present\":" << json_bool(page_setup.has_value())
                  << ",\"page_setup\":";
        if (page_setup.has_value()) {
            write_json_section_page_setup(std::cout, *page_setup);
        } else {
            std::cout << "null";
        }
        std::cout << "}\n";
        return true;
    }

    std::cout << "section: " << section_index << '\n';
    std::cout << "present: " << yes_no(page_setup.has_value()) << '\n';
    if (page_setup.has_value()) {
        print_section_page_setup_details(std::cout, *page_setup);
    }
    return true;
}

auto inspect_page_setups(featherdoc::Document &doc, std::string_view command,
                         bool json_output) -> bool {
    std::vector<std::optional<featherdoc::section_page_setup>> page_setups;
    page_setups.reserve(doc.section_count());
    for (std::size_t section_index = 0; section_index < doc.section_count();
         ++section_index) {
        auto page_setup = doc.get_section_page_setup(section_index);
        if (!page_setup.has_value() && doc.last_error().code) {
            return report_document_error(command, "inspect", doc.last_error(),
                                         json_output);
        }

        page_setups.push_back(std::move(page_setup));
    }

    if (json_output) {
        std::cout << "{\"count\":" << page_setups.size() << ",\"sections\":[";
        for (std::size_t section_index = 0; section_index < page_setups.size();
             ++section_index) {
            if (section_index != 0U) {
                std::cout << ',';
            }

            std::cout << "{\"section\":" << section_index
                      << ",\"present\":"
                      << json_bool(page_setups[section_index].has_value())
                      << ",\"page_setup\":";
            if (page_setups[section_index].has_value()) {
                write_json_section_page_setup(std::cout,
                                              *page_setups[section_index]);
            } else {
                std::cout << "null";
            }
            std::cout << '}';
        }
        std::cout << "]}\n";
        return true;
    }

    std::cout << "sections: " << page_setups.size() << '\n';
    for (std::size_t section_index = 0; section_index < page_setups.size();
         ++section_index) {
        std::cout << "section[" << section_index
                  << "]: present=" << yes_no(page_setups[section_index].has_value());
        if (page_setups[section_index].has_value()) {
            std::cout << ' ';
            print_section_page_setup_summary(std::cout, *page_setups[section_index]);
        }
        std::cout << '\n';
    }

    return true;
}

auto resolve_section_page_setup(featherdoc::Document &doc, std::size_t section_index,
                                const set_section_page_setup_options &options,
                                std::string_view command, bool json_output,
                                featherdoc::section_page_setup &setup) -> bool {
    const auto existing = doc.get_section_page_setup(section_index);
    if (!existing.has_value() && doc.last_error().code) {
        return report_document_error(command, "inspect", doc.last_error(), json_output);
    }

    std::vector<std::string> missing_options;
    auto use_existing_or_require =
        [&]<typename T>(const std::optional<T> &specified, const T *existing_value,
                        T &target, const char *option_name) {
            if (specified.has_value()) {
                target = *specified;
                return;
            }
            if (existing_value != nullptr) {
                target = *existing_value;
                return;
            }
            missing_options.emplace_back(option_name);
        };

    const auto *existing_setup = existing.has_value() ? &*existing : nullptr;
    use_existing_or_require(options.orientation,
                            existing_setup != nullptr ? &existing_setup->orientation
                                                      : nullptr,
                            setup.orientation, "--orientation");
    use_existing_or_require(options.width_twips,
                            existing_setup != nullptr ? &existing_setup->width_twips
                                                      : nullptr,
                            setup.width_twips, "--width");
    use_existing_or_require(options.height_twips,
                            existing_setup != nullptr ? &existing_setup->height_twips
                                                      : nullptr,
                            setup.height_twips, "--height");
    use_existing_or_require(options.margin_top_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.top_twips
                                : nullptr,
                            setup.margins.top_twips, "--margin-top");
    use_existing_or_require(options.margin_bottom_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.bottom_twips
                                : nullptr,
                            setup.margins.bottom_twips, "--margin-bottom");
    use_existing_or_require(options.margin_left_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.left_twips
                                : nullptr,
                            setup.margins.left_twips, "--margin-left");
    use_existing_or_require(options.margin_right_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.right_twips
                                : nullptr,
                            setup.margins.right_twips, "--margin-right");
    use_existing_or_require(options.margin_header_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.header_twips
                                : nullptr,
                            setup.margins.header_twips, "--margin-header");
    use_existing_or_require(options.margin_footer_twips,
                            existing_setup != nullptr
                                ? &existing_setup->margins.footer_twips
                                : nullptr,
                            setup.margins.footer_twips, "--margin-footer");

    if (!missing_options.empty()) {
        std::string detail =
            "section does not expose explicit page setup; supply ";
        for (std::size_t index = 0; index < missing_options.size(); ++index) {
            if (index != 0U) {
                detail += index + 1U == missing_options.size() ? ", and " : ", ";
            }
            detail += missing_options[index];
        }

        featherdoc::document_error_info error_info{};
        error_info.code = std::make_error_code(std::errc::invalid_argument);
        error_info.detail = std::move(detail);
        return report_operation_failure(command, "input", "invalid page setup input",
                                        error_info, json_output);
    }

    if (options.clear_page_number_start) {
        setup.page_number_start = std::nullopt;
    } else if (options.page_number_start.has_value()) {
        setup.page_number_start = options.page_number_start;
    } else if (existing_setup != nullptr) {
        setup.page_number_start = existing_setup->page_number_start;
    } else {
        setup.page_number_start = std::nullopt;
    }

    return true;
}

void inspect_related_parts(const std::vector<inspected_part_entry> &parts,
                           section_part_family family, bool json_output) {
    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, section_part_name(family));
        std::cout << ",\"count\":" << parts.size() << ",\"parts\":[";
        for (std::size_t index = 0; index < parts.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }

            const auto &part = parts[index];
            std::cout << "{\"index\":" << index << ",\"relationship_id\":";
            write_json_string(std::cout, part.relationship_id);
            std::cout << ",\"entry\":";
            write_json_string(std::cout, part.entry_name);
            std::cout << ",\"references\":";
            write_json_part_references(std::cout, part.references);
            std::cout << ",\"paragraphs\":";
            write_json_lines(std::cout, part.paragraphs);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << section_part_name(family) << " parts: " << parts.size() << '\n';
    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto &part = parts[index];
        std::cout << "part[" << index << "]: entry=" << part.entry_name
                  << " relationship=" << part.relationship_id << " references=";
        if (part.references.empty()) {
            std::cout << "none";
        } else {
            for (std::size_t reference_index = 0; reference_index < part.references.size();
                 ++reference_index) {
                if (reference_index != 0U) {
                    std::cout << ", ";
                }
                std::cout << "section[" << part.references[reference_index].section_index
                          << "]:" << part.references[reference_index].kind;
            }
        }
        std::cout << '\n';

        for (std::size_t paragraph_index = 0; paragraph_index < part.paragraphs.size();
             ++paragraph_index) {
            std::cout << "  paragraph[" << paragraph_index
                      << "]: " << part.paragraphs[paragraph_index] << '\n';
        }
    }
}

void write_json_command_error(std::ostream &stream, std::string_view command,
                              std::string_view stage, std::string_view message,
                              const featherdoc::document_error_info *error_info = nullptr) {
    stream << "{\"command\":";
    write_json_string(stream, command);
    stream << ",\"ok\":false,\"stage\":";
    write_json_string(stream, stage);
    stream << ",\"message\":";
    write_json_string(stream, message);

    if (error_info != nullptr) {
        if (!error_info->detail.empty()) {
            stream << ",\"detail\":";
            write_json_string(stream, error_info->detail);
        }
        if (!error_info->entry_name.empty()) {
            stream << ",\"entry\":";
            write_json_string(stream, error_info->entry_name);
        }
        if (error_info->xml_offset.has_value()) {
            stream << ",\"xml_offset\":" << *error_info->xml_offset;
        }
    }

    stream << "}\n";
}

void print_parse_error(std::string_view command, const std::string &message,
                       bool json_output) {
    if (json_output) {
        write_json_command_error(std::cerr, command, "parse", message);
        return;
    }

    print_parse_error(message);
}

auto report_operation_failure(std::string_view command, std::string_view stage,
                              std::string_view fallback_message,
                              const featherdoc::document_error_info &error_info,
                              bool json_output) -> bool {
    if (json_output) {
        const auto message = error_info.code
                                 ? error_info.code.message()
                                 : std::string{fallback_message};
        write_json_command_error(std::cerr, command, stage, message, &error_info);
        return false;
    }

    if (error_info.code) {
        print_error_info(error_info);
        return false;
    }

    std::cerr << fallback_message << '\n';
    return false;
}

auto report_document_error(std::string_view command, std::string_view stage,
                           const featherdoc::document_error_info &error_info,
                           bool json_output) -> bool {
    return report_operation_failure(command, stage, "operation failed", error_info,
                                    json_output);
}

template <typename ExtraWriter>
void write_json_mutation_result(std::string_view command, featherdoc::Document &doc,
                                const std::optional<path_type> &output_path,
                                ExtraWriter &&write_extra) {
    std::cout << "{\"command\":";
    write_json_string(std::cout, command);
    std::cout << ",\"ok\":true"
              << ",\"in_place\":" << json_bool(!output_path.has_value())
              << ",\"sections\":" << doc.section_count()
              << ",\"headers\":" << doc.header_count()
              << ",\"footers\":" << doc.footer_count();
    write_extra(std::cout);
    std::cout << "}\n";
}

void write_json_mutation_result(std::string_view command, featherdoc::Document &doc,
                                const std::optional<path_type> &output_path) {
    write_json_mutation_result(command, doc, output_path,
                               [](std::ostream &) {});
}

void inspect_sections(featherdoc::Document &doc, bool json_output) {
    if (json_output) {
        std::cout << "{\"sections\":" << doc.section_count()
                  << ",\"headers\":" << doc.header_count()
                  << ",\"footers\":" << doc.footer_count()
                  << ",\"section_layouts\":[";
        for (std::size_t section_index = 0; section_index < doc.section_count();
             ++section_index) {
            if (section_index != 0U) {
                std::cout << ',';
            }

            std::cout << "{\"index\":" << section_index << ",\"header\":";
            write_json_section_flags(std::cout, doc, section_index,
                                     section_part_family::header);
            std::cout << ",\"footer\":";
            write_json_section_flags(std::cout, doc, section_index,
                                     section_part_family::footer);
            std::cout << '}';
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "sections: " << doc.section_count() << '\n';
    std::cout << "headers: " << doc.header_count() << '\n';
    std::cout << "footers: " << doc.footer_count() << '\n';

    for (std::size_t section_index = 0; section_index < doc.section_count();
         ++section_index) {
        std::cout
            << "section[" << section_index << "]: header(default="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::default_reference))
            << ", first="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::first_page))
            << ", even="
            << yes_no(has_section_header(
                   doc, section_index,
                   featherdoc::section_reference_kind::even_page))
            << ") footer(default="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::default_reference))
            << ", first="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::first_page))
            << ", even="
            << yes_no(has_section_footer(
                   doc, section_index,
                   featherdoc::section_reference_kind::even_page))
            << ")\n";
    }
}

auto read_text_source(const section_text_options &options, std::string &text,
                      std::string &error_message) -> bool {
    if (options.text.has_value()) {
        text = *options.text;
        return true;
    }

    if (!options.text_file.has_value()) {
        error_message = "expected --text <text> or --text-file <path>";
        return false;
    }

    std::ifstream stream(*options.text_file, std::ios::binary);
    if (!stream.good()) {
        error_message =
            "failed to read text file: " + options.text_file->string();
        return false;
    }

    text.assign(std::istreambuf_iterator<char>(stream),
                std::istreambuf_iterator<char>());
    return true;
}

auto save_document(featherdoc::Document &doc,
                   const std::optional<path_type> &output_path,
                   std::string_view command = {}, bool json_output = false) -> bool {
    const auto error =
        output_path.has_value() ? doc.save_as(*output_path) : doc.save();
    if (error) {
        return report_document_error(command, "save", doc.last_error(), json_output);
    }

    return true;
}

auto open_document(const path_type &input_path, featherdoc::Document &doc,
                   std::string_view command = {}, bool json_output = false) -> bool {
    doc.set_path(input_path);
    const auto error = doc.open();
    if (error) {
        return report_document_error(command, "open", doc.last_error(), json_output);
    }

    return true;
}

auto show_section_text(featherdoc::Document &doc, section_part_family family,
                       std::size_t section_index,
                       featherdoc::section_reference_kind reference_kind,
                       bool json_output) -> bool {
    const auto lines = collect_part_lines(
        section_part_paragraphs(doc, family, section_index, reference_kind));

    if (json_output) {
        std::cout << "{\"part\":";
        write_json_string(std::cout, section_part_name(family));
        std::cout << ",\"section\":" << section_index << ",\"kind\":";
        write_json_string(std::cout,
                          featherdoc::to_xml_reference_type(reference_kind));
        std::cout << ",\"present\":" << (!lines.empty() ? "true" : "false")
                  << ",\"paragraphs\":";
        write_json_lines(std::cout, lines);
        std::cout << "}\n";
        return true;
    }

    if (lines.empty()) {
        std::cerr << "section " << section_part_name(family)
                  << " reference not found\n";
        return false;
    }

    for (const auto &line : lines) {
        std::cout << line << '\n';
    }

    return true;
}

auto assign_section_part(featherdoc::Document &doc, section_part_family family,
                         std::size_t section_index, std::size_t part_index,
                         featherdoc::section_reference_kind reference_kind,
                         std::string_view command, bool json_output) -> bool {
    auto &paragraphs = family == section_part_family::header
                           ? doc.assign_section_header_paragraphs(section_index, part_index,
                                                                 reference_kind)
                           : doc.assign_section_footer_paragraphs(section_index, part_index,
                                                                 reference_kind);
    if (!paragraphs.has_next()) {
        std::string message = "failed to assign section ";
        message += section_part_name(family);
        message += " reference";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

auto remove_section_part_reference(featherdoc::Document &doc,
                                   section_part_family family,
                                   std::size_t section_index,
                                   featherdoc::section_reference_kind reference_kind,
                                   std::string_view command, bool json_output)
    -> bool {
    const auto success =
        family == section_part_family::header
            ? doc.remove_section_header_reference(section_index, reference_kind)
            : doc.remove_section_footer_reference(section_index, reference_kind);
    if (!success) {
        std::string message = "section ";
        message += section_part_name(family);
        message += " reference not found";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

auto remove_part(featherdoc::Document &doc, section_part_family family,
                 std::size_t part_index, std::string_view command,
                 bool json_output) -> bool {
    const auto success = family == section_part_family::header
                             ? doc.remove_header_part(part_index)
                             : doc.remove_footer_part(part_index);
    if (!success) {
        std::string message = std::string(section_part_name(family)) + " part not found";
        return report_operation_failure(command, "mutate", message, doc.last_error(),
                                        json_output);
    }

    return true;
}

auto replace_section_text(featherdoc::Document &doc, section_part_family family,
                          std::size_t section_index, std::string_view replacement_text,
                          featherdoc::section_reference_kind reference_kind,
                          std::string_view command = {}, bool json_output = false)
    -> bool {
    const auto success =
        family == section_part_family::header
            ? doc.replace_section_header_text(section_index, replacement_text,
                                              reference_kind)
            : doc.replace_section_footer_text(section_index, replacement_text,
                                              reference_kind);

    if (!success) {
        return report_document_error(command, "mutate", doc.last_error(), json_output);
    }

    return success;
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

auto select_template_part(featherdoc::Document &doc,
                          validation_part_family part,
                          const std::optional<std::size_t> &part_index,
                          const std::optional<std::size_t> &section_index,
                          featherdoc::section_reference_kind reference_kind,
                          selected_template_part &selected,
                          std::string &error_message) -> bool {
    selected = {};
    selected.family = part;
    selected.part_index = part_index;
    selected.section_index = section_index;

    switch (part) {
    case validation_part_family::body:
        selected.part = doc.body_template();
        if (!selected.part) {
            error_message = "body template not available";
            return false;
        }
        break;
    case validation_part_family::header:
        selected.part_index = part_index.value_or(0U);
        selected.part = doc.header_template(*selected.part_index);
        if (!selected.part) {
            error_message = "header part not found";
            return false;
        }
        break;
    case validation_part_family::footer:
        selected.part_index = part_index.value_or(0U);
        selected.part = doc.footer_template(*selected.part_index);
        if (!selected.part) {
            error_message = "footer part not found";
            return false;
        }
        break;
    case validation_part_family::section_header:
        selected.reference_kind = reference_kind;
        selected.part = doc.section_header_template(*section_index, reference_kind);
        if (!selected.part) {
            error_message = "section header part not found";
            return false;
        }
        break;
    case validation_part_family::section_footer:
        selected.reference_kind = reference_kind;
        selected.part = doc.section_footer_template(*section_index, reference_kind);
        if (!selected.part) {
            error_message = "section footer part not found";
            return false;
        }
        break;
    }

    return true;
}

auto select_mutable_template_part(featherdoc::Document &doc,
                                  validation_part_family part,
                                  const std::optional<std::size_t> &part_index,
                                  const std::optional<std::size_t> &section_index,
                                  featherdoc::section_reference_kind reference_kind,
                                  selected_template_part &selected,
                                  std::string &error_message) -> bool {
    selected = {};
    selected.family = part;
    selected.part_index = part_index;
    selected.section_index = section_index;

    switch (part) {
    case validation_part_family::body:
        selected.part = doc.body_template();
        if (!selected.part) {
            error_message = "body template not available";
            return false;
        }
        break;
    case validation_part_family::header:
        selected.part_index = part_index.value_or(0U);
        if (*selected.part_index == 0U && doc.header_count() == 0U) {
            auto &paragraphs = doc.ensure_header_paragraphs();
            if (!paragraphs.has_next()) {
                error_message = "failed to materialize header part";
                return false;
            }
        }
        selected.part = doc.header_template(*selected.part_index);
        if (!selected.part) {
            error_message = "header part not found";
            return false;
        }
        break;
    case validation_part_family::footer:
        selected.part_index = part_index.value_or(0U);
        if (*selected.part_index == 0U && doc.footer_count() == 0U) {
            auto &paragraphs = doc.ensure_footer_paragraphs();
            if (!paragraphs.has_next()) {
                error_message = "failed to materialize footer part";
                return false;
            }
        }
        selected.part = doc.footer_template(*selected.part_index);
        if (!selected.part) {
            error_message = "footer part not found";
            return false;
        }
        break;
    case validation_part_family::section_header: {
        selected.reference_kind = reference_kind;
        auto &paragraphs = doc.ensure_section_header_paragraphs(*section_index,
                                                                reference_kind);
        if (!paragraphs.has_next()) {
            error_message = "failed to materialize section header part";
            return false;
        }
        selected.part = doc.section_header_template(*section_index, reference_kind);
        if (!selected.part) {
            error_message = "section header part not found";
            return false;
        }
        break;
    }
    case validation_part_family::section_footer: {
        selected.reference_kind = reference_kind;
        auto &paragraphs = doc.ensure_section_footer_paragraphs(*section_index,
                                                                reference_kind);
        if (!paragraphs.has_next()) {
            error_message = "failed to materialize section footer part";
            return false;
        }
        selected.part = doc.section_footer_template(*section_index, reference_kind);
        if (!selected.part) {
            error_message = "section footer part not found";
            return false;
        }
        break;
    }
    }

    return true;
}

auto append_template_part_field(featherdoc::Document &doc,
                                selected_template_part &selected,
                                std::string_view command, bool json_output) -> bool {
    const auto success = command == "append-page-number-field"
                             ? selected.part.append_page_number_field()
                             : selected.part.append_total_pages_field();
    if (!success) {
        return report_document_error(command, "mutate", doc.last_error(), json_output);
    }

    return true;
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
}
} // namespace

int main(int argc, char **argv) {
    const std::vector<std::string_view> arguments(argv + 1, argv + argc);
    if (arguments.empty()) {
        print_usage(std::cerr);
        return 2;
    }

    const auto command = arguments.front();
    featherdoc::Document doc;

    if (command == "inspect-sections") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-sections expects an input path",
                              json_output);
            return 2;
        }

        inspect_options options;
        std::string error_message;
        if (!parse_inspect_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        inspect_sections(doc, options.json_output);
        return 0;
    }

    if (command == "inspect-styles") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-styles expects an input path",
                              json_output);
            return 2;
        }

        inspect_styles_options options;
        std::string error_message;
        if (!parse_inspect_styles_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (options.usage_output && !options.style_id.has_value()) {
            print_parse_error(command, "--usage requires --style", json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (options.style_id.has_value()) {
            const auto style = doc.find_style(*options.style_id);
            if (!style.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            std::optional<featherdoc::style_usage_summary> usage;
            if (options.usage_output) {
                usage = doc.find_style_usage(*options.style_id);
                if (!usage.has_value()) {
                    report_document_error(command, "inspect", doc.last_error(),
                                          options.json_output);
                    return 1;
                }
            }

            inspect_style(*style, usage, options.json_output);
            return 0;
        }

        const auto styles = doc.list_styles();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_styles(styles, options.json_output);
        return 0;
    }

    if (command == "inspect-numbering") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-numbering expects an input path",
                              json_output);
            return 2;
        }

        inspect_numbering_options options;
        std::string error_message;
        if (!parse_inspect_numbering_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (options.instance_id.has_value()) {
            const auto instance = doc.find_numbering_instance(*options.instance_id);
            if (!instance.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            if (options.definition_id.has_value() &&
                instance->definition_id != *options.definition_id) {
                featherdoc::document_error_info error_info{};
                error_info.code = std::make_error_code(std::errc::invalid_argument);
                error_info.detail =
                    "numbering instance id '" + std::to_string(*options.instance_id) +
                    "' was not found under definition id '" +
                    std::to_string(*options.definition_id) + "' in word/numbering.xml";
                error_info.entry_name = "word/numbering.xml";
                report_operation_failure(command, "inspect",
                                         "numbering instance was not found", error_info,
                                         options.json_output);
                return 1;
            }

            inspect_numbering(*instance, options.json_output);
            return 0;
        }

        if (options.definition_id.has_value()) {
            const auto definition = doc.find_numbering_definition(*options.definition_id);
            if (!definition.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            inspect_numbering(*definition, options.json_output);
            return 0;
        }

        const auto definitions = doc.list_numbering_definitions();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_numbering(definitions, options.json_output);
        return 0;
    }

    if (command == "set-paragraph-style-numbering") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-paragraph-style-numbering expects an input path and a style id",
                json_output);
            return 2;
        }

        const auto style_id = arguments[2];
        set_paragraph_style_numbering_options options;
        std::string error_message;
        if (!parse_set_paragraph_style_numbering_options(arguments, 3U, options,
                                                         error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::numbering_definition definition;
        definition.name = *options.definition_name;
        definition.levels = options.levels;

        const auto numbering_id = doc.ensure_numbering_definition(definition);
        if (!numbering_id.has_value()) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        const auto style_level = options.style_level.value_or(0U);
        if (!doc.set_paragraph_style_numbering(style_id, *numbering_id, style_level)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [style_id, style_level, &definition](std::ostream &stream) {
                    stream << ",\"style_id\":";
                    write_json_string(stream, style_id);
                    stream << ",\"definition_name\":";
                    write_json_string(stream, definition.name);
                    stream << ",\"style_level\":" << style_level
                           << ",\"definition_levels\":[";
                    for (std::size_t index = 0; index < definition.levels.size();
                         ++index) {
                        if (index != 0U) {
                            stream << ',';
                        }
                        write_json_numbering_level_definition(stream,
                                                              definition.levels[index]);
                    }
                    stream << ']';
                });
        }

        return 0;
    }

    if (command == "clear-paragraph-style-numbering") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "clear-paragraph-style-numbering expects an input path and a style id",
                json_output);
            return 2;
        }

        const auto style_id = arguments[2];
        clear_paragraph_style_numbering_options options;
        std::string error_message;
        if (!parse_clear_paragraph_style_numbering_options(arguments, 3U, options,
                                                           error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.clear_paragraph_style_numbering(style_id)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(command, doc, options.output_path,
                                       [style_id](std::ostream &stream) {
                                           stream << ",\"style_id\":";
                                           write_json_string(stream, style_id);
                                       });
        }

        return 0;
    }

    if (command == "inspect-page-setup") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-page-setup expects an input path",
                              json_output);
            return 2;
        }

        inspect_page_setup_options options;
        std::string error_message;
        if (!parse_inspect_page_setup_options(arguments, 2U, options,
                                              error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (options.section_index.has_value()) {
            return inspect_page_setup(doc, *options.section_index, command,
                                      options.json_output)
                       ? 0
                       : 1;
        }

        return inspect_page_setups(doc, command, options.json_output) ? 0 : 1;
    }

    if (command == "set-section-page-setup") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-section-page-setup expects an input path and a section index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0U;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        set_section_page_setup_options options;
        std::string error_message;
        if (!parse_set_section_page_setup_options(arguments, 3U, options,
                                                  error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        featherdoc::section_page_setup setup{};
        if (!resolve_section_page_setup(doc, section_index, options, command,
                                        options.json_output, setup)) {
            return 1;
        }

        if (!doc.set_section_page_setup(section_index, setup)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [section_index, &setup](std::ostream &stream) {
                    stream << ",\"section\":" << section_index
                           << ",\"page_setup\":";
                    write_json_section_page_setup(stream, setup);
                });
        }

        return 0;
    }

    if (command == "inspect-bookmarks") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-bookmarks expects an input path",
                              json_output);
            return 2;
        }

        inspect_bookmarks_options options;
        std::string error_message;
        if (!parse_inspect_bookmarks_options(arguments, 2U, options, error_message)) {
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

        if (options.bookmark_name.has_value()) {
            const auto bookmark = selected.part.find_bookmark(*options.bookmark_name);
            if (!bookmark.has_value()) {
                report_document_error(command, "inspect", doc.last_error(),
                                      options.json_output);
                return 1;
            }

            inspect_bookmark(selected, *bookmark, options.json_output);
            return 0;
        }

        const auto bookmarks = selected.part.list_bookmarks();
        if (const auto &error_info = doc.last_error(); error_info.code) {
            report_document_error(command, "inspect", error_info,
                                  options.json_output);
            return 1;
        }

        inspect_bookmarks(selected, bookmarks, options.json_output);
        return 0;
    }

    if (command == "inspect-header-parts" || command == "inspect-footer-parts") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command, "inspect-header/footer-parts expects an input path",
                              json_output);
            return 2;
        }

        inspect_options options;
        std::string error_message;
        if (!parse_inspect_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "inspect-header-parts"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::vector<inspected_part_entry> parts;
        if (!load_inspected_part_entries(path_type(std::string(arguments[1])), doc, family,
                                         parts, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "inspect",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        inspect_related_parts(parts, family, options.json_output);
        return 0;
    }

    if (command == "insert-section") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "insert-section expects an input path and a section index",
                              json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 3U, true, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.insert_section(section_index, options.inherit_header_footer)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            const auto inserted_section = section_index + 1U;
            write_json_mutation_result(
                command, doc, options.output_path,
                [inserted_section, &options](std::ostream &stream) {
                    stream << ",\"section\":" << inserted_section
                           << ",\"inherit_header_footer\":"
                           << json_bool(options.inherit_header_footer);
                });
        }

        return 0;
    }

    if (command == "remove-section") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "remove-section expects an input path and a section index",
                              json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 3U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.remove_section(section_index)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(command, doc, options.output_path,
                                       [section_index](std::ostream &stream) {
                                           stream << ",\"section\":" << section_index;
                                       });
        }

        return 0;
    }

    if (command == "move-section") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "move-section expects an input path, a source index, and a "
                "target index",
                json_output);
            return 2;
        }

        std::size_t source_index = 0;
        std::size_t target_index = 0;
        if (!parse_index(arguments[2], source_index)) {
            print_parse_error(command,
                              "invalid source index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[3], target_index)) {
            print_parse_error(command,
                              "invalid target index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 4U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.move_section(source_index, target_index)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [source_index, target_index](std::ostream &stream) {
                    stream << ",\"source\":" << source_index
                           << ",\"target\":" << target_index;
                });
        }

        return 0;
    }

    if (command == "copy-section-layout") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "copy-section-layout expects an input path, a source index, "
                "and a target index",
                json_output);
            return 2;
        }

        std::size_t source_index = 0;
        std::size_t target_index = 0;
        if (!parse_index(arguments[2], source_index)) {
            print_parse_error(command,
                              "invalid source index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[3], target_index)) {
            print_parse_error(command,
                              "invalid target index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 4U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.copy_section_header_references(source_index, target_index) ||
            !doc.copy_section_footer_references(source_index, target_index)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [source_index, target_index](std::ostream &stream) {
                    stream << ",\"source\":" << source_index
                           << ",\"target\":" << target_index;
                });
        }

        return 0;
    }

    if (command == "assign-section-header" || command == "assign-section-footer") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                "assign-section-header/footer expects an input path, a section index, "
                "and a part index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0;
        std::size_t part_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }
        if (!parse_index(arguments[3], part_index)) {
            print_parse_error(command,
                              "invalid part index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        section_text_options options;
        std::string error_message;
        if (!parse_section_text_options(arguments, 4U, false, true, true, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "assign-section-header"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!assign_section_part(doc, family, section_index, part_index,
                                 options.reference_kind, command,
                                 options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [family, section_index, part_index, &options](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, section_part_name(family));
                    stream << ",\"section\":" << section_index
                           << ",\"kind\":";
                    write_json_string(
                        stream,
                        featherdoc::to_xml_reference_type(options.reference_kind));
                    stream << ",\"part_index\":" << part_index;
                });
        }

        return 0;
    }

    if (command == "remove-section-header" || command == "remove-section-footer") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "remove-section-header/footer expects an input path and a "
                "section index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        section_text_options options;
        std::string error_message;
        if (!parse_section_text_options(arguments, 3U, false, true, true, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "remove-section-header"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!remove_section_part_reference(doc, family, section_index,
                                           options.reference_kind, command,
                                           options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [family, section_index, &options](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, section_part_name(family));
                    stream << ",\"section\":" << section_index
                           << ",\"kind\":";
                    write_json_string(
                        stream,
                        featherdoc::to_xml_reference_type(options.reference_kind));
                });
        }

        return 0;
    }

    if (command == "remove-header-part" || command == "remove-footer-part") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(command,
                              "remove-header/footer-part expects an input path and a "
                              "part index",
                              json_output);
            return 2;
        }

        std::size_t part_index = 0;
        if (!parse_index(arguments[2], part_index)) {
            print_parse_error(command,
                              "invalid part index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 3U, false, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        const auto family = command == "remove-header-part"
                                ? section_part_family::header
                                : section_part_family::footer;
        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!remove_part(doc, family, part_index, command, options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [family, part_index](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, section_part_name(family));
                    stream << ",\"part_index\":" << part_index;
                });
        }

        return 0;
    }

    if (command == "show-section-header" || command == "show-section-footer") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "show-section-header/footer expects an input path and a "
                "section index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        section_text_options options;
        std::string error_message;
        if (!parse_section_text_options(arguments, 3U, false, false, true, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        return show_section_text(
                   doc,
                   command == "show-section-header"
                       ? section_part_family::header
                       : section_part_family::footer,
                   section_index, options.reference_kind, options.json_output)
                   ? 0
                   : 1;
    }

    if (command == "set-section-header" || command == "set-section-footer") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-section-header/footer expects an input path and a "
                "section index",
                json_output);
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error(command,
                              "invalid section index: " + std::string(arguments[2]),
                              json_output);
            return 2;
        }

        section_text_options options;
        std::string error_message;
        if (!parse_section_text_options(arguments, 3U, true, true, true, options,
                                        error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        std::string replacement_text;
        if (!read_text_source(options, replacement_text, error_message)) {
            if (options.json_output) {
                write_json_command_error(std::cerr, command, "input",
                                         error_message);
            } else {
                std::cerr << error_message << '\n';
            }
            return 1;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!replace_section_text(
                doc,
                command == "set-section-header" ? section_part_family::header
                                                : section_part_family::footer,
                section_index, replacement_text, options.reference_kind, command,
                options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [command, section_index, &options](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(
                        stream, command == "set-section-header" ? "header" : "footer");
                    stream << ",\"section\":" << section_index << ",\"kind\":";
                    write_json_string(
                        stream,
                        featherdoc::to_xml_reference_type(options.reference_kind));
                });
        }

        return 0;
    }

    if (command == "append-page-number-field" ||
        command == "append-total-pages-field") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                std::string(command) + " expects an input path and field target options",
                json_output);
            return 2;
        }

        append_field_options options;
        std::string error_message;
        if (!parse_append_field_options(arguments, 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        selected_template_part selected;
        if (!select_mutable_template_part(doc, options.part, options.part_index,
                                          options.section_index,
                                          options.reference_kind, selected,
                                          error_message)) {
            report_operation_failure(command, "mutate", error_message,
                                     doc.last_error(), options.json_output);
            return 1;
        }

        if (!append_template_part_field(doc, selected, command, options.json_output)) {
            return 1;
        }

        if (!save_document(doc, options.output_path, command, options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [&selected, command](std::ostream &stream) {
                    stream << ",\"part\":";
                    write_json_string(stream, validation_part_name(selected.family));
                    if (selected.part_index.has_value()) {
                        stream << ",\"part_index\":" << *selected.part_index;
                    }
                    if (selected.section_index.has_value()) {
                        stream << ",\"section\":" << *selected.section_index;
                    }
                    if (selected.reference_kind.has_value()) {
                        stream << ",\"kind\":";
                        write_json_string(
                            stream,
                            featherdoc::to_xml_reference_type(*selected.reference_kind));
                    }
                    stream << ",\"entry_name\":";
                    write_json_string(stream, std::string(selected.part.entry_name()));
                    stream << ",\"field\":";
                    write_json_string(stream, template_field_name(command));
                });
        }

        return 0;
    }

    if (command == "validate-template") {
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

    print_parse_error("unknown command: " + std::string(command));
    return 2;
}
