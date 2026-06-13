#pragma once

#include "pdf_import_test_support_common.hpp"

namespace featherdoc::test_support {

[[nodiscard]] inline std::filesystem::path
write_import_structure_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(720.0, "First paragraph line one"));
    page.text_runs.push_back(
        make_pdf_text_run(702.0, "First paragraph line two"));
    page.text_runs.push_back(
        make_pdf_text_run(650.0, "Second paragraph starts"));
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
write_blank_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc blank PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
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
write_table_like_grid_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc table-like grid PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(page, 72.0, 700.0, 120.0, 32.0);

    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Grid sample header"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Cell A1"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 646.0, "Cell B2"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 614.0, "Cell C3"));
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
write_two_row_header_data_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc two-row header-data table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two-row table sample"));
    append_grid(page, 72.0, 680.0, 132.0, 32.0, 3U, 2U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 658.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 658.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 658.0, "Due"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 626.0, "INV-2026-05"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 626.0, "QA Team"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 626.0, "2026-05-14"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 570.0, "Tail paragraph after two-row table"));
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
write_two_column_key_value_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc two-column key-value table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Key-value table sample"));
    append_grid(page, 72.0, 680.0, 180.0, 30.0, 2U, 4U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 660.0, "Invoice No"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 660.0, "INV-2026-0514"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 630.0, "Customer"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 630.0, "FeatherDoc QA"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 600.0, "Due Date"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 600.0, "2026-05-14"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 570.0, "Total"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 570.0, "USD 480"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 518.0, "Tail paragraph after key-value table"));
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
write_two_column_borderless_key_value_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc borderless key-value table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Borderless key-value sample"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 660.0, "Invoice No"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 660.0, "INV-2026-0610"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 630.0, "Customer"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 630.0, "FeatherDoc Ops"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 600.0, "Due Date"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 600.0, "2026-06-10"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 570.0, "Amount"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 570.0, "USD 960"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 518.0, "Tail paragraph after borderless key-value table"));
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
write_irregular_width_header_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc irregular-width table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Irregular width table sample"));
    append_irregular_grid(page, 72.0, 680.0, {92.0, 210.0, 96.0}, 30.0, 4U);
    page.text_runs.push_back(make_pdf_text_run(86.0, 660.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 660.0, "Description"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 660.0, "Amount"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 630.0, "SKU-01"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 630.0, "Annual subscription"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 630.0, "USD 700"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 600.0, "SKU-02"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 600.0, "Visual support"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 600.0, "USD 180"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 570.0, "Total"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 570.0, "Invoice 2026"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 570.0, "USD 880"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 518.0, "Tail paragraph after irregular width table"));
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
write_borderless_irregular_width_header_table_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc borderless irregular-width table PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Borderless irregular width table sample"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 660.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 660.0, "Description"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 660.0, "Amount"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 630.0, "SKU-11"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 630.0, "Document import"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 630.0, "USD 420"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 600.0, "SKU-12"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 600.0, "Visual evidence"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 600.0, "USD 260"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 570.0, "Total"));
    page.text_runs.push_back(make_pdf_text_run(178.0, 570.0, "Import batch 77"));
    page.text_runs.push_back(make_pdf_text_run(388.0, 570.0, "USD 680"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 518.0, "Tail paragraph after borderless irregular width table"));
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
write_two_column_short_label_prose_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc two-column short-label prose PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two-column labels sample"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Topic"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 678.0, "Scope"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 646.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 646.0, "Review"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 614.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(266.0, 614.0, "Closed"));
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
write_two_row_three_column_prose_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc two-row three-column prose PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Two-row prose sample"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Step 1"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 678.0, "Build 2"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 678.0, "Ship 3"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 646.0, "Alpha note"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 646.0, "Beta note"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 646.0, "Gamma note"));
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
write_free_form_column_drift_prose_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc free-form column drift prose PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Free-form column drift sample"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 678.0, "Topic"));
    page.text_runs.push_back(make_pdf_text_run(218.0, 678.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 678.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(118.0, 646.0, "Scope"));
    page.text_runs.push_back(make_pdf_text_run(278.0, 646.0, "Review"));
    page.text_runs.push_back(make_pdf_text_run(430.0, 646.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(74.0, 614.0, "Evidence"));
    page.text_runs.push_back(make_pdf_text_run(254.0, 614.0, "Imported"));
    page.text_runs.push_back(make_pdf_text_run(506.0, 614.0, "Closed"));
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
write_table_first_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title = "FeatherDoc table-first PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    append_three_by_three_grid(page, 72.0, 700.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 678.0, "Cell A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 646.0, "Cell B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 614.0, "Cell C3"));
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
write_paragraph_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph line one"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 706.0, "Intro paragraph line two"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Cell A1"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Cell B2"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Cell C3"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 522.0, "Tail paragraph after table"));
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
write_paragraph_merged_header_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-merged-header-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 724.0, "Intro paragraph before merged table"));
    append_three_by_three_grid(page, 72.0, 664.0, 120.0, 32.0);
    // 第一行左侧故意放一条明显宽于单列的文本簇，回归最小横向合并导入。
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 642.0, "Merged header spans two cols"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 642.0, "Status"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 610.0, "Design"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Alice"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 610.0, "Open"));
    page.text_runs.push_back(
        make_pdf_text_run(86.0, 578.0, "Review"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 578.0, "Bob"));
    page.text_runs.push_back(
        make_pdf_text_run(326.0, 578.0, "Done"));
    page.text_runs.push_back(
        make_pdf_text_run(72.0, 522.0, "Tail paragraph after merged table"));
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
write_paragraph_vertical_merged_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-vertical-merged-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before vertical merged table"));
    append_three_by_three_grid_with_first_column_row_merge(
        page, 72.0, 664.0, 120.0, 32.0);
    // 第一列顶部单元格视觉上跨两行，回归最小纵向合并导入。
    page.text_runs.push_back(make_pdf_text_run(86.0, 642.0, "Project"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 642.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 642.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 610.0, "Alpha"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 610.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 578.0, "Milestone"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 578.0, "Beta"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 578.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 522.0, "Tail paragraph after vertical merged table"));
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
write_paragraph_middle_column_merged_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-middle-column-merged-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before vertical merged table"));
    append_three_by_three_grid_with_middle_column_row_merge(
        page, 72.0, 664.0, 120.0, 32.0);
    // 中间列顶部单元格视觉上跨三行，回归更通用的纵向合并导入。
    page.text_runs.push_back(make_pdf_text_run(86.0, 642.0, "Project"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 642.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 642.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 610.0, "Alpha"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 610.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 578.0, "Milestone"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 578.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 522.0, "Tail paragraph after vertical merged table"));
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
write_paragraph_merged_corner_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-merged-corner-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before merged corner table"));
    append_four_by_three_grid_with_top_left_two_by_two_merge(
        page, 72.0, 664.0, 130.0, 32.0);
    // 左上角单元格视觉上跨两列两行，回归组合合并导入。
    page.text_runs.push_back(make_pdf_text_run(
        86.0, 642.0, "Project overview and owner assignment"));
    page.text_runs.push_back(make_pdf_text_run(346.0, 642.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(476.0, 642.0, "Phase"));
    page.text_runs.push_back(make_pdf_text_run(346.0, 610.0, "Alpha"));
    page.text_runs.push_back(make_pdf_text_run(476.0, 610.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 578.0, "Milestone"));
    page.text_runs.push_back(make_pdf_text_run(216.0, 578.0, "Beta"));
    page.text_runs.push_back(make_pdf_text_run(346.0, 578.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(476.0, 578.0, "Closed"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 522.0, "Tail paragraph after merged corner table"));
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
write_paragraph_center_merged_table_paragraph_pdf(std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-center-merged-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before center merged table"));
    append_four_by_four_grid_with_center_two_by_two_merge(
        page, 72.0, 664.0, 120.0, 32.0);
    page.text_runs.push_back(make_pdf_text_run(86.0, 642.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 642.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 642.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(446.0, 642.0, "Phase"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 610.0, "Design"));
    page.text_runs.push_back(
        make_pdf_text_run(206.0, 610.0, "Owner assignment spans review"));
    page.text_runs.push_back(make_pdf_text_run(446.0, 610.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 578.0, "Review"));
    page.text_runs.push_back(make_pdf_text_run(446.0, 578.0, "Tracked"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 546.0, "Ship"));
    page.text_runs.push_back(make_pdf_text_run(206.0, 546.0, "Dana"));
    page.text_runs.push_back(make_pdf_text_run(326.0, 546.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(446.0, 546.0, "Closed"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 490.0, "Tail paragraph after center merged table"));
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
write_paragraph_rectangular_merged_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-rectangular-merged-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        56.0, 724.0, "Intro paragraph before rectangular merged table"));
    append_five_by_five_grid_with_center_two_by_three_merge(
        page, 56.0, 664.0, 100.0, 32.0);
    page.text_runs.push_back(make_pdf_text_run(70.0, 642.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(170.0, 642.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(270.0, 642.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(370.0, 642.0, "Phase"));
    page.text_runs.push_back(make_pdf_text_run(470.0, 642.0, "Flag"));
    page.text_runs.push_back(make_pdf_text_run(70.0, 610.0, "Design"));
    page.text_runs.push_back(
        make_pdf_text_run(170.0, 610.0, "Owner assignment spans review"));
    page.text_runs.push_back(make_pdf_text_run(370.0, 610.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(470.0, 610.0, "Risk"));
    page.text_runs.push_back(make_pdf_text_run(70.0, 578.0, "Build"));
    page.text_runs.push_back(make_pdf_text_run(370.0, 578.0, "Tracked"));
    page.text_runs.push_back(make_pdf_text_run(470.0, 578.0, "Med"));
    page.text_runs.push_back(make_pdf_text_run(70.0, 546.0, "Review"));
    page.text_runs.push_back(make_pdf_text_run(370.0, 546.0, "Waiting"));
    page.text_runs.push_back(make_pdf_text_run(470.0, 546.0, "Low"));
    page.text_runs.push_back(make_pdf_text_run(70.0, 514.0, "Ship"));
    page.text_runs.push_back(make_pdf_text_run(170.0, 514.0, "Dana"));
    page.text_runs.push_back(make_pdf_text_run(270.0, 514.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(370.0, 514.0, "Closed"));
    page.text_runs.push_back(make_pdf_text_run(470.0, 514.0, "Green"));
    page.text_runs.push_back(make_pdf_text_run(
        56.0, 458.0, "Tail paragraph after rectangular merged table"));
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
write_paragraph_cross_column_header_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-cross-column-header-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before cross-column header table"));
    append_four_column_grid_with_cross_column_header(page, 72.0, 664.0, 110.0,
                                                     30.0, 4U);
    page.text_runs.push_back(
        make_pdf_text_run(186.0, 644.0, "Project delivery overview"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 614.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 614.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 614.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 614.0, "Due"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 584.0, "DOC-17"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 584.0, "Alice"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 584.0, "Open"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 584.0, "2026-05-14"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 554.0, "DOC-18"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 554.0, "Bob"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 554.0, "Done"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 554.0, "2026-05-21"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 506.0, "Tail paragraph after cross-column header table"));
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
write_paragraph_parallel_group_header_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-parallel-group-header-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before parallel group header table"));
    append_four_column_grid_with_parallel_group_headers(page, 72.0, 664.0,
                                                        110.0, 30.0, 4U);
    page.text_runs.push_back(make_pdf_text_run(130.0, 644.0, "Delivery scope"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 644.0, "Review status"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 614.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 614.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 614.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 614.0, "Due"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 584.0, "DOC-27"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 584.0, "Carol"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 584.0, "Review"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 584.0, "2026-06-04"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 554.0, "DOC-28"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 554.0, "Drew"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 554.0, "Closed"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 554.0, "2026-06-11"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 506.0, "Tail paragraph after parallel group header table"));
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
write_paragraph_multilevel_group_header_table_paragraph_pdf(
    std::string_view filename) {
    featherdoc::pdf::PdfDocumentLayout layout;
    layout.metadata.title =
        "FeatherDoc paragraph-multilevel-group-header-table-paragraph PDF import structure";
    layout.metadata.creator = "FeatherDoc test";

    featherdoc::pdf::PdfPageLayout page;
    page.size = featherdoc::pdf::PdfPageSize::letter_portrait();
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 724.0, "Intro paragraph before multilevel group header table"));
    append_four_column_grid_with_multilevel_group_headers(
        page, 72.0, 664.0, 110.0, 28.0, 5U);
    page.text_runs.push_back(
        make_pdf_text_run(178.0, 645.0, "Program delivery dashboard"));
    page.text_runs.push_back(make_pdf_text_run(130.0, 617.0, "Delivery scope"));
    page.text_runs.push_back(make_pdf_text_run(350.0, 617.0, "Review status"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 589.0, "Item"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 589.0, "Owner"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 589.0, "Status"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 589.0, "Due"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 561.0, "DOC-37"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 561.0, "Eve"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 561.0, "Review"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 561.0, "2026-06-18"));
    page.text_runs.push_back(make_pdf_text_run(86.0, 533.0, "DOC-38"));
    page.text_runs.push_back(make_pdf_text_run(196.0, 533.0, "Finn"));
    page.text_runs.push_back(make_pdf_text_run(306.0, 533.0, "Ready"));
    page.text_runs.push_back(make_pdf_text_run(416.0, 533.0, "2026-06-25"));
    page.text_runs.push_back(make_pdf_text_run(
        72.0, 486.0, "Tail paragraph after multilevel group header table"));
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
