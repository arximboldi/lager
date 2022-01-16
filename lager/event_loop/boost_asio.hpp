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

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>

#include <functional>
#include <thread>
#include <utility>

namespace lager {

/*!
 * Event loop that relies on a Boost.Asio executor.
 *
 * @note The Boost asio executor must be single threaded, since in general the
 *       store is not thread-safe -- `context::dispatch()` is thread safe, but
 *       it schedules the evaluation of actual store updates and effects in the
 *       event loop which, it assumes, evaluates them serially.  You can easily
 *       serialize a multi-threaded executor by wrapping it in a
 *       `boost::asio::strand`.
 */
template <typename Executor>
struct with_boost_asio_event_loop
{
    Executor executor;
    std::function<void()> stop = [] {};

    with_boost_asio_event_loop(Executor ex)
        : executor{std::move(ex)}
    {}

    with_boost_asio_event_loop(Executor ex, std::function<void()> st)
        : executor{std::move(ex)}
        , stop{std::move(st)}
    {}

    template <typename Fn>
    void async(Fn&& fn)
    {
        using work_t = boost::asio::executor_work_guard<Executor>;

        std::thread([fn = std::move(fn), work = work_t{executor}] {
            fn();
        }).detach();
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        boost::asio::post(executor, std::forward<Fn>(fn));
    }

    void pause() {}
    void resume() {}

    void finish() { stop(); }
};

} // namespace lager
