#include <charconv>
#include <featherdoc.hpp>

#include <filesystem>
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
};

void print_usage(std::ostream &stream) {
    stream
        << "Usage:\n"
        << "  featherdoc_cli inspect-sections <input.docx>\n"
        << "  featherdoc_cli insert-section <input.docx> <section-index>"
           " [--no-inherit] [--output <path>]\n"
        << "  featherdoc_cli remove-section <input.docx> <section-index>"
           " [--output <path>]\n"
        << "  featherdoc_cli move-section <input.docx> <source-index>"
           " <target-index> [--output <path>]\n"
        << "  featherdoc_cli copy-section-layout <input.docx> <source-index>"
           " <target-index> [--output <path>]\n";
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

auto parse_index(std::string_view text, std::size_t &value) -> bool {
    const auto *begin = text.data();
    const auto *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
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

        error_message = "unknown option: " + std::string(argument);
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

void inspect_sections(featherdoc::Document &doc) {
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

auto save_document(featherdoc::Document &doc,
                   const std::optional<path_type> &output_path) -> bool {
    const auto error =
        output_path.has_value() ? doc.save_as(*output_path) : doc.save();
    if (error) {
        print_error_info(doc.last_error());
        return false;
    }

    return true;
}

auto open_document(const path_type &input_path, featherdoc::Document &doc) -> bool {
    doc.set_path(input_path);
    const auto error = doc.open();
    if (error) {
        print_error_info(doc.last_error());
        return false;
    }

    return true;
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
        if (arguments.size() != 2U) {
            print_parse_error("inspect-sections expects exactly one input path");
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc)) {
            return 1;
        }

        inspect_sections(doc);
        return 0;
    }

    if (command == "insert-section") {
        if (arguments.size() < 3U) {
            print_parse_error(
                "insert-section expects an input path and a section index");
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error("invalid section index: " + std::string(arguments[2]));
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 3U, true, options, error_message)) {
            print_parse_error(error_message);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc)) {
            return 1;
        }

        if (!doc.insert_section(section_index, options.inherit_header_footer)) {
            print_error_info(doc.last_error());
            return 1;
        }

        return save_document(doc, options.output_path) ? 0 : 1;
    }

    if (command == "remove-section") {
        if (arguments.size() < 3U) {
            print_parse_error(
                "remove-section expects an input path and a section index");
            return 2;
        }

        std::size_t section_index = 0;
        if (!parse_index(arguments[2], section_index)) {
            print_parse_error("invalid section index: " + std::string(arguments[2]));
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 3U, false, options, error_message)) {
            print_parse_error(error_message);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc)) {
            return 1;
        }

        if (!doc.remove_section(section_index)) {
            print_error_info(doc.last_error());
            return 1;
        }

        return save_document(doc, options.output_path) ? 0 : 1;
    }

    if (command == "move-section") {
        if (arguments.size() < 4U) {
            print_parse_error(
                "move-section expects an input path, a source index, and a "
                "target index");
            return 2;
        }

        std::size_t source_index = 0;
        std::size_t target_index = 0;
        if (!parse_index(arguments[2], source_index)) {
            print_parse_error("invalid source index: " + std::string(arguments[2]));
            return 2;
        }
        if (!parse_index(arguments[3], target_index)) {
            print_parse_error("invalid target index: " + std::string(arguments[3]));
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 4U, false, options, error_message)) {
            print_parse_error(error_message);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc)) {
            return 1;
        }

        if (!doc.move_section(source_index, target_index)) {
            print_error_info(doc.last_error());
            return 1;
        }

        return save_document(doc, options.output_path) ? 0 : 1;
    }

    if (command == "copy-section-layout") {
        if (arguments.size() < 4U) {
            print_parse_error(
                "copy-section-layout expects an input path, a source index, "
                "and a target index");
            return 2;
        }

        std::size_t source_index = 0;
        std::size_t target_index = 0;
        if (!parse_index(arguments[2], source_index)) {
            print_parse_error("invalid source index: " + std::string(arguments[2]));
            return 2;
        }
        if (!parse_index(arguments[3], target_index)) {
            print_parse_error("invalid target index: " + std::string(arguments[3]));
            return 2;
        }

        command_options options;
        std::string error_message;
        if (!parse_options(arguments, 4U, false, options, error_message)) {
            print_parse_error(error_message);
            return 2;
        }

        if (!open_document(path_type(std::string(arguments[1])), doc)) {
            return 1;
        }

        if (!doc.copy_section_header_references(source_index, target_index) ||
            !doc.copy_section_footer_references(source_index, target_index)) {
            print_error_info(doc.last_error());
            return 1;
        }

        return save_document(doc, options.output_path) ? 0 : 1;
    }

    print_parse_error("unknown command: " + std::string(command));
    return 2;
}
