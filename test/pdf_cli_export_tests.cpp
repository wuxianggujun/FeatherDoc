#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cli_test_support.hpp"

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
#include <featherdoc/pdf/pdf_parser.hpp>
#endif

#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iterator>
#include <string_view>
#include <vector>

namespace {

auto read_binary_file(const fs::path &path) -> std::string {
    std::ifstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}

auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

auto read_pdf_magic(const fs::path &path) -> std::string {
    std::ifstream stream(path, std::ios::binary);
    REQUIRE(stream.good());

    std::string magic(5U, '\0');
    stream.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    REQUIRE_EQ(stream.gcount(), static_cast<std::streamsize>(magic.size()));
    return magic;
}

auto tiny_png_data() -> std::string {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U,
        0x00U, 0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x02U,
        0x00U, 0x00U, 0x00U, 0x02U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x72U,
        0xB6U, 0x0DU, 0x24U, 0x00U, 0x00U, 0x00U, 0x16U, 0x49U, 0x44U, 0x41U,
        0x54U, 0x78U, 0xDAU, 0x63U, 0xF8U, 0xCFU, 0xC0U, 0xF0U, 0x1FU, 0x08U,
        0x1BU, 0x18U, 0x80U, 0xB4U, 0xC3U, 0x7FU, 0x20U, 0x0FU, 0x00U, 0x3EU,
        0x1AU, 0x06U, 0xBBU, 0x82U, 0x99U, 0xA3U, 0xF4U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U, 0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes),
            sizeof(tiny_png_bytes)};
}

auto candidate_latin_fonts() -> std::vector<fs::path> {
    std::vector<fs::path> candidates;

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/arial.ttf");
    candidates.emplace_back("C:/Windows/Fonts/segoeui.ttf");
    candidates.emplace_back("C:/Windows/Fonts/calibri.ttf");
#else
    candidates.emplace_back(
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf");
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    return candidates;
}

auto environment_value(const char *name) -> std::string {
#if defined(_WIN32)
    char *value = nullptr;
    std::size_t size = 0;
    if (::_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result(value, size > 0 ? size - 1U : 0U);
    std::free(value);
    return result;
#else
    const char *value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

auto candidate_cjk_fonts() -> std::vector<fs::path> {
    std::vector<fs::path> candidates;

    if (const auto configured = environment_value("FEATHERDOC_TEST_CJK_FONT");
        !configured.empty()) {
        candidates.emplace_back(configured);
    }

#if defined(_WIN32)
    candidates.emplace_back("C:/Windows/Fonts/Deng.ttf");
    candidates.emplace_back("C:/Windows/Fonts/NotoSansSC-VF.ttf");
    candidates.emplace_back("C:/Windows/Fonts/msyh.ttc");
    candidates.emplace_back("C:/Windows/Fonts/simhei.ttf");
    candidates.emplace_back("C:/Windows/Fonts/simsun.ttc");
#else
    candidates.emplace_back(
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
#endif

    return candidates;
}

auto find_latin_font() -> fs::path {
    for (const auto &candidate : candidate_latin_fonts()) {
        if (fs::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

auto find_cjk_font() -> fs::path {
    for (const auto &candidate : candidate_cjk_fonts()) {
        if (fs::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
void assert_pdfium_can_read(const fs::path &path, std::size_t expected_pages,
                            std::initializer_list<std::string_view> texts) {
    featherdoc::pdf::PdfiumParser parser;
    const auto result = parser.parse(path, {});
    REQUIRE_MESSAGE(result.success, result.error_message);
    REQUIRE_EQ(result.document.pages.size(), expected_pages);

    std::string extracted_text;
    for (const auto &page : result.document.pages) {
        for (const auto &span : page.text_spans) {
            extracted_text += span.text;
        }
    }

    for (const auto text : texts) {
        CHECK_NE(extracted_text.find(std::string(text)), std::string::npos);
    }
}
#else
void assert_pdfium_can_read(const fs::path &, std::size_t,
                            std::initializer_list<std::string_view>) {}
#endif

void write_binary_file(const fs::path &path, const std::string &data) {
    std::ofstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    REQUIRE(stream.good());
}

} // namespace

TEST_CASE("cli export-pdf writes a PDF file and json summary") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "source.docx";
    const fs::path output = work_dir / "source.pdf";
    const fs::path json_output = work_dir / "source-export.json";
    const fs::path summary_json = work_dir / "source-summary.json";
    remove_if_exists(output);
    remove_if_exists(json_output);
    remove_if_exists(summary_json);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--render-headers-and-footers",
                      "--summary-json",
                      summary_json.string(),
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));
    CHECK_GT(fs::file_size(output), static_cast<std::uintmax_t>(500U));
    CHECK_EQ(read_pdf_magic(output), "%PDF-");
    assert_pdfium_can_read(output, 3U,
                           {"section 0 body", "section 1 body",
                            "section 2 body"});

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"export-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(json.find(R"("bytes_written":)"), std::string::npos);

    const auto summary = read_text_file(summary_json);
    CHECK_NE(summary.find(R"("command":"export-pdf")"), std::string::npos);
    CHECK_NE(summary.find(R"("ok":true)"), std::string::npos);
    CHECK_NE(summary.find(R"("output":)"), std::string::npos);
    CHECK_NE(summary.find(R"("bytes_written":)"), std::string::npos);
}

TEST_CASE("cli export-pdf can render inline images") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "image-source.docx";
    const fs::path image = work_dir / "inline.png";
    const fs::path output = work_dir / "image-source.pdf";
    const fs::path json_output = work_dir / "image-source-export.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    create_cli_fixture(source);
    write_binary_file(image, tiny_png_data());

    featherdoc::Document document(source);
    REQUIRE_FALSE(document.open());
    REQUIRE(document.append_image(image, 64U, 32U));
    REQUIRE_FALSE(document.save());

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--render-inline-images",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));
    const auto pdf = read_binary_file(output);
    CHECK_EQ(pdf.substr(0U, 5U), "%PDF-");
    CHECK_NE(pdf.find("/Subtype/Image"), std::string::npos);
    assert_pdfium_can_read(output, 3U,
                           {"section 0 body", "section 1 body",
                            "section 2 body"});

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"export-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":true)"), std::string::npos);
}

TEST_CASE("cli export-pdf accepts an explicit font family mapping") {
    const auto latin_font = find_latin_font();
    if (latin_font.empty()) {
        MESSAGE("skipping CLI PDF font-map test: no Latin font found");
        return;
    }

    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "font-map-source.docx";
    const fs::path output = work_dir / "font-map-source.pdf";
    const fs::path json_output = work_dir / "font-map-source-export.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--font-map",
                      "Helvetica=" + latin_font.string(),
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));
    CHECK_GT(fs::file_size(output), static_cast<std::uintmax_t>(500U));
    CHECK_EQ(read_pdf_magic(output), "%PDF-");
    assert_pdfium_can_read(output, 3U,
                           {"section 0 body", "section 1 body",
                            "section 2 body"});

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"export-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":true)"), std::string::npos);
}

TEST_CASE("cli export-pdf honors font subset toggles for CJK fonts") {
    const auto cjk_font = find_cjk_font();
    if (cjk_font.empty()) {
        MESSAGE("skipping CLI PDF subset test: no CJK font found");
        return;
    }

    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "subset-toggle-source.docx";
    const fs::path full_output = work_dir / "subset-toggle-source-full.pdf";
    const fs::path subset_output = work_dir / "subset-toggle-source-subset.pdf";
    const fs::path full_json = work_dir / "subset-toggle-source-full.json";
    const fs::path subset_json = work_dir / "subset-toggle-source-subset.json";
    remove_if_exists(full_output);
    remove_if_exists(subset_output);
    remove_if_exists(full_json);
    remove_if_exists(subset_json);

    create_cli_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        REQUIRE(paragraph.add_run(utf8_from_u8(u8"中文字体子集回归")).has_next());
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      full_output.string(),
                      "--cjk-font-file",
                      cjk_font.string(),
                      "--no-font-subset",
                      "--json"},
                     full_json),
             0);
    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      subset_output.string(),
                      "--cjk-font-file",
                      cjk_font.string(),
                      "--json"},
                     subset_json),
             0);

    REQUIRE(fs::exists(subset_output));
    REQUIRE(fs::exists(full_output));
    CHECK_GT(fs::file_size(full_output), fs::file_size(subset_output));

    const auto subset_pdf = read_binary_file(subset_output);
    const auto full_pdf = read_binary_file(full_output);
    CHECK_NE(subset_pdf.find("/ToUnicode"), std::string::npos);
    CHECK_NE(subset_pdf.find("/CIDFontType2"), std::string::npos);
    CHECK_NE(full_pdf.find("/ToUnicode"), std::string::npos);
    CHECK_NE(full_pdf.find("/CIDFontType2"), std::string::npos);

    const auto subset_summary = read_text_file(subset_json);
    const auto full_summary = read_text_file(full_json);
    CHECK_NE(subset_summary.find(R"("command":"export-pdf")"),
             std::string::npos);
    CHECK_NE(full_summary.find(R"("command":"export-pdf")"),
             std::string::npos);

    assert_pdfium_can_read(subset_output, 3U,
                           {"section 0 body", "section 1 body",
                            "section 2 body", "中文字体子集回归"});
    assert_pdfium_can_read(full_output, 3U,
                           {"section 0 body", "section 1 body",
                            "section 2 body", "中文字体子集回归"});
}

TEST_CASE("cli export-pdf rejects invalid font family mappings") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "bad-font-map-source.docx";
    const fs::path output = work_dir / "bad-font-map-source.pdf";
    const fs::path json_output = work_dir / "bad-font-map-source-export.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--font-map",
                      "Helvetica",
                      "--json"},
                     json_output),
             2);

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"export-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("stage":"parse")"), std::string::npos);
    CHECK_NE(json.find("--font-map expects <font-family>=<font-file>"),
             std::string::npos);
}

TEST_CASE("cli export-pdf reports missing input documents") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "missing-source.docx";
    const fs::path output = work_dir / "missing-source.pdf";
    const fs::path json_output = work_dir / "missing-source-export.json";
    remove_if_exists(source);
    remove_if_exists(output);
    remove_if_exists(json_output);

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--json"},
                     json_output),
             1);

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"export-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("stage":"open")"), std::string::npos);
}

TEST_CASE("cli export-pdf reports missing font files") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "missing-font-source.docx";
    const fs::path output = work_dir / "missing-font-source.pdf";
    const fs::path json_output = work_dir / "missing-font-source-export.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    create_cli_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        REQUIRE(paragraph.add_run(utf8_from_u8(u8"中文缺字体")).has_next());
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--no-system-font-fallbacks",
                      "--json"},
                     json_output),
             1);

    const auto json = read_text_file(json_output);
    CHECK_NE(json.find(R"("command":"export-pdf")"), std::string::npos);
    CHECK_NE(json.find(R"("ok":false)"), std::string::npos);
    CHECK_NE(json.find(R"("stage":"export")"), std::string::npos);
    CHECK_NE(json.find("Unicode PDF text requires an embedded font file"),
             std::string::npos);
}
