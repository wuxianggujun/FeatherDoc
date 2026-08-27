#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <unordered_set>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <pugixml.hpp>

namespace {

std::unordered_set<void *> tracked_allocations;
std::size_t allocation_calls = 0U;
std::size_t deallocation_calls = 0U;
std::size_t peak_live_allocations = 0U;
bool unexpected_deallocation = false;

auto tracked_allocate(std::size_t size) -> void * {
    void *const allocation = std::malloc(size);
    if (allocation == nullptr) {
        return nullptr;
    }

    tracked_allocations.insert(allocation);
    ++allocation_calls;
    peak_live_allocations =
        (std::max)(peak_live_allocations, tracked_allocations.size());
    return allocation;
}

auto tracked_deallocate(void *allocation) -> void {
    if (allocation == nullptr) {
        return;
    }

    if (tracked_allocations.erase(allocation) == 0U) {
        unexpected_deallocation = true;
        return;
    }

    ++deallocation_calls;
    std::free(allocation);
}

class pugi_memory_tracking_scope final {
  public:
    pugi_memory_tracking_scope()
        : previous_allocate_(pugi::get_memory_allocation_function()),
          previous_deallocate_(pugi::get_memory_deallocation_function()) {
        tracked_allocations.clear();
        allocation_calls = 0U;
        deallocation_calls = 0U;
        peak_live_allocations = 0U;
        unexpected_deallocation = false;
        pugi::set_memory_management_functions(tracked_allocate,
                                              tracked_deallocate);
    }

    pugi_memory_tracking_scope(const pugi_memory_tracking_scope &) = delete;
    auto operator=(const pugi_memory_tracking_scope &)
        -> pugi_memory_tracking_scope & = delete;

    ~pugi_memory_tracking_scope() {
        pugi::set_memory_management_functions(previous_allocate_,
                                              previous_deallocate_);

        // Keep a failed assertion from leaking memory into later test cases.
        for (void *const allocation : tracked_allocations) {
            std::free(allocation);
        }
        tracked_allocations.clear();
    }

    [[nodiscard]] auto live_allocations() const -> std::size_t {
        return tracked_allocations.size();
    }

    [[nodiscard]] auto allocations() const -> std::size_t {
        return allocation_calls;
    }

    [[nodiscard]] auto deallocations() const -> std::size_t {
        return deallocation_calls;
    }

    [[nodiscard]] auto peak_allocations() const -> std::size_t {
        return peak_live_allocations;
    }

    [[nodiscard]] auto saw_unexpected_deallocation() const -> bool {
        return unexpected_deallocation;
    }

  private:
    pugi::allocation_function previous_allocate_{};
    pugi::deallocation_function previous_deallocate_{};
};

auto make_xml_name(const std::string &prefix, std::size_t index)
    -> std::string {
    return prefix + std::to_string(index) + "_" + std::string(80U, 'n');
}

auto make_xml_value(const std::string &prefix, std::size_t index)
    -> std::string {
    return prefix + std::to_string(index) + "_" + std::string(192U, 'v');
}

auto child_count(const pugi::xml_node &parent) -> std::size_t {
    std::size_t count = 0U;
    for (auto child = parent.first_child(); child;
         child = child.next_sibling()) {
        ++count;
    }
    return count;
}

auto append_dynamic_leaf(pugi::xml_node parent, std::size_t index)
    -> pugi::xml_node {
    auto leaf = parent.append_child(pugi::node_element);
    if (!leaf) {
        return {};
    }

    const auto leaf_name = make_xml_name("leaf_", index);
    if (!leaf.set_name(leaf_name.c_str())) {
        return {};
    }

    auto attribute = leaf.append_attribute("placeholder");
    const auto attribute_name = make_xml_name("attribute_", index);
    const auto attribute_value = make_xml_value("attribute-value-", index);
    if (!attribute || !attribute.set_name(attribute_name.c_str()) ||
        !attribute.set_value(attribute_value.c_str())) {
        return {};
    }

    auto text = leaf.append_child(pugi::node_pcdata);
    const auto text_value = make_xml_value("text-value-", index);
    if (!text || !text.set_value(text_value.c_str())) {
        return {};
    }

    return leaf;
}

} // namespace

TEST_CASE("iterative remove_child preserves branching subtree siblings") {
    pugi_memory_tracking_scope memory;

    {
        pugi::xml_document document;
        auto root = document.append_child("root");
        auto before = root.append_child("before");
        auto victim = root.append_child("victim");
        auto after = root.append_child("after");
        REQUIRE(root);
        REQUIRE(before);
        REQUIRE(victim);
        REQUIRE(after);

        REQUIRE(before.append_attribute("marker").set_value("before-value"));
        REQUIRE(after.append_attribute("marker").set_value("after-value"));

        constexpr std::size_t branch_count = 12U;
        constexpr std::size_t leaves_per_branch = 24U;
        for (std::size_t branch_index = 0U; branch_index < branch_count;
             ++branch_index) {
            auto branch = victim.append_child(pugi::node_element);
            const auto branch_name = make_xml_name("branch_", branch_index);
            REQUIRE(branch);
            REQUIRE(branch.set_name(branch_name.c_str()));

            for (std::size_t leaf_index = 0U; leaf_index < leaves_per_branch;
                 ++leaf_index) {
                const auto unique_index =
                    branch_index * leaves_per_branch + leaf_index;
                REQUIRE(append_dynamic_leaf(branch, unique_index));
            }
        }

        CHECK_EQ(child_count(victim), branch_count);
        CHECK_GE(memory.peak_allocations(), 4U);

        const auto deallocations_before_remove = memory.deallocations();
        REQUIRE(root.remove_child(victim));
        CHECK_GT(memory.deallocations(), deallocations_before_remove);

        CHECK_EQ(child_count(root), 2U);
        CHECK_EQ(root.first_child(), before);
        CHECK_EQ(before.next_sibling(), after);
        CHECK_EQ(after.previous_sibling(), before);
        CHECK_FALSE(after.next_sibling());
        CHECK_EQ(std::string{before.attribute("marker").value()},
                 "before-value");
        CHECK_EQ(std::string{after.attribute("marker").value()}, "after-value");

        auto replacement = root.append_child("replacement");
        REQUIRE(replacement);
        REQUIRE(replacement.append_child(pugi::node_pcdata)
                    .set_value("allocator remains usable"));
        CHECK_EQ(child_count(root), 3U);
        CHECK_EQ(std::string{replacement.child_value()},
                 "allocator remains usable");
    }

    CHECK_FALSE(memory.saw_unexpected_deallocation());
    CHECK_EQ(memory.live_allocations(), 0U);
    CHECK_EQ(memory.allocations(), memory.deallocations());
}

TEST_CASE("iterative remove_children reclaims multiple pages before allocator "
          "reuse") {
    pugi_memory_tracking_scope memory;

    {
        pugi::xml_document document;
        auto container = document.append_child("container");
        auto outside = document.append_child("outside");
        REQUIRE(container);
        REQUIRE(outside);
        REQUIRE(outside.append_attribute("marker").set_value("survives"));

        constexpr std::size_t branch_count = 8U;
        constexpr std::size_t leaves_per_branch = 256U;
        for (std::size_t branch_index = 0U; branch_index < branch_count;
             ++branch_index) {
            auto branch = container.append_child(pugi::node_element);
            const auto branch_name = make_xml_name("multipage_", branch_index);
            REQUIRE(branch);
            REQUIRE(branch.set_name(branch_name.c_str()));

            for (std::size_t leaf_index = 0U; leaf_index < leaves_per_branch;
                 ++leaf_index) {
                const auto unique_index =
                    branch_index * leaves_per_branch + leaf_index;
                REQUIRE(append_dynamic_leaf(branch, unique_index));
            }
        }

        CHECK_EQ(child_count(container), branch_count);
        CHECK_GE(memory.peak_allocations(), 8U);

        const auto live_before_remove = memory.live_allocations();
        const auto deallocations_before_remove = memory.deallocations();
        REQUIRE(container.remove_children());
        CHECK_FALSE(container.first_child());
        CHECK_LT(memory.live_allocations(), live_before_remove);
        CHECK_GT(memory.deallocations(), deallocations_before_remove);
        CHECK_EQ(document.first_child(), container);
        CHECK_EQ(container.next_sibling(), outside);
        CHECK_EQ(std::string{outside.attribute("marker").value()}, "survives");

        const auto allocations_before_reuse = memory.allocations();
        constexpr std::size_t replacement_count = 512U;
        for (std::size_t index = 0U; index < replacement_count; ++index) {
            REQUIRE(append_dynamic_leaf(container, index + 10'000U));
        }
        CHECK_EQ(child_count(container), replacement_count);
        CHECK_GT(memory.allocations(), allocations_before_reuse);

        REQUIRE(container.remove_children());
        auto final_child = container.append_child("final");
        REQUIRE(final_child);
        REQUIRE(final_child.append_attribute("state").set_value("valid"));
        CHECK_EQ(std::string{final_child.attribute("state").value()}, "valid");
        CHECK_EQ(std::string{outside.attribute("marker").value()}, "survives");
    }

    CHECK_FALSE(memory.saw_unexpected_deallocation());
    CHECK_EQ(memory.live_allocations(), 0U);
    CHECK_EQ(memory.allocations(), memory.deallocations());
}
