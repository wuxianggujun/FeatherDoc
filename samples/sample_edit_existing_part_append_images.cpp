#include <featherdoc.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

void print_document_error(const featherdoc::Document &doc, std::string_view operation) {
    const auto &error_info = doc.last_error();
    std::cerr << operation << " failed";
    if (error_info.code) {
        std::cerr << ": " << error_info.code.message();
    }
    if (!error_info.detail.empty()) {
        std::cerr << " - " << error_info.detail;
    }
    if (!error_info.entry_name.empty()) {
        std::cerr << " [entry=" << error_info.entry_name << ']';
    }
    if (error_info.xml_offset.has_value()) {
        std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
    }
    std::cerr << '\n';
}

void write_u16_le(std::ostream &stream, std::uint16_t value) {
    stream.put(static_cast<char>(value & 0xFFU));
    stream.put(static_cast<char>((value >> 8U) & 0xFFU));
}

void write_u32_le(std::ostream &stream, std::uint32_t value) {
    stream.put(static_cast<char>(value & 0xFFU));
    stream.put(static_cast<char>((value >> 8U) & 0xFFU));
    stream.put(static_cast<char>((value >> 16U) & 0xFFU));
    stream.put(static_cast<char>((value >> 24U) & 0xFFU));
}

bool write_solid_bmp(const fs::path &path, std::uint32_t width_px,
                     std::uint32_t height_px, std::uint8_t red,
                     std::uint8_t green, std::uint8_t blue) {
    if (path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(path.parent_path(), directory_error);
        if (directory_error) {
            return false;
        }
    }

    constexpr std::uint32_t file_header_size = 14U;
    constexpr std::uint32_t dib_header_size = 40U;
    constexpr std::uint32_t bytes_per_pixel = 3U;
    const std::uint32_t row_stride =
        ((width_px * bytes_per_pixel) + 3U) & ~std::uint32_t{3U};
    const std::uint32_t pixel_data_size = row_stride * height_px;
    const std::uint32_t file_size =
        file_header_size + dib_header_size + pixel_data_size;

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        return false;
    }

    stream.put('B');
    stream.put('M');
    write_u32_le(stream, file_size);
    write_u16_le(stream, 0U);
    write_u16_le(stream, 0U);
    write_u32_le(stream, file_header_size + dib_header_size);

    write_u32_le(stream, dib_header_size);
    write_u32_le(stream, width_px);
    write_u32_le(stream, height_px);
    write_u16_le(stream, 1U);
    write_u16_le(stream, 24U);
    write_u32_le(stream, 0U);
    write_u32_le(stream, pixel_data_size);
    write_u32_le(stream, 2835U);
    write_u32_le(stream, 2835U);
    write_u32_le(stream, 0U);
    write_u32_le(stream, 0U);

    std::vector<char> row(row_stride, 0);
    for (std::uint32_t column = 0; column < width_px; ++column) {
        const auto offset = column * bytes_per_pixel;
        row[offset] = static_cast<char>(blue);
        row[offset + 1U] = static_cast<char>(green);
        row[offset + 2U] = static_cast<char>(red);
    }

    for (std::uint32_t current_row = 0; current_row < height_px; ++current_row) {
        stream.write(row.data(), static_cast<std::streamsize>(row.size()));
    }

    return stream.good();
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "edited_existing_part_append_images.docx";
    const fs::path body_inline_path =
        output_path.parent_path() / "sample_body_inline.bmp";
    const fs::path body_floating_path =
        output_path.parent_path() / "sample_body_floating.bmp";
    const fs::path header_inline_path =
        output_path.parent_path() / "sample_header_inline.bmp";
    const fs::path header_floating_path =
        output_path.parent_path() / "sample_header_floating.bmp";
    const fs::path footer_inline_path =
        output_path.parent_path() / "sample_footer_inline.bmp";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    if (!write_solid_bmp(body_inline_path, 112U, 32U, 64U, 92U, 140U) ||
        !write_solid_bmp(body_floating_path, 88U, 32U, 184U, 72U, 52U) ||
        !write_solid_bmp(header_inline_path, 96U, 32U, 44U, 107U, 214U) ||
        !write_solid_bmp(header_floating_path, 96U, 32U, 226U, 124U, 44U) ||
        !write_solid_bmp(footer_inline_path, 88U, 28U, 50U, 140U, 90U)) {
        std::cerr << "failed to create sample bitmap assets\n";
        return 1;
    }

    featherdoc::Document doc(output_path);
    if (const auto error = doc.create_empty()) {
        print_document_error(doc, "create_empty");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto paragraph = doc.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text(
            "FeatherDoc appended inline and floating images to existing body, "
            "header, and footer template parts. Expected render: body shows a "
            "steel inline bar plus a red floating badge; header shows a blue "
            "inline bar plus an orange floating badge; footer shows a green "
            "inline bar.")) {
        std::cerr << "failed to initialize body paragraph text\n";
        return 1;
    }

    auto &header = doc.ensure_section_header_paragraphs(0);
    if (!header.has_next() || !header.set_text("Existing header part image append sample.")) {
        std::cerr << "failed to initialize header paragraph text\n";
        return 1;
    }

    auto &footer = doc.ensure_section_footer_paragraphs(0);
    if (!footer.has_next() || !footer.set_text("Existing footer part image append sample.")) {
        std::cerr << "failed to initialize footer paragraph text\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save seeded document");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    featherdoc::Document reopened(output_path);
    if (const auto error = reopened.open()) {
        print_document_error(reopened, "reopen");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto body_template = reopened.body_template();
    auto header_template = reopened.section_header_template(0);
    auto footer_template = reopened.section_footer_template(0);
    if (!body_template || !header_template || !footer_template) {
        std::cerr << "failed to resolve body/header/footer template parts\n";
        return 1;
    }

    featherdoc::floating_image_options body_options;
    body_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    body_options.horizontal_offset_px = 320;
    body_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    body_options.vertical_offset_px = 12;

    featherdoc::floating_image_options header_options;
    header_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    header_options.horizontal_offset_px = 360;
    header_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    header_options.vertical_offset_px = -40;

    if (!body_template.append_image(body_inline_path, 224U, 64U) ||
        !body_template.append_floating_image(body_floating_path, 176U, 64U,
                                             body_options) ||
        !header_template.append_image(header_inline_path, 144U, 48U) ||
        !header_template.append_floating_image(header_floating_path, 144U, 48U,
                                               header_options) ||
        !footer_template.append_image(footer_inline_path, 160U, 48U)) {
        print_document_error(reopened, "append template part image");
        return 1;
    }

    const auto body_drawings = body_template.drawing_images();
    const auto header_drawings = header_template.drawing_images();
    const auto footer_drawings = footer_template.drawing_images();
    if (body_drawings.size() != 2U || body_template.inline_images().size() != 1U ||
        header_drawings.size() != 2U || header_template.inline_images().size() != 1U ||
        footer_drawings.size() != 1U || footer_template.inline_images().size() != 1U) {
        std::cerr << "template-part drawing image counts did not match expectations\n";
        return 1;
    }

    if (const auto error = reopened.save()) {
        print_document_error(reopened, "save after template part image append");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::error_code ignored_error;
    fs::remove(body_inline_path, ignored_error);
    fs::remove(body_floating_path, ignored_error);
    fs::remove(header_inline_path, ignored_error);
    fs::remove(header_floating_path, ignored_error);
    fs::remove(footer_inline_path, ignored_error);

    std::cout << output_path.string() << '\n';
    return 0;
}
