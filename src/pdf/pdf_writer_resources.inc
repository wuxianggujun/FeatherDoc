constexpr double kPi = 3.14159265358979323846;
constexpr double kGlyphPositionTolerance = 0.0001;

struct PdfioErrorState {
    std::string message;
};

bool pdfio_error_callback(pdfio_file_t *, const char *message, void *data) {
    auto *state = static_cast<PdfioErrorState *>(data);
    if (state != nullptr && message != nullptr) {
        state->message = message;
    }
    return true;
}

struct PdfioFileCloser {
    void operator()(pdfio_file_t *pdf) const {
        if (pdf != nullptr) {
            pdfioFileClose(pdf);
        }
    }
};

struct PdfioStreamCloser {
    void operator()(pdfio_stream_t *stream) const {
        if (stream != nullptr) {
            pdfioStreamClose(stream);
        }
    }
};

struct FontResourceKey {
    std::string family;
    std::filesystem::path file_path;
    bool bold{false};
    bool italic{false};
    bool unicode{false};
    bool shaped_glyphs{false};

    [[nodiscard]] friend bool operator<(const FontResourceKey &left,
                                        const FontResourceKey &right) {
        return std::tie(left.family, left.file_path, left.bold, left.italic,
                        left.unicode, left.shaped_glyphs) <
               std::tie(right.family, right.file_path, right.bold,
                        right.italic, right.unicode, right.shaped_glyphs);
    }
};

struct ImageResourceKey {
    std::filesystem::path file_path;
    bool interpolate{true};

    [[nodiscard]] friend bool operator<(const ImageResourceKey &left,
                                        const ImageResourceKey &right) {
        return std::tie(left.file_path, left.interpolate) <
               std::tie(right.file_path, right.interpolate);
    }
};

struct ImageSourceCleanup {
    std::vector<std::filesystem::path> paths;

    ~ImageSourceCleanup() {
        for (const auto &path : paths) {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    }
};

struct FontSubsetStorage {
    std::vector<std::vector<unsigned char>> buffers;
};

struct GlyphCidEntry {
    std::uint16_t cid{0U};
    std::uint16_t glyph_id{0U};
    double width_1000{0.0};
    std::string unicode_text;
};

struct GlyphFontResource {
    std::vector<GlyphCidEntry> entries;
    std::vector<std::uint32_t> glyph_ids;
};

using GlyphFontResources = std::map<FontResourceKey, GlyphFontResource>;

struct FreeTypeFaceOwner {
    FT_Library library{nullptr};
    FT_Face face{nullptr};

    explicit FreeTypeFaceOwner(const std::filesystem::path &font_file_path) {
        if (FT_Init_FreeType(&library) != 0) {
            library = nullptr;
            return;
        }

        const auto font_path = font_file_path.string();
        if (FT_New_Face(library, font_path.c_str(), 0, &face) != 0) {
            face = nullptr;
        }
    }

    ~FreeTypeFaceOwner() {
        if (face != nullptr) {
            FT_Done_Face(face);
        }
        if (library != nullptr) {
            FT_Done_FreeType(library);
        }
    }

    FreeTypeFaceOwner(const FreeTypeFaceOwner &) = delete;
    FreeTypeFaceOwner &operator=(const FreeTypeFaceOwner &) = delete;

    [[nodiscard]] bool valid() const noexcept {
        return library != nullptr && face != nullptr;
    }
};

[[nodiscard]] std::string build_error(std::string fallback,
                                      const PdfioErrorState &state) {
    if (!state.message.empty()) {
        fallback += ": ";
        fallback += state.message;
    }
    return fallback;
}

[[nodiscard]] bool is_utf8_codepoint_boundary(std::string_view text,
                                              std::size_t index) noexcept {
    if (index == 0U || index == text.size()) {
        return true;
    }
    if (index > text.size()) {
        return false;
    }

    const auto byte = static_cast<unsigned char>(text[index]);
    return (byte & 0xC0U) != 0x80U;
}

[[nodiscard]] bool
can_write_shaped_glyph_run(const PdfTextRun &text_run) noexcept {
    if (text_run.text.empty() || text_run.font_file_path.empty() ||
        text_run.font_size_points <= 0.0 ||
        !std::isfinite(text_run.font_size_points) ||
        !text_run.glyph_run.used_harfbuzz ||
        !text_run.glyph_run.error_message.empty() ||
        text_run.glyph_run.glyphs.empty() ||
        text_run.glyph_run.direction != PdfGlyphDirection::left_to_right ||
        text_run.glyph_run.text != text_run.text ||
        text_run.glyph_run.font_file_path != text_run.font_file_path ||
        std::abs(text_run.glyph_run.font_size_points -
                 text_run.font_size_points) > kGlyphPositionTolerance) {
        return false;
    }

    std::uint32_t previous_cluster = 0U;
    bool has_previous_cluster = false;
    for (const auto &glyph : text_run.glyph_run.glyphs) {
        if (glyph.glyph_id == 0U ||
            glyph.glyph_id > std::numeric_limits<std::uint16_t>::max() ||
            glyph.cluster >= text_run.glyph_run.text.size() ||
            !is_utf8_codepoint_boundary(text_run.glyph_run.text,
                                        glyph.cluster) ||
            (has_previous_cluster && glyph.cluster < previous_cluster) ||
            !std::isfinite(glyph.x_advance_points) ||
            !std::isfinite(glyph.y_advance_points) ||
            !std::isfinite(glyph.x_offset_points) ||
            !std::isfinite(glyph.y_offset_points)) {
            return false;
        }
        previous_cluster = glyph.cluster;
        has_previous_cluster = true;
    }

    return true;
}

[[nodiscard]] bool
needs_positioned_shaped_glyph_show(const PdfTextRun &text_run) noexcept {
    for (const auto &glyph : text_run.glyph_run.glyphs) {
        if (std::abs(glyph.x_offset_points) > kGlyphPositionTolerance ||
            std::abs(glyph.y_offset_points) > kGlyphPositionTolerance ||
            std::abs(glyph.y_advance_points) > kGlyphPositionTolerance) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] FontResourceKey make_font_key(const PdfTextRun &text_run) {
    return FontResourceKey{
        text_run.font_family.empty() ? "Helvetica" : text_run.font_family,
        text_run.font_file_path,
        text_run.bold,
        text_run.italic,
        text_run.unicode,
        can_write_shaped_glyph_run(text_run),
    };
}

[[nodiscard]] std::string base_font_family_name(const FontResourceKey &key) {
    if (key.family == "Helvetica") {
        if (key.bold && key.italic) {
            return "Helvetica-BoldOblique";
        }
        if (key.bold) {
            return "Helvetica-Bold";
        }
        if (key.italic) {
            return "Helvetica-Oblique";
        }
    } else if (key.family == "Times" || key.family == "Times-Roman") {
        if (key.bold && key.italic) {
            return "Times-BoldItalic";
        }
        if (key.bold) {
            return "Times-Bold";
        }
        if (key.italic) {
            return "Times-Italic";
        }
        return "Times-Roman";
    } else if (key.family == "Courier") {
        if (key.bold && key.italic) {
            return "Courier-BoldOblique";
        }
        if (key.bold) {
            return "Courier-Bold";
        }
        if (key.italic) {
            return "Courier-Oblique";
        }
    }

    return key.family;
}

[[nodiscard]] std::string font_label(const FontResourceKey &key) {
    const std::string suffix = key.shaped_glyphs ? " shaped glyphs" : "";
    if (!key.file_path.empty()) {
        return key.file_path.string() + suffix;
    }
    return base_font_family_name(key) + suffix;
}

[[nodiscard]] std::string image_label(const ImageResourceKey &key) {
    return key.file_path.string();
}

[[nodiscard]] std::uint32_t decode_utf8_codepoint(std::string_view text,
                                                  std::size_t &index) noexcept;

[[nodiscard]] pdfio_obj_t *
create_pdfio_font(pdfio_file_t *pdf, const FontResourceKey &key,
                  const std::string &used_text, bool subset_unicode_fonts,
                  FontSubsetStorage &subset_storage) {
    if (!key.file_path.empty()) {
#if defined(FEATHERDOC_ENABLE_PDF_FONT_SUBSET)
        if (subset_unicode_fonts && key.unicode && !used_text.empty()) {
            std::set<std::uint32_t> unique_codepoints;
            for (std::size_t index = 0U; index < used_text.size();) {
                unique_codepoints.insert(
                    decode_utf8_codepoint(used_text, index));
            }
            const auto subset_result = subset_font_file_for_codepoints(
                key.file_path,
                std::vector<std::uint32_t>(unique_codepoints.begin(),
                                           unique_codepoints.end()));
            if (subset_result.success && !subset_result.font_data.empty()) {
                subset_storage.buffers.push_back(
                    std::move(subset_result.font_data));
                const auto &font_data = subset_storage.buffers.back();
                pdfio_obj_t *font = pdfioFileCreateFontObjFromData(
                    pdf, font_data.data(), font_data.size(), key.unicode);
                if (font != nullptr) {
                    return font;
                }
                subset_storage.buffers.pop_back();
            }
        }
#else
        (void)used_text;
        (void)subset_unicode_fonts;
        (void)subset_storage;
#endif
        const auto font_path = key.file_path.string();
        return pdfioFileCreateFontObjFromFile(pdf, font_path.c_str(),
                                              key.unicode);
    }

    const auto family = base_font_family_name(key);
    return pdfioFileCreateFontObjFromBase(pdf, family.c_str());
}

[[nodiscard]] const char *make_pdfio_resource_name(pdfio_file_t *pdf,
                                                   const std::string &name,
                                                   std::string &error_message) {
    const auto *resource_name = pdfioStringCreate(pdf, name.c_str());
    if (resource_name == nullptr) {
        error_message = "Unable to allocate PDF resource name: " + name;
    }
    return resource_name;
}

[[nodiscard]] pdfio_linecap_t to_pdfio_line_cap(PdfLineCap line_cap) noexcept {
    switch (line_cap) {
    case PdfLineCap::butt:
        return PDFIO_LINECAP_BUTT;
    case PdfLineCap::round:
        return PDFIO_LINECAP_ROUND;
    case PdfLineCap::square:
        return PDFIO_LINECAP_SQUARE;
    }

    return PDFIO_LINECAP_BUTT;
}

void append_utf16be_hex_unit(std::string &output, std::uint16_t value) {
    constexpr char digits[] = "0123456789ABCDEF";
    output.push_back(digits[(value >> 12U) & 0x0FU]);
    output.push_back(digits[(value >> 8U) & 0x0FU]);
    output.push_back(digits[(value >> 4U) & 0x0FU]);
    output.push_back(digits[value & 0x0FU]);
}

[[nodiscard]] std::uint32_t decode_utf8_codepoint(std::string_view text,
                                                  std::size_t &index) noexcept {
    if (index >= text.size()) {
        return 0xFFFDU;
    }

    const auto first = static_cast<unsigned char>(text[index++]);
    if (first < 0x80U) {
        return first;
    }

    auto continuation = [&](std::uint32_t &value) noexcept {
        if (index >= text.size()) {
            return false;
        }
        const auto next = static_cast<unsigned char>(text[index]);
        if ((next & 0xC0U) != 0x80U) {
            return false;
        }
        ++index;
        value = (value << 6U) | (next & 0x3FU);
        return true;
    };

    if ((first & 0xE0U) == 0xC0U) {
        std::uint32_t value = first & 0x1FU;
        return continuation(value) ? value : 0xFFFDU;
    }
    if ((first & 0xF0U) == 0xE0U) {
        std::uint32_t value = first & 0x0FU;
        return continuation(value) && continuation(value) ? value : 0xFFFDU;
    }
    if ((first & 0xF8U) == 0xF0U) {
        std::uint32_t value = first & 0x07U;
        return continuation(value) && continuation(value) && continuation(value)
                   ? value
                   : 0xFFFDU;
    }

    return 0xFFFDU;
}

[[nodiscard]] std::string utf16be_hex_from_utf8(std::string_view text) {
    std::string hex{"FEFF"};
    for (std::size_t index = 0U; index < text.size();) {
        auto codepoint = decode_utf8_codepoint(text, index);
        if (codepoint > 0x10FFFFU ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
            codepoint = 0xFFFDU;
        }

        if (codepoint <= 0xFFFFU) {
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(codepoint));
        } else {
            codepoint -= 0x10000U;
            append_utf16be_hex_unit(
                hex, static_cast<std::uint16_t>(0xD800U +
                                                ((codepoint >> 10U) & 0x3FFU)));
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(
                                             0xDC00U + (codepoint & 0x3FFU)));
        }
    }
    return hex;
}

[[nodiscard]] std::string utf16be_hex_payload_from_utf8(std::string_view text) {
    std::string hex;
    for (std::size_t index = 0U; index < text.size();) {
        auto codepoint = decode_utf8_codepoint(text, index);
        if (codepoint > 0x10FFFFU ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
            codepoint = 0xFFFDU;
        }

        if (codepoint <= 0xFFFFU) {
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(codepoint));
        } else {
            codepoint -= 0x10000U;
            append_utf16be_hex_unit(
                hex, static_cast<std::uint16_t>(0xD800U +
                                                ((codepoint >> 10U) & 0x3FFU)));
            append_utf16be_hex_unit(hex, static_cast<std::uint16_t>(
                                             0xDC00U + (codepoint & 0x3FFU)));
        }
    }
    return hex;
}

[[nodiscard]] std::vector<unsigned char>
read_binary_file(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

[[nodiscard]] double font_unit_to_1000(FT_Face face, FT_Pos value) noexcept {
    if (face == nullptr || face->units_per_EM <= 0) {
        return 0.0;
    }

    return static_cast<double>(value) * 1000.0 /
           static_cast<double>(face->units_per_EM);
}

[[nodiscard]] std::string sanitized_pdf_font_name(std::string_view value) {
    std::string name;
    name.reserve(value.size());
    for (const auto byte : value) {
        const auto ch = static_cast<unsigned char>(byte);
        if (std::isalnum(ch) != 0 || ch == '-' || ch == '_') {
            name.push_back(static_cast<char>(ch));
        } else if (!name.empty() && name.back() != '-') {
            name.push_back('-');
        }
    }

    while (!name.empty() && name.back() == '-') {
        name.pop_back();
    }
    if (name.empty()) {
        name = "Font";
    }
    return "FeatherDocGlyph-" + name;
}

[[nodiscard]] std::string base_font_name_for(FT_Face face,
                                             const FontResourceKey &key) {
    if (face != nullptr) {
        if (const char *postscript_name = FT_Get_Postscript_Name(face);
            postscript_name != nullptr && postscript_name[0] != '\0') {
            return sanitized_pdf_font_name(postscript_name);
        }
        if (face->family_name != nullptr && face->family_name[0] != '\0') {
            return sanitized_pdf_font_name(face->family_name);
        }
    }

    return sanitized_pdf_font_name(base_font_family_name(key));
}

[[nodiscard]] std::string glyph_cluster_text(const PdfGlyphRun &glyph_run,
                                             std::size_t glyph_index) {
    if (glyph_index >= glyph_run.glyphs.size()) {
        return {};
    }

    const auto cluster = glyph_run.glyphs[glyph_index].cluster;
    if (cluster >= glyph_run.text.size()) {
        return {};
    }

    for (std::size_t index = 0U; index < glyph_index; ++index) {
        if (glyph_run.glyphs[index].cluster == cluster) {
            return {};
        }
    }

    auto next_cluster = static_cast<std::uint32_t>(glyph_run.text.size());
    for (const auto &glyph : glyph_run.glyphs) {
        if (glyph.cluster > cluster && glyph.cluster < next_cluster) {
            next_cluster = glyph.cluster;
        }
    }

    if (next_cluster <= cluster || next_cluster > glyph_run.text.size()) {
        return {};
    }
    return glyph_run.text.substr(cluster, next_cluster - cluster);
}

[[nodiscard]] bool collect_shaped_glyph_font_resources(
    const PdfPageLayout &page, GlyphFontResources &glyph_resources,
    std::string &error_message) {
    constexpr auto kMaxCid = std::numeric_limits<std::uint16_t>::max();
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        if (!font_key.shaped_glyphs) {
            continue;
        }

        auto &resource = glyph_resources[font_key];
        for (std::size_t index = 0U; index < text_run.glyph_run.glyphs.size();
             ++index) {
            if (resource.entries.size() >= kMaxCid) {
                error_message =
                    "Too many shaped PDF glyphs for one font resource: " +
                    font_label(font_key);
                return false;
            }

            const auto &glyph = text_run.glyph_run.glyphs[index];
            const double width_1000 =
                std::isfinite(glyph.x_advance_points)
                    ? std::max(0.0, glyph.x_advance_points * 1000.0 /
                                        text_run.font_size_points)
                    : 0.0;
            resource.entries.push_back(GlyphCidEntry{
                static_cast<std::uint16_t>(resource.entries.size() + 1U),
                static_cast<std::uint16_t>(glyph.glyph_id),
                width_1000,
                glyph_cluster_text(text_run.glyph_run, index),
            });
            resource.glyph_ids.push_back(
                static_cast<std::uint32_t>(glyph.glyph_id));
        }
    }

    return true;
}
