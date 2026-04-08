#include <featherdoc.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include <zip.h>

namespace fs = std::filesystem;

namespace {

constexpr auto zip_compression_level = 0;

constexpr auto root_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
)"}; 

constexpr auto content_types_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml"
           ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
)"}; 

constexpr auto document_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:r>
        <w:t>Header/footer inline image editing sample.</w:t>
      </w:r>
    </w:p>
    <w:p>
      <w:r>
        <w:t>Expected render: header shows blue then orange, footer shows green.</w:t>
      </w:r>
    </w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId1"/>
      <w:footerReference w:type="default" r:id="rId2"/>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440"
               w:header="720" w:footer="720" w:gutter="0"/>
    </w:sectPr>
  </w:body>
</w:document>
)"}; 

constexpr auto document_relationships_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
)"}; 

constexpr auto header_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="0" w:name="header_logo_one"/>
    <w:r><w:t>replace-me</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_logo_two"/>
    <w:r><w:t>replace-me</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
</w:hdr>
)"}; 

constexpr auto footer_xml = std::string_view{
    R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:bookmarkStart w:id="2" w:name="footer_logo"/>
    <w:r><w:t>replace-me</w:t></w:r>
    <w:bookmarkEnd w:id="2"/>
  </w:p>
</w:ftr>
)"}; 

struct seed_entry final {
    std::string_view name;
    std::string_view content;
};

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

bool write_seed_docx(const fs::path &path) {
    const std::array entries{
        seed_entry{"_rels/.rels", root_relationships_xml},
        seed_entry{"[Content_Types].xml", content_types_xml},
        seed_entry{"word/document.xml", document_xml},
        seed_entry{"word/_rels/document.xml.rels", document_relationships_xml},
        seed_entry{"word/header1.xml", header_xml},
        seed_entry{"word/footer1.xml", footer_xml},
    };

    int zip_error = 0;
    zip_t *zip = zip_openwitherror(path.string().c_str(), zip_compression_level, 'w',
                                   &zip_error);
    if (zip == nullptr) {
        return false;
    }

    bool success = true;
    for (const auto &entry : entries) {
        if (zip_entry_open(zip, entry.name.data()) != 0 ||
            zip_entry_write(zip, entry.content.data(), entry.content.size()) < 0 ||
            zip_entry_close(zip) != 0) {
            success = false;
            break;
        }
    }

    zip_close(zip);
    return success;
}

} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1])
                 : fs::current_path() / "edited_existing_part_images.docx";
    const fs::path header_source_path =
        output_path.parent_path() / "sample_header_source.bmp";
    const fs::path header_replacement_path =
        output_path.parent_path() / "sample_header_replacement.bmp";
    const fs::path footer_source_path =
        output_path.parent_path() / "sample_footer_source.bmp";
    const fs::path extracted_path =
        output_path.parent_path() / "sample_header_extracted.bmp";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    if (!write_solid_bmp(header_source_path, 96U, 32U, 44U, 107U, 214U) ||
        !write_solid_bmp(header_replacement_path, 96U, 32U, 226U, 124U, 44U) ||
        !write_solid_bmp(footer_source_path, 96U, 32U, 50U, 140U, 90U)) {
        std::cerr << "failed to create sample bitmap assets\n";
        return 1;
    }

    if (!write_seed_docx(output_path)) {
        std::cerr << "failed to seed minimal template docx\n";
        return 1;
    }

    featherdoc::Document doc(output_path);
    if (const auto error = doc.open()) {
        print_document_error(doc, "open");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    auto header_template = doc.section_header_template(0);
    auto footer_template = doc.section_footer_template(0);
    if (!header_template || !footer_template) {
        std::cerr << "failed to resolve section header/footer template parts\n";
        return 1;
    }

    if (header_template.replace_bookmark_with_image("header_logo_one", header_source_path,
                                                    144U, 48U) != 1U ||
        header_template.replace_bookmark_with_image("header_logo_two", header_source_path,
                                                    144U, 48U) != 1U ||
        footer_template.replace_bookmark_with_image("footer_logo", footer_source_path,
                                                    144U, 48U) != 1U) {
        print_document_error(doc, "replace_bookmark_with_image");
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save seeded images");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    featherdoc::Document reopened(output_path);
    if (const auto error = reopened.open()) {
        print_document_error(reopened, "reopen");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    header_template = reopened.section_header_template(0);
    footer_template = reopened.section_footer_template(0);
    if (!header_template || !footer_template) {
        std::cerr << "failed to reopen section header/footer template parts\n";
        return 1;
    }

    const auto header_images = header_template.inline_images();
    const auto footer_images = footer_template.inline_images();
    if (header_images.size() != 2U || footer_images.size() != 1U) {
        std::cerr << "unexpected template-part image counts after initial save\n";
        return 1;
    }

    if (!header_template.extract_inline_image(1U, extracted_path)) {
        print_document_error(reopened, "extract_inline_image");
        return 1;
    }

    if (!header_template.replace_inline_image(1U, header_replacement_path)) {
        print_document_error(reopened, "replace_inline_image");
        return 1;
    }

    const auto updated_header_images = header_template.inline_images();
    if (updated_header_images.size() != 2U || updated_header_images[1].width_px != 144U ||
        updated_header_images[1].height_px != 48U) {
        std::cerr << "header image metadata did not match expected replacement layout\n";
        return 1;
    }

    auto paragraph = reopened.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text(
            "FeatherDoc edited existing header/footer images in this document.")) {
        std::cerr << "failed to rewrite the first body paragraph\n";
        return 1;
    }

    if (const auto error = reopened.save()) {
        print_document_error(reopened, "save after header image replacement");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::error_code ignored_error;
    fs::remove(header_source_path, ignored_error);
    fs::remove(header_replacement_path, ignored_error);
    fs::remove(footer_source_path, ignored_error);
    fs::remove(extracted_path, ignored_error);

    std::cout << output_path.string() << '\n';
    return 0;
}
