#include <featherdoc/document.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

namespace {

template <typename T>
concept has_post_v1_13_3_paragraph_options_api =
    requires { &T::inspect_paragraphs_with_options; };

// This translation unit must remain compiled against the frozen v1.13.3
// headers. If CMake's include order regresses to the current headers, fail at
// compile time instead of silently weakening the ABI test.
static_assert(!has_post_v1_13_3_paragraph_options_api<featherdoc::Document>);
static_assert(std::is_move_constructible_v<featherdoc::Document>);
static_assert(std::is_move_assignable_v<featherdoc::Document>);

[[noreturn]] void fail(const std::string &message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string &message) {
    if (!condition) {
        fail(message);
    }
}

void require_success(const std::error_code &error,
                     const featherdoc::Document &document,
                     const std::string &operation) {
    if (!error) {
        return;
    }

    const auto &detail = document.last_error();
    fail(operation + " failed (category=" + error.category().name() +
         ", value=" + std::to_string(error.value()) +
         ", detail=" + detail.detail + ")");
}

template <typename T> class guarded_object final {
  private:
    static constexpr auto canary_value = std::byte{0xA5};
    static constexpr std::size_t minimum_guard_bytes = 4096U;
    static constexpr std::size_t guard_bytes =
        ((minimum_guard_bytes + alignof(T) - 1U) / alignof(T)) * alignof(T);
    static constexpr std::size_t storage_bytes =
        guard_bytes + sizeof(T) + guard_bytes;

    alignas(T) std::array<std::byte, storage_bytes> storage_{};
    bool live_{false};

    [[nodiscard]] T *object_address() noexcept {
        return std::launder(
            reinterpret_cast<T *>(this->storage_.data() + guard_bytes));
    }

  public:
    guarded_object() { this->storage_.fill(canary_value); }

    guarded_object(const guarded_object &) = delete;
    auto operator=(const guarded_object &) -> guarded_object & = delete;

    ~guarded_object() {
        if (this->live_) {
            std::destroy_at(this->object_address());
        }
    }

    template <typename... Args> T &emplace(Args &&...args) {
        if (this->live_) {
            fail("guarded object was constructed more than once");
        }
        auto *result = std::construct_at(this->object_address(),
                                         std::forward<Args>(args)...);
        this->live_ = true;
        return *result;
    }

    [[nodiscard]] T &get() {
        if (!this->live_) {
            fail("guarded object is not live");
        }
        return *this->object_address();
    }

    [[nodiscard]] bool guards_intact() const noexcept {
        const auto prefix_end =
            this->storage_.begin() + static_cast<std::ptrdiff_t>(guard_bytes);
        const auto suffix_begin =
            prefix_end + static_cast<std::ptrdiff_t>(sizeof(T));
        return std::all_of(
                   this->storage_.begin(), prefix_end,
                   [](std::byte value) { return value == canary_value; }) &&
               std::all_of(
                   suffix_begin, this->storage_.end(),
                   [](std::byte value) { return value == canary_value; });
    }

    void require_guards(const std::string &operation) const {
        require(this->guards_intact(),
                operation + " wrote outside the v1.13.3 Document layout");
    }

    void destroy_and_require_guards(const std::string &operation) {
        if (this->live_) {
            std::destroy_at(this->object_address());
            this->live_ = false;
        }
        this->require_guards(operation);
    }
};

class temporary_directory final {
  private:
    std::filesystem::path path_;

  public:
    temporary_directory() {
        const auto suffix =
            std::to_string(std::chrono::high_resolution_clock::now()
                               .time_since_epoch()
                               .count());
        this->path_ = std::filesystem::temp_directory_path() /
                      std::filesystem::path{u8"FeatherDoc_旧头_ABI_v1.13.3"} /
                      suffix;

        std::error_code error;
        if (!std::filesystem::create_directories(this->path_, error) || error) {
            fail("failed to create ABI test directory (error=" +
                 std::to_string(error.value()) + ")");
        }
    }

    temporary_directory(const temporary_directory &) = delete;
    auto operator=(const temporary_directory &)
        -> temporary_directory & = delete;

    ~temporary_directory() {
        std::error_code ignored;
        std::filesystem::remove_all(this->path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path &path() const noexcept {
        return this->path_;
    }
};

void run_legacy_header_runtime_test() {
    temporary_directory directory;
    const auto document_path =
        directory.path() /
        std::filesystem::path{u8"中文路径_旧版调用方_兼容文档.docx"};

    const featherdoc::archive_limits legacy_limits{
        10'000U,
        64U * 1024U * 1024U,
        256U * 1024U * 1024U,
        512U * 1024U * 1024U,
        200U,
    };
    const featherdoc::document_open_options strict_options{
        featherdoc::package_validation_mode::strict,
        legacy_limits,
    };

    guarded_object<featherdoc::Document> original_storage;
    auto &original = original_storage.emplace();
    original_storage.require_guards("v1.13.3 default construction");

    original.set_path(document_path);
    original_storage.require_guards("v1.13.3 set_path with UTF-8 path");
    require_success(original.create_empty(), original, "create_empty");
    original_storage.require_guards("v1.13.3 create_empty");
    require(original.is_open(), "create_empty did not open the document");
    require_success(original.save(), original, "save to UTF-8 path");
    original_storage.require_guards("v1.13.3 save");
    require(std::filesystem::is_regular_file(document_path),
            "save did not create the UTF-8 DOCX path");

    // The v1.13.3 move operations are implicitly generated in this old-header
    // consumer. They therefore exercise exactly the object code emitted for an
    // already-built application, not the current header's move declarations.
    guarded_object<featherdoc::Document> moved_storage;
    auto &moved = moved_storage.emplace(std::move(original));
    original_storage.require_guards("v1.13.3 move construction source");
    moved_storage.require_guards("v1.13.3 move construction destination");
    require(moved.is_open(), "move construction lost the open document");
    require(moved.path() == document_path,
            "move construction lost the UTF-8 document path");
    require_success(moved.save(), moved, "save after move construction");

    // Reusing a moved-from v1.13.3 object is an important compatibility case:
    // the old inline move leaves its shared lifetime pointer empty.
    original.set_path(document_path);
    require_success(original.open(strict_options), original,
                    "open(options) after move construction");
    original_storage.require_guards("reused move-construction source");

    guarded_object<featherdoc::Document> assigned_storage;
    auto &assigned = assigned_storage.emplace();
    assigned.set_path(directory.path() /
                      std::filesystem::path{u8"被替换的中文文档.docx"});
    require_success(assigned.create_empty(), assigned,
                    "create_empty before move assignment");

    assigned = std::move(moved);
    moved_storage.require_guards("v1.13.3 move assignment source");
    assigned_storage.require_guards("v1.13.3 move assignment destination");
    require(assigned.is_open(), "move assignment lost the open document");
    require(assigned.path() == document_path,
            "move assignment lost the UTF-8 document path");
    require_success(assigned.save(), assigned, "save after move assignment");

    moved.set_path(document_path);
    require_success(moved.open(strict_options), moved,
                    "open(options) after move assignment");
    moved_storage.require_guards("reused move-assignment source");

    guarded_object<featherdoc::Document> reopened_storage;
    auto &reopened = reopened_storage.emplace(document_path);
    require_success(reopened.open(strict_options), reopened,
                    "path constructor plus open(options)");
    reopened_storage.require_guards("v1.13.3 open(options)");
    require(reopened.is_open(), "strict reopen did not open the saved DOCX");

    reopened_storage.destroy_and_require_guards("v1.13.3 destruction");
    assigned_storage.destroy_and_require_guards(
        "v1.13.3 move-assigned destruction");
    moved_storage.destroy_and_require_guards(
        "v1.13.3 reused move-source destruction");
    original_storage.destroy_and_require_guards("v1.13.3 original destruction");
}

} // namespace

int main() {
    try {
        run_legacy_header_runtime_test();
        std::cout << "v1.13.3 legacy-header ABI runtime test passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "v1.13.3 legacy-header ABI runtime test failed: "
                  << error.what() << '\n';
        return 1;
    }
}
