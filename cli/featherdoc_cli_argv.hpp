#pragma once

#include <featherdoc/detail/utf8.hpp>

#include <cstdio>
#include <new>
#include <utility>

namespace featherdoc_cli {

inline constexpr auto invalid_utf8_argument_message =
    "featherdoc_cli: command-line argument is not valid UTF-8\n";
inline constexpr auto invalid_utf16_argument_message =
    "featherdoc_cli: command-line argument is not valid UTF-16\n";
inline constexpr auto out_of_memory_message =
    "featherdoc_cli: insufficient memory to complete the command\n";
inline constexpr auto unexpected_exception_message =
    "featherdoc_cli: unexpected internal error\n";

template <typename Operation>
[[nodiscard]] inline auto
run_with_cli_exception_boundary(Operation &&operation) noexcept -> int {
    try {
        return std::forward<Operation>(operation)();
    } catch (const std::bad_alloc &) {
        std::fputs(out_of_memory_message, stderr);
    } catch (...) {
        std::fputs(unexpected_exception_message, stderr);
    }
    return 1;
}

[[nodiscard]] inline auto
cli_arguments_are_valid_utf8(int argc, char *const *argv) noexcept -> bool {
    if (argc <= 0 || argv == nullptr) {
        return false;
    }

    for (int index = 0; index < argc; ++index) {
        if (argv[index] == nullptr ||
            !featherdoc::detail::is_valid_utf8(argv[index])) {
            return false;
        }
    }
    return true;
}

} // namespace featherdoc_cli

#ifdef _WIN32

#include <windows.h>

#include <optional>
#include <string>

namespace featherdoc_cli {

[[nodiscard]] inline auto wide_argument_to_utf8(const wchar_t *text)
    -> std::optional<std::string> {
    if (text == nullptr) {
        return std::nullopt;
    }

    const int required = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return std::nullopt;
    }

    std::string encoded(static_cast<std::size_t>(required), '\0');
    const int written =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1,
                            encoded.data(), required, nullptr, nullptr);
    if (written != required || encoded.back() != '\0') {
        return std::nullopt;
    }

    encoded.resize(static_cast<std::size_t>(required - 1));
    return encoded;
}

} // namespace featherdoc_cli

#endif
