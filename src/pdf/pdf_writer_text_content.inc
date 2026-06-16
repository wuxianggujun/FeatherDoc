[[nodiscard]] bool write_actual_text_begin(pdfio_stream_t *stream,
                                           std::string_view text,
                                           std::string &error_message) {
    const auto hex = utf16be_hex_from_utf8(text);
    const auto command =
        std::string{"/Span <</ActualText <"} + hex + ">>> BDC\n";
    if (!pdfioStreamPuts(stream, command.c_str())) {
        error_message = "Unable to write PDF ActualText marker";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_actual_text_end(pdfio_stream_t *stream,
                                         std::string &error_message) {
    if (!pdfioStreamPuts(stream, "EMC\n")) {
        error_message = "Unable to close PDF ActualText marker";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_text_underline(pdfio_stream_t *stream,
                                        const PdfTextRun &text_run,
                                        std::string &error_message) {
    if (!text_run.underline || text_run.text.empty() ||
        text_run.font_size_points <= 0.0 ||
        std::abs(text_run.rotation_degrees) > 0.0001) {
        return true;
    }

    const auto underline_width = measure_text_width_points(
        text_run.text, text_run.font_size_points,
        PdfTextMetricsOptions{text_run.font_family, text_run.font_file_path});
    if (underline_width <= 0.0) {
        return true;
    }

    const double underline_y =
        text_run.baseline_origin.y_points - text_run.font_size_points * 0.12;
    const double line_width = std::max(0.5, text_run.font_size_points / 16.0);

    if (!pdfioContentSetStrokeColorDeviceRGB(stream, text_run.fill_color.red,
                                             text_run.fill_color.green,
                                             text_run.fill_color.blue) ||
        !pdfioContentSetLineWidth(stream, line_width) ||
        !pdfioContentSetLineCap(stream, PDFIO_LINECAP_BUTT) ||
        !pdfioContentSetDashPattern(stream, 0.0, 0.0, 0.0) ||
        !pdfioContentPathMoveTo(stream, text_run.baseline_origin.x_points,
                                underline_y) ||
        !pdfioContentPathLineTo(
            stream, text_run.baseline_origin.x_points + underline_width,
            underline_y) ||
        !pdfioContentStroke(stream)) {
        error_message = "Unable to write PDF text underline";
        return false;
    }

    return true;
}

[[nodiscard]] bool write_text_strikethrough(pdfio_stream_t *stream,
                                            const PdfTextRun &text_run,
                                            std::string &error_message) {
    if (!text_run.strikethrough || text_run.text.empty() ||
        text_run.font_size_points <= 0.0 ||
        std::abs(text_run.rotation_degrees) > 0.0001) {
        return true;
    }

    const auto strikethrough_width = measure_text_width_points(
        text_run.text, text_run.font_size_points,
        PdfTextMetricsOptions{text_run.font_family, text_run.font_file_path});
    if (strikethrough_width <= 0.0) {
        return true;
    }

    const double strikethrough_y =
        text_run.baseline_origin.y_points + text_run.font_size_points * 0.30;
    const double line_width = std::max(0.5, text_run.font_size_points / 16.0);

    if (!pdfioContentSetStrokeColorDeviceRGB(stream, text_run.fill_color.red,
                                             text_run.fill_color.green,
                                             text_run.fill_color.blue) ||
        !pdfioContentSetLineWidth(stream, line_width) ||
        !pdfioContentSetLineCap(stream, PDFIO_LINECAP_BUTT) ||
        !pdfioContentSetDashPattern(stream, 0.0, 0.0, 0.0) ||
        !pdfioContentPathMoveTo(stream, text_run.baseline_origin.x_points,
                                strikethrough_y) ||
        !pdfioContentPathLineTo(
            stream,
            text_run.baseline_origin.x_points + strikethrough_width,
            strikethrough_y) ||
        !pdfioContentStroke(stream)) {
        error_message = "Unable to write PDF text strikethrough";
        return false;
    }

    return true;
}

struct TextRunTransform {
    double a{1.0};
    double b{0.0};
    double c{0.0};
    double d{1.0};
};

[[nodiscard]] TextRunTransform text_run_transform(
    const PdfTextRun &text_run) noexcept {
    if (std::abs(text_run.rotation_degrees) <= kGlyphPositionTolerance) {
        if (text_run.synthetic_italic && text_run.italic) {
            constexpr double kSyntheticItalicDegrees = 12.0;
            return TextRunTransform{
                1.0, 0.0,
                std::tan(kSyntheticItalicDegrees * kPi / 180.0), 1.0};
        }
        return {};
    }

    const auto radians = text_run.rotation_degrees * kPi / 180.0;
    const auto cosine = std::cos(radians);
    const auto sine = std::sin(radians);
    return TextRunTransform{cosine, sine, -sine, cosine};
}

[[nodiscard]] bool set_text_matrix_at(pdfio_stream_t *stream,
                                      const PdfTextRun &text_run,
                                      double local_x,
                                      double local_y) {
    const auto transform = text_run_transform(text_run);
    pdfio_matrix_t matrix = {
        {transform.a, transform.b},
        {transform.c, transform.d},
        {text_run.baseline_origin.x_points + transform.a * local_x +
             transform.c * local_y,
         text_run.baseline_origin.y_points + transform.b * local_x +
             transform.d * local_y},
    };
    return pdfioContentSetTextMatrix(stream, matrix);
}

[[nodiscard]] bool set_text_run_matrix(pdfio_stream_t *stream,
                                       const PdfTextRun &text_run) {
    if (std::abs(text_run.rotation_degrees) <= kGlyphPositionTolerance) {
        if (text_run.synthetic_italic && text_run.italic) {
            return set_text_matrix_at(stream, text_run, 0.0, 0.0);
        }
        return pdfioContentTextMoveTo(stream,
                                      text_run.baseline_origin.x_points,
                                      text_run.baseline_origin.y_points);
    }

    return set_text_matrix_at(stream, text_run, 0.0, 0.0);
}

[[nodiscard]] bool set_text_rendering_style(pdfio_stream_t *stream,
                                            const PdfTextRun &text_run) {
    const bool synthetic_bold = text_run.synthetic_bold && text_run.bold;
    if (!pdfioContentSetTextRenderingMode(
            stream, synthetic_bold ? PDFIO_TEXTRENDERING_FILL_AND_STROKE
                                   : PDFIO_TEXTRENDERING_FILL)) {
        return false;
    }

    if (!synthetic_bold) {
        return true;
    }

    const double stroke_width =
        std::max(0.2, text_run.font_size_points / 48.0);
    return pdfioContentSetStrokeColorDeviceRGB(
               stream, text_run.fill_color.red, text_run.fill_color.green,
               text_run.fill_color.blue) &&
           pdfioContentSetLineWidth(stream, stroke_width);
}

[[nodiscard]] bool write_shaped_glyph_cid_show(pdfio_stream_t *stream,
                                               std::uint16_t cid,
                                               std::string &error_message) {
    std::string command;
    command.reserve(11U);
    command.push_back('<');
    append_utf16be_hex_unit(command, cid);
    command += "> Tj\n";

    if (!pdfioStreamPuts(stream, command.c_str())) {
        error_message = "Unable to write shaped PDF glyph text";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_shaped_glyph_text_show(
    pdfio_stream_t *stream, const PdfTextRun &text_run,
    const GlyphFontResources &glyph_resources,
    std::map<FontResourceKey, std::size_t> &glyph_offsets,
    std::string &error_message) {
    const auto font_key = make_font_key(text_run);
    const auto resource = glyph_resources.find(font_key);
    if (resource == glyph_resources.end()) {
        error_message =
            "Missing shaped PDF glyph resource: " + font_label(font_key);
        return false;
    }

    auto &offset = glyph_offsets[font_key];
    const auto glyph_count = text_run.glyph_run.glyphs.size();
    if (offset + glyph_count > resource->second.entries.size()) {
        error_message =
            "Shaped PDF glyph resource is shorter than its text run: " +
            font_label(font_key);
        return false;
    }

    if (needs_positioned_shaped_glyph_show(text_run)) {
        double pen_x = 0.0;
        double pen_y = 0.0;
        for (std::size_t index = 0U; index < glyph_count; ++index) {
            const auto &entry = resource->second.entries[offset + index];
            const auto &glyph = text_run.glyph_run.glyphs[index];
            if (entry.glyph_id != glyph.glyph_id) {
                error_message =
                    "Shaped PDF glyph resource no longer matches text run: " +
                    font_label(font_key);
                return false;
            }

            if (!set_text_matrix_at(stream, text_run,
                                    pen_x + glyph.x_offset_points,
                                    pen_y + glyph.y_offset_points)) {
                error_message = "Unable to position shaped PDF glyph text";
                return false;
            }
            if (!write_shaped_glyph_cid_show(stream, entry.cid,
                                             error_message)) {
                return false;
            }

            pen_x += glyph.x_advance_points;
            pen_y += glyph.y_advance_points;
        }

        offset += glyph_count;
        return true;
    }

    std::string command;
    command.reserve(2U + glyph_count * 4U + 5U);
    command.push_back('<');
    for (std::size_t index = 0U; index < glyph_count; ++index) {
        const auto &entry = resource->second.entries[offset + index];
        const auto &glyph = text_run.glyph_run.glyphs[index];
        if (entry.glyph_id != glyph.glyph_id) {
            error_message =
                "Shaped PDF glyph resource no longer matches text run: " +
                font_label(font_key);
            return false;
        }
        append_utf16be_hex_unit(command, entry.cid);
    }
    command += "> Tj\n";
    offset += glyph_count;

    if (!pdfioStreamPuts(stream, command.c_str())) {
        error_message = "Unable to write shaped PDF glyph text";
        return false;
    }
    return true;
}

[[nodiscard]] bool write_text_run_content(
    pdfio_stream_t *stream, const PdfTextRun &text_run,
    const std::string &font_resource_name,
    const GlyphFontResources &glyph_resources,
    std::map<FontResourceKey, std::size_t> &glyph_offsets,
    std::string &error_message) {
    const auto font_key = make_font_key(text_run);
    if (!pdfioContentTextBegin(stream) ||
        !pdfioContentSetFillColorDeviceRGB(
            stream, text_run.fill_color.red, text_run.fill_color.green,
            text_run.fill_color.blue) ||
        !pdfioContentSetTextFont(stream, font_resource_name.c_str(),
                                 text_run.font_size_points) ||
        !set_text_rendering_style(stream, text_run) ||
        !set_text_run_matrix(stream, text_run)) {
        error_message = "Unable to begin PDF text run";
        return false;
    }

    const bool wrote_text =
        font_key.shaped_glyphs
            ? write_shaped_glyph_text_show(stream, text_run, glyph_resources,
                                           glyph_offsets, error_message)
            : pdfioContentTextShow(stream, text_run.unicode,
                                   text_run.text.c_str());
    if (!wrote_text) {
        if (error_message.empty()) {
            error_message = "Unable to write PDF text run";
        }
        return false;
    }

    if (!pdfioContentTextEnd(stream)) {
        error_message = "Unable to end PDF text run";
        return false;
    }
    return true;
}
