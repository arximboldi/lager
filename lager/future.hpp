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

#pragma once

#include <lager/util.hpp>

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace lager {

namespace detail {

struct promise_state
{
    std::mutex mutex;
    std::function<void(std::function<void()>)> post;
    std::function<void()> callback;

    promise_state(std::function<void(std::function<void()>)> poster)
        : post{std::move(poster)}
    {}

    ~promise_state()
    {
        if (callback) {
            post(callback);
        }
    }
};

} // namespace detail

struct promise;

/*!
 * This allows chaining delayed computations.  We use this to represent the
 * eventual execution of the reducer and its effects after dispatching an
 * action.  It is similar to other mechanisms like `std::future<void>` or
 * `boost::future<void>` but with some notable constraints that make it more
 * efficient and usable for our particular use-case:
 *
 *  - There is no way to query whether the future has completed, one can only
 *    bind a next callback with `then`.
 *
 *  - The future is move-only and you need to move it into @a then. This
 *    guarantees that, for a given computation only one callback exists, but
 *    more callbacks can be added by chaining @a then calls.
 *
 *  - All callbacks are executed in the event loop associated to the @a promise.
 */
struct future
{
    /*!
     * Creates an empty future.  An empty future can still be used but callbacks
     * are called immediatelly.
     */
    future() = default;

    // I would prefer the promise to be not copyable, however putting lambdas in
    // std::function requires the closure to be copyable, which makes this not
    // so usable.
    //
    //   future(future&&) = default;
    //   future& operator=(future&&) = default;
    //   future(const future&)       = delete;
    //   future& operator=(const future&) = delete;

    operator bool() const { return bool{state_}; }

    /*!
     * Calls `fn` when the computation represented by this future
     * completes. `fn` may return a new future or `void`.  Returns a new @a
     * future that completes when the future from `fn` completes.
     */
    template <typename Fn>
    future then(Fn&& fn) &&;

    /*!
     * Returns a future that completes when both this and `f` complete.
     */
    future also(future&& f)
    {
        return std::move(*this).then(
            [f = std::move(f)]() mutable { return std::move(f); });
    }

private:
    friend struct promise;
    future(std::shared_ptr<detail::promise_state> state)
        : state_{std::move(state)}
    {}

    std::shared_ptr<detail::promise_state> state_{nullptr};
};

/*!
 * @see @a future
 */
struct promise
{
    /*!
     * Constructs a @a promise with a `poster` that allows queuing callbacks in
     * an event loop.
     */

    // I would prefer the promise to be not copyable, however putting lambdas in
    // std::function requires the closure to be copyable, which makes this not
    // so usable.
    //
    //   promise(promise&&) = default;
    //   promise& operator=(promise&&) = default;
    //   promise(const promise&)       = delete;
    //   promise& operator=(const promise&) = delete;

    /*!
     * Constructs a promise and future associated to the given event loop.
     */
    template <typename EventLoop>
    static std::pair<promise, future> with_loop(EventLoop& loop)
    {
        return with_post([&loop](auto&& fn) { loop.post(LAGER_FWD(fn)); });
    }

    /*!
     * Constructs a promise and future associated to a given `post` function.
     */
    static std::pair<promise, future>
    with_post(std::function<void(std::function<void()>)> post)
    {
        auto state = std::make_shared<detail::promise_state>(std::move(post));
        return {promise{state}, future{state}};
    }

    /*!
     * Constructs a promise and future that do not have an associated execution
     * context. These are invalid and any function chained in the futures will
     * be invoked immediatelly.
     */
    static std::pair<promise, future> invalid()
    {
        return {promise{nullptr}, future{nullptr}};
    }

    /*!
     * Fullfils the promise. Can only be called once for any promise chain!
     */
    void operator()()
    {
        if (!state_)
            throw std::runtime_error{"promise already satisfied!"};
        {
            auto lock = std::unique_lock{state_->mutex};
            if (state_->callback) {
                // operator() must be called in the event loop of the poster. We
                // could weaken this requirement by calling post() here, but
                // that can add unnecessary overhead?
                state_->callback();
                state_->callback = nullptr;
            }
        }
        state_.reset();
    }

private:
    promise(std::shared_ptr<detail::promise_state> state)
        : state_{std::move(state)}
    {}

    std::shared_ptr<detail::promise_state> state_;
};

template <typename Fn>
future future::then(Fn&& fn) &&
{
    if (!state_) {
        if constexpr (std::is_same_v<void, decltype(fn())>) {
            fn();
            return {};
        } else {
            return fn();
        }
    } else {
        assert(state_);
        assert(!state_->callback);
        auto [p, f] = promise::with_post(state_->post);
        {
            auto lock        = std::unique_lock{state_->mutex};
            state_->callback = [p  = std::move(p),
                                fn = std::forward<Fn>(fn)]() mutable {
                if constexpr (std::is_same_v<void, decltype(fn())>) {
                    fn();
                    p();
                } else {
                    fn().then(std::move(p));
                }
            };
        }
        state_.reset();
        return std::move(f);
    }
};

} // namespace lager
