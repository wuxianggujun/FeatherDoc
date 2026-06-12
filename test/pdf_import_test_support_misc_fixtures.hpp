#pragma once

#include "pdf_import_test_support_common.hpp"

namespace featherdoc::test_support {

[[nodiscard]] inline std::filesystem::path
write_long_repeated_header_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc long repeated-header table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_table_row = [](featherdoc::pdf::PdfPageLayout &page, double y_points,
                            std::string first, std::string second,
                            std::string third) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        page.text_runs.push_back(
            make_pdf_text_run(246.0, y_points, std::move(second)));
        page.text_runs.push_back(
            make_pdf_text_run(406.0, y_points, std::move(third)));
    };

    auto add_table_page = [&](featherdoc::pdf::PdfPageLayout &page,
                              std::size_t start_index, bool add_title,
                              bool add_footer) {
        page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
        if (add_title) {
            page.text_runs.push_back(
                make_pdf_text_run(72.0, 724.0, "Long repeated-header sample"));
        }

        constexpr std::size_t kBodyRowCountPerPage = 13U;
        append_grid(page, 72.0, 700.0, 160.0, 28.0, 3U,
                    kBodyRowCountPerPage + 1U);
        add_table_row(page, 678.0, "Item", "Owner", "Status");

        for (std::size_t row_offset = 0U; row_offset < kBodyRowCountPerPage;
             ++row_offset) {
            const auto row_number = start_index + row_offset + 1U;
            const auto y_points = 650.0 - 28.0 * static_cast<double>(row_offset);
            const auto item = "Feature" + std::to_string(row_number);
            const auto owner = "Import" + std::to_string((row_number % 5U) + 1U);
            const auto status =
                (row_number % 2U == 0U) ? "Verified" : "Tracked";
            add_table_row(page, y_points, item, owner, status);
        }

        if (add_footer) {
            page.text_runs.push_back(make_pdf_text_run(
                72.0, 240.0, "Footer note: long repeated-header sample"));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    add_table_page(first_page, 0U, true, false);
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    add_table_page(second_page, 13U, false, false);
    layout.pages.push_back(std::move(second_page));

    featherdoc::pdf::PdfPageLayout third_page;
    add_table_page(third_page, 26U, false, false);
    layout.pages.push_back(std::move(third_page));

    featherdoc::pdf::PdfPageLayout fourth_page;
    add_table_page(fourth_page, 39U, false, true);
    layout.pages.push_back(std::move(fourth_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_summary_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc invoice summary PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Invoice summary"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 676.0, "Invoice No."));
    page.text_runs.push_back(
        make_pdf_text_run(240.0, 676.0, "INV-2026-0507"));
    page.text_runs.push_back(
        make_pdf_text_run(470.0, 676.0, "USD 135.00"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 642.0, "Bill To"));
    page.text_runs.push_back(
        make_pdf_text_run(240.0, 642.0, "FeatherDoc QA"));
    page.text_runs.push_back(
        make_pdf_text_run(470.0, 642.0, "Net 30"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 608.0, "Project"));
    page.text_runs.push_back(
        make_pdf_text_run(240.0, 608.0, "PDF import"));
    page.text_runs.push_back(
        make_pdf_text_run(470.0, 608.0, "Roundtrip"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 548.0, "Footer note: layout is intentionally uneven"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_two_column_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc two-column PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{306.0, 608.0},
        featherdoc::pdf::PdfPoint{306.0, 704.0},
        featherdoc::pdf::PdfRgbColor{0.82, 0.84, 0.88},
        0.75,
    });
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two column sample"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 684.0, "Left column alpha"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 660.0, "Left column beta"));
    page.text_runs.push_back(
        make_pdf_text_run(330.0, 684.0, "Right column alpha"));
    page.text_runs.push_back(
        make_pdf_text_run(330.0, 660.0, "Right column beta"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_out_of_order_two_column_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc out-of-order two-column PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.lines.push_back(featherdoc::pdf::PdfLine{
        featherdoc::pdf::PdfPoint{306.0, 608.0},
        featherdoc::pdf::PdfPoint{306.0, 704.0},
        featherdoc::pdf::PdfRgbColor{0.82, 0.84, 0.88},
        0.75,
    });
    // Append right-column text first so the parser has to recover geometry
    // order instead of trusting content-stream order.
    page.text_runs.push_back(
        make_pdf_text_run(330.0, 660.0, "Right column beta"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 660.0, "Left column beta"));
    page.text_runs.push_back(
        make_pdf_text_run(330.0, 684.0, "Right column alpha"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 684.0, "Left column alpha"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two column sample"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_aligned_list_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc aligned list PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Aligned list sample"));
    page.text_runs.push_back(make_pdf_text_run(72.0, 684.0, "1."));
    page.text_runs.push_back(
        make_pdf_text_run(112.0, 684.0, "First action item"));
    page.text_runs.push_back(make_pdf_text_run(72.0, 660.0, "2."));
    page.text_runs.push_back(
        make_pdf_text_run(112.0, 660.0, "Second action item"));
    page.text_runs.push_back(make_pdf_text_run(72.0, 636.0, "3."));
    page.text_runs.push_back(
        make_pdf_text_run(112.0, 636.0, "Third action item"));
    layout.pages.push_back(std::move(page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

}  // namespace featherdoc::test_support
