//
// lager - library for functional interactive c++ programs
// Copyright (C) 2019 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#pragma once

#include <lager/config.hpp>

#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace lager {

struct queue_event_loop
{
    using event_fn = std::function<void()>;

    void post(event_fn ev) { queue_.push_back(std::move(ev)); }
    void finish() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    void pause() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    void resume() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    template <typename Fn>
    void async(Fn&& fn)
    {
        LAGER_THROW(std::logic_error{"not implemented!"});
    }

    // If there is an exception, the step() function needs to be re-run for the
    // queue to be fully processed.
    void step()
    {
        for (auto i = std::size_t{}; i < queue_.size();) {
            try {
                auto f = std::move(queue_[i++]);
                std::move(f)();
            } catch (...) {
                queue_.erase(queue_.begin(), queue_.begin() + i);
                throw;
            }
        }
        queue_.clear();
    }

private:
    std::vector<event_fn> queue_;
};

struct with_queue_event_loop
{
    std::reference_wrapper<queue_event_loop> loop;

    template <typename Fn>
    void async(Fn&& fn)
    {
        loop.get().async(std::forward<Fn>(fn));
    }
    template <typename Fn>
    void post(Fn&& fn)
    {
        loop.get().post(std::forward<Fn>(fn));
    }
    void finish() { loop.get().finish(); }
    void pause() { loop.get().pause(); }
    void resume() { loop.get().resume(); }
};

} // namespace lager
