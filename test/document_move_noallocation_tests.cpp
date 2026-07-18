#include <featherdoc.hpp>

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <unordered_set>
#include <utility>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace {

std::atomic_bool track_allocations{false};
std::atomic_size_t observed_allocations{0U};

void record_allocation() noexcept {
    if (track_allocations.load(std::memory_order_relaxed)) {
        observed_allocations.fetch_add(1U, std::memory_order_relaxed);
    }
}

[[nodiscard]] void *allocate_unaligned(std::size_t size) {
    record_allocation();
    if (void *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

[[nodiscard]] void *allocate_aligned(std::size_t size, std::size_t alignment) {
    record_allocation();
#if defined(_WIN32)
    if (void *memory = _aligned_malloc(size == 0U ? 1U : size, alignment)) {
        return memory;
    }
#else
    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0U ? 1U : size) == 0) {
        return memory;
    }
#endif
    throw std::bad_alloc{};
}

void free_aligned(void *memory) noexcept {
#if defined(_WIN32)
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}

} // namespace

void *operator new(std::size_t size) { return allocate_unaligned(size); }
void *operator new[](std::size_t size) { return allocate_unaligned(size); }

void *operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_aligned(size, static_cast<std::size_t>(alignment));
}

void *operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_aligned(size, static_cast<std::size_t>(alignment));
}

void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept {
    std::free(memory);
}

void operator delete(void *memory, std::align_val_t) noexcept {
    free_aligned(memory);
}

void operator delete[](void *memory, std::align_val_t) noexcept {
    free_aligned(memory);
}

void operator delete(void *memory, std::size_t, std::align_val_t) noexcept {
    free_aligned(memory);
}

void operator delete[](void *memory, std::size_t, std::align_val_t) noexcept {
    free_aligned(memory);
}

#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) &&                     \
    _ITERATOR_DEBUG_LEVEL != 0
int main() {
    // MSVC's debug STL allocates iterator-proxy objects even from noexcept
    // container move constructors. The release configuration and other normal
    // non-debug-iterator standard libraries exercise the zero-allocation gate.
    std::fputs(
        "Document move allocation test skipped for MSVC debug iterators\n",
        stdout);
    return 77;
}
#else
int main() {
    // Document's v1.13 ABI contains two std::unordered_set members. Some
    // standard libraries (notably the MSVC release STL) allocate sentinel
    // nodes in unordered-container move constructors. Measure that frozen ABI
    // cost so this gate rejects allocations added by Document's own move
    // bookkeeping without pretending the standard-library cost is ours.
    auto baseline_related_entries = std::unordered_set<std::string>{"related"};
    auto baseline_archive_entries = std::unordered_set<std::string>{"archive"};
    observed_allocations.store(0U, std::memory_order_relaxed);
    track_allocations.store(true, std::memory_order_relaxed);
    auto moved_baseline_related_entries =
        std::unordered_set<std::string>{std::move(baseline_related_entries)};
    auto moved_baseline_archive_entries =
        std::unordered_set<std::string>{std::move(baseline_archive_entries)};
    track_allocations.store(false, std::memory_order_relaxed);
    const auto frozen_container_move_allocations =
        observed_allocations.load(std::memory_order_relaxed);
    if (moved_baseline_related_entries.size() != 1U ||
        moved_baseline_archive_entries.size() != 1U) {
        std::fputs("failed to establish container move baseline\n", stderr);
        return 1;
    }

    featherdoc::Document source;
    if (source.create_empty()) {
        std::fputs("failed to initialize move source\n", stderr);
        return 1;
    }
    if (!source.body_template().append_paragraph("sidecar source").valid()) {
        std::fputs("failed to populate move source\n", stderr);
        return 1;
    }

    featherdoc::Document destination;
    if (destination.create_empty()) {
        std::fputs("failed to initialize move destination\n", stderr);
        return 1;
    }

    featherdoc::Document null_token_source;
    featherdoc::Document token_holder(std::move(null_token_source));

    // Replacing global new lets the executable count every allocation made by
    // inline code and the linked library. This covers populated state, an
    // already moved-from null lifetime token, move assignment, and self move.
    observed_allocations.store(0U, std::memory_order_relaxed);
    track_allocations.store(true, std::memory_order_relaxed);
    featherdoc::Document moved(std::move(source));
    track_allocations.store(false, std::memory_order_relaxed);
    const auto populated_move_construction_allocations =
        observed_allocations.load(std::memory_order_relaxed);

    observed_allocations.store(0U, std::memory_order_relaxed);
    track_allocations.store(true, std::memory_order_relaxed);
    destination = std::move(moved);
    track_allocations.store(false, std::memory_order_relaxed);
    const auto populated_move_assignment_allocations =
        observed_allocations.load(std::memory_order_relaxed);

    observed_allocations.store(0U, std::memory_order_relaxed);
    track_allocations.store(true, std::memory_order_relaxed);
    featherdoc::Document empty_moved(std::move(null_token_source));
    track_allocations.store(false, std::memory_order_relaxed);
    const auto null_move_construction_allocations =
        observed_allocations.load(std::memory_order_relaxed);

    observed_allocations.store(0U, std::memory_order_relaxed);
    track_allocations.store(true, std::memory_order_relaxed);
    destination = std::move(null_token_source);
    track_allocations.store(false, std::memory_order_relaxed);
    const auto null_move_assignment_allocations =
        observed_allocations.load(std::memory_order_relaxed);

    observed_allocations.store(0U, std::memory_order_relaxed);
    track_allocations.store(true, std::memory_order_relaxed);
    destination = std::move(destination);
    track_allocations.store(false, std::memory_order_relaxed);
    const auto self_move_assignment_allocations =
        observed_allocations.load(std::memory_order_relaxed);

    if (populated_move_construction_allocations !=
            frozen_container_move_allocations ||
        populated_move_assignment_allocations != 0U ||
        null_move_construction_allocations !=
            frozen_container_move_allocations ||
        null_move_assignment_allocations != 0U ||
        self_move_assignment_allocations != 0U) {
        std::fprintf(stderr,
                     "move allocation regression: frozen-container=%zu "
                     "populated-ctor=%zu populated-assign=%zu null-ctor=%zu "
                     "null-assign=%zu self-assign=%zu\n",
                     frozen_container_move_allocations,
                     populated_move_construction_allocations,
                     populated_move_assignment_allocations,
                     null_move_construction_allocations,
                     null_move_assignment_allocations,
                     self_move_assignment_allocations);
        return 1;
    }

    if (destination.is_open() || source.is_open() || moved.is_open() ||
        empty_moved.is_open() || null_token_source.is_open() ||
        token_holder.is_open()) {
        std::fputs("move result did not preserve the expected closed state\n",
                   stderr);
        return 1;
    }

    if (destination.create_empty()) {
        std::fputs("move destination was not reusable\n", stderr);
        return 1;
    }
    if (null_token_source.create_empty()) {
        std::fputs("null-token move source was not reusable\n", stderr);
        return 1;
    }

    std::fprintf(stdout,
                 "Document noexcept moves added zero allocations beyond the "
                 "frozen container baseline (%zu)\n",
                 frozen_container_move_allocations);
    return 0;
}
#endif
