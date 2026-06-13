#include "pdf_document_adapter_font_test_support.hpp"

#if defined(FEATHERDOC_BUILD_PDF_IMPORT)

TEST_CASE("document PDF adapter table output round-trips through PDFium") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-table-roundtrip.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());
    auto intro = document.paragraphs();
    REQUIRE(intro.has_next());
    CHECK(intro.set_text("Invoice"));

    auto table = document.append_table(2U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_cell_text(0U, 0U, "Item"));
    CHECK(table.set_cell_text(0U, 1U, "Price"));
    CHECK(table.set_cell_text(1U, 0U, "Design"));
    CHECK(table.set_cell_text(1U, 1U, "1200"));

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title = "FeatherDoc adapter table roundtrip";
    options.use_system_font_fallbacks = false;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);

    const auto extracted_text = collect_text(parse_result.document);
    CHECK_NE(extracted_text.find("Invoice"), std::string::npos);
    CHECK_NE(extracted_text.find("Item"), std::string::npos);
    CHECK_NE(extracted_text.find("Price"), std::string::npos);
    CHECK_NE(extracted_text.find("Design"), std::string::npos);
    CHECK_NE(extracted_text.find("1200"), std::string::npos);
}

TEST_CASE("document PDF adapter table pagination round-trips through PDFium") {
    const auto output_path = std::filesystem::current_path() /
                             "featherdoc-adapter-table-pagination-roundtrip.pdf";

    featherdoc::Document document;
    REQUIRE_FALSE(document.create_empty());

    auto table = document.append_table(6U, 2U);
    REQUIRE(table.has_next());
    CHECK(table.set_width_twips(2880U));
    CHECK(table.set_column_width_twips(0U, 1440U));
    CHECK(table.set_column_width_twips(1U, 1440U));
    auto position = featherdoc::table_position{};
    position.horizontal_reference =
        featherdoc::table_position_horizontal_reference::margin;
    position.horizontal_offset_twips = 0;
    position.vertical_reference =
        featherdoc::table_position_vertical_reference::page;
    position.vertical_offset_twips = 720;
    CHECK(table.set_position(position));
    CHECK(table.set_cell_text(0U, 0U, "Header"));
    CHECK(table.set_cell_text(0U, 1U, "Amount"));
    for (std::size_t row_index = 1U; row_index < 6U; ++row_index) {
        CHECK(table.set_cell_text(row_index, 0U,
                                  "Row " + std::to_string(row_index)));
        CHECK(table.set_cell_text(row_index, 1U,
                                  std::to_string(row_index * 10U)));
    }

    auto row = table.rows();
    for (std::size_t row_index = 0U; row_index < 6U; ++row_index) {
        REQUIRE(row.has_next());
        CHECK(row.set_height_twips(720U, featherdoc::row_height_rule::exact));
        if (row_index == 0U) {
            CHECK(row.set_repeats_header());
        }
        row.next();
    }

    featherdoc::pdf::PdfDocumentAdapterOptions options;
    options.metadata.title = "FeatherDoc adapter table pagination roundtrip";
    options.use_system_font_fallbacks = false;
    options.page_size = featherdoc::pdf::PdfPageSize{300.0, 220.0};
    options.margin_left_points = 36.0;
    options.margin_right_points = 36.0;
    options.margin_top_points = 36.0;
    options.margin_bottom_points = 36.0;
    options.line_height_points = 14.0;
    options.paragraph_spacing_after_points = 0.0;

    const auto layout =
        featherdoc::pdf::layout_document_paragraphs(document, options);
    REQUIRE_GE(layout.pages.size(), 2U);

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result = generator.write(
        layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);

    featherdoc::pdf::PdfiumParser parser;
    const auto parse_result = parser.parse(output_path, {});
    REQUIRE_MESSAGE(parse_result.success, parse_result.error_message);
    REQUIRE_GE(parse_result.document.pages.size(), 2U);

    const auto first_page_text = collect_text(parse_result.document.pages[0]);
    const auto second_page_text = collect_text(parse_result.document.pages[1]);
    CHECK_NE(first_page_text.find("Header"), std::string::npos);
    CHECK_NE(second_page_text.find("Header"), std::string::npos);
    CHECK_NE(second_page_text.find("Row "), std::string::npos);
}
#endif
