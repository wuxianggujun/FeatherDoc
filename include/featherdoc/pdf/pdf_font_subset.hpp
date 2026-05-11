/*
 * FeatherDoc experimental PDF font subsetting helpers.
 *
 * Keep this behind the PDF writer boundary: layout still deals in text runs and
 * concrete font paths, while the writer decides whether a smaller embedded font
 * can be produced for the current PDF resource.
 */

#ifndef FEATHERDOC_PDF_PDF_FONT_SUBSET_HPP
#define FEATHERDOC_PDF_PDF_FONT_SUBSET_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace featherdoc::pdf {

struct PdfFontSubsetResult {
    bool success{false};
    std::vector<unsigned char> font_data;
    std::string error_message;
};

[[nodiscard]] PdfFontSubsetResult
subset_font_file_for_codepoints(const std::filesystem::path &font_file_path,
                                const std::vector<std::uint32_t> &codepoints);

} // namespace featherdoc::pdf

#endif // FEATHERDOC_PDF_PDF_FONT_SUBSET_HPP
