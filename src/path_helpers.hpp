#pragma once

#include <filesystem>
#include <string>

namespace featherdoc::detail {

inline std::string path_to_utf8_string(const std::filesystem::path &path) {
    const auto utf8_path = path.u8string();
#ifdef __cpp_char8_t
    return std::string(reinterpret_cast<const char *>(utf8_path.data()),
                       utf8_path.size());
#else
    return utf8_path;
#endif
}

} // namespace featherdoc::detail
