#include "featherdoc_cli_review_revision_commands.hpp"

#include "featherdoc_cli_command_support.hpp"
#include "featherdoc_cli_document_mutation_options_parse.hpp"
#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_review_output.hpp"

#include <cstddef>
#include <iostream>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace featherdoc_cli {
namespace {

auto validate_revision_expected_text(featherdoc::Document &doc,
                                     std::string_view command,
                                     const revision_authoring_options &options,
                                     std::size_t start_paragraph_index,
                                     std::size_t start_text_offset,
                                     std::size_t end_paragraph_index,
                                     std::size_t end_text_offset) -> bool {
    if (!options.has_expected_text) {
        return true;
    }

    const auto preview = doc.preview_text_range(
        start_paragraph_index, start_text_offset, end_paragraph_index,
        end_text_offset);
    if (!preview.has_value()) {
        return report_document_error(command, "preview", doc.last_error(),
                                     options.json_output);
    }

    if (preview->text != options.expected_text) {
        return report_expected_revision_text_mismatch(
            command, options.expected_text, *preview, options.json_output);
    }

    return true;
}

} // namespace

auto is_review_revision_command(std::string_view command) -> bool {
    return command == "preview-text-range" ||
           command == "append-insertion-revision" ||
           command == "append-deletion-revision" ||
           command == "insert-run-revision-after" ||
           command == "delete-run-revision" ||
           command == "replace-run-revision" ||
           command == "insert-paragraph-text-revision" ||
           command == "delete-paragraph-text-revision" ||
           command == "replace-paragraph-text-revision" ||
           command == "insert-text-range-revision" ||
           command == "delete-text-range-revision" ||
           command == "replace-text-range-revision" ||
           command == "set-revision-metadata" ||
           command == "accept-revision" ||
           command == "reject-revision" ||
           command == "accept-all-revisions" ||
           command == "reject-all-revisions";
}

auto run_review_revision_command(
    std::string_view command, const std::vector<std::string_view> &arguments,
    featherdoc::Document &doc) -> int {
#include "featherdoc_cli_review_revision_preview_commands.inc"

#include "featherdoc_cli_review_revision_append_run_commands.inc"

#include "featherdoc_cli_review_revision_paragraph_text_commands.inc"

#include "featherdoc_cli_review_revision_text_range_commands.inc"

#include "featherdoc_cli_review_revision_metadata_accept_commands.inc"
}

} // namespace featherdoc_cli
