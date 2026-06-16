#include "featherdoc_cli_bookmark_output.hpp"

#include "featherdoc_cli_bookmark_support.hpp"
#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_image_output.hpp"
#include "featherdoc_cli_json.hpp"
#include "featherdoc_cli_text.hpp"

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>

namespace featherdoc_cli {

void inspect_bookmarks(const selected_template_part &selected,
                       const std::vector<featherdoc::bookmark_summary> &bookmarks,
                       bool json_output) {
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
            write_json_string(
                std::cout,
                featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, std::string(selected.part.entry_name()));
        std::cout << ",\"count\":" << bookmarks.size() << ",\"bookmarks\":[";
        for (std::size_t index = 0; index < bookmarks.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_bookmark_support_summary(std::cout, bookmarks[index]);
        }
        std::cout << "]}\n";
        return;
    }

    print_selected_bookmark_part(selected);
    std::cout << "bookmarks: " << bookmarks.size() << '\n';
    for (std::size_t index = 0; index < bookmarks.size(); ++index) {
        std::cout << "bookmark[" << index << "]: ";
        print_bookmark_support_summary(std::cout, bookmarks[index]);
        std::cout << '\n';
    }
}

void inspect_bookmark(const selected_template_part &selected,
                      const featherdoc::bookmark_summary &bookmark,
                      bool json_output) {
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
            write_json_string(
                std::cout,
                featherdoc::to_xml_reference_type(*selected.reference_kind));
        }
        std::cout << ",\"entry_name\":";
        write_json_string(std::cout, std::string(selected.part.entry_name()));
        std::cout << ",\"bookmark\":";
        write_json_bookmark_support_summary(std::cout, bookmark);
        std::cout << "}\n";
        return;
    }

    print_selected_bookmark_part(selected);
    std::cout << "bookmark_name: " << bookmark.bookmark_name << '\n';
    std::cout << "occurrence_count: " << bookmark.occurrence_count << '\n';
    std::cout << "kind: " << bookmark_kind_name(bookmark.kind) << '\n';
    std::cout << "duplicate: " << yes_no(bookmark.is_duplicate()) << '\n';
}

void write_json_bookmark_image_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, const path_type &image_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"image_path\":";
    write_json_string(stream, image_path.string());
    stream << ",\"replaced\":" << inserted_images.size() << ",\"images\":[";
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_drawing_image_summary(stream, inserted_images[index]);
    }
    stream << ']';
}

void print_bookmark_image_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, const path_type &image_path,
    const std::optional<path_type> &output_path,
    const std::vector<featherdoc::drawing_image_info> &inserted_images) {
    print_bookmark_identity(selected, bookmark);
    std::cout << "image_path: " << image_path.string() << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << inserted_images.size() << '\n';
    for (std::size_t index = 0; index < inserted_images.size(); ++index) {
        std::cout << "image[" << index << "]: ";
        print_drawing_image_summary(std::cout, inserted_images[index]);
        std::cout << '\n';
    }
}

void write_json_bookmark_paragraphs_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::string> &paragraphs, std::size_t replaced) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"replaced\":" << replaced
           << ",\"paragraph_count\":" << paragraphs.size()
           << ",\"paragraphs\":[";
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        write_json_string(stream, paragraphs[index]);
    }
    stream << ']';
}

void print_bookmark_paragraphs_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::vector<std::string> &paragraphs,
    const std::optional<path_type> &output_path, std::size_t replaced) {
    print_bookmark_identity(selected, bookmark);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "paragraph_count: " << paragraphs.size() << '\n';
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        std::cout << "paragraph[" << index << "]: " << paragraphs[index] << '\n';
    }
}

void write_json_bookmark_block_removal_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, std::size_t removed) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"removed\":" << removed;
}

void print_bookmark_block_removal_result(
    const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark,
    const std::optional<path_type> &output_path, std::size_t removed) {
    print_bookmark_identity(selected, bookmark);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "removed: " << removed << '\n';
}

void write_json_bookmark_text_result(
    std::ostream &stream, const selected_template_part &selected,
    const featherdoc::bookmark_summary &bookmark, std::string_view text,
    std::size_t replaced) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"bookmark\":";
    write_json_bookmark_support_summary(stream, bookmark);
    stream << ",\"replaced\":" << replaced << ",\"text\":";
    write_json_string(stream, text);
}

void print_bookmark_text_result(const selected_template_part &selected,
                                const featherdoc::bookmark_summary &bookmark,
                                std::string_view text,
                                const std::optional<path_type> &output_path,
                                std::size_t replaced) {
    print_bookmark_identity(selected, bookmark);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "replaced: " << replaced << '\n';
    std::cout << "text: " << text << '\n';
}

namespace {

void write_json_bookmark_text_bindings(
    std::ostream &stream,
    const std::vector<featherdoc::bookmark_text_binding> &bindings) {
    stream << '[';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        if (index != 0U) {
            stream << ',';
        }
        stream << "{\"bookmark_name\":";
        write_json_string(stream, bindings[index].bookmark_name);
        stream << ",\"text\":";
        write_json_string(stream, bindings[index].text);
        stream << '}';
    }
    stream << ']';
}

} // namespace

void write_json_bookmark_fill_result(
    std::ostream &stream, const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_text_binding> &bindings,
    const featherdoc::bookmark_fill_result &result) {
    write_json_selected_bookmark_part(stream, selected);
    stream << ",\"complete\":" << json_bool(static_cast<bool>(result))
           << ",\"requested\":" << result.requested
           << ",\"matched\":" << result.matched
           << ",\"replaced\":" << result.replaced
           << ",\"bindings\":";
    write_json_bookmark_text_bindings(stream, bindings);
    stream << ",\"missing_bookmarks\":";
    write_json_strings(stream, result.missing_bookmarks);
}

void print_bookmark_fill_result(
    const selected_template_part &selected,
    const std::vector<featherdoc::bookmark_text_binding> &bindings,
    const std::optional<path_type> &output_path,
    const featherdoc::bookmark_fill_result &result) {
    print_selected_bookmark_part(selected);
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "complete: " << yes_no(static_cast<bool>(result)) << '\n';
    std::cout << "requested: " << result.requested << '\n';
    std::cout << "matched: " << result.matched << '\n';
    std::cout << "replaced: " << result.replaced << '\n';
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        std::cout << "binding[" << index << "]: "
                  << bindings[index].bookmark_name << " => "
                  << bindings[index].text << '\n';
    }
    std::cout << "missing_bookmarks: ";
    if (result.missing_bookmarks.empty()) {
        std::cout << "none\n";
    } else {
        for (std::size_t index = 0; index < result.missing_bookmarks.size();
             ++index) {
            if (index != 0U) {
                std::cout << ", ";
            }
            std::cout << result.missing_bookmarks[index];
        }
        std::cout << '\n';
    }
}

} // namespace featherdoc_cli
