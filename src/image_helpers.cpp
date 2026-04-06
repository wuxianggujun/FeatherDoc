#include "image_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <string_view>

namespace featherdoc::detail {
namespace {

auto to_lower_ascii(std::string value) -> std::string {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

auto read_u16_le(const std::string &data, std::size_t offset) -> std::uint16_t {
    return static_cast<std::uint16_t>(
        static_cast<unsigned char>(data[offset]) |
        (static_cast<unsigned char>(data[offset + 1U]) << 8U));
}

auto read_u16_be(const std::string &data, std::size_t offset) -> std::uint16_t {
    return static_cast<std::uint16_t>(
        (static_cast<unsigned char>(data[offset]) << 8U) |
        static_cast<unsigned char>(data[offset + 1U]));
}

auto read_u32_le(const std::string &data, std::size_t offset) -> std::uint32_t {
    return static_cast<std::uint32_t>(
        static_cast<unsigned char>(data[offset]) |
        (static_cast<unsigned char>(data[offset + 1U]) << 8U) |
        (static_cast<unsigned char>(data[offset + 2U]) << 16U) |
        (static_cast<unsigned char>(data[offset + 3U]) << 24U));
}

auto read_u32_be(const std::string &data, std::size_t offset) -> std::uint32_t {
    return static_cast<std::uint32_t>(
        (static_cast<unsigned char>(data[offset]) << 24U) |
        (static_cast<unsigned char>(data[offset + 1U]) << 16U) |
        (static_cast<unsigned char>(data[offset + 2U]) << 8U) |
        static_cast<unsigned char>(data[offset + 3U]));
}

auto detect_png_dimensions(const std::string &data, std::uint32_t &width,
                           std::uint32_t &height) -> bool {
    constexpr unsigned char png_signature[] = {0x89U, 0x50U, 0x4EU, 0x47U,
                                               0x0DU, 0x0AU, 0x1AU, 0x0AU};
    if (data.size() < 24U) {
        return false;
    }

    for (std::size_t i = 0; i < std::size(png_signature); ++i) {
        if (static_cast<unsigned char>(data[i]) != png_signature[i]) {
            return false;
        }
    }

    if (std::string_view{data.data() + 12, 4} != "IHDR") {
        return false;
    }

    width = read_u32_be(data, 16U);
    height = read_u32_be(data, 20U);
    return width > 0U && height > 0U;
}

auto detect_gif_dimensions(const std::string &data, std::uint32_t &width,
                           std::uint32_t &height) -> bool {
    if (data.size() < 10U) {
        return false;
    }

    const auto header = std::string_view{data.data(), 6U};
    if (header != "GIF87a" && header != "GIF89a") {
        return false;
    }

    width = read_u16_le(data, 6U);
    height = read_u16_le(data, 8U);
    return width > 0U && height > 0U;
}

auto detect_bmp_dimensions(const std::string &data, std::uint32_t &width,
                           std::uint32_t &height) -> bool {
    if (data.size() < 26U || data[0] != 'B' || data[1] != 'M') {
        return false;
    }

    const auto dib_header_size = read_u32_le(data, 14U);
    if (dib_header_size < 40U || data.size() < 26U) {
        return false;
    }

    width = read_u32_le(data, 18U);
    auto raw_height = static_cast<std::int32_t>(read_u32_le(data, 22U));
    if (raw_height < 0) {
        raw_height = -raw_height;
    }
    height = static_cast<std::uint32_t>(raw_height);
    return width > 0U && height > 0U;
}

auto is_jpeg_sof_marker(unsigned char marker) -> bool {
    switch (marker) {
    case 0xC0U:
    case 0xC1U:
    case 0xC2U:
    case 0xC3U:
    case 0xC5U:
    case 0xC6U:
    case 0xC7U:
    case 0xC9U:
    case 0xCAU:
    case 0xCBU:
    case 0xCDU:
    case 0xCEU:
    case 0xCFU:
        return true;
    default:
        return false;
    }
}

auto detect_jpeg_dimensions(const std::string &data, std::uint32_t &width,
                            std::uint32_t &height) -> bool {
    if (data.size() < 4U || static_cast<unsigned char>(data[0]) != 0xFFU ||
        static_cast<unsigned char>(data[1]) != 0xD8U) {
        return false;
    }

    std::size_t offset = 2U;
    while (offset + 1U < data.size()) {
        if (static_cast<unsigned char>(data[offset]) != 0xFFU) {
            ++offset;
            continue;
        }

        while (offset < data.size() &&
               static_cast<unsigned char>(data[offset]) == 0xFFU) {
            ++offset;
        }
        if (offset >= data.size()) {
            break;
        }

        const auto marker = static_cast<unsigned char>(data[offset++]);
        if (marker == 0xD9U || marker == 0xDAU) {
            break;
        }

        if (marker == 0x01U || (marker >= 0xD0U && marker <= 0xD7U)) {
            continue;
        }

        if (offset + 1U >= data.size()) {
            break;
        }

        const auto segment_length = read_u16_be(data, offset);
        if (segment_length < 2U || offset + segment_length > data.size()) {
            break;
        }

        if (is_jpeg_sof_marker(marker)) {
            if (segment_length < 7U || offset + 6U >= data.size()) {
                break;
            }

            height = read_u16_be(data, offset + 3U);
            width = read_u16_be(data, offset + 5U);
            return width > 0U && height > 0U;
        }

        offset += segment_length;
    }

    return false;
}

auto detect_image_dimensions(const std::string &extension, const std::string &data,
                             std::uint32_t &width, std::uint32_t &height) -> bool {
    if (extension == "png") {
        return detect_png_dimensions(data, width, height);
    }
    if (extension == "jpg" || extension == "jpeg") {
        return detect_jpeg_dimensions(data, width, height);
    }
    if (extension == "gif") {
        return detect_gif_dimensions(data, width, height);
    }
    if (extension == "bmp") {
        return detect_bmp_dimensions(data, width, height);
    }
    return false;
}

auto image_content_type_for_extension(const std::string &extension) -> std::string {
    if (extension == "png") {
        return "image/png";
    }
    if (extension == "jpg" || extension == "jpeg") {
        return "image/jpeg";
    }
    if (extension == "gif") {
        return "image/gif";
    }
    if (extension == "bmp") {
        return "image/bmp";
    }
    return {};
}

} // namespace

bool load_image_file(const std::filesystem::path &image_path, image_file_info &image_info,
                     featherdoc::document_errc &error_code, std::string &detail) {
    image_info = {};
    error_code = featherdoc::document_errc::success;
    detail.clear();

    if (image_path.empty()) {
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "image path must not be empty";
        return false;
    }

    std::ifstream stream(image_path, std::ios::binary);
    if (!stream) {
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "failed to open image file '" + image_path.string() + "'";
        return false;
    }

    image_info.data.assign(std::istreambuf_iterator<char>{stream},
                           std::istreambuf_iterator<char>{});
    if (!stream.good() && !stream.eof()) {
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "failed while reading image file '" + image_path.string() + "'";
        return false;
    }

    image_info.extension = image_path.extension().string();
    if (!image_info.extension.empty() && image_info.extension.front() == '.') {
        image_info.extension.erase(image_info.extension.begin());
    }
    image_info.extension = to_lower_ascii(std::move(image_info.extension));
    image_info.content_type = image_content_type_for_extension(image_info.extension);
    if (image_info.content_type.empty()) {
        error_code = featherdoc::document_errc::image_format_unsupported;
        detail = "unsupported image extension for '" + image_path.string() +
                 "'; supported extensions are .png, .jpg, .jpeg, .gif, and .bmp";
        return false;
    }

    if (!detect_image_dimensions(image_info.extension, image_info.data,
                                 image_info.width_px, image_info.height_px)) {
        error_code = featherdoc::document_errc::image_size_read_failed;
        detail = "failed to parse image dimensions for '" + image_path.string() + "'";
        return false;
    }

    return true;
}

} // namespace featherdoc::detail
