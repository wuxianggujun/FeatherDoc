int featherdoc_cli_main(int argc, char **argv);

#include "featherdoc_cli_argv.hpp"

#ifdef _WIN32
#include <windows.h>

#include <clocale>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

int wmain(int argc, wchar_t **argv) {
    return featherdoc_cli::run_with_cli_exception_boundary([&]() -> int {
        if (argc <= 0 || argv == nullptr) {
            std::fputs(featherdoc_cli::invalid_utf16_argument_message, stderr);
            return 2;
        }

        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
        std::setlocale(LC_ALL, ".UTF-8");

        std::vector<std::string> encoded_arguments;
        encoded_arguments.reserve(static_cast<std::size_t>(argc));
        for (int index = 0; index < argc; ++index) {
            auto encoded = featherdoc_cli::wide_argument_to_utf8(argv[index]);
            if (!encoded.has_value()) {
                std::fputs(featherdoc_cli::invalid_utf16_argument_message,
                           stderr);
                return 2;
            }
            encoded_arguments.push_back(std::move(*encoded));
        }

        std::vector<char *> argument_pointers;
        argument_pointers.reserve(encoded_arguments.size());
        for (auto &argument : encoded_arguments) {
            argument_pointers.push_back(argument.data());
        }
        return featherdoc_cli_main(argc, argument_pointers.data());
    });
}
#else
#include <cstdio>

int main(int argc, char **argv) {
    return featherdoc_cli::run_with_cli_exception_boundary([&]() -> int {
        if (!featherdoc_cli::cli_arguments_are_valid_utf8(argc, argv)) {
            std::fputs(featherdoc_cli::invalid_utf8_argument_message, stderr);
            return 2;
        }
        return featherdoc_cli_main(argc, argv);
    });
}
#endif
