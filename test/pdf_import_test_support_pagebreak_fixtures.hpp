#pragma once

#include "pdf_import_test_support_common.hpp"

namespace featherdoc::test_support {

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_middle_amount_subtotal_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid middle-amount subtotal PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice middle amount subtotal grid sample"));
    append_grid(page, 72.0, 700.0, 130.0, 28.0, 4U, 6U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 678.0, "Qty"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 678.0, "Unit"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 678.0, "Total"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 650.0, "PDF export design"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 650.0, "2"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 650.0, "USD 50"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 650.0, "USD 100"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 622.0, "Design subtotal"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 622.0, "USD 100"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 594.0, "Visual validation"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 594.0, "1"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 594.0, "USD 25"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 594.0, "USD 25"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 566.0, "Regression evidence"));
    page.text_runs.push_back(make_pdf_text_run(230.0, 566.0, "1"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 566.0, "USD 10"));
    page.text_runs.push_back(make_pdf_text_run(498.0, 566.0, "USD 10"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 538.0, "Grand total"));
    page.text_runs.push_back(make_pdf_text_run(364.0, 538.0, "USD 135"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 492.0, "Footer note: subtotal amount can occupy a middle column"));
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
write_invoice_grid_pagebreak_subtotal_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_invoice_row = [](featherdoc::pdf::PdfPageLayout &page,
                              double y_points, std::string item,
                              std::string qty, std::string unit,
                              std::string total) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(item)));
        if (!qty.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(230.0, y_points, std::move(qty)));
        }
        if (!unit.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(364.0, y_points, std::move(unit)));
        }
        if (!total.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(498.0, y_points, std::move(total)));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Invoice pagebreak subtotal sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(first_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(first_page, 650.0, "PDF export design", "2", "USD 50",
                    "USD 100");
    add_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                    "USD 100");
    add_invoice_row(first_page, 594.0, "Visual validation", "1", "USD 25",
                    "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(second_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(second_page, 650.0, "Regression evidence", "1", "USD 10",
                    "USD 10");
    add_invoice_row(second_page, 622.0, "Documentation pass", "1", "USD 15",
                    "USD 15");
    add_invoice_row(second_page, 594.0, "Grand total", "", "", "USD 150");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: pagebreak subtotal rows merge with the repeated header"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pagebreak_subtotal_missing_unit_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal missing-unit PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_invoice_row = [](featherdoc::pdf::PdfPageLayout &page,
                              double y_points, std::string item,
                              std::string qty, std::string unit,
                              std::string total) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(item)));
        if (!qty.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(230.0, y_points, std::move(qty)));
        }
        if (!unit.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(364.0, y_points, std::move(unit)));
        }
        if (!total.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(498.0, y_points, std::move(total)));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice pagebreak subtotal missing unit sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(first_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(first_page, 650.0, "PDF export design", "2", "USD 50",
                    "USD 100");
    add_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                    "USD 100");
    add_invoice_row(first_page, 594.0, "Visual validation", "1", "USD 25",
                    "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(second_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(second_page, 650.0, "Regression evidence", "1", "",
                    "USD 10");
    add_invoice_row(second_page, 622.0, "Documentation pass", "1", "USD 15",
                    "USD 15");
    add_invoice_row(second_page, 594.0, "Grand total", "", "", "USD 150");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: missing Unit cell still merges with the repeated header"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pagebreak_subtotal_sparse_body_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal sparse-body PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_invoice_row = [](featherdoc::pdf::PdfPageLayout &page,
                              double y_points, std::string item,
                              std::string qty, std::string unit,
                              std::string total) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(item)));
        if (!qty.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(230.0, y_points, std::move(qty)));
        }
        if (!unit.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(364.0, y_points, std::move(unit)));
        }
        if (!total.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(498.0, y_points, std::move(total)));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice pagebreak subtotal sparse body sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(first_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(first_page, 650.0, "PDF export design", "2", "USD 50",
                    "USD 100");
    add_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                    "USD 100");
    add_invoice_row(first_page, 594.0, "Visual validation", "1", "USD 25",
                    "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(second_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(second_page, 650.0, "Sparse evidence", "", "",
                    "USD 10");
    add_invoice_row(second_page, 622.0, "Documentation pass", "1", "USD 15",
                    "USD 15");
    add_invoice_row(second_page, 594.0, "Grand total", "", "", "USD 150");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: sparse body row still merges with the repeated header"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pagebreak_subtotal_amount_only_body_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal amount-only-body PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_invoice_row = [](featherdoc::pdf::PdfPageLayout &page,
                              double y_points, std::string item,
                              std::string qty, std::string unit,
                              std::string total) {
        if (!item.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(86.0, y_points, std::move(item)));
        }
        if (!qty.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(230.0, y_points, std::move(qty)));
        }
        if (!unit.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(364.0, y_points, std::move(unit)));
        }
        if (!total.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(498.0, y_points, std::move(total)));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice pagebreak subtotal amount only body sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(first_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(first_page, 650.0, "PDF export design", "2", "USD 50",
                    "USD 100");
    add_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                    "USD 100");
    add_invoice_row(first_page, 594.0, "Visual validation", "1", "USD 25",
                    "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(second_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(second_page, 650.0, "", "", "", "USD 10");
    add_invoice_row(second_page, 622.0, "Documentation pass", "1", "USD 15",
                    "USD 15");
    add_invoice_row(second_page, 594.0, "Grand total", "", "", "USD 150");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: amount-only body row still aligns with the repeated header"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pagebreak_subtotal_header_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal header-variant PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_invoice_row = [](featherdoc::pdf::PdfPageLayout &page,
                              double y_points, std::string item,
                              std::string quantity, std::string unit,
                              std::string amount) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(item)));
        if (!quantity.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(230.0, y_points, std::move(quantity)));
        }
        if (!unit.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(364.0, y_points, std::move(unit)));
        }
        if (!amount.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(498.0, y_points, std::move(amount)));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice pagebreak subtotal header variant sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(first_page, 678.0, "Item", "Quantity", "Unit", "Amount");
    add_invoice_row(first_page, 650.0, "PDF export design", "2", "USD 50",
                    "USD 100");
    add_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                    "USD 100");
    add_invoice_row(first_page, 594.0, "Visual validation", "1", "USD 25",
                    "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(second_page, 678.0, "Item", "Qty", "Unit", "Amt");
    add_invoice_row(second_page, 650.0, "Regression evidence", "1", "USD 10",
                    "USD 10");
    add_invoice_row(second_page, 622.0, "Documentation pass", "1", "USD 15",
                    "USD 15");
    add_invoice_row(second_page, 594.0, "Grand total", "", "", "USD 150");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: abbreviated repeated header subtotal rows merge cleanly"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pagebreak_subtotal_semantic_header_variant_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal semantic-header-variant PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_invoice_row = [](featherdoc::pdf::PdfPageLayout &page,
                              double y_points, std::string first,
                              std::string second, std::string third,
                              std::string fourth) {
        page.text_runs.push_back(
            make_pdf_text_run(86.0, y_points, std::move(first)));
        if (!second.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(230.0, y_points, std::move(second)));
        }
        if (!third.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(364.0, y_points, std::move(third)));
        }
        if (!fourth.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(498.0, y_points, std::move(fourth)));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice pagebreak subtotal semantic header variant sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(first_page, 678.0, "Item", "Quantity", "Unit", "Amount");
    add_invoice_row(first_page, 650.0, "PDF export design", "2", "USD 50",
                    "USD 100");
    add_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                    "USD 100");
    add_invoice_row(first_page, 594.0, "Visual validation", "1", "USD 25",
                    "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(second_page, 678.0, "Item", "Owner", "Phase", "Amount");
    add_invoice_row(second_page, 650.0, "Regression evidence",
                    "Import lane", "Review pass", "USD 10");
    add_invoice_row(second_page, 622.0, "Documentation pass", "Docs team",
                    "Approval", "USD 15");
    add_invoice_row(second_page, 594.0, "Grand total", "", "", "USD 25");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: semantic repeated header variant stays separate"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pagebreak_subtotal_isolated_amount_only_body_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal isolated amount-only-body PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_invoice_row = [](featherdoc::pdf::PdfPageLayout &page,
                              double y_points, std::string item,
                              std::string qty, std::string unit,
                              std::string total) {
        if (!item.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(86.0, y_points, std::move(item)));
        }
        if (!qty.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(230.0, y_points, std::move(qty)));
        }
        if (!unit.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(364.0, y_points, std::move(unit)));
        }
        if (!total.empty()) {
            page.text_runs.push_back(
                make_pdf_text_run(498.0, y_points, std::move(total)));
        }
    };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0,
        "Invoice pagebreak subtotal isolated amount only body sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_invoice_row(first_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(first_page, 650.0, "PDF export design", "2", "USD 50",
                    "USD 100");
    add_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                    "USD 100");
    add_invoice_row(first_page, 594.0, "Visual validation", "1", "USD 25",
                    "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 130.0, 28.0, 4U, 3U);
    add_invoice_row(second_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_invoice_row(second_page, 650.0, "", "", "", "USD 10");
    add_invoice_row(second_page, 622.0, "Grand total", "", "", "USD 110");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 568.0,
        "Footer note: isolated amount-only body row still aligns with the repeated header"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

[[nodiscard]] inline std::filesystem::path
write_invoice_grid_pagebreak_subtotal_anchor_mismatch_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc invoice grid pagebreak subtotal anchor-mismatch PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    auto add_standard_invoice_row =
        [](featherdoc::pdf::PdfPageLayout &page, double y_points,
           std::string item, std::string qty, std::string unit,
           std::string total) {
            page.text_runs.push_back(
                make_pdf_text_run(86.0, y_points, std::move(item)));
            if (!qty.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(230.0, y_points, std::move(qty)));
            }
            if (!unit.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(364.0, y_points, std::move(unit)));
            }
            if (!total.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(498.0, y_points, std::move(total)));
            }
        };

    auto add_wide_invoice_row =
        [](featherdoc::pdf::PdfPageLayout &page, double y_points,
           std::string item, std::string qty, std::string unit,
           std::string total) {
            page.text_runs.push_back(
                make_pdf_text_run(86.0, y_points, std::move(item)));
            if (!qty.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(252.0, y_points, std::move(qty)));
            }
            if (!unit.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(408.0, y_points, std::move(unit)));
            }
            if (!total.empty()) {
                page.text_runs.push_back(
                    make_pdf_text_run(564.0, y_points, std::move(total)));
            }
        };

    featherdoc::pdf::PdfPageLayout first_page;
    first_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    first_page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Invoice pagebreak subtotal anchor mismatch sample"));
    append_grid(first_page, 72.0, 700.0, 130.0, 28.0, 4U, 4U);
    add_standard_invoice_row(first_page, 678.0, "Item", "Qty", "Unit",
                             "Total");
    add_standard_invoice_row(first_page, 650.0, "PDF export design", "2",
                             "USD 50", "USD 100");
    add_standard_invoice_row(first_page, 622.0, "Design subtotal", "", "",
                             "USD 100");
    add_standard_invoice_row(first_page, 594.0, "Visual validation", "1",
                             "USD 25", "USD 25");
    layout.pages.push_back(std::move(first_page));

    featherdoc::pdf::PdfPageLayout second_page;
    second_page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_grid(second_page, 72.0, 700.0, 150.0, 28.0, 4U, 4U);
    add_wide_invoice_row(second_page, 678.0, "Item", "Qty", "Unit", "Total");
    add_wide_invoice_row(second_page, 650.0, "Regression evidence", "1",
                         "USD 10", "USD 10");
    add_wide_invoice_row(second_page, 622.0, "Documentation pass", "1",
                         "USD 15", "USD 15");
    add_wide_invoice_row(second_page, 594.0, "Grand total", "", "",
                         "USD 150");
    second_page.text_runs.push_back(make_pdf_text_run(
        72.0, 540.0,
        "Footer note: anchor-mismatched subtotal table stays separate"));
    layout.pages.push_back(std::move(second_page));

    const auto output_path =
        std::filesystem::current_path() / std::string{filename};

    featherdoc::pdf::PdfioGenerator generator;
    const auto write_result =
        generator.write(layout, output_path, featherdoc::pdf::PdfWriterOptions{});
    REQUIRE_MESSAGE(write_result.success, write_result.error_message);
    return output_path;
}

}  // namespace featherdoc::test_support
