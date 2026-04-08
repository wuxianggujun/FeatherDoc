#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

std::string collect_document_text(featherdoc::Document &document) {
    std::string text;

    for (auto paragraph = document.paragraphs(); paragraph.has_next(); paragraph.next()) {
        for (auto run = paragraph.runs(); run.has_next(); run.next()) {
            text += run.get_text();
        }
        text += '\n';
    }

    return text;
}

void print_document_error(const char *operation, const featherdoc::Document &document) {
    const auto &error = document.last_error();

    std::cerr << operation;
    if (error.code) {
        std::cerr << " failed: " << error.code.message();
    } else {
        std::cerr << " failed";
    }

    if (!error.detail.empty()) {
        std::cerr << " (" << error.detail << ")";
    }

    if (!error.entry_name.empty()) {
        std::cerr << " [entry=" << error.entry_name << ']';
    }

    if (error.xml_offset.has_value()) {
        std::cerr << " [xml_offset=" << *error.xml_offset << ']';
    }

    std::cerr << '\n';
}

} // namespace

int main(int argc, char **argv) {
    namespace fs = std::filesystem;

    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::path("featherdoc-install-smoke.docx");
    const std::string expected_text = "Installed FeatherDoc package smoke";

    std::error_code filesystem_error;
    if (output_path.has_parent_path()) {
        fs::create_directories(output_path.parent_path(), filesystem_error);
        if (filesystem_error) {
            std::cerr << "failed to create output directory: "
                      << filesystem_error.message() << '\n';
            return 1;
        }
    }

    fs::remove(output_path, filesystem_error);
    filesystem_error.clear();

    featherdoc::Document generated_document(output_path);
    if (generated_document.create_empty()) {
        print_document_error("create_empty()", generated_document);
        return 1;
    }

    auto paragraph = generated_document.paragraphs();
    if (!paragraph.has_next()) {
        std::cerr << "create_empty() did not produce an editable paragraph\n";
        return 1;
    }

    paragraph.add_run(expected_text);

    if (generated_document.save()) {
        print_document_error("save()", generated_document);
        return 1;
    }

    featherdoc::Document reopened_document(output_path);
    if (reopened_document.open()) {
        print_document_error("open()", reopened_document);
        return 1;
    }

    const std::string actual_text = collect_document_text(reopened_document);
    if (actual_text != expected_text + "\n") {
        std::cerr << "unexpected document text: " << actual_text << '\n';
        return 1;
    }

    std::cout << "Generated install smoke document at " << output_path.string() << '\n';
    return 0;
}
