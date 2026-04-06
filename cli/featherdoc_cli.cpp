#include <charconv>
#include <featherdoc.hpp>

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

void print_usage(std::ostream &stream) {
    stream
        << "Usage:\n"
        << "  featherdoc_cli inspect-sections <input.docx>\n"
        << "    [--json]\n"
        << "  featherdoc_cli insert-section <input.docx> <section-index>"
           " [--no-inherit] [--output <path>] [--json]\n"
        << "  featherdoc_cli remove-section <input.docx> <section-index>"
           " [--output <path>] [--json]\n"
        << "  featherdoc_cli move-section <input.docx> <source-index>"
           " <target-index> [--output <path>] [--json]\n"
        << "  featherdoc_cli copy-section-layout <input.docx> <source-index>"
           " <target-index> [--output <path>] [--json]\n"
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
           " [--kind default|first|even] [--output <path>] [--json]\n";
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

void write_json_lines(std::ostream &stream, const std::vector<std::string> &lines) {
    stream << '[';
    for (std::size_t index = 0; index < lines.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, lines[index]);
    }
    stream << ']';
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

    print_parse_error("unknown command: " + std::string(command));
    return 2;
}
