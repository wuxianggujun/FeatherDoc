/*
 * FeatherDoc
 * Original upstream author: Cihan SARI (@CihanSari)
 * Current fork branding, licensing, and maintenance notes: see README,
 * LICENSE, and LICENSE.upstream-mit.
 * Licensing: see LICENSE and LICENSE.upstream-mit.
 */

#ifndef FEATHERDOC_ITERATOR_HPP
#define FEATHERDOC_ITERATOR_HPP

#include <cstddef>
#include <featherdoc/detail/xml_handle.hpp>
#include <iterator>
#include <type_traits>
#include <utility>

namespace featherdoc {
class Run;
class Paragraph;
class Table;
class TableRow;
class TableCell;

template <class T>
inline constexpr bool is_iterator_handle_v =
    std::is_same_v<std::remove_cv_t<T>, Run> ||
    std::is_same_v<std::remove_cv_t<T>, Paragraph> ||
    std::is_same_v<std::remove_cv_t<T>, Table> ||
    std::is_same_v<std::remove_cv_t<T>, TableRow> ||
    std::is_same_v<std::remove_cv_t<T>, TableCell>;

template <class T, class P, class C = P> class Iterator {
  private:
    using ParentType = P;
    using CurrentType = C;
    ParentType parent{};
    CurrentType current{};
    mutable T buffer{};

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = const T *;
    using reference = const T &;
    using iterator_category = std::forward_iterator_tag;

    Iterator() = default;

    Iterator(ParentType parent, CurrentType current)
        : parent(parent), current(current) {}

    bool operator!=(const Iterator &other) const {
        return parent != other.parent || current != other.current;
    }

    bool operator==(const Iterator &other) const {
        return !this->operator!=(other);
    }

    Iterator &operator++() {
        this->current = this->current.next_sibling();
        return *this;
    }

    Iterator operator++(int) {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    auto operator*() const -> T const & {
        // Update the lightweight buffer only when the iterator is dereferenced.
        buffer.set_parent(parent);
        buffer.set_current(current);
        return buffer;
    }

    auto operator-> () const -> T const * { return &(this->operator*()); }
};

class IteratorHelper {
  public:
    using P = detail::tracked_xml_node;
    template <class T>
        requires is_iterator_handle_v<T>
    static auto make_begin(T const &obj) -> Iterator<T, P> {
        return Iterator<T, P>(obj.parent, obj.current);
    }

    template <class T>
        requires is_iterator_handle_v<T>
    static auto make_end(T const &obj) -> Iterator<T, P> {
        return Iterator<T, P>(obj.parent, decltype(obj.current){});
    }
};

// Entry point
template <class T>
    requires is_iterator_handle_v<T>
auto begin(T const &obj) -> Iterator<T, detail::tracked_xml_node> {
    return IteratorHelper::make_begin(obj);
}

template <class T>
    requires is_iterator_handle_v<T>
auto end(T const &obj) -> Iterator<T, detail::tracked_xml_node> {
    return IteratorHelper::make_end(obj);
}
} // namespace featherdoc

#endif
