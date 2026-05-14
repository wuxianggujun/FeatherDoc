#include <featherdoc/pdf/pdf_text_shaper.hpp>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>

#if defined(FEATHERDOC_ENABLE_PDF_TEXT_SHAPER)
#include <hb-ot.h>
#include <hb.h>
#endif

namespace featherdoc::pdf {
namespace {

#if defined(FEATHERDOC_ENABLE_PDF_TEXT_SHAPER)
struct HbBlobCloser {
    void operator()(hb_blob_t *blob) const noexcept {
        hb_blob_destroy(blob);
    }
};

struct HbBufferCloser {
    void operator()(hb_buffer_t *buffer) const noexcept {
        hb_buffer_destroy(buffer);
    }
};

struct HbFaceCloser {
    void operator()(hb_face_t *face) const noexcept {
        hb_face_destroy(face);
    }
};

struct HbFontCloser {
    void operator()(hb_font_t *font) const noexcept {
        hb_font_destroy(font);
    }
};

template <typename T, typename Closer> using HbPtr = std::unique_ptr<T, Closer>;

[[nodiscard]] double hb_position_to_points(hb_position_t value,
                                           double scale) noexcept {
    return static_cast<double>(value) * scale;
}

[[nodiscard]] PdfGlyphDirection
pdf_glyph_direction_from_harfbuzz(hb_direction_t direction) noexcept {
    switch (direction) {
    case HB_DIRECTION_LTR:
        return PdfGlyphDirection::left_to_right;
    case HB_DIRECTION_RTL:
        return PdfGlyphDirection::right_to_left;
    case HB_DIRECTION_TTB:
        return PdfGlyphDirection::top_to_bottom;
    case HB_DIRECTION_BTT:
        return PdfGlyphDirection::bottom_to_top;
    default:
        return PdfGlyphDirection::unknown;
    }
}

[[nodiscard]] std::string harfbuzz_tag_to_string(hb_tag_t tag) {
    char tag_chars[4]{};
    hb_tag_to_string(tag, tag_chars);

    std::string result{tag_chars, tag_chars + 4};
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

[[nodiscard]] std::string
pdf_glyph_script_tag_from_harfbuzz(hb_script_t script) {
    if (script == HB_SCRIPT_INVALID) {
        return {};
    }
    return harfbuzz_tag_to_string(hb_script_to_iso15924_tag(script));
}
#endif

[[nodiscard]] PdfGlyphRun make_base_run(std::string_view text,
                                        const PdfTextShaperOptions &options) {
    PdfGlyphRun run;
    run.text.assign(text.begin(), text.end());
    run.font_file_path = options.font_file_path;
    run.font_size_points = options.font_size_points;
    return run;
}

} // namespace

bool pdf_text_shaper_has_harfbuzz() noexcept {
#if defined(FEATHERDOC_ENABLE_PDF_TEXT_SHAPER)
    return true;
#else
    return false;
#endif
}

PdfGlyphRun shape_pdf_text(std::string_view text,
                           const PdfTextShaperOptions &options) {
    auto run = make_base_run(text, options);
    if (text.empty()) {
        return run;
    }
    if (options.font_size_points <= 0.0) {
        run.error_message = "Font size must be positive";
        return run;
    }
    if (options.font_file_path.empty()) {
        run.error_message = "Font path is empty";
        return run;
    }
    if (!std::filesystem::exists(options.font_file_path)) {
        run.error_message = "Font path does not exist: " +
                            options.font_file_path.string();
        return run;
    }
    if (text.size() >
        static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        run.error_message = "Text is too large for HarfBuzz shaping";
        return run;
    }

#if !defined(FEATHERDOC_ENABLE_PDF_TEXT_SHAPER)
    run.error_message = "PDF text shaping is not enabled";
    return run;
#else
    const auto font_path = options.font_file_path.string();
    HbPtr<hb_blob_t, HbBlobCloser> source_blob(
        hb_blob_create_from_file_or_fail(font_path.c_str()));
    if (!source_blob) {
        run.error_message = "Unable to load font for shaping: " +
                            options.font_file_path.string();
        return run;
    }

    HbPtr<hb_face_t, HbFaceCloser> face(
        hb_face_create_or_fail(source_blob.get(), 0));
    if (!face) {
        run.error_message = "Unable to create HarfBuzz face: " +
                            options.font_file_path.string();
        return run;
    }

    const auto units_per_em = hb_face_get_upem(face.get());
    if (units_per_em == 0U) {
        run.error_message = "HarfBuzz face has zero units per em: " +
                            options.font_file_path.string();
        return run;
    }

    HbPtr<hb_font_t, HbFontCloser> font(hb_font_create(face.get()));
    if (!font) {
        run.error_message = "Unable to create HarfBuzz font: " +
                            options.font_file_path.string();
        return run;
    }

    const auto signed_upem = static_cast<int>(std::min<std::uint32_t>(
        units_per_em, static_cast<std::uint32_t>(std::numeric_limits<int>::max())));
    hb_font_set_scale(font.get(), signed_upem, signed_upem);
    hb_ot_font_set_funcs(font.get());

    HbPtr<hb_buffer_t, HbBufferCloser> buffer(hb_buffer_create());
    if (!buffer) {
        run.error_message = "Unable to create HarfBuzz buffer";
        return run;
    }

    const auto text_size = static_cast<int>(text.size());
    hb_buffer_add_utf8(buffer.get(), text.data(), text_size, 0, text_size);
    hb_buffer_guess_segment_properties(buffer.get());
    run.direction =
        pdf_glyph_direction_from_harfbuzz(hb_buffer_get_direction(buffer.get()));
    run.script_tag =
        pdf_glyph_script_tag_from_harfbuzz(hb_buffer_get_script(buffer.get()));
    hb_shape(font.get(), buffer.get(), nullptr, 0U);

    unsigned int glyph_count = 0U;
    const hb_glyph_info_t *glyph_infos =
        hb_buffer_get_glyph_infos(buffer.get(), &glyph_count);
    const hb_glyph_position_t *glyph_positions =
        hb_buffer_get_glyph_positions(buffer.get(), &glyph_count);
    if (glyph_infos == nullptr || glyph_positions == nullptr ||
        glyph_count == 0U) {
        run.error_message = "HarfBuzz produced no glyphs";
        return run;
    }

    const double scale =
        options.font_size_points / static_cast<double>(signed_upem);
    run.glyphs.reserve(glyph_count);
    for (unsigned int index = 0U; index < glyph_count; ++index) {
        run.glyphs.push_back(PdfGlyphPosition{
            glyph_infos[index].codepoint,
            glyph_infos[index].cluster,
            hb_position_to_points(glyph_positions[index].x_advance, scale),
            hb_position_to_points(glyph_positions[index].y_advance, scale),
            hb_position_to_points(glyph_positions[index].x_offset, scale),
            hb_position_to_points(glyph_positions[index].y_offset, scale),
        });
    }

    run.used_harfbuzz = true;
    return run;
#endif
}

} // namespace featherdoc::pdf
