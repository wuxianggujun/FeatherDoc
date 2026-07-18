#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>

#include <pugixml.hpp>

namespace featherdoc::detail {

// Shared by a Document and every XML-backed handle obtained from it. A
// generation change makes old nodes unreachable before their DOM is reset.
class xml_handle_lifetime final {
  public:
    xml_handle_lifetime();
    ~xml_handle_lifetime();

    xml_handle_lifetime(const xml_handle_lifetime &) = delete;
    auto operator=(const xml_handle_lifetime &) -> xml_handle_lifetime & =
        delete;

    [[nodiscard]] auto generation() const noexcept -> std::uint64_t;
    // Generation advancement is the safety boundary. Cache cleanup is best
    // effort so a platform mutex failure can never escape a noexcept Document
    // move operation after old handles have already been invalidated.
    void invalidate() noexcept;
    [[nodiscard]] auto node_epoch(pugi::xml_node node) const noexcept
        -> std::uint64_t;
    void retire_subtree(pugi::xml_node root);
    void retire_subtrees(std::span<const pugi::xml_node> roots);
    [[nodiscard]] void *opaque_owner_state() const noexcept;
    void set_opaque_owner_state(std::shared_ptr<void> owner_state) noexcept;

  private:
    struct state;
    std::atomic<std::uint64_t> generation_{1U};
    std::unique_ptr<state> state_;
};

// A non-owning pointer whose availability is bound to the same generation as
// XML handles. Facade objects can refer to Document-owned state without
// extending the Document lifetime or dereferencing stale pointers.
template <class T> class tracked_document_ptr final {
  public:
    tracked_document_ptr() = default;

    tracked_document_ptr(
        T *pointer, const std::shared_ptr<xml_handle_lifetime> &lifetime)
        : pointer_(pointer), lifetime_(lifetime),
          generation_(lifetime != nullptr ? lifetime->generation() : 0U) {}

    [[nodiscard]] auto get() const noexcept -> T * {
        if (this->pointer_ == nullptr) {
            return nullptr;
        }
        const auto lifetime = this->lifetime_.lock();
        if (lifetime == nullptr ||
            lifetime->generation() != this->generation_) {
            return nullptr;
        }
        return this->pointer_;
    }

    explicit operator bool() const noexcept { return this->get() != nullptr; }
    auto operator*() const noexcept -> T & { return *this->get(); }
    auto operator->() const noexcept -> T * { return this->get(); }

    friend auto operator==(const tracked_document_ptr &pointer,
                           std::nullptr_t) noexcept -> bool {
        return pointer.get() == nullptr;
    }

    friend auto operator!=(const tracked_document_ptr &pointer,
                           std::nullptr_t) noexcept -> bool {
        return !(pointer == nullptr);
    }

  private:
    T *pointer_{nullptr};
    std::weak_ptr<xml_handle_lifetime> lifetime_;
    std::uint64_t generation_{0U};
};

class tracked_xml_node final {
  public:
    tracked_xml_node() = default;

    tracked_xml_node(pugi::xml_node node,
                     const std::shared_ptr<xml_handle_lifetime> &lifetime)
        : node_(node), lifetime_(lifetime), generation_(lifetime->generation()),
          node_epoch_(lifetime->node_epoch(node)), tracked_(true) {}

    [[nodiscard]] auto alive() const noexcept -> bool {
        if (!this->tracked_) {
            return true;
        }
        const auto lifetime = this->lifetime_.lock();
        return lifetime != nullptr &&
               lifetime->generation() == this->generation_ &&
               lifetime->node_epoch(this->node_) == this->node_epoch_;
    }

    [[nodiscard]] auto node() const noexcept -> pugi::xml_node {
        return this->alive() ? this->node_ : pugi::xml_node{};
    }

    [[nodiscard]] auto has_node() const noexcept -> bool {
        return this->node() != pugi::xml_node{};
    }

    [[nodiscard]] auto child(const char *name) const -> tracked_xml_node {
        return this->with_node(this->node().child(name));
    }

    [[nodiscard]] auto first_child() const -> tracked_xml_node {
        return this->with_node(this->node().first_child());
    }

    [[nodiscard]] auto last_child() const -> tracked_xml_node {
        return this->with_node(this->node().last_child());
    }

    [[nodiscard]] auto next_sibling() const -> tracked_xml_node {
        return this->with_node(this->node().next_sibling());
    }

    [[nodiscard]] auto next_sibling(const char *name) const
        -> tracked_xml_node {
        return this->with_node(this->node().next_sibling(name));
    }

    [[nodiscard]] auto previous_sibling() const -> tracked_xml_node {
        return this->with_node(this->node().previous_sibling());
    }

    [[nodiscard]] auto previous_sibling(const char *name) const
        -> tracked_xml_node {
        return this->with_node(this->node().previous_sibling(name));
    }

    [[nodiscard]] auto parent() const -> tracked_xml_node {
        return this->with_node(this->node().parent());
    }

    [[nodiscard]] auto attribute(const char *name) const
        -> pugi::xml_attribute {
        return this->node().attribute(name);
    }

    auto remove_attribute(const char *name) -> bool {
        return this->node().remove_attribute(name);
    }

    auto remove_attribute(pugi::xml_attribute attribute) -> bool {
        return this->node().remove_attribute(attribute);
    }

    [[nodiscard]] auto name() const -> const char * {
        return this->node().name();
    }

    auto append_child(const char *name) -> tracked_xml_node {
        return this->with_node(this->node().append_child(name));
    }

    auto prepend_child(const char *name) -> tracked_xml_node {
        return this->with_node(this->node().prepend_child(name));
    }

    auto insert_child_before(const char *name, pugi::xml_node node)
        -> tracked_xml_node {
        return this->with_node(this->node().insert_child_before(name, node));
    }

    auto insert_child_after(const char *name, pugi::xml_node node)
        -> tracked_xml_node {
        return this->with_node(this->node().insert_child_after(name, node));
    }

    auto remove_child(pugi::xml_node node) -> bool {
        auto parent = this->node();
        if (node == pugi::xml_node{} || node.parent() != parent) {
            return false;
        }
        if (!this->retire_subtree(node)) {
            return false;
        }
        return parent.remove_child(node);
    }

    auto remove_child(const char *name) -> bool {
        return this->remove_child(this->node().child(name));
    }

    void remove_children() {
        while (const auto child = this->node().first_child()) {
            if (!this->remove_child(child)) {
                return;
            }
        }
    }

    auto retire_subtree(pugi::xml_node node) const -> bool {
        return this->retire_subtrees(
            std::span<const pugi::xml_node>{&node, 1U});
    }

    auto retire_subtrees(std::span<const pugi::xml_node> nodes) const -> bool {
        const auto lifetime = this->lifetime_.lock();
        if (!this->alive() || lifetime == nullptr) {
            return false;
        }
        lifetime->retire_subtrees(nodes);
        return true;
    }

    void reset() noexcept { this->node_ = {}; }

    void set_node(pugi::xml_node node) noexcept {
        this->node_ = node;
        this->update_node_epoch();
    }

    [[nodiscard]] auto with_node(pugi::xml_node node) const
        -> tracked_xml_node {
        auto result = *this;
        result.node_ = node;
        result.update_node_epoch();
        return result;
    }

    operator pugi::xml_node() const noexcept { return this->node(); }

    auto operator=(pugi::xml_node node) noexcept -> tracked_xml_node & {
        this->node_ = node;
        this->update_node_epoch();
        return *this;
    }

    friend auto operator==(const tracked_xml_node &left,
                           const tracked_xml_node &right) noexcept -> bool {
        return left.node() == right.node();
    }

    friend auto operator!=(const tracked_xml_node &left,
                           const tracked_xml_node &right) noexcept -> bool {
        return !(left == right);
    }

    friend auto operator==(const tracked_xml_node &left,
                           pugi::xml_node right) noexcept -> bool {
        return left.node() == right;
    }

    friend auto operator!=(const tracked_xml_node &left,
                           pugi::xml_node right) noexcept -> bool {
        return !(left == right);
    }

    friend auto operator==(pugi::xml_node left,
                           const tracked_xml_node &right) noexcept -> bool {
        return left == right.node();
    }

    friend auto operator!=(pugi::xml_node left,
                           const tracked_xml_node &right) noexcept -> bool {
        return !(left == right);
    }

  private:
    void update_node_epoch() noexcept {
        const auto lifetime = this->lifetime_.lock();
        this->node_epoch_ = lifetime != nullptr
                                ? lifetime->node_epoch(this->node_)
                                : 0U;
    }

    pugi::xml_node node_{};
    std::weak_ptr<xml_handle_lifetime> lifetime_;
    std::uint64_t generation_{0U};
    std::uint64_t node_epoch_{0U};
    bool tracked_{false};
};

} // namespace featherdoc::detail
