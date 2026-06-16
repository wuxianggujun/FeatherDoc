#include "featherdoc_cli_review_output.hpp"

#include "featherdoc_cli_domain_names.hpp"
#include "featherdoc_cli_json.hpp"

#include <iostream>
#include <ostream>
#include <string_view>
#include <vector>

namespace featherdoc_cli {
namespace {

void write_json_review_note_summary(
    std::ostream &stream, const featherdoc::review_note_summary &note) {
    stream << "{\"index\":" << note.index << ",\"kind\":";
    write_json_string(stream, review_note_kind_name(note.kind));
    stream << ",\"id\":";
    write_json_string(stream, note.id);
    stream << ",\"author\":";
    write_json_optional_string(stream, note.author);
    stream << ",\"initials\":";
    write_json_optional_string(stream, note.initials);
    stream << ",\"date\":";
    write_json_optional_string(stream, note.date);
    stream << ",\"anchor_text\":";
    write_json_optional_string(stream, note.anchor_text);
    stream << ",\"resolved\":" << json_bool(note.resolved);
    stream << ",\"parent_index\":";
    write_json_optional_size(stream, note.parent_index);
    stream << ",\"parent_id\":";
    write_json_optional_string(stream, note.parent_id);
    stream << ",\"text\":";
    write_json_string(stream, note.text);
    stream << '}';
}

void write_json_revision_summary(
    std::ostream &stream, const featherdoc::revision_summary &revision) {
    stream << "{\"index\":" << revision.index << ",\"kind\":";
    write_json_string(stream, revision_kind_name(revision.kind));
    stream << ",\"id\":";
    write_json_string(stream, revision.id);
    stream << ",\"author\":";
    write_json_optional_string(stream, revision.author);
    stream << ",\"date\":";
    write_json_optional_string(stream, revision.date);
    stream << ",\"part_entry_name\":";
    write_json_string(stream, revision.part_entry_name);
    stream << ",\"text\":";
    write_json_string(stream, revision.text);
    stream << '}';
}

void print_review_note_summary(
    std::ostream &stream, const featherdoc::review_note_summary &note) {
    stream << "index=" << note.index
           << " kind=" << review_note_kind_name(note.kind)
           << " id=" << note.id
           << " author=" << optional_display_value(note.author)
           << " initials=" << optional_display_value(note.initials)
           << " date=" << optional_display_value(note.date)
           << " anchor_text=" << optional_display_value(note.anchor_text)
           << " resolved=" << yes_no(note.resolved)
           << " parent_index=" << optional_size_display_value(note.parent_index)
           << " parent_id=" << optional_display_value(note.parent_id)
           << " text=";
    write_json_string(stream, note.text);
}

void print_revision_summary(
    std::ostream &stream, const featherdoc::revision_summary &revision) {
    stream << "index=" << revision.index
           << " kind=" << revision_kind_name(revision.kind)
           << " id=" << revision.id
           << " author=" << optional_display_value(revision.author)
           << " date=" << optional_display_value(revision.date)
           << " part_entry_name=" << revision.part_entry_name << " text=";
    write_json_string(stream, revision.text);
}

} // namespace

void inspect_review(
    const std::vector<featherdoc::review_note_summary> &footnotes,
    const std::vector<featherdoc::review_note_summary> &endnotes,
    const std::vector<featherdoc::review_note_summary> &comments,
    const std::vector<featherdoc::revision_summary> &revisions, bool json_output) {
    if (json_output) {
        std::cout << "{\"footnotes_count\":" << footnotes.size()
                  << ",\"endnotes_count\":" << endnotes.size()
                  << ",\"comments_count\":" << comments.size()
                  << ",\"revisions_count\":" << revisions.size()
                  << ",\"footnotes\":[";
        for (std::size_t index = 0; index < footnotes.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, footnotes[index]);
        }
        std::cout << "],\"endnotes\":[";
        for (std::size_t index = 0; index < endnotes.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, endnotes[index]);
        }
        std::cout << "],\"comments\":[";
        for (std::size_t index = 0; index < comments.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_review_note_summary(std::cout, comments[index]);
        }
        std::cout << "],\"revisions\":[";
        for (std::size_t index = 0; index < revisions.size(); ++index) {
            if (index != 0U) {
                std::cout << ',';
            }
            write_json_revision_summary(std::cout, revisions[index]);
        }
        std::cout << "]}\n";
        return;
    }

    std::cout << "footnotes: " << footnotes.size() << '\n';
    for (std::size_t index = 0; index < footnotes.size(); ++index) {
        std::cout << "footnote[" << index << "]: ";
        print_review_note_summary(std::cout, footnotes[index]);
        std::cout << '\n';
    }
    std::cout << "endnotes: " << endnotes.size() << '\n';
    for (std::size_t index = 0; index < endnotes.size(); ++index) {
        std::cout << "endnote[" << index << "]: ";
        print_review_note_summary(std::cout, endnotes[index]);
        std::cout << '\n';
    }
    std::cout << "comments: " << comments.size() << '\n';
    for (std::size_t index = 0; index < comments.size(); ++index) {
        std::cout << "comment[" << index << "]: ";
        print_review_note_summary(std::cout, comments[index]);
        std::cout << '\n';
    }
    std::cout << "revisions: " << revisions.size() << '\n';
    for (std::size_t index = 0; index < revisions.size(); ++index) {
        std::cout << "revision[" << index << "]: ";
        print_revision_summary(std::cout, revisions[index]);
        std::cout << '\n';
    }
}

void print_simple_document_mutation_result(
    std::string_view command, const std::optional<path_type> &output_path,
    std::size_t affected) {
    std::cout << "command: " << command << '\n';
    if (output_path.has_value()) {
        std::cout << "output_path: " << output_path->string() << '\n';
    } else {
        std::cout << "output_path: in_place\n";
    }
    std::cout << "affected: " << affected << '\n';
}

void write_json_affected_result(std::ostream &stream, std::size_t affected) {
    stream << ",\"affected\":" << affected;
}

} // namespace featherdoc_cli
