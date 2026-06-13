#include "pdf_document_adapter_font_test_support.hpp"

TEST_CASE("document PDF adapter maps table alignment and indent") {
    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_alignment(featherdoc::table_alignment::center));
        CHECK(table.set_cell_text(0U, 0U, "Centered"));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->alignment.has_value());
        CHECK_EQ(*table_summary->alignment,
                 featherdoc::table_alignment::center);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto available_width = options.page_size.width_points -
                                     options.margin_left_points -
                                     options.margin_right_points;
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points +
                              (available_width - 72.0) / 2.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_alignment(featherdoc::table_alignment::left));
        CHECK(table.set_indent_twips(720U));
        CHECK(table.set_cell_text(0U, 0U, "Indented"));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->indent_twips.has_value());
        CHECK_EQ(*table_summary->indent_twips, 720U);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + 36.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_alignment(featherdoc::table_alignment::right));
        CHECK(table.set_cell_text(0U, 0U, "Right aligned"));

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto available_width = options.page_size.width_points -
                                     options.margin_left_points -
                                     options.margin_right_points;
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + available_width -
                              72.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Positioned"));
        CHECK(table.set_alignment(featherdoc::table_alignment::right));
        CHECK(table.set_indent_twips(720U));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::page;
        position.horizontal_offset_twips = 2160;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 1440;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        CHECK_EQ(table_summary->position->horizontal_reference,
                 featherdoc::table_position_horizontal_reference::page);
        CHECK_EQ(table_summary->position->horizontal_offset_twips, 2160);
        CHECK_EQ(table_summary->position->vertical_reference,
                 featherdoc::table_position_vertical_reference::page);
        CHECK_EQ(table_summary->position->vertical_offset_twips, 1440);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.margin_left_points = 36.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.x_points == doctest::Approx(108.0));
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(options.page_size.height_points - 72.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Page centered"));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::page;
        position.horizontal_offset_twips = 120;
        position.horizontal_spec =
            featherdoc::table_position_horizontal_spec::center;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 0;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        REQUIRE(table_summary->position->horizontal_spec.has_value());
        CHECK_EQ(*table_summary->position->horizontal_spec,
                 featherdoc::table_position_horizontal_spec::center);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx((options.page_size.width_points - 72.0) / 2.0 +
                              6.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Margin right"));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::margin;
        position.horizontal_offset_twips = -240;
        position.horizontal_spec =
            featherdoc::table_position_horizontal_spec::right;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 0;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        REQUIRE(table_summary->position->horizontal_spec.has_value());
        CHECK_EQ(*table_summary->position->horizontal_spec,
                 featherdoc::table_position_horizontal_spec::right);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
        options.margin_left_points = 30.0;
        options.margin_right_points = 42.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto available_width = options.page_size.width_points -
                                     options.margin_left_points -
                                     options.margin_right_points;
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + available_width -
                              72.0 - 12.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Column outside"));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::column;
        position.horizontal_offset_twips = 0;
        position.horizontal_spec =
            featherdoc::table_position_horizontal_spec::outside;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 0;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        REQUIRE(table_summary->position->horizontal_spec.has_value());
        CHECK_EQ(*table_summary->position->horizontal_spec,
                 featherdoc::table_position_horizontal_spec::outside);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
        options.margin_left_points = 30.0;
        options.margin_right_points = 42.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto available_width = options.page_size.width_points -
                                     options.margin_left_points -
                                     options.margin_right_points;
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + available_width -
                              72.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Page middle"));

        auto row = table.rows();
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::page;
        position.horizontal_offset_twips = 0;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 120;
        position.vertical_spec =
            featherdoc::table_position_vertical_spec::center;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        REQUIRE(table_summary->position->vertical_spec.has_value());
        CHECK_EQ(*table_summary->position->vertical_spec,
                 featherdoc::table_position_vertical_spec::center);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto table_height = 36.0;
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(options.page_size.height_points -
                              (options.page_size.height_points -
                               table_height) /
                                  2.0 -
                              6.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Margin bottom"));

        auto row = table.rows();
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::margin;
        position.horizontal_offset_twips = 0;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::margin;
        position.vertical_offset_twips = -240;
        position.vertical_spec =
            featherdoc::table_position_vertical_spec::bottom;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        REQUIRE(table_summary->position->vertical_spec.has_value());
        CHECK_EQ(*table_summary->position->vertical_spec,
                 featherdoc::table_position_vertical_spec::bottom);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
        options.margin_top_points = 30.0;
        options.margin_bottom_points = 40.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(options.margin_bottom_points + 36.0 + 12.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Page inside"));

        auto row = table.rows();
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::page;
        position.horizontal_offset_twips = 0;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 360;
        position.vertical_spec =
            featherdoc::table_position_vertical_spec::inside;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        REQUIRE(table_summary->position->vertical_spec.has_value());
        CHECK_EQ(*table_summary->position->vertical_spec,
                 featherdoc::table_position_vertical_spec::inside);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(options.page_size.height_points - 18.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Negative margin positioned"));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::margin;
        position.horizontal_offset_twips = -360;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::margin;
        position.vertical_offset_twips = -720;
        CHECK(table.set_position(position));

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.margin_left_points = 60.0;
        options.margin_top_points = 80.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points - 18.0));
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(options.page_size.height_points -
                              options.margin_top_points + 36.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        CHECK(paragraph.set_text("Before top-spaced table"));

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Top-spaced"));

        auto row = table.rows();
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::column;
        position.horizontal_offset_twips = 0;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::paragraph;
        position.vertical_offset_twips = 0;
        position.top_from_text_twips = 240U;
        CHECK(table.set_position(position));

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
        options.margin_top_points = 48.0;
        options.line_height_points = 16.0;
        options.paragraph_spacing_after_points = 6.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto expected_row_top =
            options.page_size.height_points - options.margin_top_points -
            options.line_height_points - options.paragraph_spacing_after_points -
            12.0;
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(expected_row_top));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Wrap metadata"));

        auto row = table.rows();
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::margin;
        position.horizontal_offset_twips = 240;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::page;
        position.vertical_offset_twips = 720;
        position.left_from_text_twips = 960U;
        position.right_from_text_twips = 480U;
        position.top_from_text_twips = 600U;
        position.overlap = featherdoc::table_overlap::never;
        CHECK(table.set_position(position));

        const auto table_summary = document.inspect_table(0U);
        REQUIRE(table_summary.has_value());
        REQUIRE(table_summary->position.has_value());
        REQUIRE(table_summary->position->left_from_text_twips.has_value());
        REQUIRE(table_summary->position->right_from_text_twips.has_value());
        REQUIRE(table_summary->position->top_from_text_twips.has_value());
        REQUIRE(table_summary->position->overlap.has_value());
        CHECK_EQ(*table_summary->position->left_from_text_twips, 960U);
        CHECK_EQ(*table_summary->position->right_from_text_twips, 480U);
        CHECK_EQ(*table_summary->position->top_from_text_twips, 600U);
        CHECK_EQ(*table_summary->position->overlap,
                 featherdoc::table_overlap::never);

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
        options.margin_left_points = 40.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);

        const auto &rectangle = layout.pages.front().rectangles.front();
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + 12.0));
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(options.page_size.height_points - 36.0));
    }

    {
        featherdoc::Document document;
        REQUIRE_FALSE(document.create_empty());
        auto paragraph = document.paragraphs();
        REQUIRE(paragraph.has_next());
        CHECK(paragraph.set_text("Before positioned table"));

        auto table = document.append_table(1U, 1U);
        REQUIRE(table.has_next());
        CHECK(table.set_width_twips(1440U));
        CHECK(table.set_column_width_twips(0U, 1440U));
        CHECK(table.set_cell_text(0U, 0U, "Paragraph anchored"));

        auto position = featherdoc::table_position{};
        position.horizontal_reference =
            featherdoc::table_position_horizontal_reference::column;
        position.horizontal_offset_twips = 720;
        position.vertical_reference =
            featherdoc::table_position_vertical_reference::paragraph;
        position.vertical_offset_twips = -360;
        CHECK(table.set_position(position));

        featherdoc::pdf::PdfDocumentAdapterOptions options;
        options.use_system_font_fallbacks = false;
        options.margin_left_points = 64.0;
        options.margin_top_points = 72.0;
        options.line_height_points = 16.0;
        options.paragraph_spacing_after_points = 6.0;

        const auto layout =
            featherdoc::pdf::layout_document_paragraphs(document, options);

        REQUIRE_EQ(layout.pages.size(), 1U);
        REQUIRE_EQ(layout.pages.front().rectangles.size(), 1U);
        REQUIRE_GE(layout.pages.front().text_runs.size(), 2U);
        CHECK_EQ(layout.pages.front().text_runs.front().text,
                 "Before positioned table");

        const auto &rectangle = layout.pages.front().rectangles.front();
        const auto expected_row_top =
            options.page_size.height_points - options.margin_top_points -
            options.line_height_points - options.paragraph_spacing_after_points +
            18.0;
        CHECK(rectangle.bounds.x_points ==
              doctest::Approx(options.margin_left_points + 36.0));
        CHECK(rectangle.bounds.y_points + rectangle.bounds.height_points ==
              doctest::Approx(expected_row_top));
    }
}
