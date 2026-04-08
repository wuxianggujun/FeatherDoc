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
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "edited_existing.docx";
    const fs::path template_path = fs::current_path() / "my_test.docx";
    const fs::path image_source_path =
        output_path.parent_path() / "sample_edit_existing_source.bmp";
    const fs::path image_replacement_path =
        output_path.parent_path() / "sample_edit_existing_replacement.bmp";
    const fs::path extracted_image_path =
        output_path.parent_path() / "sample_edit_existing_extracted.bmp";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    std::error_code filesystem_error;
    fs::copy_file(template_path, output_path, fs::copy_options::overwrite_existing,
                  filesystem_error);
    if (filesystem_error) {
        std::cerr << "failed to seed editable template: " << filesystem_error.message() << '\n';
        return 1;
    }

    featherdoc::Document doc(output_path);
    if (const auto error = doc.open()) {
        print_document_error(doc, "open");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto first_paragraph = doc.paragraphs();
    if (!first_paragraph.has_next() ||
        !first_paragraph.set_text("FeatherDoc edited this existing document.")) {
        std::cerr << "failed to rewrite the first paragraph\n";
        return 1;
    }

    auto removed_paragraph =
        first_paragraph.insert_paragraph_after("This temporary paragraph is removed.");
    if (!removed_paragraph.has_next()) {
        std::cerr << "failed to append removable paragraph\n";
        return 1;
    }

    auto edited_paragraph = removed_paragraph.insert_paragraph_after("");
    if (!edited_paragraph.has_next()) {
        std::cerr << "failed to append edited paragraph\n";
        return 1;
    }

    if (!doc.set_paragraph_style(edited_paragraph, "Heading2")) {
        print_document_error(doc, "set_paragraph_style");
        return 1;
    }

    if (!edited_paragraph.add_run("Editing APIs: ").has_next()) {
        std::cerr << "failed to append heading prefix\n";
        return 1;
    }

    auto removed_run = edited_paragraph.add_run("transient ");
    if (!removed_run.has_next()) {
        std::cerr << "failed to append removable run\n";
        return 1;
    }

    auto highlighted_run =
        edited_paragraph.add_run(
            "paragraph replacement, run removal, row removal, table removal, and cell rewriting.");
    if (!highlighted_run.has_next() || !doc.set_run_style(highlighted_run, "Strong")) {
        print_document_error(doc, "set_run_style");
        return 1;
    }

    if (!removed_run.remove() || !removed_paragraph.remove()) {
        std::cerr << "failed to remove temporary editing scaffolding\n";
        return 1;
    }

    auto table = doc.append_table(2, 2);
    if (!table.has_next() || !table.set_style_id("TableGrid") ||
        !table.set_layout_mode(featherdoc::table_layout_mode::fixed) ||
        !table.set_width_twips(7200U)) {
        std::cerr << "failed to append summary table\n";
        return 1;
    }

    auto row = table.rows();
    if (!row.has_next()) {
        std::cerr << "failed to access first table row\n";
        return 1;
    }

    auto cell = row.cells();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") || !cell.set_text("Operation")) {
        std::cerr << "failed to fill first header cell\n";
        return 1;
    }

    cell.next();
    if (!cell.has_next() || !cell.set_fill_color("D9EAF7") || !cell.set_text("Result")) {
        std::cerr << "failed to fill second header cell\n";
        return 1;
    }

    row.next();
    if (!row.has_next()) {
        std::cerr << "failed to access second table row\n";
        return 1;
    }

    cell = row.cells();
    if (!cell.has_next() || !cell.set_text("Existing doc edit")) {
        std::cerr << "failed to fill first body cell\n";
        return 1;
    }

    cell.next();
    if (!cell.has_next() ||
        !cell.set_text("Open, mutate, replace one inline image, and save succeeded.")) {
        std::cerr << "failed to fill second body cell\n";
        return 1;
    }

    auto removed_row = table.append_row(2);
    if (!removed_row.has_next()) {
        std::cerr << "failed to append removable row\n";
        return 1;
    }

    auto removed_cell = removed_row.cells();
    if (!removed_cell.has_next() || !removed_cell.set_text("Temporary row")) {
        std::cerr << "failed to fill removable row first cell\n";
        return 1;
    }

    removed_cell.next();
    if (!removed_cell.has_next() || !removed_cell.set_text("This row should be gone.")) {
        std::cerr << "failed to fill removable row second cell\n";
        return 1;
    }

    if (!removed_row.remove()) {
        std::cerr << "failed to remove temporary table row\n";
        return 1;
    }

    auto removed_table = doc.append_table(1, 1);
    if (!removed_table.has_next()) {
        std::cerr << "failed to append removable table\n";
        return 1;
    }

    auto removed_table_cell = removed_table.rows().cells();
    if (!removed_table_cell.has_next() || !removed_table_cell.set_text("Temporary table")) {
        std::cerr << "failed to fill removable table cell\n";
        return 1;
    }

    if (!removed_table.remove()) {
        std::cerr << "failed to remove temporary table\n";
        return 1;
    }

    if (!write_solid_bmp(image_source_path, 96U, 48U, 44U, 107U, 214U) ||
        !write_solid_bmp(image_replacement_path, 96U, 48U, 226U, 124U, 44U)) {
        std::cerr << "failed to create sample bitmap assets\n";
        return 1;
    }

    if (!doc.append_image(image_source_path, 144U, 72U) ||
        !doc.append_image(image_source_path, 144U, 72U)) {
        print_document_error(doc, "append_image");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    featherdoc::Document reopened(output_path);
    if (const auto error = reopened.open()) {
        print_document_error(reopened, "reopen");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    const auto inline_images = reopened.inline_images();
    if (inline_images.size() < 2U) {
        std::cerr << "expected two inline images after initial save\n";
        return 1;
    }

    if (!reopened.extract_inline_image(1U, extracted_image_path)) {
        print_document_error(reopened, "extract_inline_image");
        return 1;
    }

    if (!reopened.replace_inline_image(1U, image_replacement_path)) {
        print_document_error(reopened, "replace_inline_image");
        return 1;
    }

    const auto updated_images = reopened.inline_images();
    if (updated_images.size() < 2U || updated_images[1].width_px != 144U ||
        updated_images[1].height_px != 72U) {
        std::cerr << "inline image metadata did not match expected replacement layout\n";
        return 1;
    }

    if (const auto error = reopened.save()) {
        print_document_error(reopened, "save after image replacement");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::error_code ignored_error;
    fs::remove(image_source_path, ignored_error);
    fs::remove(image_replacement_path, ignored_error);
    fs::remove(extracted_image_path, ignored_error);

    std::cout << output_path.string() << '\n';
    return 0;
}
