#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace {
bool append_labeled_image(featherdoc::Document &document, featherdoc::TemplatePart &body,
                          const char *label, const fs::path &path) {
    body.append_paragraph(label);
    if (!body.append_image(path, 192U, 96U)) {
        std::cerr << "failed to append " << label << ": "
                  << document.last_error().detail << '\n';
        return false;
    }
    return true;
}
} // namespace

int main(int argc, char **argv) {
    if (argc < 5) {
        std::cerr << "usage: sample_extended_image_formats_visual <output.docx> <image.svg> <image.webp> <image.tiff>\n";
        return 2;
    }

    const fs::path output_path = argv[1];
    const fs::path svg_path = argv[2];
    const fs::path webp_path = argv[3];
    const fs::path tiff_path = argv[4];

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

    featherdoc::Document document(output_path);
    if (document.create_empty()) {
        std::cerr << "failed to create empty DOCX: " << document.last_error().detail << '\n';
        return 1;
    }

    auto body = document.body_template();
    if (!body || !body.paragraphs().set_text("Extended image format visual sample")) {
        std::cerr << "failed to initialize document body\n";
        return 1;
    }

    if (!append_labeled_image(document, body, "SVG image (image/svg+xml):", svg_path) ||
        !append_labeled_image(document, body, "WebP image (image/webp):", webp_path) ||
        !append_labeled_image(document, body, "TIFF image (image/tiff):", tiff_path)) {
        return 1;
    }

    const auto images = document.drawing_images();
    if (images.size() != 3U || images[0].content_type != "image/svg+xml" ||
        images[1].content_type != "image/webp" || images[2].content_type != "image/tiff") {
        std::cerr << "unexpected image summary after append\n";
        return 1;
    }

    if (document.save()) {
        std::cerr << "failed to save generated DOCX: " << document.last_error().detail << '\n';
        return 1;
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
