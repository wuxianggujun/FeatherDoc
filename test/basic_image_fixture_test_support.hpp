#pragma once

#include "doctest.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

auto write_binary_file(const std::filesystem::path &path, const std::string &data) -> void {
    std::ofstream stream(path, std::ios::binary);
    REQUIRE(stream.good());
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    REQUIRE(stream.good());
}

auto tiny_png_data() -> std::string {
    constexpr unsigned char tiny_png_bytes[] = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU, 0x00U, 0x00U, 0x00U,
        0x0DU, 0x49U, 0x48U, 0x44U, 0x52U, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U,
        0x00U, 0x01U, 0x08U, 0x06U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x15U, 0xC4U, 0x89U,
        0x00U, 0x00U, 0x00U, 0x0DU, 0x49U, 0x44U, 0x41U, 0x54U, 0x78U, 0x9CU, 0x63U,
        0x60U, 0x00U, 0x00U, 0x00U, 0x02U, 0x00U, 0x01U, 0xE5U, 0x27U, 0xD4U, 0xA2U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x49U, 0x45U, 0x4EU, 0x44U, 0xAEU, 0x42U, 0x60U,
        0x82U,
    };

    return {reinterpret_cast<const char *>(tiny_png_bytes), sizeof(tiny_png_bytes)};
}

auto tiny_gif_data() -> std::string {
    constexpr unsigned char tiny_gif_bytes[] = {
        0x47U, 0x49U, 0x46U, 0x38U, 0x39U, 0x61U, 0x01U, 0x00U, 0x01U, 0x00U, 0x80U,
        0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0x21U, 0xF9U, 0x04U,
        0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x2CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x00U, 0x01U, 0x00U, 0x00U, 0x02U, 0x02U, 0x44U, 0x01U, 0x00U, 0x3BU,
    };

    return {reinterpret_cast<const char *>(tiny_gif_bytes), sizeof(tiny_gif_bytes)};
}

auto tiny_jpeg_data() -> std::string {
    constexpr unsigned char tiny_jpeg_bytes[] = {
        0xFFU, 0xD8U, 0xFFU, 0xE0U, 0x00U, 0x10U, 0x4AU, 0x46U, 0x49U, 0x46U, 0x00U,
        0x01U, 0x01U, 0x01U, 0x00U, 0x60U, 0x00U, 0x60U, 0x00U, 0x00U, 0xFFU, 0xDBU,
        0x00U, 0x43U, 0x00U, 0x03U, 0x02U, 0x02U, 0x03U, 0x02U, 0x02U, 0x03U, 0x03U,
        0x03U, 0x03U, 0x04U, 0x03U, 0x03U, 0x04U, 0x05U, 0x08U, 0x05U, 0x05U, 0x04U,
        0x04U, 0x05U, 0x0AU, 0x07U, 0x07U, 0x06U, 0x08U, 0x0CU, 0x0AU, 0x0CU, 0x0CU,
        0x0BU, 0x0AU, 0x0BU, 0x0BU, 0x0DU, 0x0EU, 0x12U, 0x10U, 0x0DU, 0x0EU, 0x11U,
        0x0EU, 0x0BU, 0x0BU, 0x10U, 0x16U, 0x10U, 0x11U, 0x13U, 0x14U, 0x15U, 0x15U,
        0x15U, 0x0CU, 0x0FU, 0x17U, 0x18U, 0x16U, 0x14U, 0x18U, 0x12U, 0x14U, 0x15U,
        0x14U, 0xFFU, 0xDBU, 0x00U, 0x43U, 0x01U, 0x03U, 0x04U, 0x04U, 0x05U, 0x04U,
        0x05U, 0x09U, 0x05U, 0x05U, 0x09U, 0x14U, 0x0DU, 0x0BU, 0x0DU, 0x14U, 0x14U,
        0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U,
        0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U,
        0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U,
        0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0x14U,
        0x14U, 0x14U, 0x14U, 0x14U, 0x14U, 0xFFU, 0xC0U, 0x00U, 0x11U, 0x08U, 0x00U,
        0x02U, 0x00U, 0x03U, 0x03U, 0x01U, 0x22U, 0x00U, 0x02U, 0x11U, 0x01U, 0x03U,
        0x11U, 0x01U, 0xFFU, 0xC4U, 0x00U, 0x1FU, 0x00U, 0x00U, 0x01U, 0x05U, 0x01U,
        0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U, 0x09U,
        0x0AU, 0x0BU, 0xFFU, 0xC4U, 0x00U, 0xB5U, 0x10U, 0x00U, 0x02U, 0x01U, 0x03U,
        0x03U, 0x02U, 0x04U, 0x03U, 0x05U, 0x05U, 0x04U, 0x04U, 0x00U, 0x00U, 0x01U,
        0x7DU, 0x01U, 0x02U, 0x03U, 0x00U, 0x04U, 0x11U, 0x05U, 0x12U, 0x21U, 0x31U,
        0x41U, 0x06U, 0x13U, 0x51U, 0x61U, 0x07U, 0x22U, 0x71U, 0x14U, 0x32U, 0x81U,
        0x91U, 0xA1U, 0x08U, 0x23U, 0x42U, 0xB1U, 0xC1U, 0x15U, 0x52U, 0xD1U, 0xF0U,
        0x24U, 0x33U, 0x62U, 0x72U, 0x82U, 0x09U, 0x0AU, 0x16U, 0x17U, 0x18U, 0x19U,
        0x1AU, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x34U, 0x35U, 0x36U, 0x37U,
        0x38U, 0x39U, 0x3AU, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U, 0x48U, 0x49U, 0x4AU,
        0x53U, 0x54U, 0x55U, 0x56U, 0x57U, 0x58U, 0x59U, 0x5AU, 0x63U, 0x64U, 0x65U,
        0x66U, 0x67U, 0x68U, 0x69U, 0x6AU, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, 0x78U,
        0x79U, 0x7AU, 0x83U, 0x84U, 0x85U, 0x86U, 0x87U, 0x88U, 0x89U, 0x8AU, 0x92U,
        0x93U, 0x94U, 0x95U, 0x96U, 0x97U, 0x98U, 0x99U, 0x9AU, 0xA2U, 0xA3U, 0xA4U,
        0xA5U, 0xA6U, 0xA7U, 0xA8U, 0xA9U, 0xAAU, 0xB2U, 0xB3U, 0xB4U, 0xB5U, 0xB6U,
        0xB7U, 0xB8U, 0xB9U, 0xBAU, 0xC2U, 0xC3U, 0xC4U, 0xC5U, 0xC6U, 0xC7U, 0xC8U,
        0xC9U, 0xCAU, 0xD2U, 0xD3U, 0xD4U, 0xD5U, 0xD6U, 0xD7U, 0xD8U, 0xD9U, 0xDAU,
        0xE1U, 0xE2U, 0xE3U, 0xE4U, 0xE5U, 0xE6U, 0xE7U, 0xE8U, 0xE9U, 0xEAU, 0xF1U,
        0xF2U, 0xF3U, 0xF4U, 0xF5U, 0xF6U, 0xF7U, 0xF8U, 0xF9U, 0xFAU, 0xFFU, 0xC4U,
        0x00U, 0x1FU, 0x01U, 0x00U, 0x03U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U, 0x01U,
        0x01U, 0x01U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x02U,
        0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U, 0x09U, 0x0AU, 0x0BU, 0xFFU, 0xC4U,
        0x00U, 0xB5U, 0x11U, 0x00U, 0x02U, 0x01U, 0x02U, 0x04U, 0x04U, 0x03U, 0x04U,
        0x07U, 0x05U, 0x04U, 0x04U, 0x00U, 0x01U, 0x02U, 0x77U, 0x00U, 0x01U, 0x02U,
        0x03U, 0x11U, 0x04U, 0x05U, 0x21U, 0x31U, 0x06U, 0x12U, 0x41U, 0x51U, 0x07U,
        0x61U, 0x71U, 0x13U, 0x22U, 0x32U, 0x81U, 0x08U, 0x14U, 0x42U, 0x91U, 0xA1U,
        0xB1U, 0xC1U, 0x09U, 0x23U, 0x33U, 0x52U, 0xF0U, 0x15U, 0x62U, 0x72U, 0xD1U,
        0x0AU, 0x16U, 0x24U, 0x34U, 0xE1U, 0x25U, 0xF1U, 0x17U, 0x18U, 0x19U, 0x1AU,
        0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x35U, 0x36U, 0x37U, 0x38U, 0x39U, 0x3AU,
        0x43U, 0x44U, 0x45U, 0x46U, 0x47U, 0x48U, 0x49U, 0x4AU, 0x53U, 0x54U, 0x55U,
        0x56U, 0x57U, 0x58U, 0x59U, 0x5AU, 0x63U, 0x64U, 0x65U, 0x66U, 0x67U, 0x68U,
        0x69U, 0x6AU, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, 0x78U, 0x79U, 0x7AU, 0x82U,
        0x83U, 0x84U, 0x85U, 0x86U, 0x87U, 0x88U, 0x89U, 0x8AU, 0x92U, 0x93U, 0x94U,
        0x95U, 0x96U, 0x97U, 0x98U, 0x99U, 0x9AU, 0xA2U, 0xA3U, 0xA4U, 0xA5U, 0xA6U,
        0xA7U, 0xA8U, 0xA9U, 0xAAU, 0xB2U, 0xB3U, 0xB4U, 0xB5U, 0xB6U, 0xB7U, 0xB8U,
        0xB9U, 0xBAU, 0xC2U, 0xC3U, 0xC4U, 0xC5U, 0xC6U, 0xC7U, 0xC8U, 0xC9U, 0xCAU,
        0xD2U, 0xD3U, 0xD4U, 0xD5U, 0xD6U, 0xD7U, 0xD8U, 0xD9U, 0xDAU, 0xE2U, 0xE3U,
        0xE4U, 0xE5U, 0xE6U, 0xE7U, 0xE8U, 0xE9U, 0xEAU, 0xF2U, 0xF3U, 0xF4U, 0xF5U,
        0xF6U, 0xF7U, 0xF8U, 0xF9U, 0xFAU, 0xFFU, 0xDAU, 0x00U, 0x0CU, 0x03U, 0x01U,
        0x00U, 0x02U, 0x11U, 0x03U, 0x11U, 0x00U, 0x3FU, 0x00U, 0xF9U, 0xB2U, 0x8AU,
        0x28U, 0xAFU, 0xEAU, 0x03U, 0xF1U, 0xA3U, 0xFFU, 0xD9U,
    };

    return {reinterpret_cast<const char *>(tiny_jpeg_bytes), sizeof(tiny_jpeg_bytes)};
}

void append_test_le16(std::string &data, std::uint16_t value) {
    data.push_back(static_cast<char>(value & 0xFFU));
    data.push_back(static_cast<char>((value >> 8U) & 0xFFU));
}

void append_test_le24(std::string &data, std::uint32_t value) {
    data.push_back(static_cast<char>(value & 0xFFU));
    data.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    data.push_back(static_cast<char>((value >> 16U) & 0xFFU));
}

void append_test_le32(std::string &data, std::uint32_t value) {
    data.push_back(static_cast<char>(value & 0xFFU));
    data.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    data.push_back(static_cast<char>((value >> 16U) & 0xFFU));
    data.push_back(static_cast<char>((value >> 24U) & 0xFFU));
}

auto tiny_bmp_data() -> std::string {
    std::string data;
    data += "BM";
    append_test_le32(data, 78U);
    append_test_le16(data, 0U);
    append_test_le16(data, 0U);
    append_test_le32(data, 54U);
    append_test_le32(data, 40U);
    append_test_le32(data, 3U);
    append_test_le32(data, 2U);
    append_test_le16(data, 1U);
    append_test_le16(data, 24U);
    append_test_le32(data, 0U);
    append_test_le32(data, 24U);
    append_test_le32(data, 96U);
    append_test_le32(data, 96U);
    append_test_le32(data, 0U);
    append_test_le32(data, 0U);
    data.append(24U, '\0');
    return data;
}

auto tiny_svg_data() -> std::string {
    return R"(<svg xmlns="http://www.w3.org/2000/svg" width="3" height="2"><rect width="3" height="2" fill="#00AAFF"/></svg>)";
}

auto tiny_webp_data() -> std::string {
    std::string data;
    data += "RIFF";
    append_test_le32(data, 22U);
    data += "WEBP";
    data += "VP8X";
    append_test_le32(data, 10U);
    data.push_back('\0');
    data.append(3U, '\0');
    append_test_le24(data, 2U);
    append_test_le24(data, 1U);
    return data;
}

auto tiny_tiff_data() -> std::string {
    std::string data;
    data += "II";
    append_test_le16(data, 42U);
    append_test_le32(data, 8U);
    append_test_le16(data, 2U);
    append_test_le16(data, 256U);
    append_test_le16(data, 4U);
    append_test_le32(data, 1U);
    append_test_le32(data, 3U);
    append_test_le16(data, 257U);
    append_test_le16(data, 4U);
    append_test_le32(data, 1U);
    append_test_le32(data, 2U);
    append_test_le32(data, 0U);
    return data;
}

auto read_binary_file(const std::filesystem::path &path) -> std::vector<unsigned char> {
    std::ifstream stream(path, std::ios::binary);
    REQUIRE(stream.good());

    return {std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

auto read_le16(const std::vector<unsigned char> &data, std::size_t offset) -> std::uint16_t {
    REQUIRE_LE(offset + 2U, data.size());
    return static_cast<std::uint16_t>(data[offset]) |
           (static_cast<std::uint16_t>(data[offset + 1U]) << 8U);
}

auto read_le32(const std::vector<unsigned char> &data, std::size_t offset) -> std::uint32_t {
    REQUIRE_LE(offset + 4U, data.size());
    return static_cast<std::uint32_t>(data[offset]) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 8U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 3U]) << 24U);
}

auto extra_contains_empty_zip64_field(const std::vector<unsigned char> &data, std::size_t offset,
                                      std::size_t size) -> bool {
    const std::size_t end = offset + size;
    if (end > data.size()) {
        return false;
    }

    std::size_t cursor = offset;
    while (cursor + 4U <= end) {
        const auto header_id = read_le16(data, cursor);
        const auto field_size = read_le16(data, cursor + 2U);
        cursor += 4U;

        if (cursor + field_size > end) {
            return false;
        }

        if (header_id == 0x0001U && field_size == 0U) {
            return true;
        }

        cursor += field_size;
    }

    return false;
}

auto collect_empty_zip64_extra_locations(const std::filesystem::path &path)
    -> std::vector<std::string> {
    constexpr std::uint32_t local_file_header_signature = 0x04034B50U;
    constexpr std::uint32_t central_directory_header_signature = 0x02014B50U;
    constexpr std::uint32_t end_of_central_directory_signature = 0x06054B50U;

    const auto data = read_binary_file(path);
    std::vector<std::string> locations;

    std::size_t offset = 0U;
    while (offset + 4U <= data.size()) {
        const auto signature = read_le32(data, offset);
        if (signature != local_file_header_signature) {
            break;
        }

        const auto compressed_size = static_cast<std::size_t>(read_le32(data, offset + 18U));
        const auto filename_size = static_cast<std::size_t>(read_le16(data, offset + 26U));
        const auto extra_size = static_cast<std::size_t>(read_le16(data, offset + 28U));
        const auto header_size = 30U + filename_size + extra_size;
        REQUIRE_LE(offset + header_size, data.size());

        const std::string entry_name{
            reinterpret_cast<const char *>(data.data() + offset + 30U), filename_size};
        const auto extra_offset = offset + 30U + filename_size;
        if (extra_contains_empty_zip64_field(data, extra_offset, extra_size)) {
            locations.push_back("local:" + entry_name);
        }

        offset += header_size + compressed_size;
    }

    while (offset + 4U <= data.size()) {
        const auto signature = read_le32(data, offset);
        if (signature == end_of_central_directory_signature) {
            break;
        }

        REQUIRE_EQ(signature, central_directory_header_signature);

        const auto filename_size = static_cast<std::size_t>(read_le16(data, offset + 28U));
        const auto extra_size = static_cast<std::size_t>(read_le16(data, offset + 30U));
        const auto comment_size = static_cast<std::size_t>(read_le16(data, offset + 32U));
        const auto header_size = 46U + filename_size + extra_size + comment_size;
        REQUIRE_LE(offset + header_size, data.size());

        const std::string entry_name{
            reinterpret_cast<const char *>(data.data() + offset + 46U), filename_size};
        const auto extra_offset = offset + 46U + filename_size;
        if (extra_contains_empty_zip64_field(data, extra_offset, extra_size)) {
            locations.push_back("central:" + entry_name);
        }

        offset += header_size;
    }

    return locations;
}

auto collect_zero_version_needed_locations(const std::filesystem::path &path)
    -> std::vector<std::string> {
    constexpr std::uint32_t local_file_header_signature = 0x04034B50U;
    constexpr std::uint32_t central_directory_header_signature = 0x02014B50U;
    constexpr std::uint32_t end_of_central_directory_signature = 0x06054B50U;

    const auto data = read_binary_file(path);
    std::vector<std::string> locations;

    std::size_t offset = 0U;
    while (offset + 4U <= data.size()) {
        const auto signature = read_le32(data, offset);
        if (signature != local_file_header_signature) {
            break;
        }

        const auto version_needed = read_le16(data, offset + 4U);
        const auto compressed_size = static_cast<std::size_t>(read_le32(data, offset + 18U));
        const auto filename_size = static_cast<std::size_t>(read_le16(data, offset + 26U));
        const auto extra_size = static_cast<std::size_t>(read_le16(data, offset + 28U));
        const auto header_size = 30U + filename_size + extra_size;
        REQUIRE_LE(offset + header_size, data.size());

        const std::string entry_name{
            reinterpret_cast<const char *>(data.data() + offset + 30U), filename_size};
        if (version_needed == 0U) {
            locations.push_back("local:" + entry_name);
        }

        offset += header_size + compressed_size;
    }

    while (offset + 4U <= data.size()) {
        const auto signature = read_le32(data, offset);
        if (signature == end_of_central_directory_signature) {
            break;
        }

        REQUIRE_EQ(signature, central_directory_header_signature);

        const auto version_needed = read_le16(data, offset + 6U);
        const auto filename_size = static_cast<std::size_t>(read_le16(data, offset + 28U));
        const auto extra_size = static_cast<std::size_t>(read_le16(data, offset + 30U));
        const auto comment_size = static_cast<std::size_t>(read_le16(data, offset + 32U));
        const auto header_size = 46U + filename_size + extra_size + comment_size;
        REQUIRE_LE(offset + header_size, data.size());

        const std::string entry_name{
            reinterpret_cast<const char *>(data.data() + offset + 46U), filename_size};
        if (version_needed == 0U) {
            locations.push_back("central:" + entry_name);
        }

        offset += header_size;
    }

    return locations;
}

auto convert_nth_inline_drawing_to_anchor(std::string xml_text, std::size_t inline_index)
    -> std::string {
    constexpr auto inline_open_tag = "<wp:inline";
    constexpr auto inline_close_tag = "</wp:inline>";
    constexpr auto anchor_open_attributes =
        R"( distT="0" distB="0" distL="0" distR="0" simplePos="0" relativeHeight="0" behindDoc="0" locked="0" layoutInCell="1" allowOverlap="1")";
    constexpr auto anchor_positioning_xml =
        R"(<wp:simplePos x="0" y="0"/><wp:positionH relativeFrom="column"><wp:posOffset>0</wp:posOffset></wp:positionH><wp:positionV relativeFrom="paragraph"><wp:posOffset>0</wp:posOffset></wp:positionV><wp:wrapNone/>)";

    std::size_t open_search_from = 0U;
    std::size_t open_tag_position = std::string::npos;
    for (std::size_t current_index = 0U; current_index <= inline_index; ++current_index) {
        open_tag_position = xml_text.find(inline_open_tag, open_search_from);
        REQUIRE_NE(open_tag_position, std::string::npos);
        open_search_from = open_tag_position + std::strlen(inline_open_tag);
    }

    const auto open_tag_end = xml_text.find('>', open_tag_position);
    REQUIRE_NE(open_tag_end, std::string::npos);
    xml_text.replace(open_tag_position, std::strlen(inline_open_tag), "<wp:anchor");
    xml_text.insert(open_tag_end, anchor_open_attributes);

    const auto extent_position = xml_text.find("<wp:extent", open_tag_end);
    REQUIRE_NE(extent_position, std::string::npos);
    xml_text.insert(extent_position, anchor_positioning_xml);

    const auto close_tag_position = xml_text.find(inline_close_tag, open_tag_end);
    REQUIRE_NE(close_tag_position, std::string::npos);
    xml_text.replace(close_tag_position, std::strlen(inline_close_tag), "</wp:anchor>");

    return xml_text;
}

} // namespace
