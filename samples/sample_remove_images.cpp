#include <featherdoc.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

void print_document_error(const featherdoc::Document &doc,
                          std::string_view operation) {
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
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "remove_images.docx";
    const fs::path inline_image_path =
        output_path.parent_path() / "remove_images_inline.bmp";
    const fs::path floating_image_path =
        output_path.parent_path() / "remove_images_floating.bmp";
    const fs::path trailing_image_path =
        output_path.parent_path() / "remove_images_trailing.bmp";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    if (!write_solid_bmp(inline_image_path, 144U, 48U, 44U, 107U, 214U) ||
        !write_solid_bmp(floating_image_path, 144U, 48U, 226U, 124U, 44U) ||
        !write_solid_bmp(trailing_image_path, 120U, 36U, 63U, 158U, 83U)) {
        std::cerr << "failed to generate sample image assets\n";
        return 1;
    }

    featherdoc::Document seed(output_path);
    if (const auto error = seed.create_empty()) {
        print_document_error(seed, "create_empty");
        return 1;
    }

    auto paragraph = seed.paragraphs();
    if (!paragraph.set_text("Remove image sample")) {
        print_document_error(seed, "set_text");
        return 1;
    }

    auto intro = paragraph.insert_paragraph_after(
        "Expected final render: only the blue inline image remains.");
    if (!intro.has_next()) {
        std::cerr << "failed to insert intro paragraph\n";
        return 1;
    }

    auto detail = intro.insert_paragraph_after(
        "The orange floating image and green trailing inline image are created "
        "first, then removed through remove_drawing_image(...) and "
        "remove_inline_image(...).");
    if (!detail.has_next()) {
        std::cerr << "failed to insert detail paragraph\n";
        return 1;
    }

    if (!seed.append_image(inline_image_path, 144U, 48U)) {
        print_document_error(seed, "append_image inline");
        return 1;
    }

    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    floating_options.horizontal_offset_px = 460;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::margin;
    floating_options.vertical_offset_px = 24;
    if (!seed.append_floating_image(floating_image_path, 144U, 48U,
                                    floating_options)) {
        print_document_error(seed, "append_floating_image");
        return 1;
    }

    if (!seed.append_image(trailing_image_path, 120U, 36U)) {
        print_document_error(seed, "append_image trailing");
        return 1;
    }

    if (const auto error = seed.save()) {
        print_document_error(seed, "save initial");
        return 1;
    }

    featherdoc::Document doc(output_path);
    if (const auto error = doc.open()) {
        print_document_error(doc, "open");
        return 1;
    }

    if (!doc.remove_drawing_image(1U)) {
        print_document_error(doc, "remove_drawing_image");
        return 1;
    }

    if (!doc.remove_inline_image(1U)) {
        print_document_error(doc, "remove_inline_image");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save final");
        return 1;
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    std::cout << "remaining inline image asset: " << inline_image_path.string() << '\n';
    std::cout << "removed floating image asset: " << floating_image_path.string() << '\n';
    std::cout << "removed trailing inline image asset: "
              << trailing_image_path.string() << '\n';
    return 0;
}
