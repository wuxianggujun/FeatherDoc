#include "featherdoc_cli_field_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"
#include "featherdoc_cli_parse.hpp"

#include <filesystem>
#include <string>

namespace featherdoc_cli {

auto parse_append_field_options(std::string_view command,
                                const std::vector<std::string_view> &arguments,
                                std::size_t start_index,
                                append_field_options &options,
                                std::string &error_message) -> bool {
    const auto is_generic = command == "append-field";
    const auto is_page_number = command == "append-page-number-field";
    const auto is_total_pages = command == "append-total-pages-field";
    const auto is_table_of_contents =
        command == "append-table-of-contents-field";
    const auto is_page_reference = command == "append-page-reference-field";
    const auto is_style_reference = command == "append-style-reference-field";
    const auto is_document_property = command == "append-document-property-field";
    const auto is_date = command == "append-date-field";
    const auto is_hyperlink = command == "append-hyperlink-field";
    const auto is_reference = command == "append-reference-field";
    const auto is_sequence = command == "append-sequence-field";
    const auto is_caption = command == "append-caption";
    const auto is_index_entry = command == "append-index-entry-field";
    const auto is_index = command == "append-index-field";
    const auto is_complex = command == "append-complex-field";
    const auto is_replace = command == "replace-field";
    const auto requires_field_argument =
        is_generic || is_reference || is_sequence || is_page_reference ||
        is_style_reference || is_document_property || is_hyperlink || is_caption ||
        is_index_entry;
    const auto is_typed_field = requires_field_argument || is_date || is_index ||
                                is_complex;
    const auto supports_result_text = is_table_of_contents || is_typed_field ||
                                      is_replace;
    const auto supports_field_state =
        is_page_number || is_total_pages || is_table_of_contents ||
        is_typed_field;
    const auto supports_preserve_formatting =
        is_reference || is_page_reference || is_style_reference ||
        is_document_property || is_date || is_hyperlink || is_sequence ||
        is_caption || is_index;

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

            options.output_path =
                std::filesystem::path(std::string(arguments[index + 1U]));
            ++index;
            continue;
        }

        if (argument == "--result-text") {
            if (!supports_result_text) {
                error_message = std::string(command) +
                                " does not support --result-text";
                return false;
            }
            if (options.has_result_text) {
                error_message = "duplicate --result-text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --result-text";
                return false;
            }

            options.result_text = std::string(arguments[index + 1U]);
            options.has_result_text = true;
            ++index;
            continue;
        }

        if (argument == "--instruction") {
            if (!is_complex) {
                error_message = std::string(command) + " does not support --instruction";
                return false;
            }
            if (options.instruction.has_value()) {
                error_message = "duplicate --instruction option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --instruction";
                return false;
            }
            options.instruction = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--instruction-before") {
            if (!is_complex) {
                error_message = std::string(command) +
                                " does not support --instruction-before";
                return false;
            }
            if (options.instruction_before.has_value()) {
                error_message = "duplicate --instruction-before option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --instruction-before";
                return false;
            }
            options.instruction_before = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--instruction-after") {
            if (!is_complex) {
                error_message = std::string(command) +
                                " does not support --instruction-after";
                return false;
            }
            if (options.instruction_after.has_value()) {
                error_message = "duplicate --instruction-after option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --instruction-after";
                return false;
            }
            options.instruction_after = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--nested-instruction") {
            if (!is_complex) {
                error_message = std::string(command) +
                                " does not support --nested-instruction";
                return false;
            }
            if (options.nested_instruction.has_value()) {
                error_message = "duplicate --nested-instruction option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --nested-instruction";
                return false;
            }
            options.nested_instruction = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--nested-result-text") {
            if (!is_complex) {
                error_message = std::string(command) +
                                " does not support --nested-result-text";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --nested-result-text";
                return false;
            }
            options.nested_result_text = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--text") {
            if (!is_caption) {
                error_message = std::string(command) + " does not support --text";
                return false;
            }
            if (options.has_caption_text) {
                error_message = "duplicate --text option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --text";
                return false;
            }
            options.caption_text = std::string(arguments[index + 1U]);
            options.has_caption_text = true;
            ++index;
            continue;
        }

        if (argument == "--number-result") {
            if (!is_caption) {
                error_message = std::string(command) +
                                " does not support --number-result";
                return false;
            }
            if (options.has_number_result_text) {
                error_message = "duplicate --number-result option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --number-result";
                return false;
            }
            options.number_result_text = std::string(arguments[index + 1U]);
            options.has_number_result_text = true;
            ++index;
            continue;
        }

        if (argument == "--number-format") {
            if (!is_caption && !is_sequence) {
                error_message = std::string(command) +
                                " does not support --number-format";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --number-format";
                return false;
            }
            options.number_format = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--restart") {
            if (!is_caption && !is_sequence) {
                error_message = std::string(command) + " does not support --restart";
                return false;
            }
            if (options.restart_value.has_value()) {
                error_message = "duplicate --restart option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --restart";
                return false;
            }
            std::uint32_t restart_value = 0U;
            if (!parse_uint32(arguments[index + 1U], restart_value) ||
                restart_value == 0U) {
                error_message = "invalid restart value: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.restart_value = restart_value;
            ++index;
            continue;
        }

        if (argument == "--separator") {
            if (!is_caption) {
                error_message = std::string(command) +
                                " does not support --separator";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --separator";
                return false;
            }
            options.separator = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--columns") {
            if (!is_index) {
                error_message = std::string(command) + " does not support --columns";
                return false;
            }
            if (options.columns.has_value()) {
                error_message = "duplicate --columns option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --columns";
                return false;
            }
            std::uint32_t columns = 0U;
            if (!parse_uint32(arguments[index + 1U], columns) || columns == 0U) {
                error_message = "invalid index columns: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.columns = columns;
            ++index;
            continue;
        }

        if (argument == "--min-outline-level") {
            if (!is_table_of_contents) {
                error_message = std::string(command) +
                                " does not support --min-outline-level";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --min-outline-level";
                return false;
            }
            std::uint32_t level = 0U;
            if (!parse_uint32(arguments[index + 1U], level) || level == 0U ||
                level > 9U) {
                error_message = "invalid TOC minimum outline level: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.min_outline_level = level;
            ++index;
            continue;
        }

        if (argument == "--max-outline-level") {
            if (!is_table_of_contents) {
                error_message = std::string(command) +
                                " does not support --max-outline-level";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --max-outline-level";
                return false;
            }
            std::uint32_t level = 0U;
            if (!parse_uint32(arguments[index + 1U], level) || level == 0U ||
                level > 9U) {
                error_message = "invalid TOC maximum outline level: " +
                                std::string(arguments[index + 1U]);
                return false;
            }
            options.max_outline_level = level;
            ++index;
            continue;
        }

        if (argument == "--subentry") {
            if (!is_index_entry) {
                error_message = std::string(command) +
                                " does not support --subentry";
                return false;
            }
            if (options.subentry.has_value()) {
                error_message = "duplicate --subentry option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --subentry";
                return false;
            }
            options.subentry = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--bookmark") {
            if (!is_index_entry) {
                error_message = std::string(command) +
                                " does not support --bookmark";
                return false;
            }
            if (options.bookmark_name.has_value()) {
                error_message = "duplicate --bookmark option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing name after --bookmark";
                return false;
            }
            options.bookmark_name = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--cross-reference") {
            if (!is_index_entry) {
                error_message = std::string(command) +
                                " does not support --cross-reference";
                return false;
            }
            if (options.cross_reference.has_value()) {
                error_message = "duplicate --cross-reference option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing text after --cross-reference";
                return false;
            }
            options.cross_reference = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--bold-page-number") {
            if (!is_index_entry) {
                error_message = std::string(command) +
                                " does not support --bold-page-number";
                return false;
            }
            options.bold_page_number = true;
            continue;
        }

        if (argument == "--italic-page-number") {
            if (!is_index_entry) {
                error_message = std::string(command) +
                                " does not support --italic-page-number";
                return false;
            }
            options.italic_page_number = true;
            continue;
        }

        if (argument == "--no-hyperlinks") {
            if (!is_table_of_contents) {
                error_message = std::string(command) +
                                " does not support --no-hyperlinks";
                return false;
            }
            options.hyperlinks = false;
            continue;
        }

        if (argument == "--show-page-numbers-in-web-layout") {
            if (!is_table_of_contents) {
                error_message = std::string(command) +
                                " does not support --show-page-numbers-in-web-layout";
                return false;
            }
            options.hide_page_numbers_in_web_layout = false;
            continue;
        }

        if (argument == "--no-outline-levels") {
            if (!is_table_of_contents) {
                error_message = std::string(command) +
                                " does not support --no-outline-levels";
                return false;
            }
            options.use_outline_levels = false;
            continue;
        }

        if (argument == "--no-hyperlink") {
            if (!is_reference && !is_page_reference) {
                error_message = std::string(command) +
                                " does not support --no-hyperlink";
                return false;
            }
            options.hyperlink = false;
            continue;
        }

        if (argument == "--relative-position") {
            if (!is_page_reference && !is_style_reference) {
                error_message = std::string(command) +
                                " does not support --relative-position";
                return false;
            }
            options.relative_position = true;
            continue;
        }

        if (argument == "--paragraph-number") {
            if (!is_style_reference) {
                error_message = std::string(command) +
                                " does not support --paragraph-number";
                return false;
            }
            options.paragraph_number = true;
            continue;
        }

        if (argument == "--no-preserve-formatting") {
            if (!supports_preserve_formatting) {
                error_message = std::string(command) +
                                " does not support --no-preserve-formatting";
                return false;
            }
            options.preserve_formatting = false;
            continue;
        }

        if (argument == "--format") {
            if (!is_date) {
                error_message = std::string(command) +
                                " does not support --format";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --format";
                return false;
            }
            options.date_format = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--anchor") {
            if (!is_hyperlink) {
                error_message = std::string(command) +
                                " does not support --anchor";
                return false;
            }
            if (options.anchor.has_value()) {
                error_message = "duplicate --anchor option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --anchor";
                return false;
            }
            options.anchor = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--tooltip") {
            if (!is_hyperlink) {
                error_message = std::string(command) +
                                " does not support --tooltip";
                return false;
            }
            if (options.tooltip.has_value()) {
                error_message = "duplicate --tooltip option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --tooltip";
                return false;
            }
            options.tooltip = std::string(arguments[index + 1U]);
            ++index;
            continue;
        }

        if (argument == "--dirty") {
            if (!supports_field_state) {
                error_message = std::string(command) +
                                " does not support --dirty";
                return false;
            }
            options.dirty = true;
            continue;
        }

        if (argument == "--locked") {
            if (!supports_field_state) {
                error_message = std::string(command) +
                                " does not support --locked";
                return false;
            }
            options.locked = true;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        if (!argument.empty() && argument.front() != '-') {
            if (is_replace) {
                if (!options.has_field_index) {
                    std::size_t field_index = 0U;
                    if (!parse_index(argument, field_index)) {
                        error_message = "invalid field index: " +
                                        std::string(argument);
                        return false;
                    }
                    options.field_index = field_index;
                    options.has_field_index = true;
                    continue;
                }

                if (!options.has_field_argument) {
                    options.field_argument = std::string(argument);
                    options.has_field_argument = true;
                    continue;
                }

                error_message = "duplicate replace-field instruction: " +
                                std::string(argument);
                return false;
            }

            if (options.has_field_argument) {
                error_message = "duplicate field argument: " + std::string(argument);
                return false;
            }
            options.field_argument = std::string(argument);
            options.has_field_argument = true;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    if (requires_field_argument && !options.has_field_argument) {
        if (is_generic) {
            error_message = "append-field expects <instruction>";
        } else if (is_reference) {
            error_message = "append-reference-field expects <bookmark-name>";
        } else if (is_sequence) {
            error_message = "append-sequence-field expects <identifier>";
        } else if (is_page_reference) {
            error_message = "append-page-reference-field expects <bookmark-name>";
        } else if (is_style_reference) {
            error_message = "append-style-reference-field expects <style-name>";
        } else if (is_document_property) {
            error_message = "append-document-property-field expects <property-name>";
        } else if (is_hyperlink) {
            error_message = "append-hyperlink-field expects <target>";
        } else if (is_caption) {
            error_message = "append-caption expects <label>";
        } else {
            error_message = "append-index-entry-field expects <entry-text>";
        }
        return false;
    }

    if (is_caption && !options.has_caption_text) {
        error_message = "append-caption requires --text <caption-text>";
        return false;
    }

    if (is_complex) {
        const auto has_plain_instruction = options.instruction.has_value();
        const auto has_nested_instruction = options.nested_instruction.has_value();
        if (has_plain_instruction &&
            (options.instruction_before.has_value() || has_nested_instruction ||
             options.instruction_after.has_value())) {
            error_message =
                "append-complex-field --instruction cannot be combined with nested instruction options";
            return false;
        }
        if (!has_plain_instruction && !has_nested_instruction) {
            error_message =
                "append-complex-field requires --instruction or --nested-instruction";
            return false;
        }
        if (has_nested_instruction && (!options.instruction_before.has_value() ||
                                       !options.instruction_after.has_value())) {
            error_message =
                "append-complex-field nested mode requires --instruction-before and --instruction-after";
            return false;
        }
    }

    if (is_table_of_contents &&
        options.min_outline_level > options.max_outline_level) {
        error_message = "TOC minimum outline level must be less than or equal to maximum outline level";
        return false;
    }

    if (is_replace) {
        if (!options.has_field_index) {
            error_message = "replace-field expects <field-index>";
            return false;
        }
        if (!options.has_field_argument) {
            error_message = "replace-field expects <instruction>";
            return false;
        }
    }

    if (!requires_field_argument && !is_replace && options.has_field_argument) {
        error_message = std::string(command) +
                        " does not accept a field argument";
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

auto is_append_field_command(std::string_view command) -> bool {
    return command == "append-field" ||
           command == "append-page-number-field" ||
           command == "append-total-pages-field" ||
           command == "append-table-of-contents-field" ||
           command == "append-reference-field" ||
           command == "append-page-reference-field" ||
           command == "append-style-reference-field" ||
           command == "append-document-property-field" ||
           command == "append-date-field" ||
           command == "append-hyperlink-field" ||
           command == "append-sequence-field" ||
           command == "append-caption" ||
           command == "append-index-field" ||
           command == "append-index-entry-field" ||
           command == "append-complex-field" ||
           command == "replace-field";
}

auto template_field_name(std::string_view command) -> const char * {
    if (command == "append-field") {
        return "field";
    }
    if (command == "append-page-number-field") {
        return "page_number";
    }
    if (command == "append-total-pages-field") {
        return "total_pages";
    }
    if (command == "append-table-of-contents-field") {
        return "table_of_contents";
    }
    if (command == "append-reference-field") {
        return "reference";
    }
    if (command == "append-page-reference-field") {
        return "page_reference";
    }
    if (command == "append-style-reference-field") {
        return "style_reference";
    }
    if (command == "append-document-property-field") {
        return "document_property";
    }
    if (command == "append-date-field") {
        return "date";
    }
    if (command == "append-hyperlink-field") {
        return "hyperlink";
    }
    if (command == "append-sequence-field") {
        return "sequence";
    }
    if (command == "append-caption") {
        return "caption";
    }
    if (command == "append-index-field") {
        return "index";
    }
    if (command == "append-index-entry-field") {
        return "index_entry";
    }
    if (command == "append-complex-field") {
        return "complex";
    }
    if (command == "replace-field") {
        return "replacement";
    }
    return "field";
}

} // namespace featherdoc_cli
