#include "featherdoc_cli_template_inspect_options_parse.hpp"

#include "featherdoc_cli_domain_parse.hpp"

namespace featherdoc_cli {

auto parse_template_part_selection_option(
    const std::vector<std::string_view> &arguments, std::size_t &index,
    validation_part_family &part, std::optional<std::size_t> &part_index,
    std::optional<std::size_t> &section_index,
    featherdoc::section_reference_kind &reference_kind, bool &has_part,
    bool &has_kind, std::string &error_message) -> option_parse_result {
    const auto argument = arguments[index];
    if (argument == "--part") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --part";
            return option_parse_result::error;
        }

        if (!parse_validation_part(arguments[index + 1U], part)) {
            error_message =
                "invalid template part: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        has_part = true;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--index") {
        if (part_index.has_value()) {
            error_message = "duplicate --index option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --index";
            return option_parse_result::error;
        }

        std::size_t parsed_part_index = 0U;
        if (!parse_index(arguments[index + 1U], parsed_part_index)) {
            error_message =
                "invalid part index: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        part_index = parsed_part_index;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--section") {
        if (section_index.has_value()) {
            error_message = "duplicate --section option";
            return option_parse_result::error;
        }
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --section";
            return option_parse_result::error;
        }

        std::size_t parsed_section_index = 0U;
        if (!parse_index(arguments[index + 1U], parsed_section_index)) {
            error_message =
                "invalid section index: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        section_index = parsed_section_index;
        ++index;
        return option_parse_result::matched;
    }

    if (argument == "--kind") {
        if (index + 1U >= arguments.size()) {
            error_message = "missing value after --kind";
            return option_parse_result::error;
        }

        if (!parse_reference_kind(arguments[index + 1U], reference_kind)) {
            error_message =
                "invalid reference kind: " + std::string(arguments[index + 1U]);
            return option_parse_result::error;
        }

        has_kind = true;
        ++index;
        return option_parse_result::matched;
    }

    return option_parse_result::not_matched;
}

auto parse_template_inspect_paragraphs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_paragraphs_options &options, std::string &error_message)
    -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--paragraph") {
            if (options.paragraph_index.has_value()) {
                error_message = "duplicate --paragraph option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --paragraph";
                return false;
            }

            std::size_t paragraph_index = 0U;
            if (!parse_index(arguments[index + 1U], paragraph_index)) {
                error_message =
                    "invalid paragraph index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.paragraph_index = paragraph_index;
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        const auto parse_result = parse_template_part_selection_option(
            arguments, index, options.part, options.part_index,
            options.section_index, options.reference_kind, options.has_part,
            options.has_kind, error_message);
        if (parse_result == option_parse_result::matched) {
            continue;
        }
        if (parse_result == option_parse_result::error) {
            return false;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

auto parse_template_inspect_runs_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    template_inspect_runs_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--run") {
            if (options.run_index.has_value()) {
                error_message = "duplicate --run option";
                return false;
            }
            if (index + 1U >= arguments.size()) {
                error_message = "missing value after --run";
                return false;
            }

            std::size_t run_index = 0U;
            if (!parse_index(arguments[index + 1U], run_index)) {
                error_message =
                    "invalid run index: " + std::string(arguments[index + 1U]);
                return false;
            }

            options.run_index = run_index;
            ++index;
            continue;
        }

        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        const auto parse_result = parse_template_part_selection_option(
            arguments, index, options.part, options.part_index,
            options.section_index, options.reference_kind, options.has_part,
            options.has_kind, error_message);
        if (parse_result == option_parse_result::matched) {
            continue;
        }
        if (parse_result == option_parse_result::error) {
            return false;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return validate_template_part_selection(options.part, options.part_index,
                                            options.section_index, options.has_kind,
                                            "inspection", error_message);
}

} // namespace featherdoc_cli
