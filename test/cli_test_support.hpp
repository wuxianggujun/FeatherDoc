#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/wait.h>
#else
#include <sys/wait.h>
#endif

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

auto test_binary_directory() -> fs::path {
#if defined(_WIN32)
    std::wstring buffer(static_cast<std::size_t>(MAX_PATH), L'\0');
    while (true) {
        const DWORD copied =
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        REQUIRE(copied != 0);
        if (copied < buffer.size()) {
            buffer.resize(static_cast<std::size_t>(copied));
            return fs::path(buffer).parent_path();
        }

        buffer.resize(buffer.size() * 2U);
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    REQUIRE_EQ(_NSGetExecutablePath(nullptr, &size), -1);

    std::string buffer(static_cast<std::size_t>(size), '\0');
    REQUIRE_EQ(_NSGetExecutablePath(buffer.data(), &size), 0);
    return fs::weakly_canonical(fs::path(buffer.c_str())).parent_path();
#else
    std::error_code error;
    const fs::path executable_path = fs::read_symlink("/proc/self/exe", error);
    REQUIRE_FALSE(error);
    return executable_path.parent_path();
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

auto json_quote(std::string_view value) -> std::string {
    std::string quoted = "\"";
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            quoted += "\\\\";
            break;
        case '"':
            quoted += "\\\"";
            break;
        case '\b':
            quoted += "\\b";
            break;
        case '\f':
            quoted += "\\f";
            break;
        case '\n':
            quoted += "\\n";
            break;
        case '\r':
            quoted += "\\r";
            break;
        case '\t':
            quoted += "\\t";
            break;
        default:
            quoted += ch;
            break;
        }
    }
    quoted += '"';
    return quoted;
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

#if defined(_WIN32)
auto utf8_to_wide(std::string_view value) -> std::wstring {
    if (value.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8,
                                             0,
                                             value.data(),
                                             static_cast<int>(value.size()),
                                             nullptr,
                                             0);
    REQUIRE(required > 0);

    std::wstring converted(static_cast<std::size_t>(required), L'\0');
    const int written = MultiByteToWideChar(CP_UTF8,
                                            0,
                                            value.data(),
                                            static_cast<int>(value.size()),
                                            converted.data(),
                                            required);
    REQUIRE_EQ(written, required);
    return converted;
}

void append_windows_command_line_argument(std::wstring_view argument,
                                          std::wstring &command_line) {
    if (!command_line.empty()) {
        command_line += L' ';
    }

    const bool needs_quotes =
        argument.empty() ||
        argument.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
    if (!needs_quotes) {
        command_line.append(argument);
        return;
    }

    command_line += L'"';
    std::size_t trailing_backslashes = 0;
    for (const wchar_t ch : argument) {
        if (ch == L'\\') {
            ++trailing_backslashes;
            continue;
        }

        if (ch == L'"') {
            command_line.append(trailing_backslashes * 2 + 1, L'\\');
            command_line += L'"';
            trailing_backslashes = 0;
            continue;
        }

        command_line.append(trailing_backslashes, L'\\');
        trailing_backslashes = 0;
        command_line += ch;
    }

    command_line.append(trailing_backslashes * 2, L'\\');
    command_line += L'"';
}
#endif

auto cli_binary_path() -> fs::path {
    static const fs::path executable_path = []() {
        const fs::path binary_directory = test_binary_directory();
        const fs::path local_candidate = binary_directory / cli_binary_name();
        const fs::path parent_candidate = binary_directory.parent_path() / cli_binary_name();

        std::error_code local_error;
        const bool has_local = fs::exists(local_candidate, local_error) && !local_error;
        std::error_code parent_error;
        const bool has_parent =
            fs::exists(parent_candidate, parent_error) && !parent_error;

        if (!has_local) {
            return parent_candidate;
        }
        if (!has_parent) {
            return local_candidate;
        }

        const auto local_time = fs::last_write_time(local_candidate);
        const auto parent_time = fs::last_write_time(parent_candidate);
        if (local_time >= parent_time) {
            return local_candidate;
        }
        return parent_candidate;
    }();

    return executable_path;
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

auto read_docx_entry(const fs::path &path, const char *entry_name) -> std::string {
    int zip_error = 0;
    zip_t *archive = zip_openwitherror(path.string().c_str(),
                                       ZIP_DEFAULT_COMPRESSION_LEVEL, 'r',
                                       &zip_error);
    REQUIRE(archive != nullptr);
    REQUIRE_EQ(zip_entry_open(archive, entry_name), 0);

    void *buffer = nullptr;
    size_t buffer_size = 0;
    REQUIRE_GE(zip_entry_read(archive, &buffer, &buffer_size), 0);
    REQUIRE_EQ(zip_entry_close(archive), 0);
    zip_close(archive);

    std::string content(static_cast<const char *>(buffer), buffer_size);
    std::free(buffer);
    return content;
}


auto find_body_paragraph_xml_node(pugi::xml_node document_root, std::size_t index)
    -> pugi::xml_node {
    auto body = document_root.child("w:body");
    if (body == pugi::xml_node{}) {
        return {};
    }

    std::size_t current_index = 0U;
    for (auto paragraph = body.child("w:p"); paragraph != pugi::xml_node{};
         paragraph = paragraph.next_sibling("w:p")) {
        if (current_index == index) {
            return paragraph;
        }
        ++current_index;
    }

    return {};
}

auto find_run_xml_node(pugi::xml_node paragraph_node, std::size_t index)
    -> pugi::xml_node {
    std::size_t current_index = 0U;
    for (auto run = paragraph_node.child("w:r"); run != pugi::xml_node{};
         run = run.next_sibling("w:r")) {
        if (current_index == index) {
            return run;
        }
        ++current_index;
    }

    return {};
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

auto collect_paragraph_text(featherdoc::Paragraph paragraph) -> std::string {
    std::string text;
    for (auto run = paragraph.runs(); run.has_next(); run.next()) {
        text += run.get_text();
    }
    return text;
}

auto find_body_paragraph_index_by_text(featherdoc::Document &document,
                                       std::string_view text) -> std::size_t {
    std::size_t index = 0U;
    for (auto paragraph = document.paragraphs(); paragraph.has_next();
         paragraph.next(), ++index) {
        if (collect_paragraph_text(paragraph) == text) {
            return index;
        }
    }

    REQUIRE(false);
    return 0U;
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

auto find_header_index_by_text(featherdoc::Document &document, std::string_view text)
    -> std::size_t {
    for (std::size_t index = 0; index < document.header_count(); ++index) {
        const auto lines = collect_part_lines(document.header_paragraphs(index));
        if (!lines.empty() && lines.front() == text) {
            return index;
        }
    }

    REQUIRE(false);
    return 0;
}

auto find_footer_index_by_text(featherdoc::Document &document, std::string_view text)
    -> std::size_t {
    for (std::size_t index = 0; index < document.footer_count(); ++index) {
        const auto lines = collect_part_lines(document.footer_paragraphs(index));
        if (!lines.empty() && lines.front() == text) {
            return index;
        }
    }

    REQUIRE(false);
    return 0;
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

bool write_archive_entry(zip_t *archive, const char *entry_name,
                         std::string_view content) {
    if (zip_entry_open(archive, entry_name) != 0) {
        return false;
    }
    if (zip_entry_write(archive, content.data(), content.size()) < 0) {
        zip_entry_close(archive);
        return false;
    }
    return zip_entry_close(archive) == 0;
}

auto run_cli(const std::vector<std::string> &arguments,
             const std::optional<fs::path> &captured_output = std::nullopt) -> int {
    const fs::path executable_path = cli_binary_path();
    REQUIRE(fs::exists(executable_path));

#if defined(_WIN32)
    std::wstring command_line;
    append_windows_command_line_argument(executable_path.wstring(), command_line);
    for (const auto &argument : arguments) {
        append_windows_command_line_argument(utf8_to_wide(argument), command_line);
    }

    SECURITY_ATTRIBUTES output_attributes{};
    output_attributes.nLength = sizeof(output_attributes);
    output_attributes.bInheritHandle = TRUE;

    HANDLE output_handle = INVALID_HANDLE_VALUE;
    STARTUPINFOW startup_info{};
    startup_info.cb = sizeof(startup_info);
    if (captured_output.has_value()) {
        remove_if_exists(*captured_output);
        output_handle = CreateFileW(captured_output->c_str(),
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    &output_attributes,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr);
        REQUIRE(output_handle != INVALID_HANDLE_VALUE);

        startup_info.dwFlags |= STARTF_USESTDHANDLES;
        startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startup_info.hStdOutput = output_handle;
        startup_info.hStdError = output_handle;
    }

    std::vector<wchar_t> mutable_command_line(command_line.begin(), command_line.end());
    mutable_command_line.push_back(L'\0');

    PROCESS_INFORMATION process_info{};
    const BOOL created = CreateProcessW(executable_path.c_str(),
                                        mutable_command_line.data(),
                                        nullptr,
                                        nullptr,
                                        captured_output.has_value() ? TRUE : FALSE,
                                        0,
                                        nullptr,
                                        fs::current_path().c_str(),
                                        &startup_info,
                                        &process_info);
    if (output_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(output_handle);
    }
    REQUIRE(created != FALSE);

    REQUIRE_EQ(WaitForSingleObject(process_info.hProcess, INFINITE), WAIT_OBJECT_0);

    DWORD exit_code = static_cast<DWORD>(-1);
    REQUIRE(GetExitCodeProcess(process_info.hProcess, &exit_code) != FALSE);

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
    return static_cast<int>(exit_code);
#else
    std::string command = shell_quote(executable_path.string());
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

    return normalize_system_status(std::system(command.c_str()));
#endif
}

auto run_cli(std::initializer_list<std::string> arguments) -> int {
    return run_cli(std::vector<std::string>{arguments});
}

auto run_cli(std::initializer_list<std::string> arguments,
             const std::filesystem::path &output_path) -> int {
    return run_cli(std::vector<std::string>{arguments}, output_path);
}

} // namespace
