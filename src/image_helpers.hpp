#ifndef FEATHERDOC_IMAGE_HELPERS_HPP
#define FEATHERDOC_IMAGE_HELPERS_HPP

#include <cstdint>
#include <filesystem>
#include <istream>
#include <string>

#include <constants.hpp>

namespace featherdoc::detail {

inline constexpr std::uint64_t max_external_image_file_bytes =
    256ULL * 1024ULL * 1024ULL;

struct image_file_info final {
    std::string extension;
    std::string content_type;
    std::string data;
    std::uint32_t width_px{0U};
    std::uint32_t height_px{0U};
};

[[nodiscard]] bool load_image_file(const std::filesystem::path &image_path,
                                   image_file_info &image_info,
                                   featherdoc::document_errc &error_code,
                                   std::string &detail);

[[nodiscard]] bool load_image_file(const std::filesystem::path &image_path,
                                   image_file_info &image_info,
                                   featherdoc::document_errc &error_code,
                                   std::string &detail,
                                   std::uint64_t max_file_bytes);

[[nodiscard]] bool read_image_stream_with_limit(
    std::istream &stream, const std::filesystem::path &image_path,
    std::uint64_t max_file_bytes, std::string &data,
    featherdoc::document_errc &error_code, std::string &detail);

[[nodiscard]] constexpr auto pixels_to_emu(std::uint32_t pixels) noexcept
    -> std::int64_t {
    return static_cast<std::int64_t>(pixels) * 9525;
}

} // namespace featherdoc::detail

#endif
