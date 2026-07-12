int featherdoc_cli_main(int argc, char **argv);

#ifdef _WIN32
#include <windows.h>

#include <clocale>
#include <string>
#include <vector>

namespace {
auto wide_to_utf8(const wchar_t *text) -> std::string {
    if (text == nullptr) {
        return {};
    }
    const int required =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, nullptr, 0,
                            nullptr, nullptr);
    if (required <= 1) {
        return {};
    }
    std::string encoded(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, encoded.data(),
                        required, nullptr, nullptr);
    encoded.pop_back();
    return encoded;
}
} // namespace

int wmain(int argc, wchar_t **argv) {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF-8");

    std::vector<std::string> encoded_arguments;
    encoded_arguments.reserve(static_cast<std::size_t>(argc));
    for (int index = 0; index < argc; ++index) {
        encoded_arguments.push_back(wide_to_utf8(argv[index]));
    }

    std::vector<char *> argument_pointers;
    argument_pointers.reserve(encoded_arguments.size());
    for (auto &argument : encoded_arguments) {
        argument_pointers.push_back(argument.data());
    }
    return featherdoc_cli_main(argc, argument_pointers.data());
}
#else
int main(int argc, char **argv) {
    return featherdoc_cli_main(argc, argv);
}
#endif
