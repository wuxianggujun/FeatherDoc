/*
 * FeatherDoc experimental PDF font resolver.
 *
 * This keeps font family selection separate from PDF byte writing and layout
 * measurement. It deliberately stays small: explicit mappings first, then
 * configured defaults, then conservative system fallbacks.
 */

#ifndef FEATHERDOC_PDF_PDF_FONT_RESOLVER_HPP
#define FEATHERDOC_PDF_PDF_FONT_RESOLVER_HPP

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace featherdoc::pdf {

struct PdfFontMapping {
    std::string font_family;
    std::filesystem::path font_file_path;
    bool bold{false};
    bool italic{false};
};

struct PdfFontResolverOptions {
    std::vector<PdfFontMapping> font_mappings;
    std::filesystem::path default_font_file_path;
    std::filesystem::path default_cjk_font_file_path;
    bool use_system_font_fallbacks{true};
};

struct PdfResolvedFont {
    std::string font_family{"Helvetica"};
    std::filesystem::path font_file_path;
    bool unicode{false};
};

class PdfFontResolver final {
  public:
    explicit PdfFontResolver(PdfFontResolverOptions options = {});

    [[nodiscard]] PdfResolvedFont
    resolve(std::string_view font_family,
            std::string_view east_asia_font_family,
            std::string_view text) const;

    [[nodiscard]] PdfResolvedFont
    resolve(std::string_view font_family,
            std::string_view east_asia_font_family, std::string_view text,
            bool bold, bool italic) const;

    [[nodiscard]] const PdfFontResolverOptions &options() const noexcept;

  private:
    PdfFontResolverOptions options_;
};

[[nodiscard]] bool pdf_text_requires_unicode(std::string_view text) noexcept;
[[nodiscard]] bool pdf_text_contains_cjk(std::string_view text) noexcept;

} // namespace featherdoc::pdf

#endif // FEATHERDOC_PDF_PDF_FONT_RESOLVER_HPP
