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

#include <functional>
#include <vector>

namespace lager {

struct with_manual_event_loop
{
    template <typename Fn>
    void async(Fn&& fn)
    {
        throw std::logic_error{"manual_event_loop does not support async()"};
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        auto is_root = queue_.empty();
        queue_.push_back(std::forward<Fn>(fn));
        if (is_root) {
            for (auto i = std::size_t{}; i < queue_.size(); ++i) {
                auto f = std::move(queue_[i]);
                f();
            }
            queue_.clear();
        }
    }

    void finish() {}
    void pause() {}
    void resume() {}

private:
    using post_fn_t = std::function<void()>;

    std::vector<post_fn_t> queue_;
};

} // namespace lager
