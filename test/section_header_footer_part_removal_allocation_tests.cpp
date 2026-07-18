#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "allocation_failure_test_case.hpp"
#include "basic_docx_archive_test_support.hpp"

#include <featherdoc.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <new>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY && defined(_WIN32)
#error "Header/footer part-removal allocation tests are Linux-only"
#endif

namespace {

std::atomic_bool part_removal_allocation_window_enabled{false};
std::atomic_size_t part_removal_allocation_calls{0U};
std::atomic_size_t part_removal_failure_call{0U};
pugi::allocation_function delegated_part_removal_pugi_allocate = nullptr;

auto should_fail_part_removal_allocation() noexcept -> bool {
    if (!part_removal_allocation_window_enabled.load(
            std::memory_order_relaxed)) {
        return false;
    }

    const auto current_call =
        part_removal_allocation_calls.fetch_add(1U,
                                                std::memory_order_relaxed) +
        1U;
    const auto failure_call =
        part_removal_failure_call.load(std::memory_order_relaxed);
    return failure_call != 0U && current_call == failure_call;
}

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
[[nodiscard]] auto allocate_part_removal_unaligned(std::size_t size)
    -> void * {
    if (should_fail_part_removal_allocation()) {
        throw std::bad_alloc{};
    }
    if (void *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

[[nodiscard]] auto allocate_part_removal_aligned(std::size_t size,
                                                 std::size_t alignment)
    -> void * {
    if (should_fail_part_removal_allocation()) {
        throw std::bad_alloc{};
    }

    void *memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0U ? 1U : size) == 0) {
        return memory;
    }
    throw std::bad_alloc{};
}
#endif

auto controlled_part_removal_pugi_allocate(std::size_t size) -> void * {
    if (should_fail_part_removal_allocation()) {
        return nullptr;
    }
    return delegated_part_removal_pugi_allocate != nullptr
               ? delegated_part_removal_pugi_allocate(size)
               : nullptr;
}

class part_removal_pugi_memory_guard final {
  public:
    part_removal_pugi_memory_guard()
        : previous_allocation_(pugi::get_memory_allocation_function()),
          previous_deallocation_(pugi::get_memory_deallocation_function()) {
        delegated_part_removal_pugi_allocate = this->previous_allocation_;
        pugi::set_memory_management_functions(
            controlled_part_removal_pugi_allocate,
            this->previous_deallocation_);
    }

    part_removal_pugi_memory_guard(const part_removal_pugi_memory_guard &) =
        delete;
    auto operator=(const part_removal_pugi_memory_guard &)
        -> part_removal_pugi_memory_guard & = delete;

    ~part_removal_pugi_memory_guard() {
        part_removal_allocation_window_enabled.store(
            false, std::memory_order_relaxed);
        pugi::set_memory_management_functions(this->previous_allocation_,
                                              this->previous_deallocation_);
        delegated_part_removal_pugi_allocate = nullptr;
        part_removal_allocation_calls.store(0U, std::memory_order_relaxed);
        part_removal_failure_call.store(0U, std::memory_order_relaxed);
    }

  private:
    pugi::allocation_function previous_allocation_;
    pugi::deallocation_function previous_deallocation_;
};

class part_removal_allocation_window final {
  public:
    explicit part_removal_allocation_window(std::size_t failure_call) {
        part_removal_allocation_calls.store(0U,
                                            std::memory_order_relaxed);
        part_removal_failure_call.store(failure_call,
                                        std::memory_order_relaxed);
        part_removal_allocation_window_enabled.store(
            true, std::memory_order_release);
    }

    part_removal_allocation_window(const part_removal_allocation_window &) =
        delete;
    auto operator=(const part_removal_allocation_window &)
        -> part_removal_allocation_window & = delete;

    ~part_removal_allocation_window() {
        part_removal_allocation_window_enabled.store(
            false, std::memory_order_release);
    }
};

enum class related_part_kind { header, footer };

auto part_kind_name(related_part_kind kind) noexcept -> const char * {
    return kind == related_part_kind::header ? "header" : "footer";
}

auto remove_related_part(featherdoc::Document &document,
                         related_part_kind kind) -> bool {
    return kind == related_part_kind::header
               ? document.remove_header_part(0U)
               : document.remove_footer_part(0U);
}

void prepare_part_removal_fixture(featherdoc::Document &document,
                                  related_part_kind kind) {
    REQUIRE_FALSE(document.create_empty());

    auto body = document.paragraphs();
    REQUIRE(body.valid());
    REQUIRE(body.add_run("body text").valid());
    auto body_table = document.body_template().append_table(1U, 1U);
    REQUIRE(body_table.valid());
    REQUIRE(body_table.set_cell_text(0U, 0U, "body cell"));

    auto header = document.ensure_section_header_paragraphs(0U);
    REQUIRE(header.valid());
    REQUIRE(header.add_run("header text").valid());
    auto header_table = document.header_template(0U).append_table(1U, 1U);
    REQUIRE(header_table.valid());
    REQUIRE(header_table.set_cell_text(0U, 0U, "header cell"));

    auto footer = document.ensure_section_footer_paragraphs(0U);
    REQUIRE(footer.valid());
    REQUIRE(footer.add_run("footer text").valid());
    auto footer_table = document.footer_template(0U).append_table(1U, 1U);
    REQUIRE(footer_table.valid());
    REQUIRE(footer_table.set_cell_text(0U, 0U, "footer cell"));

    if (kind == related_part_kind::header) {
        REQUIRE(document
                    .assign_section_header_paragraphs(
                        0U, 0U,
                        featherdoc::section_reference_kind::first_page)
                    .valid());
        REQUIRE(document
                    .assign_section_header_paragraphs(
                        0U, 0U,
                        featherdoc::section_reference_kind::even_page)
                    .valid());
    } else {
        REQUIRE(document
                    .assign_section_footer_paragraphs(
                        0U, 0U,
                        featherdoc::section_reference_kind::first_page)
                    .valid());
        REQUIRE(document
                    .assign_section_footer_paragraphs(
                        0U, 0U,
                        featherdoc::section_reference_kind::even_page)
                    .valid());
    }
}

struct part_handles final {
    featherdoc::TemplatePart template_part;
    featherdoc::Paragraph paragraph;
    featherdoc::Run run;
    featherdoc::Table table;
    featherdoc::TableRow row;
    featherdoc::TableCell cell;
    featherdoc::Paragraph cell_paragraph;
    featherdoc::Run cell_run;
    std::string expected_text;
    std::string expected_cell_text;
};

auto capture_part_handles(featherdoc::TemplatePart template_part,
                          std::string expected_text,
                          std::string expected_cell_text) -> part_handles {
    auto paragraph = template_part.paragraphs();
    auto run = paragraph.runs();
    auto table = template_part.tables();
    auto row = table.rows();
    auto cell = row.cells();
    auto cell_paragraph = cell.paragraphs();
    auto cell_run = cell_paragraph.runs();

    return {std::move(template_part),
            paragraph,
            run,
            table,
            row,
            cell,
            cell_paragraph,
            cell_run,
            std::move(expected_text),
            std::move(expected_cell_text)};
}

void check_part_handles_are_live(part_handles &handles) {
    CHECK(handles.template_part);
    CHECK(handles.paragraph.valid());
    CHECK(handles.run.valid());
    CHECK(handles.table.valid());
    CHECK(handles.row.valid());
    CHECK(handles.cell.valid());
    CHECK(handles.cell_paragraph.valid());
    CHECK(handles.cell_run.valid());
    CHECK_EQ(handles.run.get_text(), handles.expected_text);
    CHECK_EQ(handles.cell_run.get_text(), handles.expected_cell_text);
}

void check_part_handles_are_stale(const part_handles &handles) {
    CHECK_FALSE(handles.template_part);
    CHECK_FALSE(handles.paragraph.valid());
    CHECK_FALSE(handles.run.valid());
    CHECK_FALSE(handles.table.valid());
    CHECK_FALSE(handles.row.valid());
    CHECK_FALSE(handles.cell.valid());
    CHECK_FALSE(handles.cell_paragraph.valid());
    CHECK_FALSE(handles.cell_run.valid());
}

auto sorted_serialized_entries(featherdoc::Document &document,
                               const std::filesystem::path &path)
    -> std::vector<std::pair<std::string, std::string>> {
    std::error_code remove_error;
    std::filesystem::remove(path, remove_error);
    REQUIRE_FALSE(document.save_as(path));

    auto entries = read_test_archive_entries(path);
    std::ranges::sort(entries, {}, &std::pair<std::string, std::string>::first);
    return entries;
}

struct temporary_part_removal_paths final {
    std::filesystem::path before;
    std::filesystem::path after;

    ~temporary_part_removal_paths() {
        std::error_code ignored_error;
        std::filesystem::remove(this->before, ignored_error);
        std::filesystem::remove(this->after, ignored_error);
    }
};

} // namespace

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
void *operator new(std::size_t size) {
    return allocate_part_removal_unaligned(size);
}
void *operator new[](std::size_t size) {
    return allocate_part_removal_unaligned(size);
}
void *operator new(std::size_t size, std::align_val_t alignment) {
    return allocate_part_removal_aligned(
        size, static_cast<std::size_t>(alignment));
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return allocate_part_removal_aligned(
        size, static_cast<std::size_t>(alignment));
}
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept {
    std::free(memory);
}
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
    "header and footer part removal is transactional for every allocation "
    "failure") {
    namespace fs = std::filesystem;

    const part_removal_pugi_memory_guard pugi_guard;

    for (const auto kind : {related_part_kind::header,
                            related_part_kind::footer}) {
        CAPTURE(part_kind_name(kind));

        std::size_t successful_allocation_count = 0U;
        {
            featherdoc::Document successful;
            prepare_part_removal_fixture(successful, kind);

            bool removed = false;
            {
                const part_removal_allocation_window allocation_window{0U};
                removed = remove_related_part(successful, kind);
            }
            successful_allocation_count = part_removal_allocation_calls.load(
                std::memory_order_relaxed);
            REQUIRE(removed);
        }
        REQUIRE_GT(successful_allocation_count, 2U);

        for (std::size_t failure_call = 1U;
             failure_call <= successful_allocation_count; ++failure_call) {
            CAPTURE(failure_call);
            CAPTURE(successful_allocation_count);

            temporary_part_removal_paths paths{
                fs::current_path() /
                    (std::string{"part_removal_before_"} +
                     part_kind_name(kind) + ".docx"),
                fs::current_path() /
                    (std::string{"part_removal_after_"} +
                     part_kind_name(kind) + ".docx")};

            featherdoc::Document document;
            prepare_part_removal_fixture(document, kind);
            const auto serialized_before =
                sorted_serialized_entries(document, paths.before);

            auto body_handles = capture_part_handles(
                document.body_template(), "body text", "body cell");
            auto header_handles = capture_part_handles(
                document.header_template(0U), "header text", "header cell");
            auto footer_handles = capture_part_handles(
                document.footer_template(0U), "footer text", "footer cell");
            check_part_handles_are_live(body_handles);
            check_part_handles_are_live(header_handles);
            check_part_handles_are_live(footer_handles);

            const auto removed_entry = std::string{
                kind == related_part_kind::header
                    ? header_handles.template_part.entry_name()
                    : footer_handles.template_part.entry_name()};
            const auto retained_entry = std::string{
                kind == related_part_kind::header
                    ? footer_handles.template_part.entry_name()
                    : header_handles.template_part.entry_name()};

            bool removal_result = true;
            bool threw = false;
            try {
                const part_removal_allocation_window allocation_window{
                    failure_call};
                removal_result = remove_related_part(document, kind);
            } catch (...) {
                threw = true;
            }

            CHECK_FALSE(threw);
            CHECK_FALSE(removal_result);
            CHECK_EQ(document.last_error().code,
                     std::make_error_code(std::errc::not_enough_memory));
            CHECK_FALSE(document.last_error().detail.empty());
            CHECK((document.last_error().entry_name ==
                       test_document_xml_entry ||
                   document.last_error().entry_name == "word/settings.xml"));
            CHECK_EQ(document.header_count(), 1U);
            CHECK_EQ(document.footer_count(), 1U);
            check_part_handles_are_live(body_handles);
            check_part_handles_are_live(header_handles);
            check_part_handles_are_live(footer_handles);

            const auto serialized_after_failure =
                sorted_serialized_entries(document, paths.after);
            CHECK_EQ(serialized_after_failure, serialized_before);

            REQUIRE(remove_related_part(document, kind));
            CHECK_EQ(document.header_count(),
                     kind == related_part_kind::header ? 0U : 1U);
            CHECK_EQ(document.footer_count(),
                     kind == related_part_kind::footer ? 0U : 1U);
            check_part_handles_are_stale(body_handles);
            check_part_handles_are_stale(header_handles);
            check_part_handles_are_stale(footer_handles);

            (void)sorted_serialized_entries(document, paths.after);
            CHECK_FALSE(test_docx_entry_exists(paths.after,
                                               removed_entry.c_str()));
            CHECK(test_docx_entry_exists(paths.after,
                                         retained_entry.c_str()));
            const auto document_xml =
                read_test_docx_entry(paths.after, test_document_xml_entry);
            const auto settings_xml =
                read_test_docx_entry(paths.after, "word/settings.xml");
            CHECK_NE(document_xml.find("<w:titlePg"), std::string::npos);
            CHECK_NE(settings_xml.find("<w:evenAndOddHeaders"),
                     std::string::npos);
        }
    }
}
