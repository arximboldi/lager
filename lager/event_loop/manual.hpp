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

#include <lager/config.hpp>

#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace lager {

struct with_manual_event_loop
{
    template <typename Fn>
    void async(Fn&& fn)
    {
        LAGER_THROW(
            std::logic_error{"manual_event_loop does not support async()"});
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        queue_.push_back(std::forward<Fn>(fn));
        auto is_root = i_ == 0;
        if (is_root) {
            try {
                while (i_ < queue_.size()) {
                    auto f = std::move(queue_[i_++]);
                    std::move(f)();
                }
            } catch (...) {
                queue_.erase(queue_.begin(), queue_.begin() + i_);
                i_ = 0;
                throw;
            }
            queue_.clear();
            i_ = 0;
        }
    }

    void finish() {}
    void pause() {}
    void resume() {}

private:
    using post_fn_t = std::function<void()>;

    std::vector<post_fn_t> queue_;
    std::size_t i_ = {};
};

} // namespace lager
