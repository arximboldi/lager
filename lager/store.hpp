//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lager is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lager.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <lager/event_loop/manual.hpp>

#include <functional>

namespace lager {

template <typename Action>
struct context
{
    using action_t      = Action;

    using finish_t      = std::function<void()>;
    using dispatch_t    = std::function<void(action_t)>;
    using async_t       = std::function<void(std::function<void()>())>;

    finish_t   finish;
    dispatch_t dispatch;
    async_t    async;

    context(const context& ctx) = default;
    context(context&& ctx) = default;

    template <typename Action_>
    context(const context<Action_>& ctx)
        : finish(ctx.finish)
        , dispatch(ctx.dispatch)
        , async(ctx.async)
    {}

    context(finish_t fn, dispatch_t ds, async_t as)
        : finish(std::move(fn))
        , dispatch(std::move(ds))
        , async(std::move(as))
    {}
};

template <typename Action>
using effect = std::function<void(const context<Action>&)>;

constexpr auto noop = [] (auto&&...) {};

template <typename Model, typename Action>
struct result : std::pair<Model, effect<Action>>
{
    using base_t = std::pair<Model, effect<Action>>;
    using base_t::base_t;
    result(Model m) : base_t{m, noop} {};
};

template <typename Model, typename Action, typename EventLoop>
struct store : context<Action>
{
    using base_t       = context<Action>;
    using model_t      = Model;
    using action_t     = Action;
    using finish_t     = typename base_t::finish_t;
    using reducer_t    = std::function<result<model_t, action_t>
                                      (model_t, action_t)>;
    using view_t       = std::function<void(const model_t&)>;
    using event_loop_t = EventLoop;

    store(const store&) = delete;
    store& operator=(const store&) = delete;

    store(event_loop_t loop,
          model_t init,
          reducer_t reducer,
          view_t view,
          finish_t finish)
        : base_t{std::move(finish),
                 [this] (auto ev) { dispatch(ev); },
                 [this] (auto fn) { loop_.async(fn); }}
        , loop_{std::move(loop)}
        , model_{std::move(init)}
        , reducer_{std::move(reducer)}
        , view_{std::move(view)}
    {
        loop_.post([=] {
            view_(model_);
        });
    };

    void dispatch(action_t action)
    {
        loop_.post([=] {
            auto [model, effect] = reducer_(model_, action);
            model_ = model;
            effect(*this);
            view_(model_);
        });
    }

private:
    event_loop_t loop_;
    model_t model_;
    reducer_t reducer_;
    view_t view_;
};

template <typename Model, typename Action>
auto make_store(
    Model init,
    std::function<result<Model, Action>(Model, Action)> reducer,
    std::function<void(const Model&)> view,
    std::function<void()> finish = {})
{
    return store<Model, Action, manual_event_loop>(
        {},
        std::move(init),
        std::move(reducer),
        std::move(view),
        std::move(finish));
}

template <typename Model, typename Action, typename EventLoop>
auto make_store(
    EventLoop loop,
    Model init,
    std::function<result<Model, Action>(Model, Action)> reducer,
    std::function<void(const Model&)> view,
    std::function<void()> finish = {})
{
    return store<Model, Action, EventLoop>{
        loop,
        std::move(init),
        std::move(reducer),
        std::move(view),
        std::move(finish)
    };
}

} // namespace lager
