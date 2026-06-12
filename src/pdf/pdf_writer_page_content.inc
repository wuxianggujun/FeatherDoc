[[nodiscard]] pdfio_rect_t
page_bounds_rect(const PdfPageSize &page_size) noexcept {
    return pdfio_rect_t{0.0, 0.0, page_size.width_points,
                        page_size.height_points};
}

[[nodiscard]] bool set_page_bounds(pdfio_dict_t *page_dict,
                                   const PdfPageLayout &page,
                                   std::string &error_message) {
    auto media_box = page_bounds_rect(page.size);
    auto crop_box = media_box;
    if (!pdfioDictSetRect(page_dict, "MediaBox", &media_box) ||
        !pdfioDictSetRect(page_dict, "CropBox", &crop_box)) {
        error_message = "Unable to set PDF page bounds";
        return false;
    }

    return true;
}

[[nodiscard]] bool has_image_crop(const PdfImage &image) noexcept {
    return image.crop_left_per_mille != 0U ||
           image.crop_top_per_mille != 0U ||
           image.crop_right_per_mille != 0U ||
           image.crop_bottom_per_mille != 0U;
}

[[nodiscard]] bool image_crop_is_valid(const PdfImage &image) noexcept {
    constexpr std::uint32_t max_crop_per_mille = 1000U;
    if (image.crop_left_per_mille > max_crop_per_mille ||
        image.crop_top_per_mille > max_crop_per_mille ||
        image.crop_right_per_mille > max_crop_per_mille ||
        image.crop_bottom_per_mille > max_crop_per_mille) {
        return false;
    }

    return image.crop_left_per_mille + image.crop_right_per_mille <
               max_crop_per_mille &&
           image.crop_top_per_mille + image.crop_bottom_per_mille <
               max_crop_per_mille;
}

[[nodiscard]] bool write_pdf_image(pdfio_stream_t *stream,
                                   const PdfImage &image,
                                   const std::string &resource_name,
                                   std::string &error_message) {
    if (!has_image_crop(image)) {
        if (!pdfioContentDrawImage(
                stream, resource_name.c_str(), image.bounds.x_points,
                image.bounds.y_points, image.bounds.width_points,
                image.bounds.height_points)) {
            error_message =
                "Unable to draw PDF image: " + image.source_path.string();
            return false;
        }
        return true;
    }

    if (!image_crop_is_valid(image)) {
        error_message =
            "Invalid PDF image crop: " + image.source_path.string();
        return false;
    }

    constexpr double crop_scale = 1000.0;
    const double crop_left =
        static_cast<double>(image.crop_left_per_mille) / crop_scale;
    const double crop_top =
        static_cast<double>(image.crop_top_per_mille) / crop_scale;
    const double crop_right =
        static_cast<double>(image.crop_right_per_mille) / crop_scale;
    const double crop_bottom =
        static_cast<double>(image.crop_bottom_per_mille) / crop_scale;
    const double visible_width = 1.0 - crop_left - crop_right;
    const double visible_height = 1.0 - crop_top - crop_bottom;
    const double draw_width = image.bounds.width_points / visible_width;
    const double draw_height = image.bounds.height_points / visible_height;

    const double draw_x = image.bounds.x_points - crop_left * draw_width;
    const double draw_y = image.bounds.y_points - crop_bottom * draw_height;

    if (!pdfioContentSave(stream) ||
        !pdfioContentPathRect(stream, image.bounds.x_points,
                              image.bounds.y_points,
                              image.bounds.width_points,
                              image.bounds.height_points) ||
        !pdfioContentClip(stream, false) ||
        !pdfioStreamPuts(stream, "n\n")) {
        error_message =
            "Unable to set PDF image crop: " + image.source_path.string();
        pdfioContentRestore(stream);
        return false;
    }

    if (!pdfioContentDrawImage(stream, resource_name.c_str(), draw_x, draw_y,
                               draw_width, draw_height)) {
        error_message =
            "Unable to draw cropped PDF image: " + image.source_path.string();
        pdfioContentRestore(stream);
        return false;
    }

    if (!pdfioContentRestore(stream)) {
        error_message =
            "Unable to restore PDF image crop: " + image.source_path.string();
        return false;
    }

    return true;
}

[[nodiscard]] bool write_page_images(
    pdfio_stream_t *stream, const PdfPageLayout &page,
    const std::map<ImageResourceKey, std::string> &image_resources,
    bool draw_behind_text, std::string &error_message) {
    for (const auto &image : page.images) {
        if (image.draw_behind_text != draw_behind_text) {
            continue;
        }

        const auto image_key =
            ImageResourceKey{image.source_path, image.interpolate};
        const auto image_resource = image_resources.find(image_key);
        if (image_resource == image_resources.end()) {
            error_message =
                "Missing PDF image resource: " + image.source_path.string();
            return false;
        }

        if (!write_pdf_image(stream, image, image_resource->second,
                             error_message)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool write_probe_page(pdfio_file_t *pdf,
                                    const PdfPageLayout &page,
                                    const PdfWriterOptions &options,
                                    FontSubsetStorage &subset_storage,
                                    std::string &error_message) {
    pdfio_dict_t *page_dict = pdfioDictCreate(pdf);
    if (page_dict == nullptr) {
        error_message = "Unable to create PDF page dictionary";
        return false;
    }

    if (!set_page_bounds(page_dict, page, error_message)) {
        return false;
    }

    GlyphFontResources glyph_font_resources;
    if (!collect_shaped_glyph_font_resources(page, glyph_font_resources,
                                             error_message)) {
        return false;
    }

    std::map<FontResourceKey, std::string> used_text_by_font;
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        if (!font_key.shaped_glyphs) {
            used_text_by_font[font_key] += text_run.text;
        }
    }

    std::map<FontResourceKey, std::string> font_resources;
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        if (font_resources.find(font_key) != font_resources.end()) {
            continue;
        }

        const std::string resource_name =
            "F" + std::to_string(font_resources.size() + 1);
        const auto *pdf_resource_name =
            make_pdfio_resource_name(pdf, resource_name, error_message);
        if (pdf_resource_name == nullptr) {
            return false;
        }
        if (font_key.shaped_glyphs) {
            const auto glyph_resource = glyph_font_resources.find(font_key);
            if (glyph_resource == glyph_font_resources.end()) {
                error_message =
                    "Missing shaped PDF glyph resource: " + font_label(font_key);
                return false;
            }
            pdfio_obj_t *font = create_shaped_glyph_font(
                pdf, font_key, glyph_resource->second,
                options.subset_unicode_fonts, error_message);
            if (font == nullptr) {
                if (error_message.empty()) {
                    error_message =
                        "Unable to create shaped PDF glyph font: " +
                        font_label(font_key);
                }
                return false;
            }

            if (!pdfioPageDictAddFont(page_dict, pdf_resource_name, font)) {
                error_message =
                    "Unable to add PDF font resource: " + font_label(font_key);
                return false;
            }

            font_resources.emplace(font_key, resource_name);
            continue;
        }

        if (font_key.unicode && font_key.file_path.empty()) {
            error_message =
                "Unicode PDF text requires an embedded font file: " +
                font_label(font_key);
            return false;
        }
        const auto used_text = used_text_by_font.find(font_key);
        pdfio_obj_t *font = create_pdfio_font(
            pdf, font_key,
            used_text == used_text_by_font.end() ? std::string{}
                                                 : used_text->second,
            options.subset_unicode_fonts, subset_storage);
        if (font == nullptr) {
            error_message =
                "Unable to create PDF font: " + font_label(font_key);
            return false;
        }

        if (!pdfioPageDictAddFont(page_dict, pdf_resource_name, font)) {
            error_message =
                "Unable to add PDF font resource: " + font_label(font_key);
            return false;
        }

        font_resources.emplace(font_key, resource_name);
    }

    std::map<ImageResourceKey, std::string> image_resources;
    for (const auto &image : page.images) {
        if (image.source_path.empty()) {
            error_message = "PDF image source path is empty";
            return false;
        }

        const auto image_key =
            ImageResourceKey{image.source_path, image.interpolate};
        if (image_resources.find(image_key) != image_resources.end()) {
            continue;
        }

        const std::string resource_name =
            "Im" + std::to_string(image_resources.size() + 1);
        const auto *pdf_resource_name =
            make_pdfio_resource_name(pdf, resource_name, error_message);
        if (pdf_resource_name == nullptr) {
            return false;
        }
        const auto image_path = image.source_path.string();
        pdfio_obj_t *image_object = pdfioFileCreateImageObjFromFile(
            pdf, image_path.c_str(), image.interpolate);
        if (image_object == nullptr) {
            error_message =
                "Unable to create PDF image: " + image_label(image_key);
            return false;
        }

        if (!pdfioPageDictAddImage(page_dict, pdf_resource_name,
                                   image_object)) {
            error_message =
                "Unable to add PDF image resource: " + image_label(image_key);
            return false;
        }

        image_resources.emplace(image_key, resource_name);
    }

    std::unique_ptr<pdfio_stream_t, PdfioStreamCloser> stream(
        pdfioFileCreatePage(pdf, page_dict));
    if (!stream) {
        error_message = "Unable to create PDF page stream";
        return false;
    }

    for (const auto &rectangle : page.rectangles) {
        if (!rectangle.stroke && !rectangle.fill) {
            continue;
        }

        if (rectangle.fill &&
            !pdfioContentSetFillColorDeviceRGB(
                stream.get(), rectangle.fill_color.red,
                rectangle.fill_color.green, rectangle.fill_color.blue)) {
            error_message = "Unable to set PDF rectangle fill color";
            return false;
        }

        if (rectangle.stroke &&
            (!pdfioContentSetStrokeColorDeviceRGB(
                 stream.get(), rectangle.stroke_color.red,
                 rectangle.stroke_color.green, rectangle.stroke_color.blue) ||
             !pdfioContentSetLineWidth(stream.get(),
                                       rectangle.line_width_points))) {
            error_message = "Unable to set PDF rectangle stroke";
            return false;
        }

        if (!pdfioContentPathRect(stream.get(), rectangle.bounds.x_points,
                                  rectangle.bounds.y_points,
                                  rectangle.bounds.width_points,
                                  rectangle.bounds.height_points)) {
            error_message = "Unable to write PDF rectangle path";
            return false;
        }

        if (rectangle.fill && rectangle.stroke) {
            if (!pdfioContentFillAndStroke(stream.get(), false)) {
                error_message = "Unable to fill and stroke PDF rectangle";
                return false;
            }
        } else if (rectangle.fill) {
            if (!pdfioContentFill(stream.get(), false)) {
                error_message = "Unable to fill PDF rectangle";
                return false;
            }
        } else if (!pdfioContentStroke(stream.get())) {
            error_message = "Unable to stroke PDF rectangle";
            return false;
        }
    }

    for (const auto &line : page.lines) {
        if (!pdfioContentSetStrokeColorDeviceRGB(
                stream.get(), line.stroke_color.red, line.stroke_color.green,
                line.stroke_color.blue) ||
            !pdfioContentSetLineWidth(stream.get(), line.line_width_points) ||
            !pdfioContentSetLineCap(stream.get(),
                                    to_pdfio_line_cap(line.line_cap)) ||
            !pdfioContentSetDashPattern(stream.get(), line.dash_phase_points,
                                        line.dash_on_points,
                                        line.dash_off_points) ||
            !pdfioContentPathMoveTo(stream.get(), line.start.x_points,
                                    line.start.y_points) ||
            !pdfioContentPathLineTo(stream.get(), line.end.x_points,
                                    line.end.y_points) ||
            !pdfioContentStroke(stream.get())) {
            error_message = "Unable to write PDF line";
            return false;
        }
    }

    if (!write_page_images(stream.get(), page, image_resources, true,
                           error_message)) {
        return false;
    }

    std::map<FontResourceKey, std::size_t> glyph_resource_offsets;
    for (const auto &text_run : page.text_runs) {
        const auto font_key = make_font_key(text_run);
        const auto font_resource = font_resources.find(font_key);
        if (font_resource == font_resources.end()) {
            error_message =
                "Missing PDF font resource: " + font_label(font_key);
            return false;
        }

        if (!write_actual_text_begin(stream.get(), text_run.text,
                                     error_message)) {
            return false;
        }

        if (!write_text_run_content(stream.get(), text_run,
                                    font_resource->second,
                                    glyph_font_resources,
                                    glyph_resource_offsets, error_message)) {
            return false;
        }

        if (!write_actual_text_end(stream.get(), error_message)) {
            return false;
        }

        if (!write_text_underline(stream.get(), text_run, error_message)) {
            return false;
        }
        if (!write_text_strikethrough(stream.get(), text_run,
                                      error_message)) {
            return false;
        }
    }

    if (!write_page_images(stream.get(), page, image_resources, false,
                           error_message)) {
        return false;
    }

    if (!pdfioStreamClose(stream.release())) {
        error_message = "Unable to close PDF page stream";
        return false;
    }

    return true;
}
