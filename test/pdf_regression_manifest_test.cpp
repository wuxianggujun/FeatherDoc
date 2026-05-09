#include <algorithm>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] std::string read_file(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

struct ManifestSample {
    std::string id;
    std::string kind;
    std::string output_file;
    std::size_t expected_pages{0};
    std::vector<std::string> expected_text;
    std::size_t expected_image_count{0};
};

[[nodiscard]] int hex_digit_value(char value) noexcept {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

void append_utf8_codepoint(std::string &output, std::uint32_t codepoint) {
    if (codepoint <= 0x7FU) {
        output.push_back(static_cast<char>(codepoint));
        return;
    }
    if (codepoint <= 0x7FFU) {
        output.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        return;
    }
    if (codepoint <= 0xFFFFU) {
        output.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
        output.push_back(
            static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        return;
    }
    output.push_back(static_cast<char>(0xF0U | (codepoint >> 18U)));
    output.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
    output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
}

class JsonReader {
  public:
    explicit JsonReader(std::string_view json) : json_(json) {}

    [[nodiscard]] std::vector<ManifestSample> parse_manifest_samples() {
        std::vector<ManifestSample> samples;
        expect('{');
        if (consume('}')) {
            return samples;
        }

        while (true) {
            const auto key = parse_string();
            expect(':');
            if (key == "samples") {
                samples = parse_samples_array();
            } else {
                skip_value();
            }

            if (consume(',')) {
                continue;
            }
            expect('}');
            break;
        }

        return samples;
    }

  private:
    [[nodiscard]] bool eof() const noexcept {
        return cursor_ >= json_.size();
    }

    void skip_whitespace() {
        while (!eof()) {
            const auto value = json_[cursor_];
            if (value != ' ' && value != '\n' && value != '\r' &&
                value != '\t') {
                break;
            }
            ++cursor_;
        }
    }

    [[nodiscard]] bool consume(char expected) {
        skip_whitespace();
        if (eof() || json_[cursor_] != expected) {
            return false;
        }
        ++cursor_;
        return true;
    }

    void expect(char expected) {
        skip_whitespace();
        REQUIRE(!eof());
        REQUIRE_EQ(json_[cursor_], expected);
        ++cursor_;
    }

    [[nodiscard]] std::string parse_string() {
        expect('"');
        std::string value;
        while (true) {
            REQUIRE(!eof());
            const auto current = json_[cursor_++];
            if (current == '"') {
                return value;
            }
            if (current != '\\') {
                value.push_back(current);
                continue;
            }

            REQUIRE(!eof());
            const auto escape = json_[cursor_++];
            switch (escape) {
            case '"':
            case '\\':
            case '/':
                value.push_back(escape);
                break;
            case 'b':
                value.push_back('\b');
                break;
            case 'f':
                value.push_back('\f');
                break;
            case 'n':
                value.push_back('\n');
                break;
            case 'r':
                value.push_back('\r');
                break;
            case 't':
                value.push_back('\t');
                break;
            case 'u':
                append_utf8_codepoint(value, parse_unicode_escape());
                break;
            default:
                FAIL("unsupported JSON string escape");
            }
        }
    }

    [[nodiscard]] std::uint32_t parse_unicode_escape() {
        REQUIRE(cursor_ + 4U <= json_.size());
        std::uint32_t codepoint = 0U;
        for (auto index = 0; index < 4; ++index) {
            const auto digit = hex_digit_value(json_[cursor_++]);
            REQUIRE_GE(digit, 0);
            codepoint = (codepoint << 4U) | static_cast<std::uint32_t>(digit);
        }
        return codepoint;
    }

    [[nodiscard]] std::size_t parse_integer() {
        skip_whitespace();
        const auto start = cursor_;
        while (!eof() && json_[cursor_] >= '0' && json_[cursor_] <= '9') {
            ++cursor_;
        }
        REQUIRE(cursor_ > start);
        return static_cast<std::size_t>(
            std::stoull(std::string{json_.substr(start, cursor_ - start)}));
    }

    [[nodiscard]] std::vector<std::string> parse_string_array() {
        std::vector<std::string> values;
        expect('[');
        if (consume(']')) {
            return values;
        }

        while (true) {
            values.push_back(parse_string());
            if (consume(',')) {
                continue;
            }
            expect(']');
            break;
        }
        return values;
    }

    [[nodiscard]] std::vector<ManifestSample> parse_samples_array() {
        std::vector<ManifestSample> samples;
        expect('[');
        if (consume(']')) {
            return samples;
        }

        while (true) {
            samples.push_back(parse_sample_object());
            if (consume(',')) {
                continue;
            }
            expect(']');
            break;
        }
        return samples;
    }

    [[nodiscard]] ManifestSample parse_sample_object() {
        ManifestSample sample;
        expect('{');
        if (consume('}')) {
            return sample;
        }

        while (true) {
            const auto key = parse_string();
            expect(':');
            if (key == "id") {
                sample.id = parse_string();
            } else if (key == "kind") {
                sample.kind = parse_string();
            } else if (key == "output_file") {
                sample.output_file = parse_string();
            } else if (key == "expected_pages") {
                sample.expected_pages = parse_integer();
            } else if (key == "expected_text") {
                sample.expected_text = parse_string_array();
            } else if (key == "expected_image_count") {
                sample.expected_image_count = parse_integer();
            } else {
                skip_value();
            }

            if (consume(',')) {
                continue;
            }
            expect('}');
            break;
        }

        return sample;
    }

    void skip_value() {
        skip_whitespace();
        REQUIRE(!eof());
        const auto current = json_[cursor_];
        if (current == '"') {
            static_cast<void>(parse_string());
            return;
        }
        if (current == '{') {
            skip_object();
            return;
        }
        if (current == '[') {
            skip_array();
            return;
        }
        if ((current >= '0' && current <= '9') || current == '-') {
            skip_number();
            return;
        }
        if (match_literal("true") || match_literal("false") ||
            match_literal("null")) {
            return;
        }
        FAIL("unsupported JSON value");
    }

    void skip_object() {
        expect('{');
        if (consume('}')) {
            return;
        }
        while (true) {
            static_cast<void>(parse_string());
            expect(':');
            skip_value();
            if (consume(',')) {
                continue;
            }
            expect('}');
            break;
        }
    }

    void skip_array() {
        expect('[');
        if (consume(']')) {
            return;
        }
        while (true) {
            skip_value();
            if (consume(',')) {
                continue;
            }
            expect(']');
            break;
        }
    }

    void skip_number() {
        if (!eof() && json_[cursor_] == '-') {
            ++cursor_;
        }
        while (!eof() && json_[cursor_] >= '0' && json_[cursor_] <= '9') {
            ++cursor_;
        }
        if (!eof() && json_[cursor_] == '.') {
            ++cursor_;
            while (!eof() && json_[cursor_] >= '0' && json_[cursor_] <= '9') {
                ++cursor_;
            }
        }
        if (!eof() && (json_[cursor_] == 'e' || json_[cursor_] == 'E')) {
            ++cursor_;
            if (!eof() && (json_[cursor_] == '+' || json_[cursor_] == '-')) {
                ++cursor_;
            }
            while (!eof() && json_[cursor_] >= '0' && json_[cursor_] <= '9') {
                ++cursor_;
            }
        }
    }

    [[nodiscard]] bool match_literal(std::string_view literal) {
        if (json_.substr(cursor_, literal.size()) != literal) {
            return false;
        }
        cursor_ += literal.size();
        return true;
    }

    std::string_view json_;
    std::size_t cursor_{0};
};

[[nodiscard]] std::vector<ManifestSample>
parse_samples_from_manifest(const std::string &json) {
    return JsonReader(json).parse_manifest_samples();
}

[[nodiscard]] const ManifestSample *
find_sample_by_id(const std::vector<ManifestSample> &samples,
                  std::string_view id) {
    const auto it =
        std::find_if(samples.begin(), samples.end(), [&](const auto &sample) {
            return sample.id == id;
        });
    return it == samples.end() ? nullptr : &(*it);
}

} // namespace

TEST_CASE("PDF regression manifest exists and declares the initial samples") {
    std::filesystem::path manifest_path;
#ifdef FEATHERDOC_PDF_REGRESSION_MANIFEST_PATH
    manifest_path = FEATHERDOC_PDF_REGRESSION_MANIFEST_PATH;
#else
    manifest_path = std::filesystem::current_path() /
                    "pdf_regression_manifest.json";
#endif
    REQUIRE(std::filesystem::exists(manifest_path));

    const auto json = read_file(manifest_path);
    const auto samples = parse_samples_from_manifest(json);
    REQUIRE_GE(samples.size(), 64U);
    std::unordered_set<std::string> sample_ids;
    for (const auto &sample : samples) {
        CHECK(sample_ids.insert(sample.id).second);
    }

    const auto expect_sample =
        [&](std::string_view id, std::string_view kind, std::size_t pages,
            std::size_t min_text_count,
            std::optional<std::size_t> image_count = std::nullopt) {
            INFO("sample id: " << id);
            const auto *sample = find_sample_by_id(samples, id);
            REQUIRE(sample != nullptr);
            CHECK_EQ(sample->kind, kind);
            CHECK_EQ(sample->expected_pages, pages);
            CHECK_GE(sample->expected_text.size(), min_text_count);
            if (image_count.has_value()) {
                CHECK_EQ(sample->expected_image_count, *image_count);
            }
        };

    expect_sample("single-text", "single_text", 1U, 1U);
    expect_sample("multi-page-text", "multi_page_text", 2U, 2U);
    expect_sample("cjk-text", "cjk_text", 1U, 2U);
    expect_sample("styled-text", "styled_text", 1U, 4U);
    expect_sample("font-size-text", "font_size_text", 1U, 3U);
    expect_sample("color-text", "color_text", 1U, 4U);
    expect_sample("mixed-style-text", "mixed_style_text", 1U, 4U);
    expect_sample("contract-cjk-style", "contract_cjk_style", 1U, 7U);
    expect_sample("document-contract-cjk-style", "document_contract_cjk_style",
                  1U, 12U);
    expect_sample("three-page-text", "three_page_text", 3U, 3U);
    expect_sample("landscape-text", "landscape_text", 1U, 3U);
    expect_sample("title-body-text", "title_body_text", 1U, 5U);
    expect_sample("dense-text", "dense_text", 1U, 6U);
    expect_sample("four-page-text", "four_page_text", 4U, 4U);
    expect_sample("underline-text", "underline_text", 1U, 4U);
    expect_sample("superscript-subscript-text",
                  "superscript_subscript_text", 1U, 3U);
    expect_sample("punctuation-text", "punctuation_text", 1U, 3U);
    expect_sample("two-page-text", "two_page_text", 2U, 2U);
    expect_sample("repeat-phrase-text", "repeat_phrase_text", 1U, 4U);
    expect_sample("bordered-box-text", "bordered_box_text", 1U, 2U);
    expect_sample("line-primitive-text", "line_primitive_text", 1U, 2U);
    expect_sample("table-like-grid-text", "table_like_grid_text", 1U, 4U);
    expect_sample("metadata-long-title-text", "metadata_long_title_text", 1U,
                  2U);
    expect_sample("header-footer-text", "header_footer_text", 1U, 3U);
    expect_sample("two-column-text", "two_column_text", 1U, 3U);
    expect_sample("invoice-grid-text", "invoice_grid_text", 1U, 3U);
    expect_sample("image-caption-text", "image_caption_text", 1U, 3U, 1U);
    expect_sample("sectioned-report-text", "sectioned_report_text", 4U, 8U);
    expect_sample("list-report-text", "list_report_text", 2U, 5U);
    expect_sample("long-report-text", "long_report_text", 1U, 4U);
    expect_sample("image-report-text", "image_report_text", 2U, 5U, 2U);
    expect_sample("cjk-report-text", "cjk_report_text", 2U, 5U);
    expect_sample("cjk-image-report-text", "cjk_image_report_text", 2U, 6U,
                  2U);
    expect_sample("document-eastasia-style-probe",
                  "document_eastasia_style_probe", 1U, 7U);
    expect_sample("document-image-semantics-text",
                  "document_image_semantics_text", 1U, 5U, 3U);
    expect_sample("document-cjk-complex-layout-text",
                  "document_cjk_complex_layout_text", 3U, 18U, 4U);
    expect_sample("document-cjk-copy-search-matrix-text",
                  "document_cjk_copy_search_matrix_text", 3U, 20U);
    expect_sample("document-cjk-font-embed-matrix-text",
                  "document_cjk_font_embed_matrix_text", 3U, 24U);
    expect_sample("document-cjk-font-embed-wrap-mix-text",
                  "document_cjk_font_embed_wrap_mix_text", 4U, 28U, 5U);
    expect_sample("document-cjk-image-wrap-stress-text",
                  "document_cjk_image_wrap_stress_text", 4U, 20U, 5U);
    expect_sample("document-cjk-extreme-page-breaks-text",
                  "document_cjk_extreme_page_breaks_text", 5U, 20U, 5U);
    expect_sample("document-cjk-table-wrap-page-flow-text",
                  "document_cjk_table_wrap_page_flow_text", 5U, 29U, 5U);
    expect_sample("document-table-semantics-text",
                  "document_table_semantics_text", 2U, 4U);
    expect_sample("document-long-flow-text", "document_long_flow_text", 5U,
                  7U);
    expect_sample("document-invoice-table-text",
                  "document_invoice_table_text", 1U, 8U);
    expect_sample("document-style-gallery-text",
                  "document_style_gallery_text", 1U, 7U);
    expect_sample("strikethrough-text", "strikethrough_text", 1U, 3U);
    expect_sample("style-superscript-subscript-text",
                  "style_superscript_subscript_text", 1U, 3U);
    expect_sample("document-rtl-bidi-text", "document_rtl_bidi_text", 1U, 7U);
    expect_sample("document-font-matrix-text", "document_font_matrix_text", 1U,
                  12U);
    expect_sample("header-footer-rtl-text", "header_footer_rtl_text", 1U, 8U);
    expect_sample("header-footer-rtl-variants-text",
                  "header_footer_rtl_variants_text", 3U, 13U);
    expect_sample("document-table-font-matrix-text",
                  "document_table_font_matrix_text", 3U, 14U);
    expect_sample("document-table-header-footer-variants-text",
                  "document_table_header_footer_variants_text", 3U, 14U);
    expect_sample("document-table-wrap-flow-text",
                  "document_table_wrap_flow_text", 3U, 12U);
    expect_sample("document-table-cjk-wrap-flow-text",
                  "document_table_cjk_wrap_flow_text", 2U, 11U);
    expect_sample("document-table-cjk-merged-repeat-text",
                  "document_table_cjk_merged_repeat_text", 4U, 10U);
    expect_sample("document-table-cant-split-text",
                  "document_table_cant_split_text", 3U, 10U);
    expect_sample("document-table-merged-cells-text",
                  "document_table_merged_cells_text", 1U, 10U);
    expect_sample("document-table-merged-header-repeat-text",
                  "document_table_merged_header_repeat_text", 4U, 10U);
    expect_sample("document-table-merged-header-footer-variants-text",
                  "document_table_merged_header_footer_variants_text", 4U,
                  10U);
    expect_sample("document-table-merged-cant-split-text",
                  "document_table_merged_cant_split_text", 3U, 10U);
    expect_sample("document-table-vertical-merged-cant-split-text",
                  "document_table_vertical_merged_cant_split_text", 4U, 10U);
    expect_sample("document-table-cjk-vertical-merged-cant-split-text",
                  "document_table_cjk_vertical_merged_cant_split_text", 3U,
                  10U);
}

TEST_CASE("PDF regression manifest parser preserves escaped strings") {
    const auto json = std::string{
        R"json({
  "samples": [
    {
      "id": "escape-test-\"id\"-\\-\/-\u4E2D\u6587",
      "kind": "escape_kind",
      "output_file": "escape-\\path\/file.pdf",
      "expected_pages": 1,
      "expected_text": [
        "Quote: \"",
        "Backslash: \\",
        "Slash: \/",
        "Unicode: \u4E2D\u6587"
      ]
    }
  ]
})json"};

    const auto samples = parse_samples_from_manifest(json);
    REQUIRE_EQ(samples.size(), 1U);
    CHECK_EQ(samples[0].id, "escape-test-\"id\"-\\-/-中文");
    CHECK_EQ(samples[0].kind, "escape_kind");
    CHECK_EQ(samples[0].output_file, "escape-\\path/file.pdf");
    CHECK_EQ(samples[0].expected_pages, 1U);
    REQUIRE_EQ(samples[0].expected_text.size(), 4U);
    CHECK_EQ(samples[0].expected_text[0], "Quote: \"");
    CHECK_EQ(samples[0].expected_text[1], "Backslash: \\");
    CHECK_EQ(samples[0].expected_text[2], "Slash: /");
    CHECK_EQ(samples[0].expected_text[3], "Unicode: 中文");
}
