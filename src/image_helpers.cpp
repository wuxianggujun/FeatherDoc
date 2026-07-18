#include "image_helpers.hpp"
#include <featherdoc/detail/path.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <new>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace featherdoc::detail {
namespace {

auto to_lower_ascii(std::string value) -> std::string {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

auto byte_range_fits(std::size_t data_size, std::size_t offset,
                     std::size_t length) -> bool {
    return offset <= data_size && length <= data_size - offset;
}

auto checked_size_add(std::size_t left, std::size_t right)
    -> std::optional<std::size_t> {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        return std::nullopt;
    }
    return left + right;
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
               static_cast<unsigned char>(data[offset])) |
           (static_cast<std::uint32_t>(
                static_cast<unsigned char>(data[offset + 1U]))
            << 8U) |
           (static_cast<std::uint32_t>(
                static_cast<unsigned char>(data[offset + 2U]))
            << 16U) |
           (static_cast<std::uint32_t>(
                static_cast<unsigned char>(data[offset + 3U]))
            << 24U);
}

auto read_u32_be(const std::string &data, std::size_t offset) -> std::uint32_t {
    return (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset]))
            << 24U) |
           (static_cast<std::uint32_t>(
                static_cast<unsigned char>(data[offset + 1U]))
            << 16U) |
           (static_cast<std::uint32_t>(
                static_cast<unsigned char>(data[offset + 2U]))
            << 8U) |
           static_cast<std::uint32_t>(
               static_cast<unsigned char>(data[offset + 3U]));
}

auto read_u24_le(const std::string &data, std::size_t offset) -> std::uint32_t {
    return static_cast<std::uint32_t>(
        static_cast<unsigned char>(data[offset]) |
        (static_cast<unsigned char>(data[offset + 1U]) << 8U) |
        (static_cast<unsigned char>(data[offset + 2U]) << 16U));
}

auto read_u16_endian(const std::string &data, std::size_t offset,
                     bool little_endian) -> std::uint16_t {
    return little_endian ? read_u16_le(data, offset)
                         : read_u16_be(data, offset);
}

auto read_u32_endian(const std::string &data, std::size_t offset,
                     bool little_endian) -> std::uint32_t {
    return little_endian ? read_u32_le(data, offset)
                         : read_u32_be(data, offset);
}

auto detect_stb_raster_dimensions(const std::string &data, std::uint32_t &width,
                                  std::uint32_t &height) -> bool {
    if (data.empty() || data.size() > static_cast<std::size_t>(
                                          std::numeric_limits<int>::max())) {
        return false;
    }

    int parsed_width = 0;
    int parsed_height = 0;
    int component_count = 0;
    const auto *bytes = reinterpret_cast<const stbi_uc *>(data.data());
    if (stbi_info_from_memory(bytes, static_cast<int>(data.size()),
                              &parsed_width, &parsed_height,
                              &component_count) == 0) {
        return false;
    }

    if (parsed_width <= 0 || parsed_height <= 0) {
        return false;
    }

    width = static_cast<std::uint32_t>(parsed_width);
    height = static_cast<std::uint32_t>(parsed_height);
    return true;
}

auto xml_attribute_value(const std::string &data,
                         std::string_view attribute_name)
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
            const auto previous =
                static_cast<unsigned char>(data[position - 1U]);
            if (std::isalnum(previous) != 0 || previous == '_' ||
                previous == '-' || previous == ':') {
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

auto parse_svg_length_pixels(std::string value)
    -> std::optional<std::uint32_t> {
    auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::nullopt;
    }
    auto end = value.find_last_not_of(" \t\r\n");
    value = value.substr(begin, end - begin + 1U);

    char *parse_end = nullptr;
    const auto scalar = std::strtod(value.c_str(), &parse_end);
    if (parse_end == value.c_str() || !std::isfinite(scalar) || scalar <= 0.0) {
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

    if (!std::isfinite(pixels) || pixels <= 0.0 ||
        pixels >
            static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
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
        !std::isfinite(min_x) || !std::isfinite(min_y) ||
        !std::isfinite(view_width) || !std::isfinite(view_height) ||
        view_width <= 0.0 || view_height <= 0.0 ||
        view_width >
            static_cast<double>(std::numeric_limits<std::uint32_t>::max()) ||
        view_height >
            static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
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
    while (byte_range_fits(data.size(), offset, 8U)) {
        const auto chunk_type = std::string_view{data.data() + offset, 4U};
        const auto chunk_size =
            static_cast<std::size_t>(read_u32_le(data, offset + 4U));
        const auto chunk_data = checked_size_add(offset, 8U);
        if (!chunk_data.has_value() ||
            !byte_range_fits(data.size(), *chunk_data, chunk_size)) {
            return false;
        }

        if (chunk_type == "VP8X" && chunk_size >= 10U) {
            width = read_u24_le(data, *chunk_data + 4U) + 1U;
            height = read_u24_le(data, *chunk_data + 7U) + 1U;
            return width > 0U && height > 0U;
        }

        if (chunk_type == "VP8 " && chunk_size >= 10U &&
            static_cast<unsigned char>(data[*chunk_data + 3U]) == 0x9DU &&
            static_cast<unsigned char>(data[*chunk_data + 4U]) == 0x01U &&
            static_cast<unsigned char>(data[*chunk_data + 5U]) == 0x2AU) {
            width = read_u16_le(data, *chunk_data + 6U) & 0x3FFFU;
            height = read_u16_le(data, *chunk_data + 8U) & 0x3FFFU;
            return width > 0U && height > 0U;
        }

        if (chunk_type == "VP8L" && chunk_size >= 5U &&
            static_cast<unsigned char>(data[*chunk_data]) == 0x2FU) {
            const auto bits = read_u32_le(data, *chunk_data + 1U);
            width = (bits & 0x3FFFU) + 1U;
            height = ((bits >> 14U) & 0x3FFFU) + 1U;
            return width > 0U && height > 0U;
        }

        const auto padded_chunk_size =
            checked_size_add(chunk_size, chunk_size % 2U);
        if (!padded_chunk_size.has_value()) {
            return false;
        }
        const auto next_offset =
            checked_size_add(*chunk_data, *padded_chunk_size);
        if (!next_offset.has_value()) {
            return false;
        }
        offset = *next_offset;
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
        const auto value_offset =
            count == 1U ? value_or_offset
                        : static_cast<std::size_t>(read_u32_endian(
                              data, value_or_offset, little_endian));
        if (!byte_range_fits(data.size(), value_offset, 2U)) {
            return false;
        }
        value = read_u16_endian(data, value_offset, little_endian);
        return value > 0U;
    }

    if (type == 4U) {
        const auto value_offset =
            count == 1U ? value_or_offset
                        : static_cast<std::size_t>(read_u32_endian(
                              data, value_or_offset, little_endian));
        if (!byte_range_fits(data.size(), value_offset, 4U)) {
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

    const auto ifd_offset =
        static_cast<std::size_t>(read_u32_endian(data, 4U, little_endian));
    if (!byte_range_fits(data.size(), ifd_offset, 2U)) {
        return false;
    }

    const auto entry_count = read_u16_endian(data, ifd_offset, little_endian);
    const auto entries_begin = checked_size_add(ifd_offset, 2U);
    if (!entries_begin.has_value()) {
        return false;
    }
    bool has_width = false;
    bool has_height = false;
    for (std::uint16_t index = 0U; index < entry_count; ++index) {
        const auto entry_offset = checked_size_add(
            *entries_begin, static_cast<std::size_t>(index) * 12U);
        if (!entry_offset.has_value() ||
            !byte_range_fits(data.size(), *entry_offset, 12U)) {
            return false;
        }

        const auto tag = read_u16_endian(data, *entry_offset, little_endian);
        if (tag == 256U) {
            has_width =
                read_tiff_scalar(data, *entry_offset, little_endian, width);
        } else if (tag == 257U) {
            has_height =
                read_tiff_scalar(data, *entry_offset, little_endian, height);
        }
    }

    return has_width && has_height && width > 0U && height > 0U;
}

auto detect_image_dimensions(const std::string &extension,
                             const std::string &data, std::uint32_t &width,
                             std::uint32_t &height) -> bool {
    if (extension == "png" || extension == "jpg" || extension == "jpeg" ||
        extension == "gif" || extension == "bmp") {
        return detect_stb_raster_dimensions(data, width, height);
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

auto image_content_type_for_extension(const std::string &extension)
    -> std::string {
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

auto file_type_name(std::filesystem::file_type type) -> std::string_view {
    switch (type) {
    case std::filesystem::file_type::none:
        return "none";
    case std::filesystem::file_type::not_found:
        return "not_found";
    case std::filesystem::file_type::regular:
        return "regular";
    case std::filesystem::file_type::directory:
        return "directory";
    case std::filesystem::file_type::symlink:
        return "symlink";
    case std::filesystem::file_type::block:
        return "block_device";
    case std::filesystem::file_type::character:
        return "character_device";
    case std::filesystem::file_type::fifo:
        return "fifo";
    case std::filesystem::file_type::socket:
        return "socket";
    case std::filesystem::file_type::unknown:
        return "unknown";
    }
    return "unknown";
}

struct image_read_result final {
    std::size_t bytes{0U};
    bool eof{false};
    bool failed{false};
    std::error_code native_error;
};

template <typename ReadChunk>
bool read_image_chunks_with_limit(ReadChunk &&read_chunk,
                                  const std::filesystem::path &image_path,
                                  std::uint64_t max_file_bytes,
                                  std::string &data,
                                  featherdoc::document_errc &error_code,
                                  std::string &detail) {
    constexpr std::size_t read_chunk_bytes = 64U * 1024U;

    data.clear();
    error_code = featherdoc::document_errc::success;
    detail.clear();

    const auto path_text = featherdoc::detail::path_to_utf8(image_path);
    const auto fail_read = [&](const std::error_code &native_error = {}) {
        std::string{}.swap(data);
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "failed while reading image file '" + path_text + "'";
        if (native_error) {
            detail += " (native_error=" + std::to_string(native_error.value()) +
                      ", native_category=" + native_error.category().name() +
                      ")";
        }
        return false;
    };

    std::array<char, read_chunk_bytes> buffer{};
    std::uint64_t total_bytes = 0U;
    try {
        while (true) {
            const auto remaining_bytes = max_file_bytes - total_bytes;
            // The extra probe byte detects a file that grew after its size was
            // queried without retaining data beyond the configured limit.
            const auto request_bytes =
                remaining_bytes < read_chunk_bytes
                    ? static_cast<std::size_t>(remaining_bytes) + 1U
                    : read_chunk_bytes;
            const auto read_result = read_chunk(buffer.data(), request_bytes);
            if (read_result.failed || read_result.bytes > request_bytes) {
                return fail_read(read_result.native_error);
            }

            if (static_cast<std::uint64_t>(read_result.bytes) >
                remaining_bytes) {
                const auto actual_bytes =
                    total_bytes >
                            std::numeric_limits<std::uint64_t>::max() -
                                static_cast<std::uint64_t>(read_result.bytes)
                        ? std::numeric_limits<std::uint64_t>::max()
                        : total_bytes +
                              static_cast<std::uint64_t>(read_result.bytes);
                std::string{}.swap(data);
                error_code =
                    featherdoc::document_errc::image_input_limit_exceeded;
                detail = "image file '" + path_text +
                         "' exceeds the external image input limit "
                         "(actual_bytes=" +
                         std::to_string(actual_bytes) +
                         ", limit_bytes=" + std::to_string(max_file_bytes) +
                         ")";
                return false;
            }

            if (read_result.bytes != 0U) {
                data.append(buffer.data(), read_result.bytes);
                total_bytes += static_cast<std::uint64_t>(read_result.bytes);
            }

            if (read_result.eof) {
                break;
            }
            if (read_result.bytes == 0U) {
                return fail_read();
            }
        }
    } catch (const std::ios_base::failure &) {
        return fail_read();
    } catch (const std::bad_alloc &) {
        std::string{}.swap(data);
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "failed to allocate memory while reading image file '" +
                 path_text + "'";
        return false;
    } catch (const std::length_error &) {
        std::string{}.swap(data);
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "image file '" + path_text +
                 "' is too large for the current process address space";
        return false;
    }

    return true;
}

bool reserve_image_buffer(std::uint64_t file_bytes,
                          const std::string &path_text, std::string &data,
                          featherdoc::document_errc &error_code,
                          std::string &detail) {
    if (file_bytes >
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return true;
    }
    try {
        data.reserve(static_cast<std::size_t>(file_bytes));
    } catch (const std::bad_alloc &) {
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "failed to allocate memory while reading image file '" +
                 path_text + "'";
        return false;
    } catch (const std::length_error &) {
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "image file '" + path_text +
                 "' is too large for the current process address space";
        return false;
    }
    return true;
}

#ifdef _WIN32

class native_image_handle final {
  public:
    explicit native_image_handle(HANDLE value) noexcept : value_(value) {}
    native_image_handle(const native_image_handle &) = delete;
    auto operator=(const native_image_handle &)
        -> native_image_handle & = delete;

    ~native_image_handle() {
        if (this->value_ != INVALID_HANDLE_VALUE) {
            CloseHandle(this->value_);
        }
    }

    [[nodiscard]] auto get() const noexcept -> HANDLE { return this->value_; }

  private:
    HANDLE value_{INVALID_HANDLE_VALUE};
};

bool read_regular_image_file(const std::filesystem::path &image_path,
                             std::uint64_t max_file_bytes, std::string &data,
                             featherdoc::document_errc &error_code,
                             std::string &detail) {
    const auto path_text = featherdoc::detail::path_to_utf8(image_path);
    const auto raw_handle =
        CreateFileW(image_path.c_str(), GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (raw_handle == INVALID_HANDLE_VALUE) {
        const auto native_error = GetLastError();
        error_code = featherdoc::document_errc::image_file_open_failed;
        detail = "failed to open image file '" + path_text +
                 "' (native_error=" + std::to_string(native_error) + ")";
        return false;
    }
    const native_image_handle handle{raw_handle};

    SetLastError(ERROR_SUCCESS);
    const auto handle_type = GetFileType(handle.get());
    const auto type_error = GetLastError();
    if (handle_type == FILE_TYPE_UNKNOWN && type_error != ERROR_SUCCESS) {
        error_code = featherdoc::document_errc::image_file_status_failed;
        detail = "failed to inspect opened image file '" + path_text +
                 "' (native_error=" + std::to_string(type_error) + ")";
        return false;
    }
    if (handle_type != FILE_TYPE_DISK) {
        error_code = featherdoc::document_errc::image_file_not_regular;
        detail = "opened image input '" + path_text +
                 "' is not a regular disk file (native_file_type=" +
                 std::to_string(handle_type) + ")";
        return false;
    }

    LARGE_INTEGER native_size{};
    if (GetFileSizeEx(handle.get(), &native_size) == 0 ||
        native_size.QuadPart < 0) {
        const auto native_error = GetLastError();
        error_code = featherdoc::document_errc::image_file_size_read_failed;
        detail = "failed to read opened image file size for '" + path_text +
                 "' (native_error=" + std::to_string(native_error) + ")";
        return false;
    }
    const auto file_bytes = static_cast<std::uint64_t>(native_size.QuadPart);
    if (file_bytes > max_file_bytes) {
        error_code = featherdoc::document_errc::image_input_limit_exceeded;
        detail = "image file '" + path_text +
                 "' exceeds the external image input limit (actual_bytes=" +
                 std::to_string(file_bytes) +
                 ", limit_bytes=" + std::to_string(max_file_bytes) + ")";
        return false;
    }
    if (!reserve_image_buffer(file_bytes, path_text, data, error_code,
                              detail)) {
        return false;
    }

    return read_image_chunks_with_limit(
        [&](char *buffer, std::size_t request_bytes) {
            DWORD bytes_read = 0U;
            if (ReadFile(handle.get(), buffer,
                         static_cast<DWORD>(request_bytes), &bytes_read,
                         nullptr) == 0) {
                return image_read_result{
                    0U, false, true,
                    std::error_code{static_cast<int>(GetLastError()),
                                    std::system_category()}};
            }
            return image_read_result{static_cast<std::size_t>(bytes_read),
                                     bytes_read == 0U,
                                     false,
                                     {}};
        },
        image_path, max_file_bytes, data, error_code, detail);
}

#else

class native_image_handle final {
  public:
    explicit native_image_handle(int value) noexcept : value_(value) {}
    native_image_handle(const native_image_handle &) = delete;
    auto operator=(const native_image_handle &)
        -> native_image_handle & = delete;

    ~native_image_handle() {
        if (this->value_ >= 0) {
            ::close(this->value_);
        }
    }

    [[nodiscard]] auto get() const noexcept -> int { return this->value_; }

  private:
    int value_{-1};
};

bool read_regular_image_file(const std::filesystem::path &image_path,
                             std::uint64_t max_file_bytes, std::string &data,
                             featherdoc::document_errc &error_code,
                             std::string &detail) {
    const auto path_text = featherdoc::detail::path_to_utf8(image_path);
    auto open_flags = O_RDONLY | O_NONBLOCK;
#ifdef O_CLOEXEC
    open_flags |= O_CLOEXEC;
#endif
    const auto raw_handle = ::open(image_path.c_str(), open_flags);
    if (raw_handle < 0) {
        const auto native_error = errno;
        error_code = featherdoc::document_errc::image_file_open_failed;
        detail = "failed to open image file '" + path_text +
                 "' (native_error=" + std::to_string(native_error) + ")";
        return false;
    }
    const native_image_handle handle{raw_handle};

    struct stat file_status{};
    if (::fstat(handle.get(), &file_status) != 0) {
        const auto native_error = errno;
        if (native_error == EOVERFLOW) {
            error_code = featherdoc::document_errc::image_input_limit_exceeded;
            detail =
                "opened image file metadata cannot represent its actual size "
                "for '" +
                path_text + "' (actual_bytes=unrepresentable, limit_bytes=" +
                std::to_string(max_file_bytes) +
                ", native_error=" + std::to_string(native_error) + ")";
            return false;
        }
        error_code = featherdoc::document_errc::image_file_status_failed;
        detail = "failed to inspect opened image file '" + path_text +
                 "' (native_error=" + std::to_string(native_error) + ")";
        return false;
    }
    if (!S_ISREG(file_status.st_mode)) {
        error_code = featherdoc::document_errc::image_file_not_regular;
        detail = "opened image input '" + path_text + "' is not a regular file";
        return false;
    }
    if (file_status.st_size < 0) {
        error_code = featherdoc::document_errc::image_file_size_read_failed;
        detail =
            "opened image file has a negative size for '" + path_text + "'";
        return false;
    }

    const auto native_file_bytes =
        static_cast<std::uintmax_t>(file_status.st_size);
    if (native_file_bytes > static_cast<std::uintmax_t>(
                                std::numeric_limits<std::uint64_t>::max())) {
        error_code = featherdoc::document_errc::image_input_limit_exceeded;
        detail = "image file '" + path_text +
                 "' exceeds the external image input limit (actual_bytes=" +
                 std::to_string(native_file_bytes) +
                 ", limit_bytes=" + std::to_string(max_file_bytes) + ")";
        return false;
    }
    const auto file_bytes = static_cast<std::uint64_t>(native_file_bytes);
    if (file_bytes > max_file_bytes) {
        error_code = featherdoc::document_errc::image_input_limit_exceeded;
        detail = "image file '" + path_text +
                 "' exceeds the external image input limit (actual_bytes=" +
                 std::to_string(file_bytes) +
                 ", limit_bytes=" + std::to_string(max_file_bytes) + ")";
        return false;
    }
    if (!reserve_image_buffer(file_bytes, path_text, data, error_code,
                              detail)) {
        return false;
    }

    return read_image_chunks_with_limit(
        [&](char *buffer, std::size_t request_bytes) {
            ssize_t bytes_read = -1;
            do {
                bytes_read = ::read(handle.get(), buffer, request_bytes);
            } while (bytes_read < 0 && errno == EINTR);
            if (bytes_read < 0) {
                return image_read_result{
                    0U, false, true,
                    std::error_code{errno, std::system_category()}};
            }
            return image_read_result{static_cast<std::size_t>(bytes_read),
                                     bytes_read == 0,
                                     false,
                                     {}};
        },
        image_path, max_file_bytes, data, error_code, detail);
}

#endif

} // namespace

bool read_image_stream_with_limit(std::istream &stream,
                                  const std::filesystem::path &image_path,
                                  std::uint64_t max_file_bytes,
                                  std::string &data,
                                  featherdoc::document_errc &error_code,
                                  std::string &detail) {
    return read_image_chunks_with_limit(
        [&](char *buffer, std::size_t request_bytes) {
            stream.read(buffer, static_cast<std::streamsize>(request_bytes));
            const auto stream_bytes = stream.gcount();
            if (stream_bytes < 0) {
                return image_read_result{0U, false, true, {}};
            }
            const auto bytes_read = static_cast<std::size_t>(stream_bytes);
            const auto failed =
                stream.bad() || (stream.fail() && !stream.eof());
            return image_read_result{bytes_read, stream.eof(), failed, {}};
        },
        image_path, max_file_bytes, data, error_code, detail);
}

bool load_image_file(const std::filesystem::path &image_path,
                     image_file_info &image_info,
                     featherdoc::document_errc &error_code,
                     std::string &detail) {
    return load_image_file(image_path, image_info, error_code, detail,
                           max_external_image_file_bytes);
}

bool load_image_file(const std::filesystem::path &image_path,
                     image_file_info &image_info,
                     featherdoc::document_errc &error_code, std::string &detail,
                     std::uint64_t max_file_bytes) {
    image_info = {};
    error_code = featherdoc::document_errc::success;
    detail.clear();

    if (image_path.empty()) {
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "image path must not be empty";
        return false;
    }
    if (featherdoc::detail::path_has_embedded_nul(image_path)) {
        error_code = featherdoc::document_errc::image_file_read_failed;
        detail = "image path must not contain an embedded NUL";
        return false;
    }

    const auto path_text = featherdoc::detail::path_to_utf8(image_path);
    std::error_code status_error;
    const auto file_status = std::filesystem::status(image_path, status_error);
    if (status_error == std::errc::value_too_large) {
        error_code = featherdoc::document_errc::image_input_limit_exceeded;
        detail = "image file metadata cannot represent its actual size for '" +
                 path_text + "' (actual_bytes=unrepresentable, limit_bytes=" +
                 std::to_string(max_file_bytes) +
                 ", error_value=" + std::to_string(status_error.value()) +
                 ", error_category=" + status_error.category().name() + ")";
        return false;
    }
    if (status_error ||
        file_status.type() == std::filesystem::file_type::none ||
        file_status.type() == std::filesystem::file_type::not_found ||
        file_status.type() == std::filesystem::file_type::unknown) {
        error_code = featherdoc::document_errc::image_file_status_failed;
        detail = "failed to inspect image file '" + path_text + "'";
        if (status_error) {
            detail += " (error_value=" + std::to_string(status_error.value()) +
                      ", error_category=" + status_error.category().name() +
                      ")";
        }
        return false;
    }
    if (!std::filesystem::is_regular_file(file_status)) {
        error_code = featherdoc::document_errc::image_file_not_regular;
        detail = "image input '" + path_text +
                 "' is not a regular file (file_type=" +
                 std::string{file_type_name(file_status.type())} + ")";
        return false;
    }

    auto extension = featherdoc::detail::path_to_utf8(image_path.extension());
    if (!extension.empty() && extension.front() == '.') {
        extension.erase(extension.begin());
    }
    extension = to_lower_ascii(std::move(extension));
    auto content_type = image_content_type_for_extension(extension);
    if (content_type.empty()) {
        error_code = featherdoc::document_errc::image_format_unsupported;
        detail = "unsupported image extension for '" + path_text +
                 "'; supported extensions are .png, .jpg, .jpeg, .gif, .bmp, "
                 ".svg, .webp, .tif, and .tiff";
        return false;
    }

    if (!read_regular_image_file(image_path, max_file_bytes, image_info.data,
                                 error_code, detail)) {
        return false;
    }

    image_info.extension = std::move(extension);
    image_info.content_type = std::move(content_type);

    if (!detect_image_dimensions(image_info.extension, image_info.data,
                                 image_info.width_px, image_info.height_px)) {
        error_code = featherdoc::document_errc::image_size_read_failed;
        detail = "failed to parse image dimensions for '" + path_text + "'";
        return false;
    }

    return true;
}

} // namespace featherdoc::detail
