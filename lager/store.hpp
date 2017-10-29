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

#include <lager/util.hpp>

#include <functional>
#include <type_traits>

namespace lager {

template <typename Action>
struct context
{
    using action_t      = Action;
    using finish_t      = std::function<void()>;
    using dispatch_t    = std::function<void(action_t)>;
    using async_t       = std::function<void(std::function<void()>())>;

    dispatch_t dispatch;
    async_t    async;
    finish_t   finish;

    context(const context& ctx) = default;
    context(context&& ctx) = default;

    template <typename Action_>
    context(const context<Action_>& ctx)
        : dispatch(ctx.dispatch)
        , async(ctx.async)
        , finish(ctx.finish)
    {}

    context(dispatch_t ds, async_t as, finish_t fn)
        : dispatch(std::move(ds))
        , async(std::move(as))
        , finish(std::move(fn))
    {}
};

template <typename Action>
using effect = std::function<void(const context<Action>&)>;

template <typename Model, typename Action>
struct result : std::pair<Model, effect<Action>>
{
    using base_t = std::pair<Model, effect<Action>>;
    using base_t::base_t;
    result(Model m) : base_t{m, noop} {};
};

template <typename Action,
          typename Model,
          typename ReducerFn,
          typename ViewFn,
          typename EventLoop>
struct store : context<Action>
{
    using base_t       = context<Action>;
    using action_t     = Action;
    using model_t      = Model;
    using reducer_t    = ReducerFn;
    using view_t       = ViewFn;
    using event_loop_t = EventLoop;
    using result_t     = result<Model, Action>;

    store(model_t init,
          reducer_t reducer,
          view_t view,
          event_loop_t loop)
        : base_t{[this] (auto ev) { dispatch(ev); },
                 [this] (auto fn) { loop_.async(fn); },
                 [this] { loop_.finish(); }}
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
            auto [model, effect] = result_t{reducer_(model_, action)};
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

template <typename Action,
          typename Model,
          typename ReducerFn,
          typename ViewFn,
          typename EventLoop,
          typename Enhancer>
auto make_store(Model&& init,
                ReducerFn&& reducer,
                ViewFn&& view,
                EventLoop&& loop,
                Enhancer&& enhancer)
{
    auto store_creator = enhancer([&] (auto action, auto&& ...args) {
        using action_t = typename decltype(action)::type;
        return store<action_t, std::decay_t<decltype(args)>...>{
            std::forward<decltype(args)>(args)...
        };
    });
    return store_creator(
        type_<Action>{},
        std::forward<Model>(init),
        std::forward<ReducerFn>(reducer),
        std::forward<ViewFn>(view),
        std::forward<EventLoop>(loop));
}

template <typename Action,
          typename Model,
          typename ReducerFn,
          typename ViewFn,
          typename EventLoop>
auto make_store(Model&& init,
                ReducerFn&& reducer,
                ViewFn&& view,
                EventLoop&& loop)
{
    return make_store<Action>(
        std::forward<Model>(init),
        std::forward<ReducerFn>(reducer),
        std::forward<ViewFn>(view),
        std::forward<EventLoop>(loop),
        identity);
}

} // namespace lager
