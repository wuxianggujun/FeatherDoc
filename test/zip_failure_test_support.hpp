#pragma once

extern "C" {
void zip_test_fail_next_close_stage(int stage);
void zip_test_fail_next_write(void);
}

namespace featherdoc_test {

inline constexpr int zip_close_failure_finalize = 1;
inline constexpr int zip_close_failure_truncate = 2;
inline constexpr int zip_close_failure_writer_end = 3;
inline constexpr int zip_close_failure_reader_end = 4;

inline void zip_fail_next_close_stage(int stage) {
    zip_test_fail_next_close_stage(stage);
}

inline void zip_fail_next_write() { zip_test_fail_next_write(); }

} // namespace featherdoc_test
