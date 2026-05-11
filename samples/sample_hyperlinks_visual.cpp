#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace {

bool build_hyperlink_sample(const fs::path &output_path) {
    featherdoc::Document document(output_path);
    if (document.create_empty()) {
        std::cerr << "failed to create DOCX: " << document.last_error().detail << '\n';
        return false;
    }

    auto body_template = document.body_template();
    if (!body_template ||
        !body_template.paragraphs().set_text(
            "External hyperlink typed API visual regression.")) {
        std::cerr << "failed to initialize body text: "
                  << document.last_error().detail << '\n';
        return false;
    }
    body_template.append_paragraph(
        "The next two lines should be clickable hyperlinks after Word opens the DOCX.");

    if (document.append_hyperlink("OpenAI external hyperlink", "https://openai.com") != 1U) {
        std::cerr << "failed to append document hyperlink: "
                  << document.last_error().detail << '\n';
        return false;
    }

    if (body_template.append_hyperlink("FeatherDoc documentation hyperlink",
                                       "https://example.com/featherdoc/docs") != 1U) {
        std::cerr << "failed to append TemplatePart hyperlink: "
                  << document.last_error().detail << '\n';
        return false;
    }

    body_template.append_paragraph(
        "Visual cues: hyperlink text should be visible, blue/underlined in Word, and no raw URL should appear in the page body.");

    if (document.save()) {
        std::cerr << "failed to save DOCX: " << document.last_error().detail << '\n';
        return false;
    }

    return true;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "hyperlinks_visual.docx";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    std::error_code remove_error;
    fs::remove(output_path, remove_error);

    if (!build_hyperlink_sample(output_path)) {
        return 1;
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
