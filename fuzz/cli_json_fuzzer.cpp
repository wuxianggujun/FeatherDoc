#include "featherdoc_cli_json_parse.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

constexpr std::size_t max_fuzz_input_bytes = 16U * 1024U * 1024U;

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data,
                                      std::size_t size) {
    if (size > max_fuzz_input_bytes) {
        return 0;
    }

    const std::string_view input{reinterpret_cast<const char *>(data), size};
    std::size_t index = 0U;
    std::string error_message;
    (void)featherdoc_cli::skip_json_patch_value(input, index, error_message);
    return 0;
}
