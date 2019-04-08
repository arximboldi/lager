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
#include <lager/util.hpp>

namespace lager {

//!
// Provide some _context_ for effectful functions, allowing them to control the
// event loop and dispatch new actions into the store.
//
// @note This is a reference type and it's life-time is bound to the associated
//       store.  It is invalid to use it after the store has been destructed.
//       Its methods may modify the store's underlying state.
//
// @todo Make constructors private.
//
template <typename Action>
struct context
{
    using action_t   = Action;
    using command_t  = std::function<void()>;
    using dispatch_t = std::function<void(action_t)>;
    using async_t    = std::function<void(std::function<void()>)>;

    dispatch_t dispatch;
    async_t async;
    command_t finish;
    command_t pause;
    command_t resume;

    context() = default;

    template <typename Action_>
    context(const context<Action_>& ctx)
        : dispatch(ctx.dispatch)
        , async(ctx.async)
        , finish(ctx.finish)
        , pause(ctx.pause)
        , resume(ctx.resume)
    {}

    context(dispatch_t dispatch_,
            async_t async_,
            command_t finish_,
            command_t pause_,
            command_t resume_)
        : dispatch(std::move(dispatch_))
        , async(std::move(async_))
        , finish(std::move(finish_))
        , pause(std::move(pause_))
        , resume(std::move(resume_))
    {}
};

//!
// Effectful procedure that uses the store context.
//
template <typename Action>
using effect = std::function<void(const context<Action>&)>;

//!
// Metafunction that returns whether the @a Reducer returns an effect when
// invoked with a given @a Model and @a Action types
//
template <typename Reducer,
          typename Model,
          typename Action,
          typename Enable = void>
struct has_effect : std::false_type
{};

template <typename Reducer, typename Model, typename Action>
struct has_effect<
    Reducer,
    Model,
    Action,
    std::enable_if_t<std::is_convertible_v<
        decltype(std::get<1>(std::invoke(std::declval<Reducer>(),
                                         std::declval<Model>(),
                                         std::declval<Action>()))),
        effect<std::decay_t<Action>>>>> : std::true_type
{};

template <typename Reducer, typename Model, typename Action>
constexpr auto has_effect_v = has_effect<Reducer, Model, Action>::value;

//!
// Invokes the @a reducer with the @a model and @a action. If the reducer
// returns an effect, it evaluates the @a handler passing the effect to it. This
// function can be used to generically handle both reducers with or without
// side-effects.
//
template <typename Reducer,
          typename Model,
          typename Action,
          typename EffectHandler,
          std::enable_if_t<has_effect_v<Reducer, Model, Action>, int> = 0>
auto invoke_reducer(Reducer&& reducer,
                    Model&& model,
                    Action&& action,
                    EffectHandler&& handler) -> std::decay_t<Model>
{
    auto [new_model, effect] =
        std::invoke(LAGER_FWD(reducer), LAGER_FWD(model), LAGER_FWD(action));
    LAGER_FWD(handler)(effect);
    return new_model;
}

template <typename Reducer,
          typename Model,
          typename Action,
          typename EffectHandler,
          std::enable_if_t<!has_effect_v<Reducer, Model, Action>, int> = 0>
auto invoke_reducer(Reducer&& reducer,
                    Model&& model,
                    Action&& action,
                    EffectHandler &&) -> std::decay_t<Model>
{
    return std::invoke(LAGER_FWD(reducer), LAGER_FWD(model), LAGER_FWD(action));
}

//!
// Returns an effects that evalates the effects @a a and @a b in order.
//
template <typename Action>
effect<Action> sequence(effect<Action> a, effect<Action> b)
{
    return (!a || a.template target<decltype(noop)>() == &noop) &&
                   (!b || b.template target<decltype(noop)>() == &noop)
               ? noop
               : !a || a.template target<decltype(noop)>() == &noop
                     ? b
                     : !b || b.template target<decltype(noop)>() == &noop
                           ? a
                           :
                           // otherwise
                           [a, b](auto&& ctx) {
                               a(ctx);
                               b(ctx);
                           };
}

} // namespace lager
