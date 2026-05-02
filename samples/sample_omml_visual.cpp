#include <featherdoc.hpp>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace {
constexpr auto display_equation =
    R"(<m:oMathPara xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:oMath><m:r><m:t>sum(i=1..n) i = n(n+1)/2</m:t></m:r></m:oMath></m:oMathPara>)";
constexpr auto placeholder_equation =
    R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>placeholder</m:t></m:r></m:oMath>)";
constexpr auto replacement_equation =
    R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>E=mc^2</m:t></m:r></m:oMath>)";
constexpr auto temporary_equation =
    R"(<m:oMath xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"><m:r><m:t>temporary equation removed before save</m:t></m:r></m:oMath>)";

bool write_sample(const fs::path &output_path) {
    featherdoc::Document document(output_path);
    if (document.create_empty()) {
        std::cerr << "failed to create empty DOCX: " << document.last_error().detail << '\n';
        return false;
    }

    auto body = document.body_template();
    if (!body) {
        std::cerr << "failed to open body template: " << document.last_error().detail << '\n';
        return false;
    }

    if (!body.paragraphs().set_text("OMML typed API visual sample")) {
        std::cerr << "failed to set title paragraph\n";
        return false;
    }

    body.append_paragraph("Display OMML inserted from a raw OMML fragment:");
    if (!body.append_omml(display_equation)) {
        std::cerr << "failed to append display OMML: " << document.last_error().detail << '\n';
        return false;
    }

    body.append_paragraph("Inline OMML after replace_omml(...):");
    if (!body.append_omml(placeholder_equation)) {
        std::cerr << "failed to append placeholder OMML: " << document.last_error().detail << '\n';
        return false;
    }

    if (!body.append_omml(temporary_equation)) {
        std::cerr << "failed to append temporary OMML: " << document.last_error().detail << '\n';
        return false;
    }

    if (!document.replace_omml(1U, replacement_equation)) {
        std::cerr << "failed to replace OMML: " << document.last_error().detail << '\n';
        return false;
    }
    if (!document.remove_omml(2U)) {
        std::cerr << "failed to remove OMML: " << document.last_error().detail << '\n';
        return false;
    }

    body.append_paragraph("Typed OMML builders for n-ary operators and delimiters:");
    if (!body.append_omml(featherdoc::make_omml_nary("∑", "i", "i=1", "n"))) {
        std::cerr << "failed to append typed n-ary OMML: " << document.last_error().detail
                  << '\n';
        return false;
    }
    if (!body.append_omml(featherdoc::make_omml_delimiter("x+1", "[", "]"))) {
        std::cerr << "failed to append typed delimiter OMML: "
                  << document.last_error().detail << '\n';
        return false;
    }

    const auto equations = document.list_omml();
    if (equations.size() != 4U || equations[0].text != "sum(i=1..n) i = n(n+1)/2" ||
        equations[1].text != "E=mc^2" ||
        equations[2].xml.find("m:nary") == std::string::npos ||
        equations[3].xml.find("m:d") == std::string::npos) {
        std::cerr << "unexpected OMML summary after mutation\n";
        return false;
    }

    if (document.save()) {
        std::cerr << "failed to save generated DOCX: " << document.last_error().detail << '\n';
        return false;
    }

    return true;
}
} // namespace

int main(int argc, char **argv) {
    const fs::path output_path =
        argc > 1 ? fs::path(argv[1]) : fs::current_path() / "omml_visual.docx";

    if (output_path.has_parent_path()) {
        std::error_code directory_error;
        fs::create_directories(output_path.parent_path(), directory_error);
        if (directory_error) {
            std::cerr << "failed to create output directory: "
                      << output_path.parent_path().string() << '\n';
            return 1;
        }
    }

    std::error_code remove_error;
    fs::remove(output_path, remove_error);

    if (!write_sample(output_path)) {
        return 1;
    }

    std::cout << "saved sample document to " << output_path.string() << '\n';
    return 0;
}
