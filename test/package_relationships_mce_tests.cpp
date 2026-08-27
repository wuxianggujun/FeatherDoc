#include "../src/package_content_types_xml_helpers.hpp"
#include "../src/package_relationships_mce_helpers.hpp"
#include "../src/package_relationships_xml_helpers.hpp"
#include "../src/wordprocessingml_namespace_helpers.hpp"
#include "../src/xml_namespace_helpers.hpp"

#include <featherdoc/document_core.hpp>

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <string_view>

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY && defined(_WIN32)
#include <malloc.h>
#endif

namespace {

std::atomic_bool fail_next_std_allocation{false};
std::atomic_size_t allocations_to_skip_before_failure{0U};
pugi::allocation_function delegated_pugi_allocate = nullptr;
std::size_t controlled_pugi_allocation_calls = 0U;
std::size_t controlled_pugi_failure_call = 0U;

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
auto should_fail_std_allocation() noexcept -> bool {
    if (!fail_next_std_allocation.load(std::memory_order_relaxed)) {
        return false;
    }
    auto allocations_to_skip =
        allocations_to_skip_before_failure.load(std::memory_order_relaxed);
    while (allocations_to_skip != 0U) {
        if (allocations_to_skip_before_failure.compare_exchange_weak(
                allocations_to_skip, allocations_to_skip - 1U,
                std::memory_order_relaxed)) {
            return false;
        }
    }
    return fail_next_std_allocation.exchange(false, std::memory_order_relaxed);
}

[[nodiscard]] auto allocate_unaligned(std::size_t size) -> void * {
    if (should_fail_std_allocation()) {
        throw std::bad_alloc{};
    }
    if (void *memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }
    throw std::bad_alloc{};
}

[[nodiscard]] auto allocate_aligned(std::size_t size, std::size_t alignment)
    -> void * {
    if (should_fail_std_allocation()) {
        throw std::bad_alloc{};
    }
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
#endif

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
    pugi::allocation_function allocation{
        pugi::get_memory_allocation_function()};
    pugi::deallocation_function deallocation{
        pugi::get_memory_deallocation_function()};

    ~pugi_memory_management_guard() {
        pugi::set_memory_management_functions(allocation, deallocation);
        delegated_pugi_allocate = nullptr;
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 0U;
    }
};

} // namespace

#if defined(FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY) &&                     \
    FEATHERDOC_ALLOCATION_FAILURE_TEST_BINARY
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
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "allocation_failure_test_case.hpp"

namespace {

constexpr auto relationships_namespace = std::string_view{
    "http://schemas.openxmlformats.org/package/2006/relationships"};

auto preprocess(pugi::xml_document &document, std::string_view xml)
    -> featherdoc::detail::package_relationships_mce_result {
    const auto parsed = document.load_buffer(
        xml.data(), xml.size(),
        featherdoc::detail::package_relationships_xml_parse_options);
    REQUIRE_MESSAGE(parsed, parsed.description());
    return featherdoc::detail::preprocess_package_relationships_mce(document);
}

auto relationship_count(const pugi::xml_document &document) -> std::size_t {
    const auto root = featherdoc::detail::package_relationships_root(document);
    std::size_t count = 0U;
    for (auto relationship =
             featherdoc::detail::first_package_relationship(root);
         relationship != pugi::xml_node{};
         relationship =
             featherdoc::detail::next_package_relationship(relationship)) {
        ++count;
    }
    return count;
}

} // namespace

TEST_CASE("Relationships MCE removes ignorable attributes and complete "
          "subtrees") {
    pugi::xml_document document;
    const auto result = preprocess(
        document,
        R"(<r:Relationships xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x" x:root="drop"><r:Relationship Id="rOne" Type="urn:one" Target="one.bin" x:item="drop"/><x:Ignored mc:MustUnderstand="x"><r:Relationship Id="rHidden" Type="urn:hidden" Target="hidden.bin"/></x:Ignored></r:Relationships>)");
    INFO("status=" << static_cast<unsigned int>(result.status)
                   << ", detail=" << result.detail);
    REQUIRE(result);
    CHECK(
        featherdoc::detail::inspect_package_relationships_document(document) ==
        featherdoc::detail::package_relationships_document_state::valid);
    CHECK_EQ(relationship_count(document), 1U);
    const auto root = featherdoc::detail::package_relationships_root(document);
    CHECK(root.attribute("mc:Ignorable") == pugi::xml_attribute{});
    CHECK(root.attribute("x:root") == pugi::xml_attribute{});
    const auto relationship =
        featherdoc::detail::first_package_relationship(root);
    CHECK(relationship.attribute("x:item") == pugi::xml_attribute{});
}

TEST_CASE("Relationships MCE rejects non-ignorable extension markup") {
    using featherdoc::detail::package_relationships_mce_status;

    SUBCASE("element") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:x="urn:extension"><x:Unexpected/></Relationships>)");
        CHECK_EQ(result.status, package_relationships_mce_status::mismatch);
    }
    SUBCASE("attribute") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:x="urn:extension" x:Unexpected="value"/>)");
        CHECK_EQ(result.status, package_relationships_mce_status::mismatch);
    }
}

TEST_CASE("Relationships MCE ProcessContent supports exact names and "
          "wildcards") {
    SUBCASE("exact name unwraps and hoists namespace bindings") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<r:Relationships xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x" mc:ProcessContent="x:Wrapper"><x:Wrapper xmlns:q="http://schemas.openxmlformats.org/package/2006/relationships"><q:Relationship Id="rOne" Type="urn:one" Target="one.bin">中文<![CDATA[🙂]]></q:Relationship></x:Wrapper></r:Relationships>)");
        REQUIRE(result);
        REQUIRE(
            featherdoc::detail::inspect_package_relationships_document(
                document) ==
            featherdoc::detail::package_relationships_document_state::valid);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.name()}, "q:Relationship");
        CHECK_EQ(std::string_view{relationship.attribute("xmlns:q").value()},
                 relationships_namespace);
        CHECK_EQ(std::string_view{relationship.first_child().value()}, "中文");
        CHECK_EQ(relationship.first_child().next_sibling().type(),
                 pugi::node_cdata);
    }

    SUBCASE("wildcard unwraps every element in the ignorable namespace") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x" mc:ProcessContent="x:*"><x:First><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></x:First><x:Second><Relationship Id="rTwo" Type="urn:two" Target="two.bin"/></x:Second></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 2U);
    }

    SUBCASE("ProcessContent namespace must be ignorable") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:ProcessContent="x:*"/>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("a foreign element can declare its own namespace ignorable") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><x:Wrapper mc:Ignorable="x" mc:MustUnderstand="x"><Relationship Id="rIgnored" Type="urn:ignored" Target="ignored.bin"/></x:Wrapper></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 0U);
    }

    SUBCASE("a foreign element can locally select its own content") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><x:Wrapper mc:Ignorable="x" mc:ProcessContent="x:Wrapper"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></x:Wrapper></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 1U);
    }

    SUBCASE("an unwrapped foreign element permits an empty MustUnderstand") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><x:Wrapper mc:Ignorable="x" mc:ProcessContent="x:Wrapper" mc:MustUnderstand=""><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></x:Wrapper></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 1U);
    }

    SUBCASE("an unwrapped foreign element enforces MustUnderstand") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><x:Wrapper mc:Ignorable="x" mc:ProcessContent="x:Wrapper" mc:MustUnderstand="x"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></x:Wrapper></Relationships>)");
        CHECK_EQ(
            result.status,
            featherdoc::detail::package_relationships_mce_status::mismatch);
    }

    SUBCASE("empty Ignorable and ProcessContent lists are no-ops") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" mc:Ignorable=" &#x9;&#xA;" mc:ProcessContent=""><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 1U);
    }
}

TEST_CASE("Relationships MCE MustUnderstand fails closed only in processed "
          "markup") {
    SUBCASE("retained markup requires an unsupported namespace") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:MustUnderstand="x"/>)");
        CHECK_EQ(
            result.status,
            featherdoc::detail::package_relationships_mce_status::mismatch);
    }

    SUBCASE("MustUnderstand cannot name the MCE namespace") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" mc:MustUnderstand="mc"/>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("the MCE namespace cannot be declared ignorable") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" mc:Ignorable="mc"/>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("MustUnderstand in a completely ignored subtree is not run") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x"><x:Ignored mc:MustUnderstand="x"/></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 0U);
    }

    SUBCASE("MustUnderstand syntax in a completely ignored subtree is still "
            "validated") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x"><x:Ignored mc:MustUnderstand="missing"/></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
        CHECK(document.child("Relationships").child("x:Ignored") !=
              pugi::xml_node{});
    }

    SUBCASE("an empty MustUnderstand list is a no-op") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" mc:MustUnderstand=" &#x9;&#xA;"/>)");
        REQUIRE(result);
    }
}

TEST_CASE("Relationships MCE AlternateContent chooses Choice, Fallback, or "
          "nothing") {
    SUBCASE("first understood Choice wins") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="r"><Relationship Id="rChoice" Type="urn:choice" Target="choice.bin"/></mc:Choice><mc:Fallback><Relationship Id="rFallback" Type="urn:fallback" Target="fallback.bin" mc:MustUnderstand="x"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                 "rChoice");
    }

    SUBCASE("Fallback is used when no Choice is understood") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="x"><Relationship Id="rChoice" Type="urn:choice" Target="choice.bin" mc:MustUnderstand="x"/></mc:Choice><mc:Fallback><Relationship Id="rFallback" Type="urn:fallback" Target="fallback.bin"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                 "rFallback");
    }

    SUBCASE("AlternateContent without a matching branch emits nothing") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="x"><Relationship Id="rChoice" Type="urn:choice" Target="choice.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 0U);
    }

    SUBCASE("Requires cannot name the MCE namespace") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"><mc:AlternateContent><mc:Choice Requires="mc"><Relationship Id="rChoice" Type="urn:choice" Target="choice.bin"/></mc:Choice><mc:Fallback><Relationship Id="rFallback" Type="urn:fallback" Target="fallback.bin"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("MCE wrappers reject unknown unqualified attributes") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"><mc:AlternateContent Extra="bad"><mc:Choice Requires="r"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("ignorable foreign children of AlternateContent are skipped") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent mc:Ignorable="x"><x:FutureChoice mc:MustUnderstand="x"/><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                 "rSelected");
    }

    SUBCASE("an AlternateContent foreign child can declare itself ignorable") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><x:FutureChoice mc:Ignorable="x" mc:MustUnderstand="x"/><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 1U);
    }

    SUBCASE("ProcessContent foreign children of AlternateContent mismatch") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent mc:Ignorable="x" mc:ProcessContent="x:FutureChoice"><x:FutureChoice/><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        CHECK_EQ(
            result.status,
            featherdoc::detail::package_relationships_mce_status::mismatch);
    }

    SUBCASE("local ProcessContent on an AlternateContent foreign child "
            "mismatches") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><x:FutureChoice mc:Ignorable="x" mc:ProcessContent="x:FutureChoice"/><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        CHECK_EQ(
            result.status,
            featherdoc::detail::package_relationships_mce_status::mismatch);
    }

    SUBCASE("a selected Choice applies its local Ignorable declaration") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="r" mc:Ignorable="x" x:metadata="drop"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 1U);
    }

    SUBCASE("a selected Fallback applies its local Ignorable declaration") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="x"/><mc:Fallback mc:Ignorable="x" x:metadata="drop"><Relationship Id="rFallback" Type="urn:fallback" Target="fallback.bin"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                 "rFallback");
    }

    SUBCASE("an unsupported Choice validates but does not execute directives") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="x" mc:Ignorable="x" x:metadata="ignored" mc:ProcessContent="x:*" mc:MustUnderstand="x"><Relationship Id="rChoice" Type="urn:choice" Target="choice.bin"/></mc:Choice><mc:Fallback><Relationship Id="rFallback" Type="urn:fallback" Target="fallback.bin"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                 "rFallback");
    }

    SUBCASE("Choices after the selected Choice validate but do not execute") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice><mc:Choice Requires="x" mc:Ignorable="x" x:metadata="ignored" mc:ProcessContent="x:*" mc:MustUnderstand="x"><Relationship Id="rIgnored" Type="urn:ignored" Target="ignored.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                 "rSelected");
    }

    SUBCASE(
        "an unselected Fallback validates but does not execute directives") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice><mc:Fallback mc:Ignorable="x" x:metadata="ignored" mc:ProcessContent="x:*" mc:MustUnderstand="x"><Relationship Id="rIgnored" Type="urn:ignored" Target="ignored.bin"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                 "rSelected");
    }

    SUBCASE("an unselected branch still validates nested MCE syntax") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"><mc:AlternateContent><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice><mc:Fallback><mc:AlternateContent><mc:Choice Requires="missing"/></mc:AlternateContent></mc:Fallback></mc:AlternateContent></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
        CHECK(document.child("Relationships").child("mc:AlternateContent") !=
              pugi::xml_node{});
    }

    SUBCASE("valid MustUnderstand inside an unselected branch is not "
            "executed") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice><mc:Fallback mc:Ignorable="x"><x:Ignored mc:MustUnderstand="x"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        const auto relationship =
            featherdoc::detail::first_package_relationship(
                featherdoc::detail::package_relationships_root(document));
        REQUIRE(relationship != pugi::xml_node{});
        CHECK_EQ(std::string_view{relationship.attribute("Id").value()},
                 "rSelected");
    }

    SUBCASE("a selected Choice executes malformed ProcessContent") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"><mc:AlternateContent><mc:Choice Requires="r" mc:ProcessContent="malformed"><Relationship Id="rChoice" Type="urn:choice" Target="choice.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("a selected Fallback executes malformed ProcessContent") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="x"><Relationship Id="rChoice" Type="urn:choice" Target="choice.bin"/></mc:Choice><mc:Fallback mc:ProcessContent="malformed"><Relationship Id="rFallback" Type="urn:fallback" Target="fallback.bin"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("all Choice markup remains syntax-validated before selection") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension"><mc:AlternateContent><mc:Choice Requires="x" mc:ProcessContent="malformed"/><mc:Fallback/></mc:AlternateContent></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("a later Choice still rejects malformed Requires") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"><mc:AlternateContent><mc:Choice Requires="r"/><mc:Choice Requires=""/></mc:AlternateContent></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("an unselected Fallback remains syntax-validated") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"><mc:AlternateContent><mc:Choice Requires="r"/><mc:Fallback mc:MustUnderstand="missing"/></mc:AlternateContent></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }
}

TEST_CASE(
    "Relationships MCE statuses map allocation failure without throwing") {
    CHECK_EQ(featherdoc::detail::package_relationships_mce_error_code(
                 featherdoc::detail::package_relationships_mce_status::
                     allocation_failure),
             std::make_error_code(std::errc::not_enough_memory));

    featherdoc::document_error_info allocation_error;
    allocation_error.detail = "stale detail";
    allocation_error.entry_name = "stale entry";
    featherdoc::detail::package_relationships_mce_result allocation_result;
    allocation_result.status = featherdoc::detail::
        package_relationships_mce_status::allocation_failure;
    CHECK_EQ(featherdoc::detail::set_package_relationships_mce_last_error(
                 allocation_error, allocation_result, "_rels/.rels"),
             std::make_error_code(std::errc::not_enough_memory));
    CHECK(allocation_error.detail.empty());
    CHECK(allocation_error.entry_name.empty());
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "Relationships MCE output construction fails closed for every "
    "pugixml allocation failure") {
    const auto load_source = [](pugi::xml_document &document) {
        const auto long_namespace =
            std::string{"urn:extension:"} + std::string(40'000U, 'n');
        const auto long_target = std::string(40'000U, 't');
        const auto long_text = std::string(40'000U, 'p');
        const auto long_cdata = std::string(40'000U, 'c');
        const auto long_comment = std::string(40'000U, 'm');
        const auto long_pi = std::string(40'000U, 'i');

        auto xml = std::string{"<?probe "};
        xml += long_pi;
        xml +=
            R"(?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x=")";
        xml += long_namespace;
        xml +=
            R"(" mc:Ignorable="x"><Relationship Id="rOne" Type="urn:test" Target=")";
        xml += long_target;
        xml += R"(">)";
        xml += long_text;
        xml += "<![CDATA[";
        xml += long_cdata;
        xml += "]]><!--";
        xml += long_comment;
        xml += "--></Relationship><x:Ignored/></Relationships>";

        controlled_pugi_failure_call = 0U;
        const auto parsed = document.load_buffer(
            xml.data(), xml.size(),
            featherdoc::detail::package_relationships_xml_parse_options);
        REQUIRE_MESSAGE(parsed, parsed.description());
    };

    pugi_memory_management_guard guard;
    delegated_pugi_allocate = guard.allocation;
    pugi::set_memory_management_functions(controlled_pugi_allocate,
                                          guard.deallocation);

    std::size_t successful_preprocess_allocation_count = 0U;
    {
        pugi::xml_document baseline;
        load_source(baseline);
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = 0U;
        REQUIRE(
            featherdoc::detail::preprocess_package_relationships_mce(baseline));
        successful_preprocess_allocation_count =
            controlled_pugi_allocation_calls;
        REQUIRE_GT(successful_preprocess_allocation_count, 1U);
    }

    for (std::size_t failure_call = 1U;
         failure_call <= successful_preprocess_allocation_count;
         ++failure_call) {
        pugi::xml_document document;
        load_source(document);
        controlled_pugi_allocation_calls = 0U;
        controlled_pugi_failure_call = failure_call;

        const auto result =
            featherdoc::detail::preprocess_package_relationships_mce(document);
        CAPTURE(failure_call);
        CAPTURE(successful_preprocess_allocation_count);
        CAPTURE(controlled_pugi_allocation_calls);
        CAPTURE(result.detail);
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     allocation_failure);
        CHECK(result.detail.empty());
        CHECK(document.child("Relationships") != pugi::xml_node{});
        CHECK(document.child("Relationships").attribute("mc:Ignorable") !=
              pugi::xml_attribute{});
        CHECK(document.child("Relationships").child("x:Ignored") !=
              pugi::xml_node{});
        CHECK_EQ(relationship_count(document), 1U);
    }

    controlled_pugi_failure_call = 0U;
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "XML security error formatters convert diagnostic allocation "
    "failures to not_enough_memory") {
    SUBCASE("Relationships MCE diagnostic copy") {
        featherdoc::detail::package_relationships_mce_result result;
        result.status = featherdoc::detail::package_relationships_mce_status::
            invalid_mce_markup;
        result.detail.assign(256U, 'm');
        featherdoc::document_error_info error_info;
        error_info.detail = "stale detail";
        error_info.entry_name = "stale entry";

        allocations_to_skip_before_failure.store(0U, std::memory_order_relaxed);
        fail_next_std_allocation.store(true, std::memory_order_relaxed);
        const auto code =
            featherdoc::detail::set_package_relationships_mce_last_error(
                error_info, result, "_rels/.rels");
        const auto failure_was_consumed = !fail_next_std_allocation.exchange(
            false, std::memory_order_relaxed);

        CHECK(failure_was_consumed);
        CHECK_EQ(code, std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(error_info.code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK(error_info.detail.empty());
        CHECK(error_info.entry_name.empty());
    }

    SUBCASE("WordprocessingML root-mismatch diagnostic construction") {
        featherdoc::detail::wordprocessingml_namespace_result result;
        result.actual_root_namespace_uri = "urn:unexpected-root-namespace";
        result.actual_root_local_name = "unexpected";
        const std::string entry_name(256U, 'e');
        featherdoc::document_error_info error_info;
        error_info.detail = "stale detail";
        error_info.entry_name = "stale entry";

        allocations_to_skip_before_failure.store(0U, std::memory_order_relaxed);
        fail_next_std_allocation.store(true, std::memory_order_relaxed);
        const auto code =
            featherdoc::detail::set_wordprocessingml_root_mismatch_last_error(
                error_info, result,
                featherdoc::detail::wordprocessingml_part_kind::header,
                entry_name);
        const auto failure_was_consumed = !fail_next_std_allocation.exchange(
            false, std::memory_order_relaxed);

        CHECK(failure_was_consumed);
        CHECK_EQ(code, std::make_error_code(std::errc::not_enough_memory));
        CHECK_EQ(error_info.code,
                 std::make_error_code(std::errc::not_enough_memory));
        CHECK(error_info.detail.empty());
        CHECK(error_info.entry_name.empty());
    }
}

FEATHERDOC_ALLOCATION_FAILURE_TEST_CASE(
    "Relationships MCE catches a real standard allocation failure") {
    pugi::xml_document document;
    const auto parsed = document.load_string(
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></Relationships>)",
        featherdoc::detail::package_relationships_xml_parse_options);
    REQUIRE_MESSAGE(parsed, parsed.description());

// MSVC's checked-iterator debug vector constructor allocates its container
// proxy from a noexcept constructor. Skip only that implementation allocation
// so the injected failure reaches the first ordinary, catchable traversal
// allocation. Other standard libraries do not allocate for an empty vector.
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) &&                     \
    _ITERATOR_DEBUG_LEVEL != 0
    allocations_to_skip_before_failure.store(1U, std::memory_order_relaxed);
#else
    allocations_to_skip_before_failure.store(0U, std::memory_order_relaxed);
#endif
    fail_next_std_allocation.store(true, std::memory_order_relaxed);
    const auto result =
        featherdoc::detail::preprocess_package_relationships_mce(document);

    CHECK_FALSE(fail_next_std_allocation.load(std::memory_order_relaxed));
    CHECK_EQ(result.status,
             featherdoc::detail::package_relationships_mce_status::
                 allocation_failure);
    CHECK(document.child("Relationships") != pugi::xml_node{});
}

TEST_CASE("Relationships MCE preserves declarations comments and processing "
          "instructions") {
    pugi::xml_document document;
    const auto result = preprocess(
        document,
        R"(<?xml version="1.0" encoding="UTF-8"?><!--中文注释--><?featherdoc 中文?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rOne" Type="urn:one" Target="中文.bin"/></Relationships>)");
    REQUIRE(result);

    auto node = document.first_child();
    REQUIRE(node != pugi::xml_node{});
    CHECK_EQ(node.type(), pugi::node_declaration);
    node = node.next_sibling();
    REQUIRE(node != pugi::xml_node{});
    CHECK_EQ(node.type(), pugi::node_comment);
    CHECK_EQ(std::string_view{node.value()}, "中文注释");
    node = node.next_sibling();
    REQUIRE(node != pugi::xml_node{});
    CHECK_EQ(node.type(), pugi::node_pi);
    CHECK_EQ(std::string_view{node.name()}, "featherdoc");
    CHECK_EQ(std::string_view{node.value()}, "中文");
}

TEST_CASE("Relationships namespace validation enforces XML Namespaces and "
          "Unicode NCNames") {
    using featherdoc::detail::validate_xml_namespace_well_formedness;

    SUBCASE("valid Chinese prefix and local name") {
        pugi::xml_document document;
        REQUIRE(document.load_string(
            R"(<关系:Relationships xmlns:关系="http://schemas.openxmlformats.org/package/2006/relationships"><关系:Relationship Id="rOne" Type="urn:one" Target="one.bin"/></关系:Relationships>)"));
        CHECK(validate_xml_namespace_well_formedness(document).valid);
        const auto result =
            featherdoc::detail::preprocess_package_relationships_mce(document);
        REQUIRE(result);
        CHECK(featherdoc::detail::inspect_package_relationships_document(
                  document) ==
              featherdoc::detail::package_relationships_document_state::valid);
    }

    SUBCASE("digit-start and combining-start NCNames are rejected") {
        pugi::xml_document digit_start;
        auto digit_root = digit_start.append_child("p:1bad");
        digit_root.append_attribute("xmlns:p").set_value("urn:test");
        CHECK_FALSE(validate_xml_namespace_well_formedness(digit_start).valid);

        pugi::xml_document combining_start;
        auto combining_root = combining_start.append_child("p:́bad");
        combining_root.append_attribute("xmlns:p").set_value("urn:test");
        CHECK_FALSE(
            validate_xml_namespace_well_formedness(combining_start).valid);
    }

    SUBCASE("malformed and reserved namespace declarations are rejected") {
        const auto check_invalid = [](const char *attribute_name,
                                      const char *value) {
            pugi::xml_document document;
            auto root = document.append_child("Relationships");
            root.append_attribute("xmlns").set_value(
                relationships_namespace.data());
            root.append_attribute(attribute_name).set_value(value);
            CHECK_FALSE(validate_xml_namespace_well_formedness(document).valid);
        };
        check_invalid("xmlns:a:b", "urn:test");
        check_invalid("xmlns:xmlns", "urn:test");
        check_invalid("xmlns:xml", "urn:wrong");
        check_invalid("xmlns:x", "http://www.w3.org/2000/xmlns/");
    }

    SUBCASE("unbound and multiple-colon QNames are rejected") {
        pugi::xml_document unbound;
        unbound.append_child("missing:Relationships");
        CHECK_FALSE(validate_xml_namespace_well_formedness(unbound).valid);

        pugi::xml_document multiple_colons;
        multiple_colons.append_child("a:b:c");
        CHECK_FALSE(
            validate_xml_namespace_well_formedness(multiple_colons).valid);
    }

    SUBCASE("duplicate expanded attributes are rejected") {
        pugi::xml_document document;
        REQUIRE(document.load_string(
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:a="urn:same" xmlns:b="urn:same" a:value="one" b:value="two"/>)"));
        CHECK_FALSE(validate_xml_namespace_well_formedness(document).valid);
    }
}

TEST_CASE("Relationships schema permits text and CDATA but not element "
          "content") {
    pugi::xml_document text_document;
    auto result = preprocess(
        text_document,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rOne" Type="urn:one" Target="one.bin">中文<![CDATA[🙂]]></Relationship></Relationships>)");
    REQUIRE(result);
    CHECK(featherdoc::detail::inspect_package_relationships_document(
              text_document) ==
          featherdoc::detail::package_relationships_document_state::valid);

    pugi::xml_document child_document;
    result = preprocess(
        child_document,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rOne" Type="urn:one" Target="one.bin"><Nested/></Relationship></Relationships>)");
    REQUIRE(result);
    CHECK(featherdoc::detail::inspect_package_relationships_document(
              child_document) ==
          featherdoc::detail::package_relationships_document_state::invalid);
}

TEST_CASE("Content Types declarations reject all text and CDATA content") {
    const auto check_invalid = [](std::string_view content) {
        pugi::xml_document document;
        const auto parsed =
            document.load_buffer(content.data(), content.size(),
                                 pugi::parse_default | pugi::parse_ws_pcdata);
        REQUIRE_MESSAGE(parsed, parsed.description());
        CHECK(
            featherdoc::detail::
                inspect_package_content_types_document_structure(document)
                    .state ==
            featherdoc::detail::package_content_types_document_state::invalid);
    };
    check_invalid(
        R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="xml" ContentType="application/xml"> </Default></Types>)");
    check_invalid(
        R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Override PartName="/word/document.xml" ContentType="application/xml"><![CDATA[ ]]></Override></Types>)");
}

TEST_CASE("Relationships xsi attributes follow MCE ignorable rules") {
    using featherdoc::detail::package_relationships_mce_status;
    pugi::xml_document rejected;
    auto result = preprocess(
        rejected,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:nil="false"/>)");
    CHECK_EQ(result.status, package_relationships_mce_status::mismatch);

    pugi::xml_document accepted_as_ignorable;
    result = preprocess(
        accepted_as_ignorable,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" mc:Ignorable="xsi" xsi:nil="false"/>)");
    REQUIRE(result);
    CHECK(accepted_as_ignorable.child("Relationships").attribute("xsi:nil") ==
          pugi::xml_attribute{});

    pugi::xml_document relationship_xml_attribute;
    result = preprocess(
        relationship_xml_attribute,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" mc:Ignorable="xml"><Relationship Id="rOne" Type="urn:one" Target="one.bin" xml:lang="zh-CN"/></Relationships>)");
    REQUIRE(result);
    const auto relationship =
        relationship_xml_attribute.child("Relationships").child("Relationship");
    REQUIRE(relationship != pugi::xml_node{});
    CHECK(relationship.attribute("xml:lang") == pugi::xml_attribute{});
}

TEST_CASE("Relationships xml namespace elements cannot be made ignorable") {
    pugi::xml_document direct_element;
    auto result = preprocess(
        direct_element,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><xml:evil/></Relationships>)");
    CHECK_EQ(result.status,
             featherdoc::detail::package_relationships_mce_status::mismatch);

    pugi::xml_document ignorable_xml;
    result = preprocess(
        ignorable_xml,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" mc:Ignorable="xml"><xml:evil/></Relationships>)");
    CHECK_EQ(result.status,
             featherdoc::detail::package_relationships_mce_status::mismatch);
}

TEST_CASE("ProcessContent wrappers reject only inheritable xml attributes") {
    const auto check_inheritable_attribute_rejected = [](std::string_view
                                                             attribute) {
        pugi::xml_document document;
        auto xml = std::string{
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x" mc:ProcessContent="x:*"><x:Wrapper )"};
        xml += attribute;
        xml +=
            R"(><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></x:Wrapper></Relationships>)";
        const auto result = preprocess(document, xml);
        CHECK_EQ(
            result.status,
            featherdoc::detail::package_relationships_mce_status::mismatch);
    };

    check_inheritable_attribute_rejected(R"(xml:base="../")");
    check_inheritable_attribute_rejected(R"(xml:lang="zh-CN")");
    check_inheritable_attribute_rejected(R"(xml:space="preserve")");

    pugi::xml_document discarded_non_inheritable_attributes;
    const auto result = preprocess(
        discarded_non_inheritable_attributes,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" mc:Ignorable="x" mc:ProcessContent="x:*"><x:Wrapper xml:id="wrapper" xsi:nil="false"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></x:Wrapper></Relationships>)");
    REQUIRE(result);
    CHECK_EQ(relationship_count(discarded_non_inheritable_attributes), 1U);
}

TEST_CASE("selected MCE branch wrappers reject xml namespace attributes") {
    pugi::xml_document document;
    const auto result = preprocess(
        document,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" mc:Ignorable="xml"><mc:AlternateContent><mc:Choice Requires="r" xml:id="choice"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
    CHECK_EQ(result.status,
             featherdoc::detail::package_relationships_mce_status::mismatch);
}

TEST_CASE("MCE wrappers treat xsi attributes as an ordinary ignorable "
          "namespace") {
    SUBCASE("ignorable xsi attributes are accepted on every MCE wrapper") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" mc:Ignorable="xsi"><mc:AlternateContent xsi:future="alternate"><mc:Choice Requires="r" xsi:future="choice"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></mc:Choice><mc:Fallback xsi:future="fallback"><Relationship Id="rFallback" Type="urn:fallback" Target="fallback.bin"/></mc:Fallback></mc:AlternateContent></Relationships>)");
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 1U);
        CHECK(document.child("Relationships").child("mc:AlternateContent") ==
              pugi::xml_node{});
    }

    SUBCASE("non-ignorable xsi attributes still cause a mismatch") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><mc:AlternateContent><mc:Choice Requires="r" xsi:future="choice"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/></mc:Choice></mc:AlternateContent></Relationships>)");
        CHECK_EQ(
            result.status,
            featherdoc::detail::package_relationships_mce_status::mismatch);
    }
}

TEST_CASE("ignorable xsi elements are discarded like other foreign markup") {
    pugi::xml_document document;
    const auto result = preprocess(
        document,
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" mc:Ignorable="xsi"><xsi:Future><Relationship Id="rIgnored" Type="urn:ignored" Target="ignored.bin"/></xsi:Future><Relationship Id="rKept" Type="urn:kept" Target="kept.bin"/></Relationships>)");
    REQUIRE(result);
    CHECK_EQ(relationship_count(document), 1U);
    CHECK(document.child("Relationships").child("xsi:Future") ==
          pugi::xml_node{});
}

TEST_CASE("Relationships MCE rejects obsolete Preserve directives") {
    SUBCASE("retained markup rejects Preserve directives") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x" mc:PreserveElements="x:Old"/>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
    }

    SUBCASE("ignored subtrees still reject Preserve directives") {
        pugi::xml_document document;
        const auto result = preprocess(
            document,
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x"><x:Ignored mc:PreserveAttributes="x:old"/></Relationships>)");
        CHECK_EQ(result.status,
                 featherdoc::detail::package_relationships_mce_status::
                     invalid_mce_markup);
        CHECK(document.child("Relationships").child("x:Ignored") !=
              pugi::xml_node{});
    }
}

TEST_CASE("Relationships MCE bounds namespace-hoist amplification") {
    using featherdoc::detail::package_relationships_mce_limits;
    using featherdoc::detail::package_relationships_mce_status;

    const package_relationships_mce_limits defaults;
    CHECK_EQ(defaults.maximum_elements, 1'000'000U);
    CHECK_EQ(defaults.maximum_output_attributes, 262'144U);
    CHECK_EQ(defaults.maximum_hoisted_bindings_per_element, 4'096U);
    CHECK_EQ(defaults.maximum_namespace_work_bytes, 64U * 1024U * 1024U);

    const auto load_amplifying_document = [](pugi::xml_document &document) {
        const auto parsed = document.load_string(
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x" mc:ProcessContent="x:Wrapper"><x:Wrapper xmlns:a="urn:a" xmlns:b="urn:b"><Relationship Id="rOne" Type="urn:one" Target="one.bin"/><Relationship Id="rTwo" Type="urn:two" Target="two.bin"/></x:Wrapper></Relationships>)",
            featherdoc::detail::package_relationships_xml_parse_options);
        REQUIRE_MESSAGE(parsed, parsed.description());
    };

    SUBCASE("the exact output-attribute boundary succeeds") {
        pugi::xml_document document;
        load_amplifying_document(document);
        package_relationships_mce_limits limits;
        limits.maximum_output_attributes = 19U;
        limits.maximum_hoisted_bindings_per_element = 5U;
        const auto result =
            featherdoc::detail::preprocess_package_relationships_mce(document,
                                                                     limits);
        REQUIRE(result);
        CHECK_EQ(relationship_count(document), 2U);
    }

    SUBCASE("one fewer output attribute fails without replacing the source") {
        pugi::xml_document document;
        load_amplifying_document(document);
        package_relationships_mce_limits limits;
        limits.maximum_output_attributes = 18U;
        limits.maximum_hoisted_bindings_per_element = 5U;
        const auto result =
            featherdoc::detail::preprocess_package_relationships_mce(document,
                                                                     limits);
        CHECK_EQ(result.status,
                 package_relationships_mce_status::invalid_mce_markup);
        CHECK(document.child("Relationships").child("x:Wrapper") !=
              pugi::xml_node{});
        CHECK(document.child("Relationships").attribute("mc:ProcessContent") !=
              pugi::xml_attribute{});
    }

    SUBCASE("the exact effective-binding boundary succeeds") {
        pugi::xml_document document;
        load_amplifying_document(document);
        package_relationships_mce_limits limits;
        limits.maximum_output_attributes = 19U;
        limits.maximum_hoisted_bindings_per_element = 5U;
        REQUIRE(featherdoc::detail::preprocess_package_relationships_mce(
            document, limits));
    }

    SUBCASE("one fewer effective binding fails before replacing the source") {
        pugi::xml_document document;
        load_amplifying_document(document);
        package_relationships_mce_limits limits;
        limits.maximum_output_attributes = 19U;
        limits.maximum_hoisted_bindings_per_element = 4U;
        const auto result =
            featherdoc::detail::preprocess_package_relationships_mce(document,
                                                                     limits);
        CHECK_EQ(result.status,
                 package_relationships_mce_status::invalid_mce_markup);
        CHECK(document.child("Relationships").child("x:Wrapper") !=
              pugi::xml_node{});
    }

    SUBCASE("the element limit includes unselected branch contents") {
        pugi::xml_document document;
        const auto parsed = document.load_string(
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"><mc:AlternateContent><mc:Choice Requires="r"><Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/></mc:Choice><mc:Fallback><Relationship Id="rUnselected" Type="urn:unselected" Target="unselected.bin"/></mc:Fallback></mc:AlternateContent></Relationships>)",
            featherdoc::detail::package_relationships_xml_parse_options);
        REQUIRE_MESSAGE(parsed, parsed.description());

        package_relationships_mce_limits limits;
        limits.maximum_elements = 5U;
        const auto result =
            featherdoc::detail::preprocess_package_relationships_mce(document,
                                                                     limits);
        CHECK_EQ(result.status,
                 package_relationships_mce_status::invalid_mce_markup);
        CHECK(document.child("Relationships").child("mc:AlternateContent") !=
              pugi::xml_node{});
    }
}

TEST_CASE("Relationships MCE bounds cumulative namespace string work") {
    using featherdoc::detail::package_relationships_mce_limits;
    using featherdoc::detail::package_relationships_mce_status;

    const auto load_repeated_directives = [](pugi::xml_document &document,
                                             std::size_t child_count) {
        const auto long_namespace_uri =
            std::string{"urn:extension:"} + std::string(1'024U, 'u');
        auto xml = std::string{
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x=")"};
        xml += long_namespace_uri;
        xml += R"(" mc:Ignorable="x">)";
        for (std::size_t index = 0U; index < child_count; ++index) {
            xml += R"(<Relationship mc:Ignorable="x" Id="r)";
            xml += std::to_string(index);
            xml += R"(" Type="urn:test" Target="target.bin"/>)";
        }
        xml += "</Relationships>";
        const auto parsed = document.load_buffer(
            xml.data(), xml.size(),
            featherdoc::detail::package_relationships_xml_parse_options);
        REQUIRE_MESSAGE(parsed, parsed.description());
    };

    const auto load_repeated_hoists = [](pugi::xml_document &document,
                                         std::size_t child_count) {
        const auto long_namespace_uri =
            std::string{"urn:hoisted:"} + std::string(1'024U, 'h');
        auto xml = std::string{
            R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" xmlns:x="urn:extension" mc:Ignorable="x" mc:ProcessContent="x:Wrapper"><x:Wrapper xmlns:a=")"};
        xml += long_namespace_uri;
        xml += R"(">)";
        for (std::size_t index = 0U; index < child_count; ++index) {
            xml += R"(<Relationship Id="r)";
            xml += std::to_string(index);
            xml += R"(" Type="urn:test" Target="target.bin"/>)";
        }
        xml += "</x:Wrapper></Relationships>";
        const auto parsed = document.load_buffer(
            xml.data(), xml.size(),
            featherdoc::detail::package_relationships_xml_parse_options);
        REQUIRE_MESSAGE(parsed, parsed.description());
    };

    package_relationships_mce_limits limits;
    limits.maximum_namespace_work_bytes = 64U * 1024U;

    SUBCASE("a normal number of long namespace references remains supported") {
        pugi::xml_document document;
        load_repeated_directives(document, 1U);
        REQUIRE(featherdoc::detail::preprocess_package_relationships_mce(
            document, limits));
        CHECK_EQ(relationship_count(document), 1U);
    }

    SUBCASE(
        "repeated inherited namespace references exhaust one shared budget") {
        pugi::xml_document document;
        load_repeated_directives(document, 64U);
        const auto result =
            featherdoc::detail::preprocess_package_relationships_mce(document,
                                                                     limits);
        CHECK_EQ(result.status,
                 package_relationships_mce_status::resource_limit);
        CHECK_EQ(result.detail,
                 "Relationships MCE cumulative namespace work exceeds the "
                 "supported limit");
        CHECK(document.child("Relationships").attribute("mc:Ignorable") !=
              pugi::xml_attribute{});
        CHECK_EQ(relationship_count(document), 64U);

        featherdoc::document_error_info error_info;
        CHECK_EQ(featherdoc::detail::set_package_relationships_mce_last_error(
                     error_info, result, "_rels/.rels"),
                 featherdoc::make_error_code(
                     featherdoc::document_errc::archive_limit_exceeded));
        CHECK_EQ(error_info.entry_name, "_rels/.rels");
        CHECK_EQ(error_info.detail, result.detail);
    }

    SUBCASE("repeated namespace hoists are charged before output copies") {
        pugi::xml_document document;
        load_repeated_hoists(document, 64U);
        const auto result =
            featherdoc::detail::preprocess_package_relationships_mce(document,
                                                                     limits);
        CHECK_EQ(result.status,
                 package_relationships_mce_status::resource_limit);
        CHECK(document.child("Relationships").child("x:Wrapper") !=
              pugi::xml_node{});
        CHECK_EQ(relationship_count(document), 0U);
    }
}

TEST_CASE("deep AlternateContent choices use scoped namespace lookup and a "
          "bounded work budget") {
    constexpr std::size_t nesting_depth = 48U;
    constexpr std::size_t choices_per_level = 12U;

    auto xml = std::string{
        R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:r="http://schemas.openxmlformats.org/package/2006/relationships" xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006">)"};
    for (std::size_t depth = 0U; depth < nesting_depth; ++depth) {
        xml += R"(<mc:AlternateContent><mc:Choice Requires="r">)";
    }
    xml +=
        R"(<Relationship Id="rSelected" Type="urn:selected" Target="selected.bin"/>)";
    for (std::size_t depth = 0U; depth < nesting_depth; ++depth) {
        xml += "</mc:Choice>";
        for (std::size_t choice = 1U; choice < choices_per_level; ++choice) {
            xml += R"(<mc:Choice Requires="r"/>)";
        }
        xml += "</mc:AlternateContent>";
    }
    xml += "</Relationships>";

    const auto load = [&](pugi::xml_document &document) {
        const auto parsed = document.load_buffer(
            xml.data(), xml.size(),
            featherdoc::detail::package_relationships_xml_parse_options);
        REQUIRE_MESSAGE(parsed, parsed.description());
    };

    pugi::xml_document accepted;
    load(accepted);
    REQUIRE(featherdoc::detail::preprocess_package_relationships_mce(accepted));
    CHECK_EQ(relationship_count(accepted), 1U);

    pugi::xml_document limited;
    load(limited);
    featherdoc::detail::package_relationships_mce_limits limits;
    limits.maximum_namespace_work_bytes = 16U * 1024U;
    const auto result =
        featherdoc::detail::preprocess_package_relationships_mce(limited,
                                                                 limits);
    CHECK_EQ(
        result.status,
        featherdoc::detail::package_relationships_mce_status::resource_limit);
    CHECK(limited.child("Relationships").child("mc:AlternateContent") !=
          pugi::xml_node{});
}
