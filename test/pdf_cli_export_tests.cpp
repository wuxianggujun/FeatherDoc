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

void add_header_footer_placeholder_fixture(const fs::path &path) {
    featherdoc::Document document(path);
    REQUIRE_FALSE(document.open());

    for (std::size_t section_index = 0U; section_index < document.section_count();
         ++section_index) {
        auto header = document.ensure_section_header_paragraphs(section_index);
        REQUIRE(header.has_next());
        CHECK(header.set_text("Header page {{page}} of {{total_pages}}"));

        auto footer = document.ensure_section_footer_paragraphs(section_index);
        REQUIRE(footer.has_next());
        CHECK(footer.set_text("Footer page {{page}} of {{total_pages}}"));
    }

    REQUIRE_FALSE(document.save());
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

auto is_json_whitespace(char ch) -> bool {
    return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

auto is_complete_json_object(std::string_view text) -> bool {
    std::size_t begin = 0U;
    while (begin < text.size() && is_json_whitespace(text[begin])) {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && is_json_whitespace(text[end - 1U])) {
        --end;
    }

    if (begin == end || text[begin] != '{' || text[end - 1U] != '}') {
        return false;
    }

    int object_depth = 0;
    int array_depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t index = begin; index < end; ++index) {
        const char ch = text[index];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }

        switch (ch) {
        case '"':
            in_string = true;
            break;
        case '{':
            ++object_depth;
            break;
        case '}':
            if (object_depth == 0) {
                return false;
            }
            --object_depth;
            if (object_depth == 0 && array_depth == 0 && index + 1U != end) {
                return false;
            }
            break;
        case '[':
            ++array_depth;
            break;
        case ']':
            if (array_depth == 0) {
                return false;
            }
            --array_depth;
            break;
        default:
            break;
        }
    }

    return !in_string && !escaped && object_depth == 0 && array_depth == 0;
}

void expect_json_object_text(const std::string &text) {
    CHECK_MESSAGE(is_complete_json_object(text),
                  "expected a complete JSON object, got: " << text);
}

void expect_json_field(const std::string &text, std::string_view fragment) {
    CHECK_NE(text.find(std::string(fragment)), std::string::npos);
}

void expect_pdf_export_options_json(
    const std::string &text, bool render_headers_and_footers,
    bool render_inline_images, bool expand_header_footer_page_placeholders,
    bool subset_unicode_fonts, bool use_system_font_fallbacks) {
    expect_json_field(text, R"("options":{)");
    expect_json_field(text, std::string(R"("render_headers_and_footers":)") +
                                (render_headers_and_footers ? "true" : "false"));
    expect_json_field(text, std::string(R"("render_inline_images":)") +
                                (render_inline_images ? "true" : "false"));
    expect_json_field(
        text,
        std::string(R"("expand_header_footer_page_placeholders":)") +
            (expand_header_footer_page_placeholders ? "true" : "false"));
    expect_json_field(text, std::string(R"("subset_unicode_fonts":)") +
                                (subset_unicode_fonts ? "true" : "false"));
    expect_json_field(text, std::string(R"("use_system_font_fallbacks":)") +
                                (use_system_font_fallbacks ? "true" : "false"));
}

void expect_pdf_export_success_json(
    const std::string &text, const fs::path &output_path,
    bool render_headers_and_footers, bool render_inline_images,
    bool expand_header_footer_page_placeholders, bool subset_unicode_fonts,
    bool use_system_font_fallbacks) {
    expect_json_object_text(text);
    expect_json_field(text, R"("command":"export-pdf")");
    expect_json_field(text, R"("ok":true)");
    expect_json_field(text,
                      std::string(R"("output":)") +
                          json_quote(output_path.string()));
    expect_json_field(text,
                      std::string(R"("bytes_written":)") +
                          std::to_string(static_cast<unsigned long long>(
                              fs::file_size(output_path))));
    expect_pdf_export_options_json(text,
                                   render_headers_and_footers,
                                   render_inline_images,
                                   expand_header_footer_page_placeholders,
                                   subset_unicode_fonts,
                                   use_system_font_fallbacks);
}

void expect_pdf_export_error_json(const std::string &text,
                                  std::string_view stage,
                                  std::string_view detail_fragment = {}) {
    expect_json_object_text(text);
    expect_json_field(text, R"("command":"export-pdf")");
    expect_json_field(text, R"("ok":false)");
    expect_json_field(text, std::string(R"("stage":)") + json_quote(stage));
    expect_json_field(text, R"("message":)");
    if (!detail_fragment.empty()) {
        expect_json_field(text, detail_fragment);
    }
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

    expect_pdf_export_success_json(read_text_file(json_output),
                                   output,
                                   true,
                                   false,
                                   false,
                                   true,
                                   true);

    expect_pdf_export_success_json(read_text_file(summary_json),
                                   output,
                                   true,
                                   false,
                                   false,
                                   true,
                                   true);
}

TEST_CASE("cli export-pdf expands header and footer page placeholders") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "placeholders-source.docx";
    const fs::path output = work_dir / "placeholders-source.pdf";
    const fs::path json_output = work_dir / "placeholders-source-export.json";
    const fs::path summary_json = work_dir / "placeholders-source-summary.json";
    remove_if_exists(output);
    remove_if_exists(json_output);
    remove_if_exists(summary_json);

    create_cli_fixture(source);
    add_header_footer_placeholder_fixture(source);

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--render-headers-and-footers",
                      "--expand-header-footer-page-placeholders",
                      "--summary-json",
                      summary_json.string(),
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));
    CHECK_GT(fs::file_size(output), static_cast<std::uintmax_t>(500U));
    CHECK_EQ(read_pdf_magic(output), "%PDF-");
    assert_pdfium_can_read(output, 3U,
                           {"Header page 1 of 3",
                            "Header page 3 of 3",
                            "Footer page 1 of 3",
                            "Footer page 3 of 3"});

    expect_pdf_export_success_json(read_text_file(json_output),
                                   output,
                                   true,
                                   false,
                                   true,
                                   true,
                                   true);

    expect_pdf_export_success_json(read_text_file(summary_json),
                                   output,
                                   true,
                                   false,
                                   true,
                                   true,
                                   true);
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

    expect_pdf_export_success_json(read_text_file(json_output),
                                   output,
                                   false,
                                   true,
                                   false,
                                   true,
                                   true);
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

    expect_pdf_export_success_json(read_text_file(json_output),
                                   output,
                                   false,
                                   false,
                                   false,
                                   true,
                                   true);
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

    expect_pdf_export_success_json(read_text_file(subset_json),
                                   subset_output,
                                   false,
                                   false,
                                   false,
                                   true,
                                   true);
    expect_pdf_export_success_json(read_text_file(full_json),
                                   full_output,
                                   false,
                                   false,
                                   false,
                                   false,
                                   true);

    assert_pdfium_can_read(subset_output, 3U,
                           {"section 0 body", "section 1 body",
                            "section 2 body", "中文字体子集回归"});
    assert_pdfium_can_read(full_output, 3U,
                           {"section 0 body", "section 1 body",
                            "section 2 body", "中文字体子集回归"});
}

TEST_CASE("cli export-pdf accepts explicit CJK font without system fallback") {
    const auto cjk_font = find_cjk_font();
    if (cjk_font.empty()) {
        MESSAGE("skipping CLI PDF cjk-font-file test: no CJK font found");
        return;
    }

    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "cjk-font-source.docx";
    const fs::path output = work_dir / "cjk-font-source.pdf";
    const fs::path json_output = work_dir / "cjk-font-source-export.json";
    remove_if_exists(output);
    remove_if_exists(json_output);

    const auto cjk_text =
        utf8_from_u8(u8"\u4E2D\u6587\u5BFC\u51FA\u8DEF\u5F84");
    create_cli_fixture(source);
    {
        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        REQUIRE(paragraph.add_run(cjk_text).has_next());
        REQUIRE_FALSE(document.save());
    }

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--cjk-font-file",
                      cjk_font.string(),
                      "--no-system-font-fallbacks",
                      "--json"},
                     json_output),
             0);

    REQUIRE(fs::exists(output));
    CHECK_GT(fs::file_size(output), static_cast<std::uintmax_t>(500U));
    CHECK_EQ(read_pdf_magic(output), "%PDF-");
    assert_pdfium_can_read(output, 3U,
                           {"section 0 body", "section 1 body",
                            "section 2 body", std::string_view{cjk_text}});

    expect_pdf_export_success_json(read_text_file(json_output),
                                   output,
                                   false,
                                   false,
                                   false,
                                   true,
                                   false);
}

TEST_CASE("cli export-pdf reports summary json write failures") {
    const fs::path work_dir = test_binary_directory() / "pdf_cli_export";
    std::error_code error;
    fs::create_directories(work_dir, error);
    REQUIRE_FALSE(error);

    const fs::path source = work_dir / "summary-failure-source.docx";
    const fs::path output = work_dir / "summary-failure-source.pdf";
    const fs::path json_output = work_dir / "summary-failure-source-export.json";
    const fs::path summary_json_dir = work_dir / "summary-json-directory";
    remove_if_exists(output);
    remove_if_exists(json_output);
    remove_if_exists(summary_json_dir);

    fs::create_directories(summary_json_dir, error);
    REQUIRE_FALSE(error);

    create_cli_fixture(source);

    CHECK_EQ(run_cli({"export-pdf",
                      source.string(),
                      "--output",
                      output.string(),
                      "--summary-json",
                      summary_json_dir.string(),
                      "--json"},
                     json_output),
             1);

    REQUIRE(fs::exists(output));
    CHECK_GT(fs::file_size(output), static_cast<std::uintmax_t>(500U));
    CHECK_EQ(read_pdf_magic(output), "%PDF-");
    expect_pdf_export_error_json(read_text_file(json_output),
                                 "summary",
                                 "failed to open PDF export summary");

    remove_if_exists(summary_json_dir);
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

    expect_pdf_export_error_json(read_text_file(json_output),
                                 "parse",
                                 "--font-map expects <font-family>=<font-file>");
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

    expect_pdf_export_error_json(read_text_file(json_output), "open");
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

    expect_pdf_export_error_json(
        read_text_file(json_output),
        "export",
        "Unicode PDF text requires an embedded font file");
}
