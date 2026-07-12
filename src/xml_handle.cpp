#include <featherdoc/detail/xml_handle.hpp>

#include <limits>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace featherdoc::detail {

struct xml_handle_lifetime::state final {
    mutable std::mutex node_epochs_mutex;
    std::unordered_map<const void *, std::uint64_t> node_epochs;
};

xml_handle_lifetime::xml_handle_lifetime() : state_(std::make_unique<state>()) {}

xml_handle_lifetime::~xml_handle_lifetime() = default;

auto xml_handle_lifetime::generation() const noexcept -> std::uint64_t {
    return this->generation_.load(std::memory_order_acquire);
}

void xml_handle_lifetime::invalidate() {
    this->generation_.fetch_add(1U, std::memory_order_acq_rel);
    const auto lock = std::scoped_lock{this->state_->node_epochs_mutex};
    this->state_->node_epochs.clear();
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
    if (root == pugi::xml_node{}) {
        return;
    }

    auto nodes = std::vector<pugi::xml_node>{root};
    for (std::size_t index = 0U; index < nodes.size(); ++index) {
        for (auto child = nodes[index].first_child();
             child != pugi::xml_node{}; child = child.next_sibling()) {
            nodes.push_back(child);
        }
    }

    const auto lock = std::scoped_lock{this->state_->node_epochs_mutex};
    for (const auto node : nodes) {
        ++this->state_->node_epochs[node.internal_object()];
    }
}

} // namespace featherdoc::detail
