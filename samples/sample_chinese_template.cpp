#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {
auto utf8_from_u8(std::u8string_view text) -> std::string {
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

void print_document_error(const featherdoc::Document &doc, std::string_view operation) {
    const auto &error_info = doc.last_error();
    std::cerr << operation << " failed";
    if (error_info.code) {
        std::cerr << ": " << error_info.code.message();
    }
    if (!error_info.detail.empty()) {
        std::cerr << " - " << error_info.detail;
    }
    if (!error_info.entry_name.empty()) {
        std::cerr << " [entry=" << error_info.entry_name << ']';
    }
    if (error_info.xml_offset.has_value()) {
        std::cerr << " [xml_offset=" << *error_info.xml_offset << ']';
    }
    std::cerr << '\n';
}

auto default_template_path(const fs::path &argv0) -> fs::path {
    std::error_code error;
    const auto absolute_path = fs::absolute(argv0, error);
    if (!error && absolute_path.has_parent_path()) {
        return absolute_path.parent_path() / "chinese_invoice_template.docx";
    }
    return fs::current_path() / "chinese_invoice_template.docx";
}
} // namespace

int main(int argc, char **argv) {
    fs::path template_path = default_template_path(argv[0]);
    fs::path output_path = fs::current_path() / "sample_chinese_invoice_output.docx";

    if (argc == 2) {
        output_path = fs::path(argv[1]);
    } else if (argc >= 3) {
        template_path = fs::path(argv[1]);
        output_path = fs::path(argv[2]);
    }

    if (!fs::exists(template_path)) {
        std::cerr << "template not found: " << template_path.string() << '\n';
        return 1;
    }

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    featherdoc::Document doc(template_path);
    if (const auto error = doc.open()) {
        print_document_error(doc, "open");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    if (!doc.set_default_run_font_family("Segoe UI") ||
        !doc.set_default_run_east_asia_font_family("Microsoft YaHei") ||
        !doc.set_default_run_language("en-US") ||
        !doc.set_default_run_east_asia_language("zh-CN")) {
        print_document_error(doc, "configure default run fonts/languages");
        return 1;
    }

    const auto fill_result = doc.fill_bookmarks(
        {
            {"customer_name",
             utf8_from_u8(
                 u8"\u4e0a\u6d77\u661f\u7fbd\u79d1\u6280\u6709\u9650\u516c\u53f8")},
            {"invoice_number", "CN-QUOTE-2026-0410"},
            {"issue_date", "2026-04-10"},
        });
    if (!fill_result) {
        std::cerr << "missing bookmarks during fill_bookmarks:\n";
        for (const auto &bookmark_name : fill_result.missing_bookmarks) {
            std::cerr << "  - " << bookmark_name << '\n';
        }
        return 1;
    }

    const std::vector<std::vector<std::string>> item_rows{
        {
            utf8_from_u8(u8"\u9700\u6c42\u68b3\u7406"),
            utf8_from_u8(
                u8"\u68b3\u7406\u4e2d\u6587\u6a21\u677f\u5b57\u6bb5\u3001"
                u8"\u4e66\u7b7e\u4f4d\u7f6e\u4e0e\u4ea4\u4ed8\u7ea6\u675f"),
            "3,200.00",
        },
        {
            utf8_from_u8(u8"\u6587\u6863\u751f\u6210\u5f00\u53d1"),
            utf8_from_u8(
                u8"\u5b9e\u73b0\u4e2d\u6587\u6bb5\u843d\u3001\u8868\u683c"
                u8"\u6837\u5f0f\u4e0e\u81ea\u52a8\u586b\u5145\u6d41\u7a0b"),
            "6,800.00",
        },
        {
            utf8_from_u8(u8"\u9a8c\u6536\u4e0e\u57f9\u8bad"),
            utf8_from_u8(
                u8"\u8f93\u51fa\u4f7f\u7528\u8bf4\u660e\u5e76\u966a\u8dd1"
                u8"\u4e00\u6b21\u4e1a\u52a1\u9a8c\u6536"),
            "2,800.00",
        },
    };
    if (doc.replace_bookmark_with_table_rows("line_item_row", item_rows) != 1U) {
        print_document_error(doc, "replace_bookmark_with_table_rows");
        return 1;
    }

    const std::vector<std::string> note_lines{
        utf8_from_u8(
            u8"1. \u9ed8\u8ba4\u4e2d\u6587\u5b57\u4f53\u4f7f\u7528 Microsoft YaHei"
            u8"\uff0c\u907f\u514d East Asia \u6587\u672c\u56de\u9000\u5230"
            u8"\u4e0d\u53ef\u63a7\u5b57\u4f53\u3002"),
        utf8_from_u8(
            u8"2. \u8868\u683c\u884c\u6765\u81ea\u4e66\u7b7e\u6a21\u677f\u884c"
            u8"\u6269\u5c55\uff0c\u9002\u5408\u62a5\u4ef7\u5355\u3001\u6e05"
            u8"\u5355\u3001\u5bf9\u8d26\u5355\u7b49\u4e1a\u52a1\u573a\u666f\u3002"),
        utf8_from_u8(
            u8"3. \u5982\u9700\u7ee7\u7eed\u6269\u5c55\uff0c\u53ef\u628a\u5907\u6ce8"
            u8"\u3001\u5ba1\u6279\u4eba\u3001\u9875\u7709\u9875\u811a\u4e5f\u8fc1"
            u8"\u79fb\u5230 TemplatePart \u63a5\u53e3\u3002"),
    };
    if (doc.replace_bookmark_with_paragraphs("note_lines", note_lines) != 1U) {
        print_document_error(doc, "replace_bookmark_with_paragraphs");
        return 1;
    }

    if (const auto error = doc.save_as(output_path)) {
        print_document_error(doc, "save_as");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << output_path.string() << '\n';
    return 0;
}
