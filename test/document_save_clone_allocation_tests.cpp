#include "document_core_unit_test_support.hpp"
#include "allocation_failure_test_case.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <new>
#include <string>
#include <string_view>
#include <vector>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY && defined(_WIN32)
#error "Document save transaction allocation tests are Linux-only"
#endif

namespace {

std::atomic_bool save_allocation_window_enabled{false};
std::atomic_size_t save_allocation_calls{0U};
std::atomic_size_t save_failure_call{0U};

auto should_fail_save_allocation() noexcept -> bool {
    if (!save_allocation_window_enabled.load(std::memory_order_relaxed)) {
        return false;
    }

    const auto current_call =
        save_allocation_calls.fetch_add(1U, std::memory_order_relaxed) + 1U;
    const auto failure_call =
        save_failure_call.load(std::memory_order_relaxed);
    return failure_call != 0U && current_call == failure_call;
}

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
[[nodiscard]] auto allocate_save_unaligned(std::size_t size) -> void * {
    if (should_fail_save_allocation()) {
        throw std::bad_alloc{};
    }
    if (void *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

[[nodiscard]] auto allocate_save_aligned(std::size_t size,
                                         std::size_t alignment) -> void * {
    if (should_fail_save_allocation()) {
        throw std::bad_alloc{};
    }

    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0U ? 1U : size) == 0) {
        return memory;
    }
    throw std::bad_alloc{};
}
#endif

class save_allocation_window final {
  public:
    explicit save_allocation_window(std::size_t failure_call) noexcept {
        save_allocation_calls.store(0U, std::memory_order_relaxed);
        save_failure_call.store(failure_call, std::memory_order_relaxed);
        save_allocation_window_enabled.store(true, std::memory_order_release);
    }

    save_allocation_window(const save_allocation_window &) = delete;
    auto operator=(const save_allocation_window &)
        -> save_allocation_window & = delete;

    ~save_allocation_window() {
        save_allocation_window_enabled.store(false,
                                              std::memory_order_release);
    }
};

class save_allocation_state_guard final {
  public:
    save_allocation_state_guard() = default;
    save_allocation_state_guard(const save_allocation_state_guard &) = delete;
    auto operator=(const save_allocation_state_guard &)
        -> save_allocation_state_guard & = delete;

    ~save_allocation_state_guard() {
        save_allocation_window_enabled.store(false,
                                              std::memory_order_relaxed);
        save_allocation_calls.store(0U, std::memory_order_relaxed);
        save_failure_call.store(0U, std::memory_order_relaxed);
    }
};

class temporary_save_test_directory final {
  public:
    temporary_save_test_directory() {
        const auto timestamp = static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
#if defined(_WIN32)
        const auto process_id =
            static_cast<std::uint64_t>(GetCurrentProcessId());
#else
        const auto process_id = static_cast<std::uint64_t>(getpid());
#endif
        this->path_ =
            std::filesystem::current_path() /
            ("save_transaction_allocation_" + std::to_string(process_id) +
             "_" + std::to_string(timestamp));
        std::error_code create_error;
        const auto created =
            std::filesystem::create_directory(this->path_, create_error);
        REQUIRE(created);
        REQUIRE_FALSE(create_error);
    }

    temporary_save_test_directory(const temporary_save_test_directory &) =
        delete;
    auto operator=(const temporary_save_test_directory &)
        -> temporary_save_test_directory & = delete;

    ~temporary_save_test_directory() {
        std::error_code ignored_error;
        std::filesystem::remove_all(this->path_, ignored_error);
    }

    [[nodiscard]] auto path() const noexcept
        -> const std::filesystem::path & {
        return this->path_;
    }

  private:
    std::filesystem::path path_;
};

auto transaction_temp_files(const std::filesystem::path &directory)
    -> std::vector<std::filesystem::path> {
    std::vector<std::filesystem::path> paths;
    std::error_code iteration_error;
    for (std::filesystem::directory_iterator iterator{directory,
                                                       iteration_error},
         end;
         !iteration_error && iterator != end;
         iterator.increment(iteration_error)) {
        if (!iterator->is_regular_file()) {
            continue;
        }
        const auto filename = iterator->path().filename().string();
        const auto name = std::string_view{filename};
        if (name.starts_with(".featherdoc-") && name.ends_with(".tmp")) {
            paths.push_back(iterator->path().filename());
        }
    }
    std::ranges::sort(paths);
    return paths;
}

} // namespace

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
void *operator new(std::size_t size) { return allocate_save_unaligned(size); }
void *operator new[](std::size_t size) {
    return allocate_save_unaligned(size);
}
void *operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_save_aligned(size, static_cast<std::size_t>(alignment));
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_save_aligned(size, static_cast<std::size_t>(alignment));
}
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete(void *memory, std::size_t,
                     std::align_val_t) noexcept {
    std::free(memory);
}
void operator delete[](void *memory, std::size_t,
                       std::align_val_t) noexcept {
    std::free(memory);
}
#endif

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "save pruning preserves the destination for every checked clone "
    "allocation failure") {
    namespace fs = std::filesystem;

    const auto source =
        fs::current_path() / "save_clone_allocation_source.docx";
    const auto baseline_output =
        fs::current_path() / "save_clone_allocation_baseline.docx";
    const auto failed_output =
        fs::current_path() / "save_clone_allocation_failed.docx";
    fs::remove(source);
    fs::remove(baseline_output);
    fs::remove(failed_output);
    const auto temp_files_before = transaction_temp_files(fs::current_path());
    write_test_docx_with_header_footer(source, "body", "header", "footer");

    pugi_memory_management_guard guard;
    delegated_pugi_allocate = guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          guard.deallocation);

    controlled_pugi_allocation_calls = 0U;
    controlled_pugi_failure_call = 0U;
    {
        featherdoc::Document baseline(source);
        REQUIRE_FALSE(baseline.open());
        REQUIRE(baseline.remove_header_part(0U));

        controlled_pugi_allocation_calls = 0U;
        REQUIRE_FALSE(baseline.save_as(baseline_output));
    }
    CHECK_EQ(transaction_temp_files(fs::current_path()), temp_files_before);
    const auto successful_save_allocation_count =
        controlled_pugi_allocation_calls;
    // Pruning checks both document relationships and Content Types clones.
    REQUIRE_GT(successful_save_allocation_count, 1U);

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_save_allocation_count;
         ++current_failure_call) {
        controlled_pugi_failure_call = 0U;
        fs::copy_file(source, failed_output,
                      fs::copy_options::overwrite_existing);
        const auto destination_before = read_binary_file(failed_output);

        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.remove_header_part(0U));

        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = current_failure_call;
        const auto error = document.save_as(failed_output);
        controlled_pugi_failure_call = 0U;

        CAPTURE(current_failure_call);
        CAPTURE(successful_save_allocation_count);
        CAPTURE(controlled_pugi_allocation_calls);
        CHECK_EQ(error, std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.last_error().code, error);
        const bool destination_unchanged =
            read_binary_file(failed_output) == destination_before;
        CHECK(destination_unchanged);
        CHECK_EQ(transaction_temp_files(fs::current_path()),
                 temp_files_before);
    }

    controlled_pugi_failure_call = 0U;
    fs::remove(source);
    fs::remove(baseline_output);
    fs::remove(failed_output);
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "save_as closes and removes its temporary output for every standard "
    "allocation failure") {
    namespace fs = std::filesystem;

    const save_allocation_state_guard allocation_state_guard;
    const temporary_save_test_directory test_directory;
    const auto source = test_directory.path() / "source.docx";
    const auto baseline_output = test_directory.path() / "baseline.docx";
    const auto failed_output = test_directory.path() / "failed.docx";
    const auto temp_files_before =
        transaction_temp_files(test_directory.path());
    write_test_docx_with_header_footer(source, "body", "header", "footer");
    // Match the destination state used by every fail-Nth iteration. Filesystem
    // path resolution can legitimately allocate differently for an existing
    // entry than for a not-yet-created entry, so those cases need independent
    // baselines instead of borrowing one allocation count from the other.
    fs::copy_file(source, baseline_output,
                  fs::copy_options::overwrite_existing);

    std::size_t successful_save_allocation_count = 0U;
    {
        featherdoc::Document baseline(source);
        REQUIRE_FALSE(baseline.open());
        REQUIRE(baseline.remove_header_part(0U));

        auto target = baseline_output;
        std::error_code save_error;
        {
            const save_allocation_window allocation_window{0U};
            save_error = baseline.save_as(std::move(target));
        }
        successful_save_allocation_count =
            save_allocation_calls.load(std::memory_order_relaxed);
        REQUIRE_FALSE(save_error);
    }
    REQUIRE_GT(successful_save_allocation_count, 1U);
    CHECK_EQ(transaction_temp_files(test_directory.path()),
             temp_files_before);

    for (std::size_t current_failure_call = 1U;
         current_failure_call <= successful_save_allocation_count;
         ++current_failure_call) {
        CAPTURE(current_failure_call);
        CAPTURE(successful_save_allocation_count);

        fs::copy_file(source, failed_output,
                      fs::copy_options::overwrite_existing);
        const auto destination_before = read_binary_file(failed_output);

        featherdoc::Document document(source);
        REQUIRE_FALSE(document.open());
        REQUIRE(document.remove_header_part(0U));

        auto target = failed_output;
        std::error_code save_error;
        bool threw = false;
        try {
            const save_allocation_window allocation_window{
                current_failure_call};
            save_error = document.save_as(std::move(target));
        } catch (...) {
            threw = true;
        }

        CAPTURE(save_allocation_calls.load(std::memory_order_relaxed));
        CHECK_FALSE(threw);
        CHECK_EQ(save_error,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(document.last_error().code, save_error);
        const bool destination_unchanged =
            read_binary_file(failed_output) == destination_before;
        CHECK(destination_unchanged);
        CHECK_EQ(transaction_temp_files(test_directory.path()),
                 temp_files_before);

        // The failed save must not poison the in-memory document. Reusing the
        // exact Document also proves that the writer/archive handles were not
        // retained after the allocation failure.
        REQUIRE_FALSE(document.save_as(failed_output));
        CHECK_EQ(transaction_temp_files(test_directory.path()),
                 temp_files_before);

        featherdoc::Document reopened(failed_output);
        REQUIRE_FALSE(reopened.open());
        CHECK_EQ(reopened.header_count(), 0U);
        CHECK_EQ(reopened.footer_count(), 1U);
    }

}
