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

#include <lager/deps.hpp>
#include <lager/util.hpp>

#include <functional>

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
template <typename Action, typename Deps = lager::deps<>>
struct context : Deps
{
    using deps_t     = Deps;
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

    template <typename Action_, typename Deps_>
    context(const context<Action_, Deps_>& ctx)
        : deps_t{ctx}
        , dispatch{ctx.dispatch}
        , async{ctx.async}
        , finish{ctx.finish}
        , pause{ctx.pause}
        , resume{ctx.resume}
    {}

    context(dispatch_t dispatch_,
            async_t async_,
            command_t finish_,
            command_t pause_,
            command_t resume_,
            deps_t deps_)
        : deps_t{std::move(deps_)}
        , dispatch{std::move(dispatch_)}
        , async{std::move(async_)}
        , finish{std::move(finish_)}
        , pause{std::move(pause_)}
        , resume{std::move(resume_)}
    {}
};

//!
// Effectful procedure that uses the store context.
//
template <typename Action, typename Deps = lager::deps<>>
using effect = std::function<void(const context<Action, Deps>&)>;

//!
// Metafunction that returns whether the @a Reducer returns an effect when
// invoked with a given @a Model and @a Action types
//
template <typename Reducer,
          typename Model,
          typename Action,
          typename Deps,
          typename Enable = void>
struct has_effect : std::false_type
{};

template <typename Reducer, typename Model, typename Action, typename Deps>
struct has_effect<
    Reducer,
    Model,
    Action,
    Deps,
    std::enable_if_t<std::is_convertible_v<
        decltype(std::get<1>(std::invoke(std::declval<Reducer>(),
                                         std::declval<Model>(),
                                         std::declval<Action>()))),
        effect<std::decay_t<Action>, std::decay_t<Deps>>>>> : std::true_type
{};

template <typename Reducer, typename Model, typename Action, typename Deps>
constexpr auto has_effect_v = has_effect<Reducer, Model, Action, Deps>::value;

//!
// Invokes the @a reducer with the @a model and @a action and stores the result
// in the given model. If the reducer returns an effect, it evaluates the @a
// handler passing the effect to it. This function can be used to generically
// handle both reducers with or without side-effects.
//
// @note When effects do exist, they are evaluated after updating the model.
//
template <typename Deps = lager::deps<>,
          typename Reducer,
          typename Model,
          typename Action,
          typename EffectHandler,
          std::enable_if_t<has_effect_v<Reducer, Model, Action, Deps>, int> = 0>
void invoke_reducer(Reducer&& reducer,
                    Model& model,
                    Action&& action,
                    EffectHandler&& handler)
{
    auto [new_model, effect] =
        std::invoke(LAGER_FWD(reducer), std::move(model), LAGER_FWD(action));
    model = std::move(new_model);
    LAGER_FWD(handler)(effect);
}

template <
    typename Deps = lager::deps<>,
    typename Reducer,
    typename Model,
    typename Action,
    typename EffectHandler,
    std::enable_if_t<!has_effect_v<Reducer, Model, Action, Deps>, int> = 0>
void invoke_reducer(Reducer&& reducer,
                    Model& model,
                    Action&& action,
                    EffectHandler&&)
{
    model =
        std::invoke(LAGER_FWD(reducer), std::move(model), LAGER_FWD(action));
}

//!
// Returns an effects that evalates the effects @a a and @a b in order.
//
template <typename Action, typename Deps1, typename Deps2>
auto sequence(effect<Action, Deps1> a, effect<Action, Deps2> b)
{
    using deps_t = decltype(std::declval<Deps1>().merge(std::declval<Deps2>()));
    using result_t = effect<Action, deps_t>;

    return (!a || a.template target<decltype(noop)>() == &noop) &&
                   (!b || b.template target<decltype(noop)>() == &noop)
               ? result_t{noop}
               : !a || a.template target<decltype(noop)>() == &noop
                     ? result_t{b}
                     : !b || b.template target<decltype(noop)>() == &noop
                           ? result_t{a}
                           : result_t{[a, b](auto&& ctx) {
                                 a(ctx);
                                 b(ctx);
                             }};
}

template <typename Action1, typename Deps1, typename Action2, typename Deps2>
auto sequence(effect<Action1, Deps1> a, effect<Action2, Deps2> b)
{
    // When the actions are disimilar we can not deduce a sensible effect type,
    // so we can only just return a generic function and rely on the context
    // conversions working when the function is instantiated.  This can be
    // improved when/if we provide our own variant type for actions that is
    // tailored towards our use-cases and provides subset cherry-picking
    // conversions.
    return [a, b](auto&& ctx) {
        if (a)
            a(ctx);
        if (b)
            b(ctx);
    };
}

template <typename A1, typename D1, typename A2, typename D2, typename... Effs>
auto sequence(effect<A1, D1> a, effect<A2, D2> b, Effs&&... effects)
{
    return sequence(sequence(std::move(a), std::move(b)),
                    std::forward<Effs>(effects)...);
}

} // namespace lager
