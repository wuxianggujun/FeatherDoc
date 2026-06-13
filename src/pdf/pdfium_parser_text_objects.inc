struct PdfiumDocumentCloser {
    void operator()(FPDF_DOCUMENT document) const {
        if (document != nullptr) {
            FPDF_CloseDocument(document);
        }
    }
};

struct PdfiumPageCloser {
    void operator()(FPDF_PAGE page) const {
        if (page != nullptr) {
            FPDF_ClosePage(page);
        }
    }
};

struct PdfiumTextPageCloser {
    void operator()(FPDF_TEXTPAGE text_page) const {
        if (text_page != nullptr) {
            FPDFText_ClosePage(text_page);
        }
    }
};

using PdfiumDocumentPtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_DOCUMENT>, PdfiumDocumentCloser>;
using PdfiumPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_PAGE>, PdfiumPageCloser>;
using PdfiumTextPagePtr =
    std::unique_ptr<std::remove_pointer_t<FPDF_TEXTPAGE>, PdfiumTextPageCloser>;

struct TextCluster {
    std::string text;
    PdfRect bounds;
};

struct CandidateLine {
    std::size_t line_index{0U};
    std::vector<TextCluster> clusters;
    double center_y{0.0};
};

struct PdfIndirectRef {
    unsigned int object_number{0U};
    unsigned int generation{0U};
};

struct PdfiumLibrary {
    PdfiumLibrary() {
        FPDF_InitLibrary();
    }

    ~PdfiumLibrary() {
        FPDF_DestroyLibrary();
    }

    PdfiumLibrary(const PdfiumLibrary &) = delete;
    PdfiumLibrary &operator=(const PdfiumLibrary &) = delete;
};

void ensure_pdfium_initialized() {
    static PdfiumLibrary library;
}

[[nodiscard]] std::string pdfium_error_to_string(unsigned long error_code) {
    switch (error_code) {
    case FPDF_ERR_SUCCESS:
        return "success";
    case FPDF_ERR_UNKNOWN:
        return "unknown error";
    case FPDF_ERR_FILE:
        return "file access or format error";
    case FPDF_ERR_FORMAT:
        return "invalid PDF format";
    case FPDF_ERR_PASSWORD:
        return "password is required or incorrect";
    case FPDF_ERR_SECURITY:
        return "unsupported security scheme";
    case FPDF_ERR_PAGE:
        return "page not found or content error";
    default:
        return "unrecognized PDFium error " + std::to_string(error_code);
    }
}

void append_utf8(std::string &output, std::uint32_t codepoint) {
    if (codepoint <= 0x7F) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        output.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

[[nodiscard]] PdfParsedTextSpan parse_text_span(FPDF_TEXTPAGE text_page,
                                                int char_index,
                                                const PdfParseOptions &options) {
    PdfParsedTextSpan span;
    append_utf8(span.text,
                static_cast<std::uint32_t>(
                    FPDFText_GetUnicode(text_page, char_index)));
    span.font_size_points = FPDFText_GetFontSize(text_page, char_index);

    if (options.extract_geometry) {
        double left = 0.0;
        double right = 0.0;
        double bottom = 0.0;
        double top = 0.0;
        if (FPDFText_GetCharBox(text_page, char_index, &left, &right, &bottom,
                                &top)) {
            span.bounds = PdfRect{left, bottom, right - left, top - bottom};
        }
    }

    return span;
}

void append_utf16_to_utf8(std::string &output,
                          const std::vector<FPDF_WCHAR> &text,
                          std::size_t char_count) {
    for (std::size_t index = 0; index < char_count; ++index) {
        const auto value = static_cast<std::uint32_t>(text[index]);
        if (value == 0U) {
            continue;
        }

        if (value >= 0xD800U && value <= 0xDBFFU &&
            index + 1U < char_count) {
            const auto low = static_cast<std::uint32_t>(text[index + 1U]);
            if (low >= 0xDC00U && low <= 0xDFFFU) {
                const auto codepoint =
                    0x10000U + ((value - 0xD800U) << 10U) +
                    (low - 0xDC00U);
                append_utf8(output, codepoint);
                ++index;
                continue;
            }
        }

        append_utf8(output, value);
    }
}

[[nodiscard]] std::size_t visible_text_byte_count(std::string_view text) {
    std::size_t count = 0U;
    for (const unsigned char ch : text) {
        if (!std::isspace(ch)) {
            ++count;
        }
    }

    return count;
}

[[nodiscard]] std::size_t
visible_text_byte_count(const std::vector<PdfParsedTextSpan> &spans) {
    std::size_t count = 0U;
    for (const auto &span : spans) {
        count += visible_text_byte_count(span.text);
    }

    return count;
}

[[nodiscard]] std::string read_pdf_bytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

[[nodiscard]] bool is_pdf_name_character(unsigned char ch) noexcept {
    return std::isalnum(ch) || ch == '#' || ch == '-' || ch == '_';
}

void skip_pdf_whitespace(std::string_view text, std::size_t &offset) {
    while (offset < text.size() &&
           std::isspace(static_cast<unsigned char>(text[offset]))) {
        ++offset;
    }
}

[[nodiscard]] std::optional<unsigned int>
parse_pdf_unsigned_integer(std::string_view text, std::size_t &offset) {
    skip_pdf_whitespace(text, offset);
    if (offset >= text.size() ||
        !std::isdigit(static_cast<unsigned char>(text[offset]))) {
        return std::nullopt;
    }

    unsigned int value = 0U;
    while (offset < text.size() &&
           std::isdigit(static_cast<unsigned char>(text[offset]))) {
        value = value * 10U +
                static_cast<unsigned int>(text[offset] - '0');
        ++offset;
    }
    return value;
}

[[nodiscard]] std::optional<PdfIndirectRef>
parse_pdf_indirect_ref(std::string_view text, std::size_t &offset) {
    const auto original_offset = offset;
    const auto object_number = parse_pdf_unsigned_integer(text, offset);
    if (!object_number.has_value()) {
        offset = original_offset;
        return std::nullopt;
    }
    const auto generation = parse_pdf_unsigned_integer(text, offset);
    if (!generation.has_value()) {
        offset = original_offset;
        return std::nullopt;
    }

    skip_pdf_whitespace(text, offset);
    if (offset >= text.size() || text[offset] != 'R') {
        offset = original_offset;
        return std::nullopt;
    }
    ++offset;

    return PdfIndirectRef{*object_number, *generation};
}

[[nodiscard]] std::vector<PdfIndirectRef>
extract_page_content_refs(std::string_view page_object) {
    std::vector<PdfIndirectRef> refs;
    const auto contents_pos = page_object.find("/Contents");
    if (contents_pos == std::string_view::npos) {
        return refs;
    }

    std::size_t offset = contents_pos + std::string_view{"/Contents"}.size();
    skip_pdf_whitespace(page_object, offset);
    if (offset >= page_object.size()) {
        return refs;
    }

    if (page_object[offset] != '[') {
        if (auto ref = parse_pdf_indirect_ref(page_object, offset);
            ref.has_value()) {
            refs.push_back(*ref);
        }
        return refs;
    }

    const auto array_end = page_object.find(']', offset + 1U);
    if (array_end == std::string_view::npos) {
        return refs;
    }

    ++offset;
    while (offset < array_end) {
        const auto ref_offset = offset;
        if (auto ref = parse_pdf_indirect_ref(page_object, offset);
            ref.has_value()) {
            refs.push_back(*ref);
            continue;
        }
        offset = ref_offset + 1U;
    }
    return refs;
}

[[nodiscard]] std::vector<std::vector<PdfIndirectRef>>
collect_page_content_refs(std::string_view pdf_bytes) {
    std::vector<std::vector<PdfIndirectRef>> pages;

    constexpr std::string_view kTypePage = "/Type/Page";
    std::size_t offset = 0U;
    while ((offset = pdf_bytes.find(kTypePage, offset)) !=
           std::string_view::npos) {
        const auto after_type = offset + kTypePage.size();
        if (after_type < pdf_bytes.size() &&
            is_pdf_name_character(
                static_cast<unsigned char>(pdf_bytes[after_type]))) {
            offset = after_type;
            continue;
        }

        const auto obj_keyword = pdf_bytes.rfind(" obj", offset);
        const auto end_obj = pdf_bytes.find("endobj", offset);
        if (obj_keyword == std::string_view::npos ||
            end_obj == std::string_view::npos) {
            offset = after_type;
            continue;
        }

        const auto object_start = pdf_bytes.rfind('\n', obj_keyword);
        const auto page_object_start =
            object_start == std::string_view::npos ? 0U : object_start + 1U;
        auto refs = extract_page_content_refs(
            pdf_bytes.substr(page_object_start, end_obj - page_object_start));
        if (!refs.empty()) {
            pages.push_back(std::move(refs));
        }
        offset = end_obj + std::string_view{"endobj"}.size();
    }

    return pages;
}

[[nodiscard]] std::optional<std::string_view>
find_pdf_stream_payload(std::string_view pdf_bytes, PdfIndirectRef ref) {
    const auto object_header = std::to_string(ref.object_number) + " " +
                               std::to_string(ref.generation) + " obj";
    std::size_t object_pos = std::string_view::npos;
    for (std::size_t search_offset = 0U;;) {
        const auto candidate = pdf_bytes.find(object_header, search_offset);
        if (candidate == std::string_view::npos) {
            break;
        }
        const bool starts_at_boundary =
            candidate == 0U ||
            std::isspace(
                static_cast<unsigned char>(pdf_bytes[candidate - 1U]));
        const auto after_header = candidate + object_header.size();
        const bool ends_at_boundary =
            after_header >= pdf_bytes.size() ||
            std::isspace(
                static_cast<unsigned char>(pdf_bytes[after_header]));
        if (starts_at_boundary && ends_at_boundary) {
            object_pos = candidate;
            break;
        }
        search_offset = candidate + 1U;
    }
    if (object_pos == std::string_view::npos) {
        return std::nullopt;
    }

    const auto end_obj = pdf_bytes.find("endobj", object_pos);
    const auto stream_pos = pdf_bytes.find("stream", object_pos);
    if (end_obj == std::string_view::npos ||
        stream_pos == std::string_view::npos || stream_pos > end_obj) {
        return std::nullopt;
    }

    auto data_start = stream_pos + std::string_view{"stream"}.size();
    if (data_start + 1U < pdf_bytes.size() && pdf_bytes[data_start] == '\r' &&
        pdf_bytes[data_start + 1U] == '\n') {
        data_start += 2U;
    } else if (data_start < pdf_bytes.size() && pdf_bytes[data_start] == '\n') {
        ++data_start;
    }

    auto data_end = pdf_bytes.find("endstream", data_start);
    if (data_end == std::string_view::npos || data_end > end_obj) {
        return std::nullopt;
    }
    while (data_end > data_start &&
           (pdf_bytes[data_end - 1U] == '\r' ||
            pdf_bytes[data_end - 1U] == '\n')) {
        --data_end;
    }

    return pdf_bytes.substr(data_start, data_end - data_start);
}

[[nodiscard]] std::optional<std::string>
inflate_pdf_stream(std::string_view stream_payload) {
    size_t output_size = 0U;
    void *output = tinfl_decompress_mem_to_heap(
        stream_payload.data(), stream_payload.size(), &output_size,
        TINFL_FLAG_PARSE_ZLIB_HEADER);
    if (output == nullptr) {
        return std::nullopt;
    }

    std::string inflated(static_cast<const char *>(output), output_size);
    mz_free(output);
    return inflated;
}

[[nodiscard]] std::optional<unsigned int>
parse_hex_byte(std::string_view hex, std::size_t offset) {
    if (offset + 1U >= hex.size()) {
        return std::nullopt;
    }

    auto parse_digit = [](char ch) -> std::optional<unsigned int> {
        if (ch >= '0' && ch <= '9') {
            return static_cast<unsigned int>(ch - '0');
        }
        if (ch >= 'A' && ch <= 'F') {
            return static_cast<unsigned int>(ch - 'A' + 10);
        }
        if (ch >= 'a' && ch <= 'f') {
            return static_cast<unsigned int>(ch - 'a' + 10);
        }
        return std::nullopt;
    };

    const auto high = parse_digit(hex[offset]);
    const auto low = parse_digit(hex[offset + 1U]);
    if (!high.has_value() || !low.has_value()) {
        return std::nullopt;
    }

    return (*high << 4U) | *low;
}

[[nodiscard]] std::optional<std::string>
decode_actual_text_hex(std::string_view hex) {
    if (hex.size() % 2U != 0U) {
        return std::nullopt;
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2U);
    for (std::size_t offset = 0U; offset < hex.size(); offset += 2U) {
        const auto byte = parse_hex_byte(hex, offset);
        if (!byte.has_value()) {
            return std::nullopt;
        }
        bytes.push_back(static_cast<unsigned char>(*byte));
    }

    bool little_endian = false;
    std::size_t byte_offset = 0U;
    if (bytes.size() >= 2U && bytes[0] == 0xFEU && bytes[1] == 0xFFU) {
        byte_offset = 2U;
    } else if (bytes.size() >= 2U && bytes[0] == 0xFFU && bytes[1] == 0xFEU) {
        little_endian = true;
        byte_offset = 2U;
    }

    if ((bytes.size() - byte_offset) % 2U != 0U) {
        return std::nullopt;
    }

    std::vector<FPDF_WCHAR> units;
    units.reserve((bytes.size() - byte_offset) / 2U);
    for (; byte_offset + 1U < bytes.size(); byte_offset += 2U) {
        const auto high = little_endian ? bytes[byte_offset + 1U]
                                        : bytes[byte_offset];
        const auto low = little_endian ? bytes[byte_offset]
                                       : bytes[byte_offset + 1U];
        units.push_back(
            static_cast<FPDF_WCHAR>((static_cast<unsigned int>(high) << 8U) |
                                    static_cast<unsigned int>(low)));
    }

    std::string output;
    append_utf16_to_utf8(output, units, units.size());
    return output;
}

[[nodiscard]] std::vector<std::string>
extract_actual_text_spans(std::string_view content_stream) {
    std::vector<std::string> spans;
    constexpr std::string_view kActualTextMarker = "/Span <</ActualText <";
    constexpr std::string_view kActualTextClose = ">>> BDC";

    std::size_t offset = 0U;
    while ((offset = content_stream.find(kActualTextMarker, offset)) !=
           std::string_view::npos) {
        const auto hex_start = offset + kActualTextMarker.size();
        const auto hex_end = content_stream.find('>', hex_start);
        if (hex_end == std::string_view::npos) {
            break;
        }

        if (content_stream.substr(hex_end, kActualTextClose.size()) ==
            kActualTextClose) {
            if (auto decoded = decode_actual_text_hex(
                    content_stream.substr(hex_start, hex_end - hex_start));
                decoded.has_value() && !decoded->empty()) {
                spans.push_back(std::move(*decoded));
            }
        }
        offset = hex_end + 1U;
    }

    return spans;
}

[[nodiscard]] std::vector<std::vector<std::string>>
extract_page_actual_text_spans(const std::filesystem::path &input_path) {
    const auto pdf_bytes = read_pdf_bytes(input_path);
    if (pdf_bytes.empty()) {
        return {};
    }

    std::vector<std::vector<std::string>> page_actual_texts;
    const auto page_refs = collect_page_content_refs(pdf_bytes);
    page_actual_texts.reserve(page_refs.size());
    for (const auto &refs : page_refs) {
        std::vector<std::string> actual_texts;
        for (const auto &ref : refs) {
            const auto payload = find_pdf_stream_payload(pdf_bytes, ref);
            if (!payload.has_value()) {
                continue;
            }
            const auto inflated = inflate_pdf_stream(*payload);
            if (!inflated.has_value()) {
                continue;
            }
            auto spans = extract_actual_text_spans(*inflated);
            actual_texts.insert(actual_texts.end(),
                                std::make_move_iterator(spans.begin()),
                                std::make_move_iterator(spans.end()));
        }
        page_actual_texts.push_back(std::move(actual_texts));
    }

    return page_actual_texts;
}

[[nodiscard]] std::string
collect_span_text(const std::vector<PdfParsedTextSpan> &spans) {
    std::string text;
    for (const auto &span : spans) {
        text += span.text;
    }
    return text;
}

[[nodiscard]] bool missing_actual_text_span(
    const std::vector<PdfParsedTextSpan> &spans,
    const std::vector<std::string> &actual_text_spans) {
    const auto text = collect_span_text(spans);
    return std::any_of(actual_text_spans.begin(), actual_text_spans.end(),
                       [&text](const std::string &actual_text) {
                           return visible_text_byte_count(actual_text) > 0U &&
                                  text.find(actual_text) == std::string::npos;
                       });
}

[[nodiscard]] std::size_t count_text_occurrences(std::string_view haystack,
                                                 std::string_view needle) {
    if (needle.empty()) {
        return 0U;
    }

    std::size_t count = 0U;
    std::size_t offset = 0U;
    while ((offset = haystack.find(needle, offset)) !=
           std::string_view::npos) {
        ++count;
        offset += needle.size();
    }
    return count;
}

[[nodiscard]] bool should_prefer_actual_text_spans(
    const std::vector<PdfParsedTextSpan> &spans,
    const std::vector<std::string> &actual_text_spans) {
    const auto text = collect_span_text(spans);
    return std::any_of(actual_text_spans.begin(), actual_text_spans.end(),
                       [&text](const std::string &actual_text) {
                           return visible_text_byte_count(actual_text) > 0U &&
                                  count_text_occurrences(text, actual_text) !=
                                      1U;
                       });
}

[[nodiscard]] std::vector<PdfParsedTextSpan> build_actual_text_spans(
    const std::vector<PdfParsedTextSpan> &object_spans,
    const std::vector<std::string> &actual_text_spans) {
    std::vector<PdfParsedTextSpan> spans;
    spans.reserve(actual_text_spans.size());
    for (std::size_t index = 0U; index < actual_text_spans.size(); ++index) {
        const auto &actual_text = actual_text_spans[index];
        if (visible_text_byte_count(actual_text) == 0U) {
            continue;
        }

        PdfParsedTextSpan span;
        span.text = actual_text;
        if (index < object_spans.size()) {
            span.bounds = object_spans[index].bounds;
            span.font_name = object_spans[index].font_name;
            span.font_size_points = object_spans[index].font_size_points;
        }
        spans.push_back(std::move(span));
    }

    if (spans.size() == 1U && !object_spans.empty()) {
        PdfRect bounds;
        for (const auto &object_span : object_spans) {
            if (bounds.width_points <= 0.0 && bounds.height_points <= 0.0) {
                bounds = object_span.bounds;
                continue;
            }

            const double left =
                (std::min)(bounds.x_points, object_span.bounds.x_points);
            const double bottom =
                (std::min)(bounds.y_points, object_span.bounds.y_points);
            const double right = (std::max)(
                bounds.x_points + bounds.width_points,
                object_span.bounds.x_points + object_span.bounds.width_points);
            const double top = (std::max)(
                bounds.y_points + bounds.height_points,
                object_span.bounds.y_points + object_span.bounds.height_points);
            bounds = PdfRect{left, bottom, right - left, top - bottom};
        }
        spans.front().bounds = bounds;
    }

    return spans;
}

[[nodiscard]] bool should_prefer_text_object_spans(
    const std::vector<PdfParsedTextSpan> &char_spans,
    const std::vector<PdfParsedTextSpan> &object_spans) {
    const auto object_visible = visible_text_byte_count(object_spans);
    if (object_visible == 0U) {
        return false;
    }

    const auto char_visible = visible_text_byte_count(char_spans);
    if (char_visible == 0U) {
        return true;
    }

    return object_visible >= char_visible + 8U &&
           object_visible >= char_visible * 2U;
}

[[nodiscard]] std::vector<std::string> split_utf8_characters(
    std::string_view text) {
    std::vector<std::string> characters;
    characters.reserve(text.size());

    for (std::size_t index = 0U; index < text.size();) {
        const auto lead = static_cast<unsigned char>(text[index]);
        std::size_t length = 1U;
        if ((lead & 0xE0U) == 0xC0U) {
            length = 2U;
        } else if ((lead & 0xF0U) == 0xE0U) {
            length = 3U;
        } else if ((lead & 0xF8U) == 0xF0U) {
            length = 4U;
        }
        if (index + length > text.size()) {
            length = 1U;
        }

        characters.emplace_back(text.substr(index, length));
        index += length;
    }

    return characters;
}

[[nodiscard]] std::string read_text_object_text(FPDF_PAGEOBJECT text_object,
                                                FPDF_TEXTPAGE text_page) {
    const unsigned long byte_count =
        FPDFTextObj_GetText(text_object, text_page, nullptr, 0);
    if (byte_count <= sizeof(FPDF_WCHAR)) {
        return {};
    }

    const auto char_capacity =
        static_cast<std::size_t>((byte_count + sizeof(FPDF_WCHAR) - 1U) /
                                 sizeof(FPDF_WCHAR));
    std::vector<FPDF_WCHAR> buffer(char_capacity, 0);
    const unsigned long copied =
        FPDFTextObj_GetText(text_object, text_page, buffer.data(), byte_count);
    if (copied <= sizeof(FPDF_WCHAR)) {
        return {};
    }

    const auto copied_chars =
        static_cast<std::size_t>(copied / sizeof(FPDF_WCHAR));
    std::string output;
    output.reserve(copied_chars);
    append_utf16_to_utf8(output, buffer, copied_chars);
    return output;
}

[[nodiscard]] std::string
read_mark_string_value(FPDF_PAGEOBJECTMARK mark, FPDF_BYTESTRING key) {
    unsigned long byte_count = 0;
    if (!FPDFPageObjMark_GetParamStringValue(mark, key, nullptr, 0,
                                             &byte_count) ||
        byte_count <= sizeof(FPDF_WCHAR)) {
        return {};
    }

    const auto char_capacity =
        static_cast<std::size_t>((byte_count + sizeof(FPDF_WCHAR) - 1U) /
                                 sizeof(FPDF_WCHAR));
    std::vector<FPDF_WCHAR> buffer(char_capacity, 0);
    unsigned long copied = 0;
    if (!FPDFPageObjMark_GetParamStringValue(mark, key, buffer.data(),
                                             byte_count, &copied) ||
        copied <= sizeof(FPDF_WCHAR)) {
        return {};
    }

    const auto copied_chars =
        static_cast<std::size_t>(copied / sizeof(FPDF_WCHAR));
    std::string output;
    output.reserve(copied_chars);
    append_utf16_to_utf8(output, buffer, copied_chars);
    return output;
}

[[nodiscard]] std::string read_text_object_actual_text(
    FPDF_PAGEOBJECT text_object) {
    const int mark_count = FPDFPageObj_CountMarks(text_object);
    for (int mark_index = 0; mark_index < mark_count; ++mark_index) {
        auto *mark = FPDFPageObj_GetMark(
            text_object, static_cast<unsigned long>(mark_index));
        if (mark == nullptr) {
            continue;
        }

        auto actual_text = read_mark_string_value(mark, "ActualText");
        if (!actual_text.empty()) {
            return actual_text;
        }
    }

    return {};
}

void append_text_object_span(FPDF_PAGEOBJECT text_object,
                             FPDF_TEXTPAGE text_page,
                             PdfParsedPage &page,
                             const PdfParseOptions &options,
                             bool split_multichar_text) {
    auto text = read_text_object_text(text_object, text_page);
    auto actual_text = read_text_object_actual_text(text_object);
    if (visible_text_byte_count(text) == 0U &&
        visible_text_byte_count(actual_text) > 0U) {
        text = std::move(actual_text);
    }
    if (text.empty()) {
        return;
    }

    PdfParsedTextSpan span;
    span.text = std::move(text);

    float font_size = 0.0F;
    if (FPDFTextObj_GetFontSize(text_object, &font_size)) {
        span.font_size_points = static_cast<double>(font_size);
    }

    if (options.extract_geometry) {
        float left = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;
        float top = 0.0F;
        if (FPDFPageObj_GetBounds(text_object, &left, &bottom, &right,
                                  &top)) {
            span.bounds =
                PdfRect{left, bottom, right - left, top - bottom};
        }
    }

    if (!split_multichar_text || !options.extract_geometry ||
        span.bounds.width_points <= 0.0 || span.text.size() <= 1U) {
        page.text_spans.push_back(std::move(span));
        return;
    }

    const auto characters = split_utf8_characters(span.text);
    if (characters.size() <= 1U) {
        page.text_spans.push_back(std::move(span));
        return;
    }

    const double character_width =
        span.bounds.width_points / static_cast<double>(characters.size());
    for (std::size_t index = 0U; index < characters.size(); ++index) {
        PdfParsedTextSpan character_span;
        character_span.text = characters[index];
        character_span.font_size_points = span.font_size_points;
        character_span.bounds =
            PdfRect{span.bounds.x_points +
                        character_width * static_cast<double>(index),
                    span.bounds.y_points, character_width,
                    span.bounds.height_points};
        page.text_spans.push_back(std::move(character_span));
    }
}

void append_text_object_spans(FPDF_PAGEOBJECT page_object,
                              FPDF_TEXTPAGE text_page,
                              PdfParsedPage &page,
                              const PdfParseOptions &options,
                              bool split_multichar_text,
                              int depth = 0) {
    if (page_object == nullptr || depth > 8) {
        return;
    }

    const int object_type = FPDFPageObj_GetType(page_object);
    if (object_type == FPDF_PAGEOBJ_TEXT) {
        append_text_object_span(page_object, text_page, page, options,
                                split_multichar_text);
        return;
    }

    if (object_type != FPDF_PAGEOBJ_FORM) {
        return;
    }

    const int child_count = FPDFFormObj_CountObjects(page_object);
    for (int child_index = 0; child_index < child_count; ++child_index) {
        append_text_object_spans(
            FPDFFormObj_GetObject(
                page_object, static_cast<unsigned long>(child_index)),
            text_page, page, options, split_multichar_text, depth + 1);
    }
}

void append_page_object_text_spans(FPDF_PAGE page,
                                   FPDF_TEXTPAGE text_page,
                                   PdfParsedPage &parsed_page,
                                   const PdfParseOptions &options,
                                   bool split_multichar_text) {
    const int object_count = FPDFPage_CountObjects(page);
    for (int object_index = 0; object_index < object_count; ++object_index) {
        append_text_object_spans(FPDFPage_GetObject(page, object_index),
                                 text_page, parsed_page, options,
                                 split_multichar_text);
    }
}
