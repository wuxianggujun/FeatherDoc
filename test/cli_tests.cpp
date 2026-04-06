#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <featherdoc.hpp>

namespace {
namespace fs = std::filesystem;

auto cli_binary_name() -> const char * {
#if defined(_WIN32)
    return "featherdoc_cli.exe";
#else
    return "featherdoc_cli";
#endif
}

auto shell_quote(std::string_view value) -> std::string {
#if defined(_WIN32)
    std::string quoted = "\"";
    for (const char ch : value) {
        if (ch == '"') {
            quoted += "\\\"";
        } else {
            quoted += ch;
        }
    }
    quoted += '"';
    return quoted;
#else
    std::string quoted = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += '\'';
    return quoted;
#endif
}

auto normalize_system_status(int status) -> int {
#if defined(_WIN32)
    return status;
#else
    if (status == -1) {
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return status;
#endif
}

auto cli_binary_path() -> fs::path {
    return fs::current_path() / cli_binary_name();
}

void remove_if_exists(const fs::path &path) {
    std::error_code error;
    fs::remove(path, error);
}

auto read_text_file(const fs::path &path) -> std::string {
    std::ifstream stream(path);
    REQUIRE(stream.good());
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

auto collect_part_lines(featherdoc::Paragraph paragraph) -> std::vector<std::string> {
    std::vector<std::string> lines;
    for (; paragraph.has_next(); paragraph.next()) {
        std::string text;
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            text += run.get_text();
        }
        lines.push_back(std::move(text));
    }
    return lines;
}

auto collect_non_empty_document_text(featherdoc::Document &document) -> std::string {
    std::ostringstream stream;
    for (auto paragraph = document.paragraphs(); paragraph.has_next(); paragraph.next()) {
        std::string text;
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            text += run.get_text();
        }

        if (!text.empty()) {
            stream << text << '\n';
        }
    }

    return stream.str();
}

void append_body_paragraph(featherdoc::Document &document, const char *text) {
    auto paragraph = document.paragraphs();
    while (paragraph.has_next()) {
        paragraph.next();
    }

    const auto inserted = paragraph.insert_paragraph_after(text);
    REQUIRE(inserted.has_next());
}

void create_cli_fixture(const fs::path &path) {
    remove_if_exists(path);

    featherdoc::Document document(path);
    REQUIRE_FALSE(document.create_empty());

    REQUIRE(document.paragraphs().add_run("section 0 body").has_next());

    auto section0_header = document.ensure_section_header_paragraphs(0);
    REQUIRE(section0_header.has_next());
    CHECK(section0_header.add_run("section 0 header").has_next());

    CHECK(document.append_section(false));
    append_body_paragraph(document, "section 1 body");

    auto section1_header = document.ensure_section_header_paragraphs(1);
    REQUIRE(section1_header.has_next());
    CHECK(section1_header.add_run("section 1 header").has_next());

    auto section1_first_footer = document.ensure_section_footer_paragraphs(
        1, featherdoc::section_reference_kind::first_page);
    REQUIRE(section1_first_footer.has_next());
    CHECK(section1_first_footer.add_run("section 1 first footer").has_next());

    CHECK(document.append_section(false));
    append_body_paragraph(document, "section 2 body");

    CHECK_EQ(document.section_count(), 3);
    REQUIRE_FALSE(document.save());
}

auto run_cli(const std::vector<std::string> &arguments,
             const std::optional<fs::path> &captured_output = std::nullopt) -> int {
    const fs::path executable_path = cli_binary_path();
    REQUIRE(fs::exists(executable_path));

#if defined(_WIN32)
    std::string command = "\"";
    command += shell_quote(executable_path.string());
#else
    std::string command = shell_quote(executable_path.string());
#endif
    for (const auto &argument : arguments) {
        command += ' ';
        command += shell_quote(argument);
    }

    if (captured_output.has_value()) {
        remove_if_exists(*captured_output);
        command += " > ";
        command += shell_quote(captured_output->string());
        command += " 2>&1";
    }

#if defined(_WIN32)
    command += '"';
#endif

    return normalize_system_status(std::system(command.c_str()));
}
} // namespace

TEST_CASE("cli section commands modify layout end to end") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_sections_source.docx";
    const fs::path inserted = working_directory / "cli_sections_inserted.docx";
    const fs::path copied = working_directory / "cli_sections_copied.docx";
    const fs::path moved = working_directory / "cli_sections_moved.docx";
    const fs::path removed = working_directory / "cli_sections_removed.docx";
    const fs::path inspect_output = working_directory / "cli_sections_inspect.txt";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(copied);
    remove_if_exists(moved);
    remove_if_exists(removed);
    remove_if_exists(inspect_output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"inspect-sections", source.string()}, inspect_output), 0);

    const auto inspect_text = read_text_file(inspect_output);
    CHECK_NE(inspect_text.find("sections: 3"), std::string::npos);
    CHECK_NE(inspect_text.find("headers: 2"), std::string::npos);
    CHECK_NE(inspect_text.find("footers: 1"), std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[0]: header(default=yes, first=no, even=no) "
                 "footer(default=no, first=no, even=no)"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[1]: header(default=yes, first=no, even=no) "
                 "footer(default=no, first=yes, even=no)"),
             std::string::npos);
    CHECK_NE(inspect_text.find(
                 "section[2]: header(default=no, first=no, even=no) "
                 "footer(default=no, first=no, even=no)"),
             std::string::npos);

    CHECK_EQ(run_cli({"insert-section", source.string(), "1", "--no-inherit", "--output",
                      inserted.string()}),
             0);

    featherdoc::Document unchanged_source(source);
    REQUIRE_FALSE(unchanged_source.open());
    CHECK_EQ(unchanged_source.section_count(), 3);

    featherdoc::Document inserted_doc(inserted);
    REQUIRE_FALSE(inserted_doc.open());
    CHECK_EQ(inserted_doc.section_count(), 4);
    CHECK_EQ(inserted_doc.header_count(), 2);
    CHECK_EQ(inserted_doc.footer_count(), 1);
    CHECK_FALSE(inserted_doc.section_header_paragraphs(2).has_next());
    CHECK_FALSE(inserted_doc.section_footer_paragraphs(
        2, featherdoc::section_reference_kind::first_page)
                    .has_next());

    CHECK_EQ(run_cli({"copy-section-layout", inserted.string(), "1", "2", "--output",
                      copied.string()}),
             0);

    featherdoc::Document copied_doc(copied);
    REQUIRE_FALSE(copied_doc.open());
    CHECK_EQ(copied_doc.section_count(), 4);
    CHECK_EQ(copied_doc.header_count(), 2);
    CHECK_EQ(copied_doc.footer_count(), 1);
    CHECK_EQ(copied_doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(copied_doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");

    CHECK_EQ(run_cli({"move-section", copied.string(), "3", "0", "--output",
                      moved.string()}),
             0);

    featherdoc::Document moved_doc(moved);
    REQUIRE_FALSE(moved_doc.open());
    CHECK_EQ(moved_doc.section_count(), 4);
    CHECK_EQ(moved_doc.header_count(), 2);
    CHECK_EQ(moved_doc.footer_count(), 1);
    CHECK_FALSE(moved_doc.section_header_paragraphs(0).has_next());
    CHECK_EQ(moved_doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(moved_doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(moved_doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_EQ(collect_non_empty_document_text(moved_doc),
             "section 2 body\nsection 0 body\nsection 1 body\n");

    CHECK_EQ(run_cli({"remove-section", moved.string(), "3", "--output",
                      removed.string()}),
             0);

    featherdoc::Document removed_doc(removed);
    REQUIRE_FALSE(removed_doc.open());
    CHECK_EQ(removed_doc.section_count(), 3);
    CHECK_EQ(removed_doc.header_count(), 2);
    CHECK_EQ(removed_doc.footer_count(), 1);
    CHECK_FALSE(removed_doc.section_header_paragraphs(0).has_next());
    CHECK_EQ(removed_doc.section_header_paragraphs(1).runs().get_text(), "section 0 header");
    CHECK_EQ(removed_doc.section_header_paragraphs(2).runs().get_text(), "section 1 header");
    CHECK_EQ(removed_doc.section_footer_paragraphs(
                 2, featherdoc::section_reference_kind::first_page)
                 .runs()
                 .get_text(),
             "section 1 first footer");
    CHECK_EQ(collect_non_empty_document_text(removed_doc),
             "section 2 body\nsection 0 body\nsection 1 body\n");

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(copied);
    remove_if_exists(moved);
    remove_if_exists(removed);
    remove_if_exists(inspect_output);
}

TEST_CASE("cli can show and replace section header footer text") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_section_text_source.docx";
    const fs::path header_output = working_directory / "cli_section_header.txt";
    const fs::path text_source = working_directory / "cli_section_text_input.txt";
    const fs::path header_updated = working_directory / "cli_section_header_updated.docx";
    const fs::path shown_even_header = working_directory / "cli_section_even_header.txt";
    const fs::path footer_updated = working_directory / "cli_section_footer_updated.docx";
    const fs::path shown_footer = working_directory / "cli_section_footer.txt";

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(text_source);
    remove_if_exists(header_updated);
    remove_if_exists(shown_even_header);
    remove_if_exists(footer_updated);
    remove_if_exists(shown_footer);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"show-section-header", source.string(), "1"}, header_output), 0);
    CHECK_EQ(read_text_file(header_output), std::string("section 1 header\n"));

    {
        std::ofstream stream(text_source);
        REQUIRE(stream.good());
        stream << "alpha\nbeta\n";
    }

    CHECK_EQ(run_cli({"set-section-header", source.string(), "2", "--kind", "even",
                      "--text-file", text_source.string(), "--output",
                      header_updated.string()}),
             0);
    CHECK_EQ(run_cli({"show-section-header", header_updated.string(), "2", "--kind",
                      "even"},
                     shown_even_header),
             0);
    CHECK_EQ(read_text_file(shown_even_header), std::string("alpha\nbeta\n"));

    featherdoc::Document header_doc(header_updated);
    REQUIRE_FALSE(header_doc.open());
    CHECK_EQ(collect_part_lines(header_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"alpha", "beta"}));

    CHECK_EQ(run_cli({"set-section-footer", header_updated.string(), "0", "--text",
                      "front footer", "--output", footer_updated.string()}),
             0);
    CHECK_EQ(run_cli({"show-section-footer", footer_updated.string(), "0"},
                     shown_footer),
             0);
    CHECK_EQ(read_text_file(shown_footer), std::string("front footer\n"));

    featherdoc::Document footer_doc(footer_updated);
    REQUIRE_FALSE(footer_doc.open());
    CHECK_EQ(collect_part_lines(footer_doc.section_footer_paragraphs(0)),
             std::vector<std::string>({"front footer"}));
    CHECK_EQ(collect_part_lines(footer_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"alpha", "beta"}));

    remove_if_exists(source);
    remove_if_exists(header_output);
    remove_if_exists(text_source);
    remove_if_exists(header_updated);
    remove_if_exists(shown_even_header);
    remove_if_exists(footer_updated);
    remove_if_exists(shown_footer);
}

TEST_CASE("cli inspect and show commands support json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_sections_json_source.docx";
    const fs::path inspect_output = working_directory / "cli_sections_json_inspect.txt";
    const fs::path shown_header = working_directory / "cli_sections_json_header.txt";
    const fs::path shown_missing_footer =
        working_directory / "cli_sections_json_missing_footer.txt";

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(shown_header);
    remove_if_exists(shown_missing_footer);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"inspect-sections", source.string(), "--json"}, inspect_output), 0);
    CHECK_EQ(
        read_text_file(inspect_output),
        std::string{
            "{\"sections\":3,\"headers\":2,\"footers\":1,\"section_layouts\":["
            "{\"index\":0,\"header\":{\"default\":true,\"first\":false,\"even\":false},"
            "\"footer\":{\"default\":false,\"first\":false,\"even\":false}},"
            "{\"index\":1,\"header\":{\"default\":true,\"first\":false,\"even\":false},"
            "\"footer\":{\"default\":false,\"first\":true,\"even\":false}},"
            "{\"index\":2,\"header\":{\"default\":false,\"first\":false,\"even\":false},"
            "\"footer\":{\"default\":false,\"first\":false,\"even\":false}}]}\n"});

    CHECK_EQ(run_cli({"show-section-header", source.string(), "1", "--json"},
                     shown_header),
             0);
    CHECK_EQ(read_text_file(shown_header),
             std::string{
                 "{\"part\":\"header\",\"section\":1,\"kind\":\"default\","
                 "\"present\":true,\"paragraphs\":[\"section 1 header\"]}\n"});

    CHECK_EQ(run_cli({"show-section-footer", source.string(), "2", "--json"},
                     shown_missing_footer),
             0);
    CHECK_EQ(read_text_file(shown_missing_footer),
             std::string{
                 "{\"part\":\"footer\",\"section\":2,\"kind\":\"default\","
                 "\"present\":false,\"paragraphs\":[]}\n"});

    remove_if_exists(source);
    remove_if_exists(inspect_output);
    remove_if_exists(shown_header);
    remove_if_exists(shown_missing_footer);
}

TEST_CASE("cli mutating commands support json output") {
    const fs::path working_directory = fs::current_path();
    const fs::path source = working_directory / "cli_mutation_json_source.docx";
    const fs::path inserted = working_directory / "cli_mutation_json_inserted.docx";
    const fs::path moved = working_directory / "cli_mutation_json_moved.docx";
    const fs::path header_updated =
        working_directory / "cli_mutation_json_header.docx";
    const fs::path insert_output = working_directory / "cli_mutation_insert.json";
    const fs::path move_output = working_directory / "cli_mutation_move.json";
    const fs::path set_header_output =
        working_directory / "cli_mutation_set_header.json";
    const fs::path parse_error_output =
        working_directory / "cli_mutation_parse_error.json";

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(moved);
    remove_if_exists(header_updated);
    remove_if_exists(insert_output);
    remove_if_exists(move_output);
    remove_if_exists(set_header_output);
    remove_if_exists(parse_error_output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"insert-section", source.string(), "1", "--no-inherit", "--output",
                      inserted.string(), "--json"},
                     insert_output),
             0);
    CHECK_EQ(
        read_text_file(insert_output),
        std::string{
            "{\"command\":\"insert-section\",\"ok\":true,\"in_place\":false,"
            "\"sections\":4,\"headers\":2,\"footers\":1,\"section\":2,"
            "\"inherit_header_footer\":false}\n"});

    CHECK_EQ(run_cli({"move-section", inserted.string(), "3", "0", "--output",
                      moved.string(), "--json"},
                     move_output),
             0);
    CHECK_EQ(
        read_text_file(move_output),
        std::string{
            "{\"command\":\"move-section\",\"ok\":true,\"in_place\":false,"
            "\"sections\":4,\"headers\":2,\"footers\":1,\"source\":3,"
            "\"target\":0}\n"});

    CHECK_EQ(run_cli({"set-section-header", moved.string(), "2", "--kind", "even",
                      "--text", "json header", "--output",
                      header_updated.string(), "--json"},
                     set_header_output),
             0);
    CHECK_EQ(
        read_text_file(set_header_output),
        std::string{
            "{\"command\":\"set-section-header\",\"ok\":true,"
            "\"in_place\":false,\"sections\":4,\"headers\":3,\"footers\":1,"
            "\"part\":\"header\",\"section\":2,\"kind\":\"even\"}\n"});

    featherdoc::Document header_doc(header_updated);
    REQUIRE_FALSE(header_doc.open());
    CHECK_EQ(collect_part_lines(header_doc.section_header_paragraphs(
                 2, featherdoc::section_reference_kind::even_page)),
             std::vector<std::string>({"json header"}));

    CHECK_EQ(run_cli({"set-section-footer", source.string(), "0", "--json"},
                     parse_error_output),
             2);
    CHECK_EQ(
        read_text_file(parse_error_output),
        std::string{
            "{\"command\":\"set-section-footer\",\"ok\":false,"
            "\"stage\":\"parse\",\"message\":\"expected --text <text> or "
            "--text-file <path>\"}\n"});

    remove_if_exists(source);
    remove_if_exists(inserted);
    remove_if_exists(moved);
    remove_if_exists(header_updated);
    remove_if_exists(insert_output);
    remove_if_exists(move_output);
    remove_if_exists(set_header_output);
    remove_if_exists(parse_error_output);
}
