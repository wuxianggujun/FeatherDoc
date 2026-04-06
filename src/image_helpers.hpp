#ifndef FEATHERDOC_IMAGE_HELPERS_HPP
#define FEATHERDOC_IMAGE_HELPERS_HPP

#include <cstdint>
#include <filesystem>
#include <string>

#include <constants.hpp>

namespace featherdoc::detail {

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

[[nodiscard]] constexpr auto pixels_to_emu(std::uint32_t pixels) noexcept
    -> std::int64_t {
    return static_cast<std::int64_t>(pixels) * 9525;
}

} // namespace featherdoc::detail

#endif
