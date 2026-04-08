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

void print_document_error(const featherdoc::Document &doc,
                          std::string_view operation) {
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
    fs::path output_path = fs::current_path() / "remove_bookmark_block.docx";

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
            {"invoice_number", "CN-QUOTE-2026-0411"},
            {"issue_date", "2026-04-11"},
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
            utf8_from_u8(u8"\u6a21\u677f\u6574\u7406"),
            utf8_from_u8(
                u8"\u4fdd\u7559\u62a5\u4ef7\u5355\u4e3b\u4f53\uff0c\u79fb\u9664"
                u8"\u53ef\u9009\u5907\u6ce8\u5757"),
            "1,800.00",
        },
        {
            utf8_from_u8(u8"\u53ef\u89c6\u5316\u590d\u6838"),
            utf8_from_u8(
                u8"\u751f\u6210 Word \u6e32\u67d3\u6587\u6863\u5e76\u786e\u8ba4"
                u8"\u5907\u6ce8\u5757\u4e0d\u518d\u51fa\u73b0"),
            "600.00",
        },
    };
    if (doc.replace_bookmark_with_table_rows("line_item_row", item_rows) != 1U) {
        print_document_error(doc, "replace_bookmark_with_table_rows");
        return 1;
    }

    if (doc.remove_bookmark_block("note_lines") != 1U) {
        print_document_error(doc, "remove_bookmark_block");
        return 1;
    }

    if (const auto error = doc.save_as(output_path)) {
        print_document_error(doc, "save_as");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << output_path.string() << '\n';
    std::cout << "removed standalone bookmark block: note_lines\n";
    return 0;
}
