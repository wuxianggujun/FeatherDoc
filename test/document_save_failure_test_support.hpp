#pragma once

#include <cstdint>

extern "C" {
void document_test_fail_next_sync_stage(int stage);
std::uint32_t document_test_last_reserved_temp_mode();
}

namespace featherdoc_test {

inline constexpr int document_sync_failure_file = 1;
inline constexpr int document_sync_failure_parent_directory = 2;

inline void document_fail_next_sync_stage(int stage) {
    document_test_fail_next_sync_stage(stage);
}

inline auto document_last_reserved_temp_mode() -> std::uint32_t {
    return document_test_last_reserved_temp_mode();
}

} // namespace featherdoc_test
