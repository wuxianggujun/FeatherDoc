#include "featherdoc_cli_run_recipe_commands.hpp"

#include "featherdoc_cli_errors.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_parse.hpp"
#include "featherdoc_cli_review_mutation_plan.hpp"
#include "featherdoc_cli_run_recipe_parse.hpp"
#include "featherdoc_cli_text.hpp"

#include <featherdoc.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace featherdoc_cli {
namespace {

using path_type = std::filesystem::path;

struct run_recipe_output_record {
    std::string id;
    std::string type;
    std::string label;
    path_type path;
};

struct run_recipe_batch_replace_result {
    std::size_t documents_count = 0U;
    std::size_t changed_count = 0U;
    std::size_t copied_count = 0U;
    std::size_t replacements_count = 0U;
    std::vector<run_recipe_output_record> outputs;
};

auto require_run_recipe_input(
    const std::unordered_map<std::string, std::string> &inputs,
    std::string_view key, std::string &value, std::string &error_message)
    -> bool {
    const auto entry = inputs.find(std::string(key));
    if (entry == inputs.end()) {
        error_message = "missing required recipe input: " + std::string(key);
        return false;
    }

    value = entry->second;
    return true;
}

auto collect_run_recipe_docx_files(const path_type &source_dir,
                                   std::vector<path_type> &docx_files,
                                   std::string &error_message) -> bool {
    docx_files.clear();

    std::error_code error_code;
    if (!std::filesystem::exists(source_dir, error_code) ||
        !std::filesystem::is_directory(source_dir, error_code)) {
        error_message = "source_dir is not a directory: " + source_dir.string();
        return false;
    }
    if (error_code) {
        error_message = "failed to inspect source_dir: " + error_code.message();
        return false;
    }

    std::filesystem::directory_iterator iterator(source_dir, error_code);
    if (error_code) {
        error_message = "failed to enumerate source_dir: " + error_code.message();
        return false;
    }

    for (const auto &entry : iterator) {
        std::error_code entry_error;
        if (!entry.is_regular_file(entry_error)) {
            if (entry_error) {
                error_message =
                    "failed to inspect source entry: " + entry_error.message();
                return false;
            }
            continue;
        }

        const auto path = entry.path();
        if (is_docx_path(path) && !is_word_temporary_path(path)) {
            docx_files.push_back(path);
        }
    }

    std::sort(docx_files.begin(), docx_files.end());
    if (docx_files.empty()) {
        error_message = "source_dir does not contain any DOCX files";
        return false;
    }

    return true;
}

auto make_run_recipe_output_path(const path_type &output_dir,
                                 const path_type &input_path) -> path_type {
    return output_dir /
           (input_path.stem().string() + std::string("_replaced") +
            input_path.extension().string());
}

auto run_recipe_document_error_message(
    const featherdoc::document_error_info &error) -> std::string {
    if (!error.detail.empty()) {
        return error.detail;
    }
    if (error.code) {
        return error.code.message();
    }
    return "document operation failed";
}

auto execute_run_recipe_batch_replace_document(
    const path_type &input_path, const path_type &output_path,
    std::string_view find_text, std::string_view replace_text,
    std::size_t &replacements_count, bool &copied_without_changes,
    std::string &error_message) -> bool {
    copied_without_changes = false;
    replacements_count = 0U;

    featherdoc::Document doc(input_path);
    if (doc.open()) {
        error_message = input_path.string() + ": " +
                        run_recipe_document_error_message(doc.last_error());
        return false;
    }

    const auto matches = doc.find_text_ranges(find_text);
    if (const auto &error = doc.last_error(); error.code) {
        error_message = input_path.string() + ": " +
                        run_recipe_document_error_message(error);
        return false;
    }

    if (matches.empty()) {
        std::error_code error_code;
        std::filesystem::copy_file(
            input_path, output_path,
            std::filesystem::copy_options::overwrite_existing, error_code);
        if (error_code) {
            error_message =
                "failed to copy unchanged document: " + error_code.message();
            return false;
        }

        copied_without_changes = true;
        return true;
    }

    std::vector<review_mutation_plan_build_request_operation> requests;
    requests.reserve(matches.size());
    for (std::size_t index = 0U; index < matches.size(); ++index) {
        review_mutation_plan_build_request_operation request;
        request.kind =
            review_mutation_plan_operation_kind::replace_text_range_revision;
        request.find_text = std::string(find_text);
        request.occurrence = index;
        request.text = std::string(replace_text);
        request.author = "FeatherDoc Studio";
        requests.push_back(std::move(request));
    }

    std::vector<review_mutation_plan_operation> operations;
    std::vector<review_mutation_plan_build_resolution> resolutions;
    std::size_t failed_operation_index = 0U;
    std::size_t failed_matches_count = 0U;
    std::size_t failed_raw_matches_count = 0U;
    if (!build_review_mutation_plan_operations(
            doc, requests, operations, resolutions, error_message,
            failed_operation_index, failed_matches_count,
            failed_raw_matches_count)) {
        error_message = input_path.string() + ": " + error_message;
        return false;
    }

    const auto previews =
        preview_review_mutation_plan_operations(doc, operations);
    for (const auto &preview : previews) {
        if (!preview.ok) {
            error_message = input_path.string() + ": " + preview.message;
            return false;
        }
    }

    if (find_review_mutation_plan_overlap(operations, error_message)) {
        error_message = input_path.string() + ": " + error_message;
        return false;
    }

    std::size_t applied_count = 0U;
    if (!apply_review_mutation_plan_operations(doc, operations, applied_count,
                                               error_message)) {
        error_message = input_path.string() + ": " + error_message;
        return false;
    }
    static_cast<void>(doc.accept_all_revisions());

    if (doc.save_as(output_path)) {
        error_message = output_path.string() + ": " +
                        run_recipe_document_error_message(doc.last_error());
        return false;
    }

    replacements_count = applied_count;
    return true;
}

auto execute_run_recipe_batch_replace(
    const std::unordered_map<std::string, std::string> &inputs,
    const path_type &output_dir, run_recipe_batch_replace_result &result,
    std::string &error_message) -> bool {
    result = {};

    std::string source_dir_text;
    std::string find_text;
    std::string replace_text;
    if (!require_run_recipe_input(inputs, "source_dir", source_dir_text,
                                  error_message) ||
        !require_run_recipe_input(inputs, "find_text", find_text,
                                  error_message) ||
        !require_run_recipe_input(inputs, "replace_text", replace_text,
                                  error_message)) {
        return false;
    }

    if (source_dir_text.empty()) {
        error_message = "source_dir must not be empty";
        return false;
    }
    if (find_text.empty()) {
        error_message = "find_text must not be empty";
        return false;
    }
    if (replace_text.empty()) {
        error_message = "replace_text must not be empty";
        return false;
    }

    std::error_code error_code;
    std::filesystem::create_directories(output_dir, error_code);
    if (error_code) {
        error_message =
            "failed to create output directory: " + error_code.message();
        return false;
    }

    std::vector<path_type> docx_files;
    if (!collect_run_recipe_docx_files(path_type(source_dir_text), docx_files,
                                       error_message)) {
        return false;
    }

    result.documents_count = docx_files.size();
    for (const auto &input_path : docx_files) {
        const auto output_path =
            make_run_recipe_output_path(output_dir, input_path);

        std::size_t document_replacements_count = 0U;
        bool copied_without_changes = false;
        if (!execute_run_recipe_batch_replace_document(
                input_path, output_path, find_text, replace_text,
                document_replacements_count, copied_without_changes,
                error_message)) {
            return false;
        }

        if (copied_without_changes) {
            ++result.copied_count;
        } else {
            ++result.changed_count;
            result.replacements_count += document_replacements_count;
        }
    }

    result.outputs.push_back(
        {"result_folder", "folder", "Output folder", output_dir});
    return true;
}

void write_json_run_recipe_outputs(
    std::ostream &stream, const std::vector<run_recipe_output_record> &outputs) {
    stream << '[';
    for (std::size_t index = 0U; index < outputs.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }

        const auto &output = outputs[index];
        stream << "{\"id\":";
        write_json_string(stream, output.id);
        stream << ",\"type\":";
        write_json_string(stream, output.type);
        stream << ",\"label\":";
        write_json_string(stream, output.label);
        stream << ",\"path\":";
        write_json_string(stream, output.path.string());
        stream << '}';
    }
    stream << ']';
}

void write_json_run_recipe_batch_replace_success(
    std::string_view recipe_id, const run_recipe_batch_replace_result &result) {
    std::cout << "{\"command\":\"run-recipe\",\"ok\":true,\"recipe_id\":";
    write_json_string(std::cout, recipe_id);
    std::cout << ",\"documents_count\":" << result.documents_count
              << ",\"changed_count\":" << result.changed_count
              << ",\"copied_count\":" << result.copied_count
              << ",\"replacements_count\":" << result.replacements_count
              << ",\"outputs\":";
    write_json_run_recipe_outputs(std::cout, result.outputs);
    std::cout << "}\n";
}

void print_run_recipe_batch_replace_success(
    const run_recipe_batch_replace_result &result) {
    std::cout << "run-recipe batch_replace completed: "
              << result.documents_count << " document(s), "
              << result.changed_count << " changed, " << result.copied_count
              << " copied, " << result.replacements_count
              << " replacement(s)\n";
    if (!result.outputs.empty()) {
        std::cout << "output: " << result.outputs.front().path.string() << '\n';
    }
}

auto report_run_recipe_execution_error(std::string_view stage,
                                       const std::string &message,
                                       bool json_output) -> int {
    if (json_output) {
        write_json_command_error(std::cerr, "run-recipe", stage, message);
    } else {
        std::cerr << message << '\n';
    }
    return 1;
}

auto execute_run_recipe(const run_recipe_options &options) -> int {
    std::string recipe_id;
    std::string error_message;
    if (!read_run_recipe_recipe_id(*options.recipe_path, recipe_id,
                                   error_message)) {
        return report_run_recipe_execution_error("input", error_message,
                                                options.json_output);
    }

    std::unordered_map<std::string, std::string> inputs;
    if (!read_run_recipe_inputs_file(*options.inputs_path, inputs,
                                     error_message)) {
        return report_run_recipe_execution_error("input", error_message,
                                                options.json_output);
    }

    if (recipe_id == "batch_replace") {
        run_recipe_batch_replace_result result;
        if (!execute_run_recipe_batch_replace(inputs, *options.output_dir, result,
                                              error_message)) {
            return report_run_recipe_execution_error("execute", error_message,
                                                    options.json_output);
        }

        if (options.json_output) {
            write_json_run_recipe_batch_replace_success(recipe_id, result);
        } else {
            print_run_recipe_batch_replace_success(result);
        }
        return 0;
    }

    return report_run_recipe_execution_error(
        "execute", "unsupported run-recipe recipe id: " + recipe_id,
        options.json_output);
}

} // namespace

auto run_recipe_command(std::string_view command,
                        const std::vector<std::string_view> &arguments)
    -> int {
    const auto json_output = has_json_flag(arguments);

    run_recipe_options options;
    std::string error_message;
    if (!parse_run_recipe_options(arguments, options, error_message)) {
        print_parse_error(command, error_message, json_output);
        return 2;
    }

    return execute_run_recipe(options);
}

} // namespace featherdoc_cli
