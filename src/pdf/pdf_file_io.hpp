#pragma once

#include <featherdoc/detail/path.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace featherdoc::pdf::detail {

using binary_file_buffer = std::vector<unsigned char>;

[[nodiscard]] inline auto
read_binary_file(const std::filesystem::path &path) -> binary_file_buffer {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

[[nodiscard]] inline auto
path_for_diagnostic(const std::filesystem::path &path) -> std::string {
    return featherdoc::detail::path_to_utf8(path);
}

} // namespace featherdoc::pdf::detail
