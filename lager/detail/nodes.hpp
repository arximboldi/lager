//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

/*!
 * @file
 *
 * This module implements the cursor node flow in Lager.  Nodes can be
 * connected forming two superinposed directed acyclical graphs, where
 * nodes flow *down* or *up*.  Nodes are derived from eachother
 * using transducers.
 *
 * The APIs for flowing up and down are asymmetric because of the way
 * the graph is constructed and the semantics of information flow.
 *
 *   - An up-down-node can be constructed from a up-down-node.
 *   - A down-node can be constructed from a up-down-signa or
 *     a down-node.
 *   - A node can be appended children, but can not be appended
 *     parents.
 *   - Information propagates upwardes immediately, but propagates
 *     upwards in two fases.
 *
 * In general, sucessors know a lot about their predecessors, but
 * sucessors need to know very little or nothing from their sucessors.
 *
 * @todo We could eventually flatten nodes when the sucessors knows
 * the transducer of its predecessor, this could be done heuristically.
 */

#pragma once

#include <lager/detail/signal.hpp>
#include <lager/util.hpp>

#include <zug/meta/pack.hpp>
#include <zug/tuplify.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace lager {
namespace detail {

/*!
 * Allows comparing shared and weak pointers based on their owner.
 */
constexpr struct
{
    template <typename T1, typename T2>
    bool operator()(const T1& a, const T2& b)
    {
        return !a.owner_before(b) && !b.owner_before(a);
    }
} owner_equals{};

/*!
 * Interface for children of a node and is used to propagate
 * notifications.  The notifications are propagated in two steps,
 * `propagate()` and `notify()`, not ensure that the outside world sees a
 * consistent state when it receives notifications.
 */
struct reader_node_base
{
    reader_node_base()                        = default;
    reader_node_base(reader_node_base&&)      = default;
    reader_node_base(const reader_node_base&) = delete;
    reader_node_base& operator=(reader_node_base&&) = default;
    reader_node_base& operator=(const reader_node_base&) = delete;

    virtual ~reader_node_base() = default;
    virtual void send_down()    = 0;
    virtual void notify()       = 0;
};

/*!
 * Interface for nodes that can send values back to their parents.
 */
template <typename T>
struct writer_node_base
{
    writer_node_base()                        = default;
    writer_node_base(writer_node_base&&)      = default;
    writer_node_base(const writer_node_base&) = delete;
    writer_node_base& operator=(writer_node_base&&) = default;
    writer_node_base& operator=(const writer_node_base&) = delete;

    virtual ~writer_node_base()    = default;
    virtual void send_up(const T&) = 0;
    virtual void send_up(T&&)      = 0;
};

template <typename T, typename U>
auto has_changed(T&& a, U&& b) -> decltype(!(a == b))
{
    return !(a == b);
}

template <typename T>
auto has_changed(const T&, const T&)
{
    return true;
}

struct notifying_guard_t
{
    notifying_guard_t(bool& target)
        : value_{target}
        , target_{target}
    {
        target_ = true;
    }

    ~notifying_guard_t() { target_ = value_; }

    bool value_;
    bool& target_;
};

/*!
 * Base class for the various node types.  Provides basic
 * functionality for setting values and propagating them to children.
 */
template <typename T>
class reader_node : public reader_node_base
{
public:
    using value_type  = T;
    using signal_type = signal<const value_type&>;

    reader_node(T value)
        : current_(std::move(value))
        , last_(current_)
    {}

    virtual void recompute() = 0;
    virtual void refresh()   = 0;

    const value_type& current() const { return current_; }
    const value_type& last() const { return last_; }

    void link(std::weak_ptr<reader_node_base> child)
    {
        using namespace std;
        using std::placeholders::_1;
        assert(find_if(begin(children_),
                       end(children_),
                       bind(owner_equals, child, _1)) == end(children_) &&
               "Child node must not be linked twice");
        children_.push_back(child);
    }

    template <typename U>
    void push_down(U&& value)
    {
        if (has_changed(value, current_)) {
            current_         = std::forward<U>(value);
            needs_send_down_ = true;
        }
    }

    void send_down() final
    {
        recompute();
        if (needs_send_down_) {
            last_            = current_;
            needs_send_down_ = false;
            needs_notify_    = true;
            for (auto& wchild : children_) {
                if (auto child = wchild.lock()) {
                    child->send_down();
                }
            }
        }
    }

    void notify() final
    {
        using namespace std;
        if (needs_notify_ && !needs_send_down_) {
            needs_notify_ = false;

            notifying_guard_t notifying_guard(notifying_);
            bool garbage = false;

            observers_(last_);
            for (size_t i = 0, size = children_.size(); i < size; ++i) {
                if (auto child = children_[i].lock()) {
                    child->notify();
                } else {
                    garbage = true;
                }
            }

            if (garbage && !notifying_guard.value_) {
                collect();
            }
        }
    }

    auto observers() -> signal_type& { return observers_; }

private:
    void collect()
    {
        using namespace std;
        children_.erase(remove_if(begin(children_),
                                  end(children_),
                                  mem_fn(&weak_ptr<reader_node_base>::expired)),
                        end(children_));
    }

    value_type current_;
    value_type last_;
    std::vector<std::weak_ptr<reader_node_base>> children_;
    signal_type observers_;

    bool needs_send_down_ = false;
    bool needs_notify_    = false;
    bool notifying_       = false;
};

/*!
 * Base class for nodes that can send values up the node chain.
 */
template <typename T>
class cursor_node
    : public reader_node<T>
    , public writer_node_base<T>
{
    using reader_node<T>::reader_node;
};

template <typename T,
          typename Parents            = zug::meta::pack<>,
          template <class> class Base = reader_node>
class inner_node;

template <typename ValueT, typename... Parents, template <class> class Base>
class inner_node<ValueT, zug::meta::pack<Parents...>, Base>
    : public Base<ValueT>
{
    using base_t = Base<ValueT>;

    std::tuple<std::shared_ptr<Parents>...> parents_;

public:
    inner_node(ValueT init, std::tuple<std::shared_ptr<Parents>...>&& parents)
        : base_t{std::move(init)}
        , parents_{std::move(parents)}
    {}

    void refresh() final
    {
        std::apply([&](auto&&... ps) { noop((ps->refresh(), 0)...); },
                   parents_);
        this->recompute();
    }

    const std::tuple<std::shared_ptr<Parents>...>& parents() const
    {
        return parents_;
    }

    template <typename T>
    void push_up(T&& value)
    {
        push_up(std::forward<T>(value),
                std::make_index_sequence<sizeof...(Parents)>{});
    }

private:
    template <typename T, std::size_t... Indices>
    void push_up(T&& value, std::index_sequence<Indices...>)
    {
        auto& parents = this->parents();
        noop((std::get<Indices>(parents)->send_up(
                  std::get<Indices>(std::forward<T>(value))),
              0)...);
    }

    template <typename T>
    void push_up(T&& value, std::index_sequence<0>)
    {
        std::get<0>(this->parents())->send_up(std::forward<T>(value));
    }
};

template <typename T, template <class> class Base = reader_node>
struct root_node : Base<T>
{
    using base_t = Base<T>;
    using base_t::base_t;

    void refresh() final {}
};

template <typename... Nodes>
decltype(auto)
current_from(const std::tuple<std::shared_ptr<Nodes>...>& parents)
{
    return std::apply(
        [&](auto&&... ptrs) { return zug::tuplify(ptrs->current()...); },
        parents);
}

template <typename Node>
std::shared_ptr<Node> link_to_parents(std::shared_ptr<Node> n)
{
    std::apply([&](auto&&... ps) { noop((ps->link(n), 0)...); }, n->parents());
    return n;
}

} // namespace detail
} // namespace lager
