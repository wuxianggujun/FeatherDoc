#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

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

auto append_paragraph(featherdoc::Paragraph paragraph, std::string_view text,
                      featherdoc::formatting_flag formatting =
                          featherdoc::formatting_flag::none) -> featherdoc::Paragraph {
    return paragraph.insert_paragraph_after(std::string{text}, formatting);
}

auto append_paragraph_u8(featherdoc::Paragraph paragraph, std::u8string_view text,
                         featherdoc::formatting_flag formatting =
                             featherdoc::formatting_flag::none) -> featherdoc::Paragraph {
    return append_paragraph(paragraph, utf8_from_u8(text), formatting);
}
} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "sample_chinese.docx";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    featherdoc::Document doc(output_path);
    if (const auto error = doc.create_empty()) {
        print_document_error(doc, "create_empty");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    if (!doc.set_default_run_font_family("Segoe UI") ||
        !doc.set_default_run_east_asia_font_family("Microsoft YaHei") ||
        !doc.set_default_run_language("en-US") ||
        !doc.set_default_run_east_asia_language("zh-CN")) {
        print_document_error(doc, "configure default run fonts/languages");
        return 1;
    }

    auto paragraph = doc.paragraphs();
    if (!paragraph.has_next() ||
        !paragraph.add_run(utf8_from_u8(u8"\u0046\u0065\u0061\u0074\u0068\u0065\u0072\u0044\u006f\u0063 \u4e2d\u6587/CJK \u793a\u4f8b"),
                           featherdoc::formatting_flag::bold)
             .has_next()) {
        std::cerr << "failed to write title paragraph\n";
        return 1;
    }

    paragraph = append_paragraph_u8(
        paragraph,
        u8"\u8fd9\u4efd\u6587\u6863\u7531 create_empty() \u76f4\u63a5\u751f\u6210\uff0c"
        u8"\u5e76\u663e\u5f0f\u5199\u5165 East Asia \u5b57\u4f53\u548c\u8bed\u8a00\u5143\u6570\u636e\u3002");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append Chinese overview paragraph\n";
        return 1;
    }

    paragraph = append_paragraph(
        paragraph,
        "Latin text still inherits the default run font/language settings.");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append Latin overview paragraph\n";
        return 1;
    }

    paragraph = append_paragraph(
        paragraph, "Mixed paragraph: ",
        featherdoc::formatting_flag::none);
    if (!paragraph.has_next()) {
        std::cerr << "failed to append mixed paragraph prefix\n";
        return 1;
    }

    auto chinese_run = paragraph.add_run(
        utf8_from_u8(u8"\u4e2d\u6587/CJK \u7247\u6bb5"),
        featherdoc::formatting_flag::bold);
    if (!chinese_run.has_next() || !chinese_run.set_font_family("Segoe UI") ||
        !chinese_run.set_east_asia_font_family("Microsoft YaHei") ||
        !chinese_run.set_language("en-US") ||
        !chinese_run.set_east_asia_language("zh-CN")) {
        print_document_error(doc, "configure mixed run override");
        return 1;
    }

    if (!paragraph.add_run(" with explicit run-level overrides.").has_next()) {
        std::cerr << "failed to append mixed paragraph suffix\n";
        return 1;
    }

    paragraph = append_paragraph_u8(
        paragraph,
        u8"\u4fdd\u5b58\u540e\u53ef\u5728 Word \u4e2d\u68c0\u67e5\uff1a\u4e2d\u6587\u5e94"
        u8"\u4f7f\u7528 Microsoft YaHei\uff0c\u82f1\u6587\u4ecd\u4fdd\u6301 Latin \u5b57\u4f53"
        u8"\u8bbe\u7f6e\u3002");
    if (!paragraph.has_next()) {
        std::cerr << "failed to append review guidance paragraph\n";
        return 1;
    }

    if (const auto error = doc.save()) {
        print_document_error(doc, "save");
        return static_cast<int>(error.value() == 0 ? 1 : error.value());
    }

    std::cout << output_path.string() << '\n';
    return 0;
}
