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
 * This module implements the cursor signal flow in Lager.  Signals can be
 * connected forming two superinposed directed acyclical graphs, where
 * signals flow *down* or *up*.  Signals are derived from eachother
 * using transducers.
 *
 * The APIs for flowing up and down are asymmetric because of the way
 * the graph is constructed and the semantics of information flow.
 *
 *   - An up-down-signal can be constructed from a up-down-signal.
 *   - A down-signal can be constructed from a up-down-signa or
 *     a down-signal.
 *   - A signal can be appended children, but can not be appended
 *     parents.
 *   - Information propagates upwardes immediately, but propagates
 *     upwards in two fases.
 *
 * In general, sucessors know a lot about their predecessors, but
 * sucessors need to know very little or nothing from their sucessors.
 *
 * @todo We could eventually flatten signals when the sucessors knows
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
 * Interface for children of a signal and is used to propagate
 * notifications.  The notifications are propagated in two steps,
 * `send_down()` and `notify()`, not ensure that the outside world sees a
 * consistent state when it receives notifications.
 */
struct down_signal_base
{
    virtual ~down_signal_base() = default;
    virtual void send_down()    = 0;
    virtual void notify()       = 0;
};

/*!
 * Interface for signals that can send values back to their parents.
 */
template <typename T>
struct up_signal_base
{
    virtual ~up_signal_base()      = default;
    virtual void send_up(const T&) = 0;
    virtual void send_up(T&&)      = 0;
};

/*!
 * Base class for the various signal types.  Provides basic
 * functionality for setting values and propagating them to children.
 */
template <typename T>
class down_signal
    : public std::enable_shared_from_this<down_signal<T>>
    , public down_signal_base
{
public:
    using value_type = T;

    down_signal(down_signal&&)      = default;
    down_signal(const down_signal&) = delete;
    down_signal& operator=(down_signal&&) = default;
    down_signal& operator=(const down_signal&) = delete;

    down_signal(T value)
        : current_(std::move(value))
        , last_(current_)
        , last_notified_(current_)
    {}

    virtual void recompute() {}
    virtual void recompute_deep() {}

    const value_type& current() const { return current_; }
    const value_type& last() const { return last_; }

    void link(std::weak_ptr<down_signal_base> child)
    {
        using namespace std;
        using std::placeholders::_1;
        assert(find_if(begin(children_),
                       end(children_),
                       bind(owner_equals, child, _1)) == end(children_) &&
               "Child signal must not be linked twice");
        children_.push_back(child);
    }

    template <typename U>
    void push_down(U&& value)
    {
        if (value != current_) {
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
                                  mem_fn(&weak_ptr<down_signal_base>::expired)),
                        end(children_));
    }

    bool needs_send_down_ = false;
    bool needs_notify_    = false;
    value_type current_;
    value_type last_;
    value_type last_notified_;
    std::vector<std::weak_ptr<down_signal_base>> children_;
    boost::signals2::signal<void(const value_type&, const value_type&)>
        observers_;
};

/*!
 * Base class for signals that can send values up the signal chain.
 */
template <typename T>
class up_down_signal
    : public down_signal<T>
    , public up_signal_base<T>
{
    using down_signal<T>::down_signal;
};

} // namespace detail

} // namespace lager
