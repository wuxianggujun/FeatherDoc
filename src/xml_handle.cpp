#include <featherdoc/detail/xml_handle.hpp>

#include <limits>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace featherdoc::detail {

struct xml_handle_lifetime::state final {
    mutable std::mutex node_epochs_mutex;
    std::unordered_map<const void *, std::uint64_t> node_epochs;
    std::shared_ptr<void> opaque_owner_state;
};

xml_handle_lifetime::xml_handle_lifetime() : state_(std::make_unique<state>()) {}

xml_handle_lifetime::~xml_handle_lifetime() = default;

auto xml_handle_lifetime::generation() const noexcept -> std::uint64_t {
    return this->generation_.load(std::memory_order_acquire);
}

void xml_handle_lifetime::invalidate() noexcept {
    this->generation_.fetch_add(1U, std::memory_order_acq_rel);
    try {
        const auto lock = std::scoped_lock{this->state_->node_epochs_mutex};
        this->state_->node_epochs.clear();
    } catch (...) {
        // A generation mismatch already makes every existing handle stale.
        // Keeping old epoch-cache entries is safe and avoids terminating a
        // noexcept Document move if mutex acquisition reports a system error.
    }
}

auto xml_handle_lifetime::node_epoch(pugi::xml_node node) const noexcept
    -> std::uint64_t {
    if (node == pugi::xml_node{}) {
        return 0U;
    }

    try {
        const auto lock =
            std::scoped_lock{this->state_->node_epochs_mutex};
        const auto iterator =
            this->state_->node_epochs.find(node.internal_object());
        return iterator != this->state_->node_epochs.end() ? iterator->second
                                                            : 0U;
    } catch (...) {
        // A failed lock must make the handle unusable, never accidentally
        // validate a node whose epoch could not be checked.
        return std::numeric_limits<std::uint64_t>::max();
    }
}

void xml_handle_lifetime::retire_subtree(pugi::xml_node root) {
    this->retire_subtrees(
        std::span<const pugi::xml_node>{&root, 1U});
}

void xml_handle_lifetime::retire_subtrees(
    std::span<const pugi::xml_node> roots) {
    auto nodes = std::vector<pugi::xml_node>{};
    auto seen_nodes = std::unordered_set<const void *>{};

    const auto append_unseen_node = [&](pugi::xml_node node) {
        if (node == pugi::xml_node{}) {
            return;
        }
        if (seen_nodes.insert(node.internal_object()).second) {
            nodes.push_back(node);
        }
    };

    for (const auto root : roots) {
        append_unseen_node(root);
    }

    for (std::size_t index = 0U; index < nodes.size(); ++index) {
        for (auto child = nodes[index].first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            append_unseen_node(child);
        }
    }

    if (nodes.empty()) {
        return;
    }

    const auto lock = std::scoped_lock{this->state_->node_epochs_mutex};

    // Preparing every cache entry across every root before changing an epoch
    // gives batch retirement a strong observable guarantee. If collection or
    // try_emplace allocation fails, newly inserted zero entries are equivalent
    // to absent entries, so every existing handle keeps its original epoch.
    for (const auto node : nodes) {
        (void)this->state_->node_epochs.try_emplace(node.internal_object(),
                                                   0U);
    }
    for (const auto node : nodes) {
        const auto iterator =
            this->state_->node_epochs.find(node.internal_object());
        ++iterator->second;
    }
}

void *xml_handle_lifetime::opaque_owner_state() const noexcept {
    return this->state_->opaque_owner_state.get();
}

void xml_handle_lifetime::set_opaque_owner_state(
    std::shared_ptr<void> owner_state) noexcept {
    this->state_->opaque_owner_state = std::move(owner_state);
}

} // namespace featherdoc::detail
