#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "doctest.h"
#include "basic_docx_archive_test_support.hpp"
#include "basic_document_xml_test_support.hpp"

#include <featherdoc.hpp>
#include <zip.h>
namespace {
template <class T, class = void>
inline constexpr bool supports_featherdoc_adl_begin_end = false;

template <class T>
inline constexpr bool supports_featherdoc_adl_begin_end<
    T, std::void_t<decltype(begin(std::declval<const T &>())),
                   decltype(end(std::declval<const T &>()))>> = true;

static_assert(supports_featherdoc_adl_begin_end<featherdoc::Paragraph>);
static_assert(!supports_featherdoc_adl_begin_end<featherdoc::template_schema>);
static_assert(!std::is_constructible_v<featherdoc::Run, pugi::xml_node,
                                       pugi::xml_node>);
static_assert(!std::is_constructible_v<featherdoc::Paragraph, pugi::xml_node,
                                       pugi::xml_node>);
static_assert(!std::is_constructible_v<featherdoc::Table, pugi::xml_node,
                                       pugi::xml_node>);
static_assert(!std::is_constructible_v<featherdoc::TableRow, pugi::xml_node,
                                       pugi::xml_node>);
static_assert(!std::is_constructible_v<featherdoc::TableCell, pugi::xml_node,
                                       pugi::xml_node>);

std::unordered_set<void *> tracked_pugi_allocations;
bool saw_unexpected_pugi_deallocation = false;
pugi::allocation_function delegated_pugi_allocate = nullptr;
std::size_t controlled_pugi_allocation_calls = 0U;
std::size_t controlled_pugi_failure_call = 0U;

auto tracked_pugi_allocate(std::size_t size) -> void * {
    void *ptr = std::malloc(size);
    if (ptr != nullptr) {
        tracked_pugi_allocations.insert(ptr);
    }
    return ptr;
}

auto tracked_pugi_deallocate(void *ptr) -> void {
    if (ptr == nullptr) {
        return;
    }

    if (tracked_pugi_allocations.erase(ptr) == 0U) {
        saw_unexpected_pugi_deallocation = true;
    }

    std::free(ptr);
}

auto controlled_pugi_allocate(std::size_t size) -> void * {
    ++controlled_pugi_allocation_calls;
    if (controlled_pugi_failure_call != 0U &&
        controlled_pugi_allocation_calls == controlled_pugi_failure_call) {
        return nullptr;
    }
    return delegated_pugi_allocate != nullptr ? delegated_pugi_allocate(size)
                                              : nullptr;
}

struct pugi_memory_management_guard final {
    pugi::allocation_function allocation{};
    pugi::deallocation_function deallocation{};

    pugi_memory_management_guard()
        : allocation(pugi::get_memory_allocation_function()),
          deallocation(pugi::get_memory_deallocation_function()) {}

    ~pugi_memory_management_guard() {
        pugi::set_memory_management_functions(this->allocation, this->deallocation);
        tracked_pugi_allocations.clear();
        saw_unexpected_pugi_deallocation = false;
        delegated_pugi_allocate = nullptr;
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 0U;
    }
};

} // namespace
