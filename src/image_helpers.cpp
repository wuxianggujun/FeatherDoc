#include "image_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
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

auto read_u24_le(const std::string &data, std::size_t offset) -> std::uint32_t {
    return static_cast<std::uint32_t>(
        static_cast<unsigned char>(data[offset]) |
        (static_cast<unsigned char>(data[offset + 1U]) << 8U) |
        (static_cast<unsigned char>(data[offset + 2U]) << 16U));
}

auto read_u16_endian(const std::string &data, std::size_t offset, bool little_endian)
    -> std::uint16_t {
    return little_endian ? read_u16_le(data, offset) : read_u16_be(data, offset);
}

auto read_u32_endian(const std::string &data, std::size_t offset, bool little_endian)
    -> std::uint32_t {
    return little_endian ? read_u32_le(data, offset) : read_u32_be(data, offset);
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

auto xml_attribute_value(const std::string &data, std::string_view attribute_name)
    -> std::optional<std::string> {
    const std::string attribute{attribute_name};
    std::size_t search_from = 0U;
    while (search_from < data.size()) {
        const auto position = data.find(attribute, search_from);
        if (position == std::string::npos) {
            return std::nullopt;
        }
        search_from = position + attribute.size();

        if (position > 0U) {
            const auto previous = static_cast<unsigned char>(data[position - 1U]);
            if (std::isalnum(previous) != 0 || previous == '_' || previous == '-' ||
                previous == ':') {
                continue;
            }
        }

        std::size_t offset = position + attribute.size();
        while (offset < data.size() &&
               std::isspace(static_cast<unsigned char>(data[offset])) != 0) {
            ++offset;
        }
        if (offset >= data.size() || data[offset] != '=') {
            continue;
        }
        ++offset;
        while (offset < data.size() &&
               std::isspace(static_cast<unsigned char>(data[offset])) != 0) {
            ++offset;
        }
        if (offset >= data.size()) {
            continue;
        }

        const auto quote = data[offset];
        if (quote == '"' || quote == '\'') {
            const auto value_begin = offset + 1U;
            const auto value_end = data.find(quote, value_begin);
            if (value_end == std::string::npos) {
                return std::nullopt;
            }
            return data.substr(value_begin, value_end - value_begin);
        }

        const auto value_begin = offset;
        while (offset < data.size() &&
               std::isspace(static_cast<unsigned char>(data[offset])) == 0 &&
               data[offset] != '>') {
            ++offset;
        }
        return data.substr(value_begin, offset - value_begin);
    }

    return std::nullopt;
}

auto parse_svg_length_pixels(std::string value) -> std::optional<std::uint32_t> {
    auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::nullopt;
    }
    auto end = value.find_last_not_of(" \t\r\n");
    value = value.substr(begin, end - begin + 1U);

    char *parse_end = nullptr;
    const auto scalar = std::strtod(value.c_str(), &parse_end);
    if (parse_end == value.c_str() || scalar <= 0.0) {
        return std::nullopt;
    }

    std::string unit{parse_end};
    begin = unit.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        unit.clear();
    } else {
        end = unit.find_last_not_of(" \t\r\n");
        unit = to_lower_ascii(unit.substr(begin, end - begin + 1U));
    }

    double pixels = scalar;
    if (unit.empty() || unit == "px") {
        pixels = scalar;
    } else if (unit == "in") {
        pixels = scalar * 96.0;
    } else if (unit == "cm") {
        pixels = scalar * 96.0 / 2.54;
    } else if (unit == "mm") {
        pixels = scalar * 96.0 / 25.4;
    } else if (unit == "pt") {
        pixels = scalar * 96.0 / 72.0;
    } else if (unit == "pc") {
        pixels = scalar * 16.0;
    } else {
        return std::nullopt;
    }

    if (pixels <= 0.0 ||
        pixels > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(pixels + 0.5);
}

auto parse_svg_viewbox_size(std::string value, std::uint32_t &width,
                            std::uint32_t &height) -> bool {
    std::replace(value.begin(), value.end(), ',', ' ');
    std::istringstream stream(value);
    double min_x = 0.0;
    double min_y = 0.0;
    double view_width = 0.0;
    double view_height = 0.0;
    if (!(stream >> min_x >> min_y >> view_width >> view_height) ||
        view_width <= 0.0 || view_height <= 0.0 ||
        view_width > static_cast<double>(std::numeric_limits<std::uint32_t>::max()) ||
        view_height > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    width = static_cast<std::uint32_t>(view_width + 0.5);
    height = static_cast<std::uint32_t>(view_height + 0.5);
    return width > 0U && height > 0U;
}

auto detect_svg_dimensions(const std::string &data, std::uint32_t &width,
                           std::uint32_t &height) -> bool {
    if (data.find("<svg") == std::string::npos &&
        data.find("<svg:") == std::string::npos) {
        return false;
    }

    const auto width_attribute = xml_attribute_value(data, "width");
    const auto height_attribute = xml_attribute_value(data, "height");
    if (width_attribute.has_value() && height_attribute.has_value()) {
        const auto parsed_width = parse_svg_length_pixels(*width_attribute);
        const auto parsed_height = parse_svg_length_pixels(*height_attribute);
        if (parsed_width.has_value() && parsed_height.has_value()) {
            width = *parsed_width;
            height = *parsed_height;
            return width > 0U && height > 0U;
        }
    }

    const auto viewbox_attribute = xml_attribute_value(data, "viewBox");
    return viewbox_attribute.has_value() &&
           parse_svg_viewbox_size(*viewbox_attribute, width, height);
}

auto detect_webp_dimensions(const std::string &data, std::uint32_t &width,
                            std::uint32_t &height) -> bool {
    if (data.size() < 20U || std::string_view{data.data(), 4U} != "RIFF" ||
        std::string_view{data.data() + 8U, 4U} != "WEBP") {
        return false;
    }

    std::size_t offset = 12U;
    while (offset + 8U <= data.size()) {
        const auto chunk_type = std::string_view{data.data() + offset, 4U};
        const auto chunk_size = read_u32_le(data, offset + 4U);
        const auto chunk_data = offset + 8U;
        if (chunk_data + chunk_size > data.size()) {
            return false;
        }

        if (chunk_type == "VP8X" && chunk_size >= 10U) {
            width = read_u24_le(data, chunk_data + 4U) + 1U;
            height = read_u24_le(data, chunk_data + 7U) + 1U;
            return width > 0U && height > 0U;
        }

        if (chunk_type == "VP8 " && chunk_size >= 10U &&
            static_cast<unsigned char>(data[chunk_data + 3U]) == 0x9DU &&
            static_cast<unsigned char>(data[chunk_data + 4U]) == 0x01U &&
            static_cast<unsigned char>(data[chunk_data + 5U]) == 0x2AU) {
            width = read_u16_le(data, chunk_data + 6U) & 0x3FFFU;
            height = read_u16_le(data, chunk_data + 8U) & 0x3FFFU;
            return width > 0U && height > 0U;
        }

        if (chunk_type == "VP8L" && chunk_size >= 5U &&
            static_cast<unsigned char>(data[chunk_data]) == 0x2FU) {
            const auto bits = read_u32_le(data, chunk_data + 1U);
            width = (bits & 0x3FFFU) + 1U;
            height = ((bits >> 14U) & 0x3FFFU) + 1U;
            return width > 0U && height > 0U;
        }

        offset = chunk_data + chunk_size + (chunk_size % 2U);
    }

    return false;
}

auto read_tiff_scalar(const std::string &data, std::size_t entry_offset,
                      bool little_endian, std::uint32_t &value) -> bool {
    const auto type = read_u16_endian(data, entry_offset + 2U, little_endian);
    const auto count = read_u32_endian(data, entry_offset + 4U, little_endian);
    const auto value_or_offset = entry_offset + 8U;
    if (count == 0U) {
        return false;
    }

    if (type == 3U) {
        const auto value_offset = count == 1U
                                      ? value_or_offset
                                      : read_u32_endian(data, value_or_offset, little_endian);
        if (value_offset + 2U > data.size()) {
            return false;
        }
        value = read_u16_endian(data, value_offset, little_endian);
        return value > 0U;
    }

    if (type == 4U) {
        const auto value_offset = count == 1U
                                      ? value_or_offset
                                      : read_u32_endian(data, value_or_offset, little_endian);
        if (value_offset + 4U > data.size()) {
            return false;
        }
        value = read_u32_endian(data, value_offset, little_endian);
        return value > 0U;
    }

    return false;
}

auto detect_tiff_dimensions(const std::string &data, std::uint32_t &width,
                            std::uint32_t &height) -> bool {
    if (data.size() < 8U) {
        return false;
    }

    bool little_endian = false;
    if (data[0] == 'I' && data[1] == 'I') {
        little_endian = true;
    } else if (data[0] == 'M' && data[1] == 'M') {
        little_endian = false;
    } else {
        return false;
    }

    if (read_u16_endian(data, 2U, little_endian) != 42U) {
        return false;
    }

    const auto ifd_offset = read_u32_endian(data, 4U, little_endian);
    if (ifd_offset + 2U > data.size()) {
        return false;
    }

    const auto entry_count = read_u16_endian(data, ifd_offset, little_endian);
    bool has_width = false;
    bool has_height = false;
    for (std::uint16_t index = 0U; index < entry_count; ++index) {
        const auto entry_offset = ifd_offset + 2U + static_cast<std::size_t>(index) * 12U;
        if (entry_offset + 12U > data.size()) {
            return false;
        }

        const auto tag = read_u16_endian(data, entry_offset, little_endian);
        if (tag == 256U) {
            has_width = read_tiff_scalar(data, entry_offset, little_endian, width);
        } else if (tag == 257U) {
            has_height = read_tiff_scalar(data, entry_offset, little_endian, height);
        }
    }

    return has_width && has_height && width > 0U && height > 0U;
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
    if (extension == "svg") {
        return detect_svg_dimensions(data, width, height);
    }
    if (extension == "webp") {
        return detect_webp_dimensions(data, width, height);
    }
    if (extension == "tif" || extension == "tiff") {
        return detect_tiff_dimensions(data, width, height);
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
    if (extension == "svg") {
        return "image/svg+xml";
    }
    if (extension == "webp") {
        return "image/webp";
    }
    if (extension == "tif" || extension == "tiff") {
        return "image/tiff";
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
                 "'; supported extensions are .png, .jpg, .jpeg, .gif, .bmp, .svg, .webp, .tif, and .tiff";
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
