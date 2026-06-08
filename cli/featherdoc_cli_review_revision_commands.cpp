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
    if (command == "preview-text-range") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 6U) {
            print_parse_error(
                command,
                "preview-text-range expects an input path, start paragraph index, start offset, end paragraph index, and end offset",
                json_output);
            return 2;
        }
        if (arguments.size() > 7U ||
            (arguments.size() == 7U && arguments[6] != "--json")) {
            print_parse_error(command, "unknown option: " +
                                           std::string(arguments[6]),
                              json_output);
            return 2;
        }

        std::size_t start_paragraph_index = 0U;
        if (!parse_index(arguments[2], start_paragraph_index)) {
            print_parse_error(command,
                              "invalid start paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t start_text_offset = 0U;
        if (!parse_index(arguments[3], start_text_offset)) {
            print_parse_error(command,
                              "invalid start text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t end_paragraph_index = 0U;
        if (!parse_index(arguments[4], end_paragraph_index)) {
            print_parse_error(command,
                              "invalid end paragraph index: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        std::size_t end_text_offset = 0U;
        if (!parse_index(arguments[5], end_text_offset)) {
            print_parse_error(command,
                              "invalid end text offset: " +
                                  std::string(arguments[5]),
                              json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           json_output)) {
            return 1;
        }

        const auto preview = doc.preview_text_range(
            start_paragraph_index, start_text_offset, end_paragraph_index,
            end_text_offset);
        if (!preview.has_value()) {
            report_document_error(command, "preview", doc.last_error(),
                                  json_output);
            return 1;
        }

        if (json_output) {
            std::cout << "{\"command\":\"preview-text-range\",\"ok\":true,"
                      << "\"preview\":";
            write_json_text_range_preview(std::cout, *preview);
            std::cout << "}\n";
        } else {
            print_text_range_preview(std::cout, *preview);
            std::cout << '\n';
        }
        return 0;
    }

    if (command == "append-insertion-revision" ||
        command == "append-deletion-revision") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 2U) {
            print_parse_error(command,
                              std::string(command) + " expects an input path",
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        if (!parse_revision_authoring_options(arguments, 2U, options,
                                              error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        const bool insertion = command == "append-insertion-revision";
        const auto affected = insertion
                                  ? doc.append_insertion_revision(
                                        options.text, options.author, options.date)
                                  : doc.append_deletion_revision(
                                        options.text, options.author, options.date);
        if (affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    if (command == "insert-run-revision-after" ||
        command == "delete-run-revision" ||
        command == "replace-run-revision") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 4U) {
            print_parse_error(
                command,
                std::string(command) +
                    " expects an input path, paragraph index, and run index",
                json_output);
            return 2;
        }

        std::size_t paragraph_index = 0U;
        if (!parse_index(arguments[2], paragraph_index)) {
            print_parse_error(command,
                              "invalid paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t run_index = 0U;
        if (!parse_index(arguments[3], run_index)) {
            print_parse_error(command,
                              "invalid run index: " + std::string(arguments[3]),
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-run-revision";
        if (!parse_revision_authoring_options(arguments, 4U, options,
                                              error_message, require_text)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        bool ok = false;
        if (command == "insert-run-revision-after") {
            ok = doc.insert_run_revision_after(
                paragraph_index, run_index, options.text, options.author,
                options.date);
        } else if (command == "delete-run-revision") {
            ok = doc.delete_run_revision(paragraph_index, run_index,
                                         options.author, options.date);
        } else {
            ok = doc.replace_run_revision(paragraph_index, run_index,
                                          options.text, options.author,
                                          options.date);
        }
        if (!ok) {
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
                [paragraph_index, run_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"paragraph_index\":" << paragraph_index
                           << ",\"run_index\":" << run_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "insert-paragraph-text-revision" ||
        command == "delete-paragraph-text-revision" ||
        command == "replace-paragraph-text-revision") {
        const auto json_output = has_json_flag(arguments);
        const bool insert_revision = command == "insert-paragraph-text-revision";
        if (arguments.size() < (insert_revision ? 4U : 5U)) {
            print_parse_error(
                command,
                insert_revision
                    ? std::string(command) +
                          " expects an input path, paragraph index, and text offset"
                    : std::string(command) +
                          " expects an input path, paragraph index, text offset, and text length",
                json_output);
            return 2;
        }

        std::size_t paragraph_index = 0U;
        if (!parse_index(arguments[2], paragraph_index)) {
            print_parse_error(command,
                              "invalid paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t text_offset = 0U;
        if (!parse_index(arguments[3], text_offset)) {
            print_parse_error(command,
                              "invalid text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t text_length = 0U;
        if (!insert_revision && !parse_index(arguments[4], text_length)) {
            print_parse_error(command,
                              "invalid text length: " +
                                  std::string(arguments[4]),
                              json_output);
            return 2;
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-paragraph-text-revision";
        const auto options_start = insert_revision ? 4U : 5U;
        if (!parse_revision_authoring_options(
                arguments, options_start, options, error_message, require_text,
                "delete-paragraph-text-revision does not accept --text",
                !insert_revision,
                "insert-paragraph-text-revision does not accept --expected-text")) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!insert_revision) {
            if (text_length >
                std::numeric_limits<std::size_t>::max() - text_offset) {
                featherdoc::document_error_info error_info{};
                error_info.code =
                    std::make_error_code(std::errc::invalid_argument);
                error_info.entry_name = "word/document.xml";
                error_info.detail = "text range is out of range";
                report_operation_failure(command, "validate",
                                         "text range is out of range",
                                         error_info, options.json_output);
                return 1;
            }

            if (!validate_revision_expected_text(
                    doc, command, options, paragraph_index, text_offset,
                    paragraph_index, text_offset + text_length)) {
                return 1;
            }
        }

        bool ok = false;
        if (insert_revision) {
            ok = doc.insert_paragraph_text_revision(
                paragraph_index, text_offset, options.text, options.author,
                options.date);
        } else if (command == "delete-paragraph-text-revision") {
            ok = doc.delete_paragraph_text_revision(
                paragraph_index, text_offset, text_length, options.author,
                options.date);
        } else {
            ok = doc.replace_paragraph_text_revision(
                paragraph_index, text_offset, text_length, options.text,
                options.author, options.date);
        }
        if (!ok) {
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
                [insert_revision, paragraph_index, text_offset,
                 text_length](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"paragraph_index\":" << paragraph_index
                           << ",\"text_offset\":" << text_offset;
                    if (!insert_revision) {
                        stream << ",\"text_length\":" << text_length;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "insert-text-range-revision" ||
        command == "delete-text-range-revision" ||
        command == "replace-text-range-revision") {
        const auto json_output = has_json_flag(arguments);
        const bool insert_revision = command == "insert-text-range-revision";
        if (arguments.size() < (insert_revision ? 4U : 6U)) {
            print_parse_error(
                command,
                insert_revision
                    ? std::string(command) +
                          " expects an input path, paragraph index, and text offset"
                    : std::string(command) +
                          " expects an input path, start paragraph index, start offset, end paragraph index, and end offset",
                json_output);
            return 2;
        }

        std::size_t start_paragraph_index = 0U;
        if (!parse_index(arguments[2], start_paragraph_index)) {
            print_parse_error(command,
                              "invalid start paragraph index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        std::size_t start_text_offset = 0U;
        if (!parse_index(arguments[3], start_text_offset)) {
            print_parse_error(command,
                              "invalid start text offset: " +
                                  std::string(arguments[3]),
                              json_output);
            return 2;
        }

        std::size_t end_paragraph_index = start_paragraph_index;
        std::size_t end_text_offset = start_text_offset;
        if (!insert_revision) {
            if (!parse_index(arguments[4], end_paragraph_index)) {
                print_parse_error(command,
                                  "invalid end paragraph index: " +
                                      std::string(arguments[4]),
                                  json_output);
                return 2;
            }
            if (!parse_index(arguments[5], end_text_offset)) {
                print_parse_error(command,
                                  "invalid end text offset: " +
                                      std::string(arguments[5]),
                                  json_output);
                return 2;
            }
        }

        revision_authoring_options options;
        std::string error_message;
        const bool require_text = command != "delete-text-range-revision";
        const auto options_start = insert_revision ? 4U : 6U;
        if (!parse_revision_authoring_options(
                arguments, options_start, options, error_message, require_text,
                "delete-text-range-revision does not accept --text",
                !insert_revision,
                "insert-text-range-revision does not accept --expected-text")) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!insert_revision &&
            !validate_revision_expected_text(
                doc, command, options, start_paragraph_index, start_text_offset,
                end_paragraph_index, end_text_offset)) {
            return 1;
        }

        bool ok = false;
        if (insert_revision) {
            ok = doc.insert_text_range_revision(
                start_paragraph_index, start_text_offset, options.text,
                options.author, options.date);
        } else if (command == "delete-text-range-revision") {
            ok = doc.delete_text_range_revision(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.author, options.date);
        } else {
            ok = doc.replace_text_range_revision(
                start_paragraph_index, start_text_offset, end_paragraph_index,
                end_text_offset, options.text, options.author, options.date);
        }
        if (!ok) {
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
                [insert_revision, start_paragraph_index, start_text_offset,
                 end_paragraph_index, end_text_offset](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"start_paragraph_index\":"
                           << start_paragraph_index
                           << ",\"start_text_offset\":" << start_text_offset;
                    if (!insert_revision) {
                        stream << ",\"end_paragraph_index\":"
                               << end_paragraph_index
                               << ",\"end_text_offset\":" << end_text_offset;
                    }
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "set-revision-metadata") {
        const auto json_output = has_json_flag(arguments);
        if (arguments.size() < 3U) {
            print_parse_error(
                command,
                "set-revision-metadata expects an input path and revision index",
                json_output);
            return 2;
        }

        std::size_t revision_index = 0U;
        if (!parse_index(arguments[2], revision_index)) {
            print_parse_error(command,
                              "invalid revision index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        revision_metadata_mutation_options options;
        std::string error_message;
        if (!parse_revision_metadata_mutation_options(arguments, 3U, options,
                                                      error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        if (!doc.set_revision_metadata(revision_index, options.metadata)) {
            report_document_error(command, "mutate", doc.last_error(),
                                  options.json_output);
            return 1;
        }

        if (!save_document(doc, options.output_path, command,
                           options.json_output)) {
            return 1;
        }

        if (options.json_output) {
            write_json_mutation_result(
                command, doc, options.output_path,
                [revision_index](std::ostream &stream) {
                    write_json_affected_result(stream, 1U);
                    stream << ",\"revision_index\":" << revision_index;
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  1U);
        }
        return 0;
    }

    if (command == "accept-revision" || command == "reject-revision" ||
        command == "accept-all-revisions" || command == "reject-all-revisions") {
        const auto json_output = has_json_flag(arguments);
        const bool accept_one = command == "accept-revision";
        const bool reject_one = command == "reject-revision";
        const bool one_revision = accept_one || reject_one;
        if (arguments.size() < (one_revision ? 3U : 2U)) {
            print_parse_error(command,
                              one_revision ? std::string(command) +
                                                 " expects an input path and revision index"
                                           : std::string(command) +
                                                 " expects an input path",
                              json_output);
            return 2;
        }

        std::size_t revision_index = 0U;
        if (one_revision && !parse_index(arguments[2], revision_index)) {
            print_parse_error(command,
                              "invalid revision index: " +
                                  std::string(arguments[2]),
                              json_output);
            return 2;
        }

        simple_document_mutation_options options;
        std::string error_message;
        if (!parse_simple_document_mutation_options(
                arguments, one_revision ? 3U : 2U, options, error_message)) {
            print_parse_error(command, error_message, json_output);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc, command,
                           options.json_output)) {
            return 1;
        }

        std::size_t affected = 0U;
        if (accept_one) {
            affected = doc.accept_revision(revision_index) ? 1U : 0U;
        } else if (reject_one) {
            affected = doc.reject_revision(revision_index) ? 1U : 0U;
        } else if (command == "accept-all-revisions") {
            affected = doc.accept_all_revisions();
        } else {
            affected = doc.reject_all_revisions();
        }
        if (one_revision && affected == 0U) {
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
                [affected](std::ostream &stream) {
                    write_json_affected_result(stream, affected);
                });
        } else {
            print_simple_document_mutation_result(command, options.output_path,
                                                  affected);
        }
        return 0;
    }

    print_parse_error(command, "unknown review revision command",
                      has_json_flag(arguments));
    return 2;
}

} // namespace featherdoc_cli
