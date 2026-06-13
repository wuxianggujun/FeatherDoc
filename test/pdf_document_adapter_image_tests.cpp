#include "pdf_document_adapter_font_test_support.hpp"

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
TEST_CASE("PDF writer preserves per-page layout sizes") {
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-mixed-page-sizes.pdf";

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc mixed page size writer test";
    layout.metadata.creator = "FeatherDoc tests";

    featherdoc::pdf::PdfPageLayout portrait_page;
    portrait_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    portrait_page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 720.0},
        "Portrait page size",
        "Helvetica",
        {},
        14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    layout.pages.push_back(std::move(portrait_page));

    featherdoc::pdf::PdfPageLayout landscape_page;
    landscape_page.size = featherdoc::pdf::PdfPageSize{792.0, 612.0};
    landscape_page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 540.0},
        "Landscape page size",
        "Helvetica",
        {},
        14.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    layout.pages.push_back(std::move(landscape_page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_EQ(parse_result.document.pages.size(), 2U);

    CHECK_EQ(parse_result.document.pages[0].size.width_points,
             doctest::Approx(612.0));
    CHECK_EQ(parse_result.document.pages[0].size.height_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages[1].size.width_points,
             doctest::Approx(792.0));
    CHECK_EQ(parse_result.document.pages[1].size.height_points,
             doctest::Approx(612.0));

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Portrait page size"), std::string::npos);
    CHECK_NE(extracted_text.find("Landscape page size"), std::string::npos);
}

TEST_CASE("document PDF adapter output round-trips CJK text through PDFium") {
    const auto latin_font = first_existing_path(candidate_latin_fonts());
    const auto cjk_font = first_existing_path(candidate_cjk_fonts());
    if (latin_font.empty() || cjk_font.empty()) {
        MESSAGE(
            "skipping adapter PDF roundtrip: configure test Latin/CJK fonts");
        return;
    }

    const auto cjk_text = utf8_from_u8(u8"中文合同测试");
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-cjk-roundtrip.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    CHECK(document.set_default_run_font_family("Unit Latin"));
    CHECK(document.set_default_run_east_asia_font_family("Unit CJK"));

    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    REQUIRE(paragraph.add_run("Contract ABC ").has_next());
    REQUIRE(paragraph.add_run(cjk_text).has_next());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title = "FeatherDoc adapter CJK roundtrip";
    options.font_mappings = {
        featherdoc::pdf::PdfFontMapping{"Unit Latin", latin_font},
        featherdoc::pdf::PdfFontMapping{"Unit CJK", cjk_font},
    };
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/Font<</F1"), std::string::npos);
    CHECK_NE(pdf_bytes.find("/F2 "), std::string::npos);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Contract ABC "), std::string::npos);
    CHECK_NE(extracted_text.find(cjk_text), std::string::npos);
}
#endif

TEST_CASE("document PDF adapter emits inline images as PDF image blocks") {
    const auto image_path =
        std::filesystem::current_path() / "featherdoc-adapter-inline-image.png";
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-adapter-inline-image.pdf";
    write_binary_file(image_path, tiny_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Image intro"));
    REQUIRE(document.append_image(image_path, 20U, 10U));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);

    const auto &image = layout.pages.front().images.front();
    CHECK_EQ(image.content_type, "image/png");
    CHECK_EQ(image.bounds.x_points,
             doctest::Approx(options.margin_left_points));
    CHECK_EQ(image.bounds.width_points, doctest::Approx(15.0));
    CHECK_EQ(image.bounds.height_points, doctest::Approx(7.5));
    CHECK(image.cleanup_source_after_write);
    CHECK(std::filesystem::exists(image.source_path));

    const auto images = document.inline_images();
    REQUIRE_EQ(images.size(), 1U);
    REQUIRE(images.front().paragraph_index.has_value());
    CHECK_EQ(*images.front().paragraph_index, 1U);

    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);
    CHECK_GT(layout.pages.front().text_runs.front().baseline_origin.y_points,
             image.bounds.y_points + image.bounds.height_points);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
    CHECK_FALSE(std::filesystem::exists(image.source_path));

    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/XObject<</Im1"), std::string::npos);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document).find("Image intro"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter positions floating images on PDF pages") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-floating-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-floating-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Floating image anchor"));

    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::page;
    floating_options.horizontal_offset_px = 144;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::page;
    floating_options.vertical_offset_px = 120;
    floating_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    floating_options.crop = featherdoc::floating_image_crop{500U, 0U, 0U, 0U};
    REQUIRE(document.append_floating_image(image_path, 96U, 48U,
                                           floating_options));

    const auto drawing_images = document.drawing_images();
    REQUIRE_EQ(drawing_images.size(), 1U);
    CHECK_EQ(drawing_images.front().placement,
             featherdoc::drawing_image_placement::anchored_object);
    REQUIRE(drawing_images.front().floating_options.has_value());

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    CHECK_NE(collect_layout_text(layout).find("Floating image anchor"),
             std::string::npos);

    const auto &image = layout.pages.front().images.front();
    CHECK_EQ(image.content_type, "image/png");
    CHECK_EQ(image.bounds.x_points, doctest::Approx(108.0));
    CHECK_EQ(image.bounds.width_points, doctest::Approx(72.0));
    CHECK_EQ(image.bounds.height_points, doctest::Approx(36.0));
    CHECK_EQ(image.bounds.y_points,
             doctest::Approx(options.page_size.height_points - 90.0 - 36.0));
    CHECK(image.cleanup_source_after_write);
    CHECK_FALSE(image.draw_behind_text);
    CHECK_EQ(image.crop_left_per_mille, 500U);
    CHECK_EQ(image.crop_top_per_mille, 0U);
    CHECK_EQ(image.crop_right_per_mille, 0U);
    CHECK_EQ(image.crop_bottom_per_mille, 0U);
    CHECK(std::filesystem::exists(image.source_path));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
    CHECK_FALSE(std::filesystem::exists(image.source_path));

    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/XObject<</Im1"), std::string::npos);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document).find("Floating image anchor"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter marks behind-text floating images") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-behind-text-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-behind-text-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Behind text floating image anchor"));

    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::margin;
    floating_options.horizontal_offset_px = 24;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    floating_options.vertical_offset_px = 0;
    floating_options.wrap_mode = featherdoc::floating_image_wrap_mode::none;
    floating_options.behind_text = true;
    REQUIRE(document.append_floating_image(image_path, 96U, 48U,
                                           floating_options));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    CHECK_NE(collect_layout_text(layout).find("Behind text floating image anchor"),
             std::string::npos);

    const auto &image = layout.pages.front().images.front();
    CHECK_EQ(image.content_type, "image/png");
    CHECK(image.draw_behind_text);
    CHECK_EQ(image.bounds.x_points,
             doctest::Approx(options.margin_left_points + 18.0));
    CHECK_EQ(image.bounds.width_points, doctest::Approx(72.0));
    CHECK_EQ(image.bounds.height_points, doctest::Approx(36.0));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document)
                 .find("Behind text floating image anchor"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter wraps text around square floating images") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-square-wrap-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-square-wrap-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    floating_options.horizontal_offset_px = 0;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    floating_options.vertical_offset_px = 0;
    floating_options.wrap_mode = featherdoc::floating_image_wrap_mode::square;
    floating_options.wrap_distance_right_px = 16U;
    REQUIRE(document.append_floating_image(image_path, 96U, 72U,
                                           floating_options));
    append_body_paragraph(
        document,
        "Square wrap text should flow beside the floating image before it "
        "continues below the image on later PDF lines. The paragraph keeps "
        "running long enough to leave the vertical image area and return to "
        "the normal left margin once the wrapped region has ended.");

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);

    const auto &image = layout.pages.front().images.front();
    const auto &first_text = layout.pages.front().text_runs.front();
    CHECK_EQ(image.bounds.x_points, doctest::Approx(options.margin_left_points));
    CHECK_EQ(first_text.baseline_origin.x_points,
             doctest::Approx(image.bounds.x_points +
                             image.bounds.width_points + 12.0));
    CHECK_GT(first_text.baseline_origin.y_points, image.bounds.y_points);
    CHECK_LT(first_text.baseline_origin.y_points,
             image.bounds.y_points + image.bounds.height_points);

    auto returned_to_margin = false;
    for (const auto &text_run : layout.pages.front().text_runs) {
        if (text_run.baseline_origin.y_points < image.bounds.y_points &&
            text_run.baseline_origin.x_points ==
                doctest::Approx(options.margin_left_points)) {
            returned_to_margin = true;
        }
    }
    CHECK(returned_to_margin);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
}

TEST_CASE("document PDF adapter pushes top-bottom wrapped image text below") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-top-bottom-wrap-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-top-bottom-wrap-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    featherdoc::floating_image_options floating_options;
    floating_options.horizontal_reference =
        featherdoc::floating_image_horizontal_reference::column;
    floating_options.horizontal_offset_px = 0;
    floating_options.vertical_reference =
        featherdoc::floating_image_vertical_reference::paragraph;
    floating_options.vertical_offset_px = 0;
    floating_options.wrap_mode =
        featherdoc::floating_image_wrap_mode::top_bottom;
    floating_options.wrap_distance_bottom_px = 16U;
    REQUIRE(document.append_floating_image(image_path, 96U, 72U,
                                           floating_options));
    append_body_paragraph(document,
                          "Top bottom text starts below the image.");

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;
    options.line_height_points = 14.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    REQUIRE_GE(layout.pages.front().text_runs.size(), 1U);

    const auto &image = layout.pages.front().images.front();
    const auto &first_text = layout.pages.front().text_runs.front();
    CHECK_EQ(first_text.baseline_origin.x_points,
             doctest::Approx(options.margin_left_points));
    CHECK_LE(first_text.baseline_origin.y_points + options.font_size_points,
             image.bounds.y_points - 12.0);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
}

TEST_CASE(
    "document PDF adapter lays out inline images in body paragraph order") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-ordered-inline-image.png";
    write_binary_file(image_path, tiny_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("Before image"));
    REQUIRE(document.append_image(image_path, 20U, 10U));

    auto body_template = document.body_template();
    REQUIRE(static_cast<bool>(body_template));
    auto after = body_template.append_paragraph("After image");
    REQUIRE(after.has_next());

    const auto images = document.inline_images();
    REQUIRE_EQ(images.size(), 1U);
    REQUIRE(images.front().paragraph_index.has_value());
    CHECK_EQ(*images.front().paragraph_index, 1U);

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    REQUIRE_EQ(layout.pages.front().text_runs.size(), 2U);

    const auto &before_text = layout.pages.front().text_runs[0];
    const auto &after_text = layout.pages.front().text_runs[1];
    const auto &image = layout.pages.front().images.front();

    CHECK_EQ(before_text.text, "Before image");
    CHECK_EQ(after_text.text, "After image");
    CHECK_GT(before_text.baseline_origin.y_points,
             image.bounds.y_points + image.bounds.height_points);
    CHECK_GT(image.bounds.y_points, after_text.baseline_origin.y_points);
}

TEST_CASE("PDF writer accepts inline image layout blocks") {
    const auto image_path =
        std::filesystem::current_path() / "featherdoc-writer-inline-image.png";
    const auto output_path =
        std::filesystem::current_path() / "featherdoc-writer-inline-image.pdf";
    write_binary_file(image_path, tiny_png_data());

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc image writer test";
    layout.metadata.creator = "FeatherDoc tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize{};
    page.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 640.0, 24.0, 24.0},
        image_path,
        "image/png",
        "Tiny image",
        true,
        false,
    });
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 620.0},
        "Image PDF text",
        "Helvetica",
        {},
        12.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document).find("Image PDF text"),
             std::string::npos);
#endif
}

TEST_CASE("PDF writer draws foreground images after text") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-writer-foreground-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-writer-foreground-image.pdf";
    write_binary_file(image_path, quadrant_png_data());

    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc foreground image writer test";
    layout.metadata.creator = "FeatherDoc tests";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize{};
    page.text_runs.push_back(featherdoc::pdf::PdfTextRun{
        featherdoc::pdf::PdfPoint{72.0, 724.0},
        "Foreground image layer",
        "Helvetica",
        {},
        18.0,
        featherdoc::pdf::PdfRgbColor{0.0, 0.0, 0.0},
        false,
        false,
        false,
        false,
    });
    page.images.push_back(featherdoc::pdf::PdfImage{
        featherdoc::pdf::PdfRect{72.0, 700.0, 144.0, 48.0},
        image_path,
        "image/png",
        "Foreground image",
        true,
        false,
    });
    page.images.back().draw_behind_text = false;
    layout.pages.push_back(std::move(page));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);

    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/XObject<</Im1"), std::string::npos);

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)
    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    CHECK_NE(collect_text(parse_result.document).find("Foreground image layer"),
             std::string::npos);
#endif
}

TEST_CASE("document PDF adapter preserves inline RGBA PNG images for PDFio") {
    const auto image_path = std::filesystem::current_path() /
                            "featherdoc-adapter-inline-rgba-image.png";
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-inline-rgba-image.pdf";
    write_binary_file(image_path, tiny_rgba_png_data());

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto paragraph = document.paragraphs();
    REQUIRE(paragraph.has_next());
    CHECK(paragraph.set_text("RGBA image intro"));
    REQUIRE(document.append_image(image_path, 20U, 10U));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.use_system_font_fallbacks = false;
    options.render_inline_images = true;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    REQUIRE_EQ(layout.pages.size(), 1U);
    REQUIRE_EQ(layout.pages.front().images.size(), 1U);
    CHECK_NE(collect_layout_text(layout).find("RGBA image intro"),
             std::string::npos);

    const auto &image = layout.pages.front().images.front();
    CHECK_EQ(image.content_type, "image/png");
    CHECK_EQ(image.bounds.width_points, doctest::Approx(15.0));
    CHECK_EQ(image.bounds.height_points, doctest::Approx(7.5));
    CHECK(image.cleanup_source_after_write);
    CHECK(std::filesystem::exists(image.source_path));

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    CHECK_GT(write_result.bytes_written, 0U);
    const auto pdf_bytes = read_binary_file(output_path);
    CHECK_NE(pdf_bytes.find("/SMask"), std::string::npos);
    CHECK_FALSE(std::filesystem::exists(image.source_path));
    CHECK_NE(pdf_bytes.find("/XObject<</Im1"), std::string::npos);
}
