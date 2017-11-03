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

#include <functional>
#include <lager/util.hpp>

namespace lager {

template <typename Action>
struct context
{
    using action_t      = Action;
    using finish_t      = std::function<void()>;
    using dispatch_t    = std::function<void(action_t)>;
    using async_t       = std::function<void(std::function<void()>)>;

    dispatch_t dispatch;
    async_t    async;
    finish_t   finish;

    context() = default;

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

template <typename Reducer, typename Model, typename Action,
          typename Enable=void>
struct has_effect
    : std::false_type {};

template <typename Reducer, typename Model, typename Action>
struct has_effect<
    Reducer, Model, Action,
    std::enable_if_t<
        std::is_convertible_v<
            decltype(std::get<1>(std::invoke(std::declval<Reducer>(),
                                           std::declval<Model>(),
                                           std::declval<Action>()))),
            effect<std::decay_t<Action>>>>>
    : std::true_type {};

template <typename Reducer, typename Model, typename Action>
constexpr auto has_effect_v = has_effect<Reducer, Model, Action>::value;

template <typename Reducer, typename Model, typename Action,
          typename EffectHandler,
          std::enable_if_t<has_effect_v<Reducer, Model, Action>,int> =0>
auto invoke_reducer(Reducer&& reducer, Model&& model, Action&& action,
                    EffectHandler&& handler)
    -> std::decay_t<Model>
{
    auto [new_model, effect] = std::invoke(LAGER_FWD(reducer),
                                          LAGER_FWD(model),
                                          LAGER_FWD(action));
    LAGER_FWD(handler)(effect);
    return new_model;
}

template <typename Reducer, typename Model, typename Action,
          typename EffectHandler,
          std::enable_if_t<!has_effect_v<Reducer, Model, Action>,int> =0>
auto invoke_reducer(Reducer&& reducer, Model&& model, Action&& action,
                    EffectHandler&&)
    -> std::decay_t<Model>
{
    return std::invoke(LAGER_FWD(reducer),
                      LAGER_FWD(model),
                      LAGER_FWD(action));
}

} // namespace lager
