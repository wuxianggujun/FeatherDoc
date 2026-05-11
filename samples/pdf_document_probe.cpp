#include <featherdoc.hpp>
#include <featherdoc/pdf/pdf_document_adapter.hpp>
#include <featherdoc/pdf/pdf_writer.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] std::string utf8_from_u8(std::u8string_view text) {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

void print_document_error(const featherdoc::Document &document,
                          std::string_view operation) {
    const auto &error_info = document.last_error();
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

[[nodiscard]] std::string tiny_rgba_png_data() {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U,
        0x00U, 0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U,
        0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x02U, 0x08U, 0x06U, 0x00U,
        0x00U, 0x00U, 0x72U, 0xB6U, 0x0DU, 0x24U, 0x00U, 0x00U, 0x00U,
        0x16U, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0xDAU, 0x63U, 0xF8U,
        0xCFU, 0xC0U, 0xF0U, 0x1FU, 0x08U, 0x1BU, 0x18U, 0x80U, 0xB4U,
        0xC3U, 0x7FU, 0x20U, 0x0FU, 0x00U, 0x3EU, 0x1AU, 0x06U, 0xBBU,
        0x82U, 0x99U, 0xA3U, 0xF4U, 0x00U, 0x00U, 0x00U, 0x00U, 0x49U,
        0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes),
            sizeof(tiny_png_bytes)};
}

[[nodiscard]] bool write_binary_file(const std::filesystem::path &path,
                                     std::string_view data) {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    return stream.good();
}

[[nodiscard]] std::string environment_value(const char *name) {
#if defined(_WIN32)
    char *value = nullptr;
    std::size_t size = 0;
    if (::_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result(value, size > 0 ? size - 1 : 0);
    std::free(value);
    return result;
#else
    const char *value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_proportional_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured =
            environment_value("FEATHERDOC_TEST_PROPORTIONAL_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeui.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibri.ttf");
#elif defined(__APPLE__)
    candidates.emplace_back("/System/Library/Fonts/Helvetica.ttc");
    candidates.emplace_back("/System/Library/Fonts/Arial.ttf");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path>
candidate_proportional_bold_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured =
            environment_value("FEATHERDOC_TEST_PROPORTIONAL_BOLD_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arialbd.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeuib.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibrib.ttf");
#elif defined(__APPLE__)
    candidates.emplace_back("/System/Library/Fonts/Helvetica.ttc");
    candidates.emplace_back("/System/Library/Fonts/Arial Bold.ttf");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path>
candidate_proportional_italic_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured =
            environment_value("FEATHERDOC_TEST_PROPORTIONAL_ITALIC_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/ariali.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeuii.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibrii.ttf");
#elif defined(__APPLE__)
    candidates.emplace_back("/System/Library/Fonts/Helvetica.ttc");
    candidates.emplace_back("/System/Library/Fonts/Arial Italic.ttf");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Italic.ttf");
    candidates.emplace_back(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Oblique.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path>
candidate_proportional_bold_italic_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value(
            "FEATHERDOC_TEST_PROPORTIONAL_BOLD_ITALIC_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arialbi.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeuiz.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibriz.ttf");
#elif defined(__APPLE__)
    candidates.emplace_back("/System/Library/Fonts/Helvetica.ttc");
    candidates.emplace_back("/System/Library/Fonts/Arial Bold Italic.ttf");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/liberation2/LiberationSans-BoldItalic.ttf");
    candidates.emplace_back(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-BoldOblique.ttf");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_cjk_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_TEST_CJK_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }
    if (const auto configured = environment_value("FEATHERDOC_PDF_CJK_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/Deng.ttf");
    candidates.emplace_back("C:/Windows/Fonts/NotoSansSC-VF.ttf");
    candidates.emplace_back("C:/Windows/Fonts/msyh.ttc");
    candidates.emplace_back("C:/Windows/Fonts/simhei.ttf");
    candidates.emplace_back("C:/Windows/Fonts/simsun.ttc");
#elif defined(__APPLE__)
    candidates.emplace_back("/System/Library/Fonts/PingFang.ttc");
    candidates.emplace_back("/System/Library/Fonts/STHeiti Light.ttc");
#else
    candidates.emplace_back(
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
    candidates.emplace_back(
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc");
    candidates.emplace_back("/usr/share/fonts/truetype/arphic/ukai.ttc");
    candidates.emplace_back("/usr/share/fonts/truetype/arphic/uming.ttc");
#endif

    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> candidate_cjk_bold_fonts() {
    std::vector<std::filesystem::path> candidates;

    if (const auto configured =
            environment_value("FEATHERDOC_TEST_CJK_BOLD_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }
    if (const auto configured =
            environment_value("FEATHERDOC_PDF_CJK_BOLD_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/Dengb.ttf");
    candidates.emplace_back("C:/Windows/Fonts/msyhbd.ttc");
    candidates.emplace_back("C:/Windows/Fonts/simhei.ttf");
    candidates.emplace_back("C:/Windows/Fonts/NotoSansSC-VF.ttf");
#elif defined(__APPLE__)
    candidates.emplace_back("/System/Library/Fonts/PingFang.ttc");
#else
    candidates.emplace_back(
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc");
    candidates.emplace_back(
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
#endif

    return candidates;
}

[[nodiscard]] std::filesystem::path
first_existing_path(const std::vector<std::filesystem::path> &candidates) {
    for (const auto &candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

[[nodiscard]] std::filesystem::path resolve_proportional_font() {
    return first_existing_path(candidate_proportional_fonts());
}

[[nodiscard]] std::filesystem::path resolve_proportional_bold_font() {
    return first_existing_path(candidate_proportional_bold_fonts());
}

[[nodiscard]] std::filesystem::path resolve_proportional_italic_font() {
    return first_existing_path(candidate_proportional_italic_fonts());
}

[[nodiscard]] std::filesystem::path resolve_proportional_bold_italic_font() {
    return first_existing_path(candidate_proportional_bold_italic_fonts());
}

[[nodiscard]] std::filesystem::path resolve_cjk_font() {
    return first_existing_path(candidate_cjk_fonts());
}

[[nodiscard]] std::filesystem::path resolve_cjk_bold_font() {
    return first_existing_path(candidate_cjk_bold_fonts());
}

} // namespace

int main(int argc, char **argv) {
    const std::filesystem::path output =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path("featherdoc-document-probe.pdf");
    auto source_path = output;
    source_path.replace_extension(".docx");

    featherdoc::Document document(source_path);
    if (const auto error = document.create_empty()) {
        print_document_error(document, "create_empty");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    const auto proportional_font = resolve_proportional_font();
    const auto proportional_bold_font = resolve_proportional_bold_font();
    const auto proportional_italic_font = resolve_proportional_italic_font();
    const auto proportional_bold_italic_font =
        resolve_proportional_bold_italic_font();
    const auto cjk_font = resolve_cjk_font();
    const auto cjk_bold_font = resolve_cjk_bold_font();
    if (!proportional_font.empty() &&
        !document.set_default_run_font_family("Probe Latin")) {
        print_document_error(document, "set_default_run_font_family");
        return 1;
    }
    if (!cjk_font.empty() &&
        !document.set_default_run_east_asia_font_family("Probe CJK")) {
        print_document_error(document, "set_default_run_east_asia_font_family");
        return 1;
    }

    if (!cjk_font.empty()) {
        auto east_asia_style = featherdoc::character_style_definition{};
        east_asia_style.name = "PDF Probe East Asia Accent";
        east_asia_style.run_font_family = std::string{"Probe Latin"};
        east_asia_style.run_east_asia_font_family = std::string{"Probe CJK"};
        east_asia_style.run_text_color = std::string{"C00000"};
        east_asia_style.run_bold = true;
        east_asia_style.run_underline = true;
        east_asia_style.run_font_size_points = 18.0;
        if (!document.ensure_character_style("PdfProbeEastAsiaAccent",
                                             east_asia_style) ||
            !document.materialize_style_run_properties(
                "PdfProbeEastAsiaAccent")) {
            print_document_error(document,
                                 "ensure_character_style(PdfProbeEastAsiaAccent)");
            return 1;
        }

        auto east_asia_note_style = featherdoc::character_style_definition{};
        east_asia_note_style.name = "PDF Probe East Asia Note";
        east_asia_note_style.run_font_family = std::string{"Probe Latin"};
        east_asia_note_style.run_east_asia_font_family = std::string{"Probe CJK"};
        east_asia_note_style.run_text_color = std::string{"336699"};
        east_asia_note_style.run_italic = true;
        east_asia_note_style.run_font_size_points = 14.0;
        if (!document.ensure_character_style("PdfProbeEastAsiaNote",
                                             east_asia_note_style) ||
            !document.materialize_style_run_properties("PdfProbeEastAsiaNote")) {
            print_document_error(document,
                                 "ensure_character_style(PdfProbeEastAsiaNote)");
            return 1;
        }
    }

    auto latin_emphasis_style = featherdoc::character_style_definition{};
    latin_emphasis_style.name = "PDF Probe Latin Emphasis";
    latin_emphasis_style.run_font_family = std::string{"Probe Latin"};
    latin_emphasis_style.run_text_color = std::string{"1F4E79"};
    latin_emphasis_style.run_bold = true;
    latin_emphasis_style.run_italic = true;
    latin_emphasis_style.run_font_size_points = 16.0;
    if (!document.ensure_character_style("PdfProbeLatinEmphasis",
                                         latin_emphasis_style) ||
        !document.materialize_style_run_properties("PdfProbeLatinEmphasis")) {
        print_document_error(document,
                             "ensure_character_style(PdfProbeLatinEmphasis)");
        return 1;
    }

    auto paragraph = document.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.set_text("FeatherDoc document-to-PDF adapter probe")) {
        std::cerr << "failed to seed title paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        "This file starts as a FeatherDoc Document, moves through the "
        "backend-neutral PdfDocumentLayout model, and is finally written by "
        "the PDFio generator.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append pipeline paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        "The current adapter intentionally supports a narrow text-only subset: "
        "paragraph order, basic line wrapping, page breaks, A4 size, margins, "
        "document metadata, and default section headers and footers.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append scope paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after(
        "FreeType metrics probe: iiiiiiiiiii WWWWWWWWWWW iiiiiiiiiii "
        "WWWWWWWWWWW keeps the wrap sensitive to actual glyph advances.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append metrics paragraph\n";
        return 1;
    }

    paragraph = paragraph.insert_paragraph_after("");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append styled Latin paragraph\n";
        return 1;
    }
    if (!paragraph.add_run("Styled Latin run: ").has_next()) {
        std::cerr << "failed to append styled Latin prefix\n";
        return 1;
    }
    auto latin_styled_run = paragraph.add_run("bold italic blue 16pt");
    if (!latin_styled_run.has_next() ||
        !document.set_run_style(latin_styled_run, "PdfProbeLatinEmphasis") ||
        !paragraph.add_run(" against the default document font.").has_next()) {
        print_document_error(document, "configure styled Latin run");
        return 1;
    }

    if (!cjk_font.empty()) {
        paragraph = paragraph.insert_paragraph_after("");
        if (!paragraph.has_next()) {
            std::cerr << "failed to append eastAsia paragraph\n";
            return 1;
        }
        if (!paragraph.add_run("East Asia run mapping: ").has_next()) {
            std::cerr << "failed to append eastAsia prefix\n";
            return 1;
        }
        auto cjk_styled_run =
            paragraph.add_run(utf8_from_u8(u8"中文样式映射 ABC 123"));
        if (!cjk_styled_run.has_next() ||
            !document.set_run_style(cjk_styled_run,
                                    "PdfProbeEastAsiaAccent") ||
            !paragraph.add_run(" should resolve to the Probe CJK font file.")
                 .has_next()) {
            print_document_error(document, "configure eastAsia styled run");
            return 1;
        }

        paragraph = paragraph.insert_paragraph_after("");
        if (!paragraph.has_next()) {
            std::cerr << "failed to append eastAsia note paragraph\n";
            return 1;
        }
        if (!paragraph.add_run("East Asia note style: ").has_next()) {
            std::cerr << "failed to append eastAsia note prefix\n";
            return 1;
        }
        auto cjk_note_run =
            paragraph.add_run(utf8_from_u8(u8"蓝色 14pt 中文备注"));
        if (!cjk_note_run.has_next() ||
            !document.set_run_style(cjk_note_run, "PdfProbeEastAsiaNote") ||
            !paragraph.add_run(" keeps the same eastAsia family with a "
                                "different size and color.")
                 .has_next()) {
            print_document_error(document, "configure eastAsia note run");
            return 1;
        }
    } else {
        paragraph = paragraph.insert_paragraph_after(
            "East Asia run mapping skipped because no CJK font file was found.");
        if (!paragraph.has_next()) {
            std::cerr << "failed to append CJK skip paragraph\n";
            return 1;
        }
    }

    const auto image_path = std::filesystem::temp_directory_path() /
                            "featherdoc-document-probe-inline-image.png";
    if (!write_binary_file(image_path, tiny_rgba_png_data())) {
        std::cerr << "failed to seed inline image fixture\n";
        return 1;
    }
    if (!document.append_image(image_path, 48U, 24U)) {
        print_document_error(document, "append_image");
        return 1;
    }
    std::error_code remove_error;
    std::filesystem::remove(image_path, remove_error);

    auto body_template = document.body_template();
    if (!static_cast<bool>(body_template) ||
        !body_template.append_paragraph(
             "This paragraph is intentionally after the inline image.")
             .has_next()) {
        std::cerr << "failed to append post-image paragraph\n";
        return 1;
    }

    auto &header = document.ensure_section_header_paragraphs(0U);
    if (!header.has_next() ||
        !header.set_text("FeatherDoc document PDF header")) {
        std::cerr << "failed to seed header paragraph\n";
        return 1;
    }

    auto &footer = document.ensure_section_footer_paragraphs(0U);
    if (!footer.has_next() ||
        !footer.set_text("FeatherDoc document PDF footer")) {
        std::cerr << "failed to seed footer paragraph\n";
        return 1;
    }

    featherdoc::pdf::PdfDocumentAdapterOptions adapter_options;
    adapter_options.metadata.title = "FeatherDoc document PDF adapter probe";
    adapter_options.render_headers_and_footers = true;
    adapter_options.render_inline_images = true;
    if (!proportional_font.empty()) {
        adapter_options.font_family = "Probe Latin";
        adapter_options.font_file_path = proportional_font;
        adapter_options.font_mappings.push_back(
            featherdoc::pdf::PdfFontMapping{"Probe Latin", proportional_font});
        if (!proportional_bold_font.empty()) {
            adapter_options.font_mappings.push_back(
                featherdoc::pdf::PdfFontMapping{"Probe Latin",
                                                proportional_bold_font, true,
                                                false});
        }
        if (!proportional_italic_font.empty()) {
            adapter_options.font_mappings.push_back(
                featherdoc::pdf::PdfFontMapping{"Probe Latin",
                                                proportional_italic_font, false,
                                                true});
        }
        if (!proportional_bold_italic_font.empty()) {
            adapter_options.font_mappings.push_back(
                featherdoc::pdf::PdfFontMapping{"Probe Latin",
                                                proportional_bold_italic_font,
                                                true, true});
        }
        adapter_options.use_system_font_fallbacks = false;
        adapter_options.line_height_points = 0.0;
    }
    if (!cjk_font.empty()) {
        adapter_options.cjk_font_file_path = cjk_font;
        adapter_options.font_mappings.push_back(
            featherdoc::pdf::PdfFontMapping{"Probe CJK", cjk_font});
        if (!cjk_bold_font.empty()) {
            adapter_options.font_mappings.push_back(
                featherdoc::pdf::PdfFontMapping{"Probe CJK", cjk_bold_font,
                                                true, false});
            adapter_options.font_mappings.push_back(
                featherdoc::pdf::PdfFontMapping{"Probe CJK", cjk_bold_font,
                                                true, true});
        }
    }
    auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, adapter_options);

    // 用浅色底板把 PNG 的透明区域显出来，方便肉眼核对。
    for (auto &page : layout.pages) {
        for (const auto &image : page.images) {
            page.rectangles.push_back(featherdoc::pdf::PdfRectangle{
                image.bounds,
                featherdoc::pdf::PdfRgbColor{0.18, 0.30, 0.45},
                featherdoc::pdf::PdfRgbColor{0.96, 0.94, 0.82},
                0.5,
                false,
                true,
            });
        }
    }

    featherdoc::pdf::PdfWriterOptions writer_options;
    writer_options.title = adapter_options.metadata.title;
    writer_options.page_size = adapter_options.page_size;

    featherdoc::pdf::PdfioGenerator generator;
    featherdoc::pdf::IPdfGenerator &pdf_generator = generator;
    const auto result = pdf_generator.write(layout, output, writer_options);
    if (!result) {
        std::cerr << result.error_message << '\n';
        return 1;
    }

    std::cout << "wrote " << output.string() << " (" << result.bytes_written
              << " bytes)\n";
    return 0;
}
