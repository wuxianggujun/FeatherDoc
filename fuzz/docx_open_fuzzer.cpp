#include <featherdoc.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

namespace {

constexpr std::size_t max_fuzz_input_bytes = 16U * 1024U * 1024U;

class fuzz_archive_files final {
  public:
    fuzz_archive_files() {
        const auto suffix = std::to_string(static_cast<long long>(::getpid()));
        const auto directory = std::filesystem::temp_directory_path();
        this->input_path_ =
            directory / ("featherdoc-fuzz-input-" + suffix + ".docx");
        this->roundtrip_path_ =
            directory / ("featherdoc-fuzz-roundtrip-" + suffix + ".docx");
    }

    ~fuzz_archive_files() {
        std::error_code ignored;
        std::filesystem::remove(this->input_path_, ignored);
        std::filesystem::remove(this->roundtrip_path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path &input_path() const noexcept {
        return this->input_path_;
    }

    [[nodiscard]] const std::filesystem::path &roundtrip_path() const noexcept {
        return this->roundtrip_path_;
    }

  private:
    std::filesystem::path input_path_;
    std::filesystem::path roundtrip_path_;
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data,
                                      std::size_t size) {
    if (size > max_fuzz_input_bytes) {
        return 0;
    }

    static fuzz_archive_files files;

    {
        std::ofstream stream(files.input_path(),
                             std::ios::binary | std::ios::trunc);
        if (!stream) {
            return 0;
        }
        stream.write(reinterpret_cast<const char *>(data),
                     static_cast<std::streamsize>(size));
        if (!stream) {
            return 0;
        }
    }

    featherdoc::Document document(files.input_path());
    if (document.open()) {
        return 0;
    }

    if (document.save_as(files.roundtrip_path())) {
        return 0;
    }

    featherdoc::Document reopened(files.roundtrip_path());
    (void)reopened.open();
    return 0;
}
