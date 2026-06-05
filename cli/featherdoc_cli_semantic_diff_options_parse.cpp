#include "featherdoc_cli_semantic_diff_options_parse.hpp"

#include "featherdoc_cli_parse.hpp"

namespace featherdoc_cli {

auto parse_semantic_diff_options(
    const std::vector<std::string_view> &arguments, std::size_t start_index,
    semantic_diff_options &options, std::string &error_message) -> bool {
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        const auto argument = arguments[index];
        if (argument == "--json") {
            options.json_output = true;
            continue;
        }

        if (argument == "--fail-on-diff") {
            options.fail_on_diff = true;
            continue;
        }

        if (argument == "--include-image-relationship-ids") {
            options.diff_options.compare_image_relationship_ids = true;
            continue;
        }

        if (argument == "--include-content-control-ids") {
            options.diff_options.compare_content_control_ids = true;
            continue;
        }

        if (argument == "--index-alignment") {
            options.diff_options.align_sequences_by_content = false;
            continue;
        }

        if (argument == "--content-alignment") {
            options.diff_options.align_sequences_by_content = true;
            continue;
        }

        if (argument == "--alignment-cell-limit") {
            if (index + 1U >= arguments.size() ||
                !parse_index(arguments[index + 1U],
                             options.diff_options.alignment_cell_limit)) {
                error_message = "--alignment-cell-limit expects a non-negative integer";
                return false;
            }
            ++index;
            continue;
        }

        if (argument == "--no-paragraphs") {
            options.diff_options.compare_paragraphs = false;
            continue;
        }

        if (argument == "--no-tables") {
            options.diff_options.compare_tables = false;
            continue;
        }

        if (argument == "--no-images") {
            options.diff_options.compare_images = false;
            continue;
        }

        if (argument == "--no-content-controls") {
            options.diff_options.compare_content_controls = false;
            continue;
        }

        if (argument == "--no-fields") {
            options.diff_options.compare_fields = false;
            continue;
        }

        if (argument == "--no-styles") {
            options.diff_options.compare_styles = false;
            continue;
        }

        if (argument == "--no-numbering") {
            options.diff_options.compare_numbering = false;
            continue;
        }

        if (argument == "--no-footnotes") {
            options.diff_options.compare_footnotes = false;
            continue;
        }

        if (argument == "--no-endnotes") {
            options.diff_options.compare_endnotes = false;
            continue;
        }

        if (argument == "--no-comments") {
            options.diff_options.compare_comments = false;
            continue;
        }

        if (argument == "--no-revisions") {
            options.diff_options.compare_revisions = false;
            continue;
        }

        if (argument == "--no-template-parts") {
            options.diff_options.compare_template_parts = false;
            continue;
        }

        if (argument == "--no-resolved-section-template-parts") {
            options.diff_options.compare_resolved_section_template_parts = false;
            continue;
        }

        if (argument == "--no-sections") {
            options.diff_options.compare_sections = false;
            continue;
        }

        error_message = "unknown option: " + std::string(argument);
        return false;
    }

    return true;
}

} // namespace featherdoc_cli
