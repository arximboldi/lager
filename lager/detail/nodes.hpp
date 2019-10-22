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

#include <boost/signals2/signal.hpp>

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
    virtual ~writer_node_base()    = default;
    virtual void send_up(const T&) = 0;
    virtual void send_up(T&&)      = 0;
};

template <typename T>
auto has_changed(T&& a, T&& b) -> decltype(!(a == b))
{
    return !(a == b);
}

template <typename T>
auto has_changed(const T& a, const T& b)
{
    return true;
}

/*!
 * Base class for the various node types.  Provides basic
 * functionality for setting values and propagating them to children.
 */
template <typename T>
class reader_node
    : public std::enable_shared_from_this<reader_node<T>>
    , public reader_node_base
{
public:
    using value_type = T;

    reader_node(reader_node&&)      = default;
    reader_node(const reader_node&) = delete;
    reader_node& operator=(reader_node&&) = default;
    reader_node& operator=(const reader_node&) = delete;

    reader_node(T value)
        : current_(std::move(value))
        , last_(current_)
        , last_notified_(current_)
    {}

    virtual void recompute() {}
    virtual void recompute_deep() {}

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
        if (!needs_send_down_ && needs_notify_) {
            needs_notify_ = false;
            observers_(last_notified_, last_);
            last_notified_ = last_;

            auto garbage = false;
            for (std::size_t i = 0, size = children_.size(); i < size; ++i) {
                if (auto child = children_[i].lock()) {
                    child->notify();
                } else {
                    garbage = true;
                }
            }

            if (garbage) {
                collect();
            }
        }
    }

    template <typename Fn>
    auto observe(Fn&& f) -> boost::signals2::connection
    {
        return observers_.connect(std::forward<Fn>(f));
    }

    auto observers()
        -> boost::signals2::signal<void(const value_type&, const value_type&)>&
    {
        return observers_;
    }

private:
    void collect()
    {
        using namespace std;
        children_.erase(remove_if(begin(children_),
                                  end(children_),
                                  mem_fn(&weak_ptr<reader_node_base>::expired)),
                        end(children_));
    }

    bool needs_send_down_ = false;
    bool needs_notify_    = false;
    value_type current_;
    value_type last_;
    value_type last_notified_;
    std::vector<std::weak_ptr<reader_node_base>> children_;
    boost::signals2::signal<void(const value_type&, const value_type&)>
        observers_;
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

} // namespace detail

} // namespace lager
