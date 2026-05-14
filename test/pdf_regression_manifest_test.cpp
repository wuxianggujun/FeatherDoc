#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
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
    CHECK_NE(json.find("\"single-text\""), std::string::npos);
    CHECK_NE(json.find("\"multi-page-text\""), std::string::npos);
    CHECK_NE(json.find("\"cjk-text\""), std::string::npos);
    CHECK_NE(json.find("\"styled-text\""), std::string::npos);
    CHECK_NE(json.find("\"font-size-text\""), std::string::npos);
    CHECK_NE(json.find("\"color-text\""), std::string::npos);
    CHECK_NE(json.find("\"mixed-style-text\""), std::string::npos);
    CHECK_NE(json.find("\"contract-cjk-style\""), std::string::npos);
    CHECK_NE(json.find("\"document-contract-cjk-style\""), std::string::npos);
    CHECK_NE(json.find("\"three-page-text\""), std::string::npos);
    CHECK_NE(json.find("\"landscape-text\""), std::string::npos);
    CHECK_NE(json.find("\"title-body-text\""), std::string::npos);
    CHECK_NE(json.find("\"dense-text\""), std::string::npos);
    CHECK_NE(json.find("\"four-page-text\""), std::string::npos);
    CHECK_NE(json.find("\"underline-text\""), std::string::npos);
    CHECK_NE(json.find("\"punctuation-text\""), std::string::npos);
    CHECK_NE(json.find("\"two-page-text\""), std::string::npos);
    CHECK_NE(json.find("\"repeat-phrase-text\""), std::string::npos);
    CHECK_NE(json.find("\"bordered-box-text\""), std::string::npos);
    CHECK_NE(json.find("\"line-primitive-text\""), std::string::npos);
    CHECK_NE(json.find("\"table-like-grid-text\""), std::string::npos);
    CHECK_NE(json.find("\"metadata-long-title-text\""), std::string::npos);
    CHECK_NE(json.find("\"header-footer-text\""), std::string::npos);
    CHECK_NE(json.find("\"two-column-text\""), std::string::npos);
    CHECK_NE(json.find("\"invoice-grid-text\""), std::string::npos);
    CHECK_NE(json.find("\"image-caption-text\""), std::string::npos);
    CHECK_NE(json.find("\"sectioned-report-text\""), std::string::npos);
    CHECK_NE(json.find("\"list-report-text\""), std::string::npos);
    CHECK_NE(json.find("\"long-report-text\""), std::string::npos);
    CHECK_NE(json.find("\"image-report-text\""), std::string::npos);
    CHECK_NE(json.find("\"cjk-report-text\""), std::string::npos);
    CHECK_NE(json.find("\"cjk-image-report-text\""), std::string::npos);
    CHECK_NE(json.find("\"document-eastasia-style-probe\""), std::string::npos);
    CHECK_NE(json.find("\"document-image-semantics-text\""),
             std::string::npos);
    CHECK_NE(json.find("\"document-table-semantics-text\""),
             std::string::npos);
    CHECK_NE(json.find("\"document-long-flow-text\""), std::string::npos);
    CHECK_NE(json.find("\"document-invoice-table-text\""), std::string::npos);
    CHECK_NE(json.find("\"mixed-cjk-punctuation-text\""), std::string::npos);
    CHECK_NE(json.find("\"latin-ligature-text\""), std::string::npos);

    const auto samples = parse_samples_from_manifest(json);
    REQUIRE_EQ(samples.size(), 39U);
    CHECK_EQ(samples[0].id, "single-text");
    CHECK_EQ(samples[0].kind, "single_text");
    CHECK_EQ(samples[0].expected_pages, 1U);
    CHECK_GE(samples[0].expected_text.size(), 1U);
    CHECK_EQ(samples[1].id, "multi-page-text");
    CHECK_EQ(samples[1].expected_pages, 2U);
    CHECK_EQ(samples[2].id, "cjk-text");
    CHECK_EQ(samples[2].kind, "cjk_text");
    CHECK_EQ(samples[3].id, "styled-text");
    CHECK_EQ(samples[4].id, "font-size-text");
    CHECK_EQ(samples[4].kind, "font_size_text");
    CHECK_EQ(samples[5].id, "color-text");
    CHECK_EQ(samples[6].id, "mixed-style-text");
    CHECK_EQ(samples[7].id, "contract-cjk-style");
    CHECK_EQ(samples[7].kind, "contract_cjk_style");
    CHECK_EQ(samples[8].id, "document-contract-cjk-style");
    CHECK_EQ(samples[8].kind, "document_contract_cjk_style");
    CHECK_EQ(samples[9].id, "three-page-text");
    CHECK_EQ(samples[9].expected_pages, 3U);
    CHECK_EQ(samples[10].id, "landscape-text");
    CHECK_EQ(samples[11].id, "title-body-text");
    CHECK_EQ(samples[12].id, "dense-text");
    CHECK_EQ(samples[13].id, "four-page-text");
    CHECK_EQ(samples[13].expected_pages, 4U);
    CHECK_EQ(samples[14].id, "underline-text");
    CHECK_EQ(samples[15].id, "punctuation-text");
    CHECK_GE(samples[15].expected_text.size(), 3U);
    CHECK_EQ(samples[16].id, "two-page-text");
    CHECK_EQ(samples[16].expected_pages, 2U);
    CHECK_EQ(samples[17].id, "repeat-phrase-text");
    CHECK_EQ(samples[18].id, "bordered-box-text");
    CHECK_EQ(samples[18].kind, "bordered_box_text");
    CHECK_EQ(samples[19].id, "line-primitive-text");
    CHECK_EQ(samples[19].kind, "line_primitive_text");
    CHECK_EQ(samples[20].id, "table-like-grid-text");
    CHECK_EQ(samples[20].kind, "table_like_grid_text");
    CHECK_EQ(samples[21].id, "metadata-long-title-text");
    CHECK_EQ(samples[21].kind, "metadata_long_title_text");
    CHECK_EQ(samples[22].id, "header-footer-text");
    CHECK_EQ(samples[22].kind, "header_footer_text");
    CHECK_EQ(samples[23].id, "two-column-text");
    CHECK_EQ(samples[23].kind, "two_column_text");
    CHECK_EQ(samples[24].id, "invoice-grid-text");
    CHECK_EQ(samples[24].kind, "invoice_grid_text");
    CHECK_EQ(samples[25].id, "image-caption-text");
    CHECK_EQ(samples[25].kind, "image_caption_text");
    CHECK_EQ(samples[25].expected_image_count, 1U);
    CHECK_EQ(samples[26].id, "sectioned-report-text");
    CHECK_EQ(samples[26].kind, "sectioned_report_text");
    CHECK_EQ(samples[26].expected_pages, 2U);
    CHECK_GE(samples[26].expected_text.size(), 3U);
    CHECK_EQ(samples[27].id, "list-report-text");
    CHECK_EQ(samples[27].kind, "list_report_text");
    CHECK_EQ(samples[28].id, "long-report-text");
    CHECK_EQ(samples[28].kind, "long_report_text");
    CHECK_EQ(samples[29].id, "image-report-text");
    CHECK_EQ(samples[29].kind, "image_report_text");
    CHECK_EQ(samples[29].expected_image_count, 2U);
    CHECK_EQ(samples[29].expected_pages, 2U);
    CHECK_EQ(samples[30].id, "cjk-report-text");
    CHECK_EQ(samples[30].kind, "cjk_report_text");
    CHECK_EQ(samples[30].expected_pages, 2U);
    CHECK_EQ(samples[31].id, "cjk-image-report-text");
    CHECK_EQ(samples[31].kind, "cjk_image_report_text");
    CHECK_EQ(samples[31].expected_image_count, 2U);
    CHECK_EQ(samples[31].expected_pages, 2U);
    CHECK_EQ(samples[32].id, "document-eastasia-style-probe");
    CHECK_EQ(samples[32].kind, "document_eastasia_style_probe");
    CHECK_EQ(samples[32].expected_pages, 1U);
    CHECK_GE(samples[32].expected_text.size(), 7U);
    CHECK_EQ(samples[33].id, "document-image-semantics-text");
    CHECK_EQ(samples[33].kind, "document_image_semantics_text");
    CHECK_EQ(samples[33].expected_image_count, 3U);
    CHECK_EQ(samples[33].expected_pages, 1U);
    CHECK_GE(samples[33].expected_text.size(), 5U);
    CHECK_EQ(samples[34].id, "document-table-semantics-text");
    CHECK_EQ(samples[34].kind, "document_table_semantics_text");
    CHECK_EQ(samples[34].expected_pages, 2U);
    CHECK_GE(samples[34].expected_text.size(), 4U);
    CHECK_EQ(samples[35].id, "document-long-flow-text");
    CHECK_EQ(samples[35].kind, "document_long_flow_text");
    CHECK_EQ(samples[35].expected_pages, 5U);
    CHECK_GE(samples[35].expected_text.size(), 7U);
    CHECK_EQ(samples[36].id, "document-invoice-table-text");
    CHECK_EQ(samples[36].kind, "document_invoice_table_text");
    CHECK_EQ(samples[36].expected_pages, 1U);
    CHECK_GE(samples[36].expected_text.size(), 8U);
    CHECK_EQ(samples[37].id, "mixed-cjk-punctuation-text");
    CHECK_EQ(samples[37].kind, "mixed_cjk_punctuation_text");
    CHECK_EQ(samples[37].expected_pages, 1U);
    CHECK_GE(samples[37].expected_text.size(), 3U);
    CHECK_EQ(samples[38].id, "latin-ligature-text");
    CHECK_EQ(samples[38].kind, "latin_ligature_text");
    CHECK_EQ(samples[38].expected_pages, 1U);
    CHECK_GE(samples[38].expected_text.size(), 3U);
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
