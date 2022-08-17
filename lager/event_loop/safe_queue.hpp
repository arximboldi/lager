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
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace lager {

struct safe_queue_event_loop
{
    using event_fn = std::function<void()>;

    void post(event_fn ev)
    {
        auto id = std::this_thread::get_id();
        if (id == thread_id_)
            local_queue_.emplace_back(std::move(ev));
        else {
            std::lock_guard<std::mutex> guard{mutex_};
            shared_queue_.push_back(std::move(ev));
        }
    }

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
        assert(thread_id_ == std::this_thread::get_id());
        run_local_queue_();
        swap_queues_();
        run_local_queue_();
    }

    void adopt()
    {
        assert(local_queue_.size() == 0);
        thread_id_ = std::this_thread::get_id();
    }

private:
    void swap_queues_()
    {
        using std::swap;
        assert(local_queue_.empty());
        std::lock_guard<std::mutex> guard{mutex_};
        swap(local_queue_, shared_queue_);
    }

    void run_local_queue_()
    {
        for (auto i = std::size_t{}; i < local_queue_.size();) {
            try {
                auto fn = std::move(local_queue_[i++]);
                std::move(fn)();
            } catch (...) {
                local_queue_.erase(local_queue_.begin(),
                                   local_queue_.begin() + i);
                throw;
            }
        }
        local_queue_.clear();
    }

    std::thread::id thread_id_ = std::this_thread::get_id();
    std::mutex mutex_;
    std::vector<event_fn> shared_queue_;
    std::vector<event_fn> local_queue_;
};

struct with_safe_queue_event_loop
{
    std::reference_wrapper<safe_queue_event_loop> loop;

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
