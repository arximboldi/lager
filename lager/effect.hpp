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

#include <lager/context.hpp>

namespace lager {

//! @defgroup effects
//! @{

/*!
 * Effectful procedure that uses the store context.
 */
template <typename Action, typename Deps = lager::deps<>>
struct effect : std::function<future(const context<Action, Deps>&)>
{
    static_assert(
        is_deps<Deps>::value,
        LAGER_STATIC_ASSERT_MESSAGE_BEGIN
        "The second template argument of `lager::effect<...>`, must be a \
lager::deps<>. \n\nMaybe you are trying to specify an effect that can dispatch \
multiple action types? In that case, use the syntax: \
lager::effect<lager::actions<...>, ...> " //
        LAGER_STATIC_ASSERT_MESSAGE_END);

    using action_t  = Action;
    using deps_t    = Deps;
    using context_t = context<action_t, deps_t>;
    using base_t    = std::function<future(const context_t&)>;

    using base_t::base_t;
    using base_t::operator=;

    effect(const effect&) = default;
    effect(effect&&)      = default;
    effect& operator=(const effect&) = default;
    effect& operator=(effect&&) = default;

    template <typename A2,
              typename D2,
              std::enable_if_t<detail::are_compatible_actions_v<A2, Action> &&
                                   std::is_convertible_v<Deps, D2>,
                               int> = 0>
    effect(effect<A2, D2> e)
        : base_t{std::move(e)}
    {}

    template <
        typename Fn,
        std::enable_if_t<
            std::is_same_v<void, std::invoke_result_t<Fn&, const context_t&>>,
            int> = 0>
    effect(Fn&& fn)
        : base_t{[fn = std::forward<Fn>(fn)](auto&& ctx) -> future {
            fn(ctx);
            return {};
        }}
    {}

    template <
        typename Fn,
        std::enable_if_t<
            !std::is_convertible_v<std::decay_t<Fn>, effect> &&
                std::is_same_v<future,
                               std::invoke_result_t<Fn&, const context_t&>>,
            int> = 0>
    effect(Fn&& fn)
        : base_t{std::forward<Fn>(fn)}
    {}
};

/*!
 * Convenience type for specifying the result of reducers that return both a
 * model and an effect.
 */
template <typename Model, typename Action = void, typename Deps = lager::deps<>>
struct result : std::pair<Model, lager::effect<Action, Deps>>
{
    using model_t  = Model;
    using action_t = Action;
    using deps_t   = Deps;
    using effect_t = lager::effect<Action, Deps>;
    using base_t   = std::pair<model_t, effect_t>;

    result(const result&) = default;
    result(result&&)      = default;
    result& operator=(const result&) = default;
    result& operator=(result&&) = default;

    result(Model m)
        : base_t{std::move(m), lager::noop}
    {}

    template <typename M2, typename A2, typename D2>
    result(result<M2, A2, D2> r)
        : base_t{[&]() -> decltype(auto) {
            static_assert(check<M2, A2, D2>(), "");
            return std::move(r);
        }()}
    {}

    template <typename M2, typename A2, typename D2>
    result(M2 m, effect<A2, D2> e)
        : base_t{std::move(m), [&]() -> decltype(auto) {
                     static_assert(check<M2, A2, D2>(), "");
                     return std::move(e);
                 }()}
    {}

    template <typename M2, typename Effect>
    result(M2 m, Effect e)
        : base_t{std::move(m), std::move(e)}
    {}

    template <typename M2, typename A2, typename D2>
    constexpr static bool check()
    {
        static_assert(std::is_convertible_v<M2, Model>,
                      LAGER_STATIC_ASSERT_MESSAGE_BEGIN
                      "The model of the result types are not convertible" //
                      LAGER_STATIC_ASSERT_MESSAGE_END);
        static_assert(detail::are_compatible_actions_v<A2, Action>,
                      LAGER_STATIC_ASSERT_MESSAGE_BEGIN
                      "The actions of the given effect are not compatible to \
those in this result.  This effect's action must be a superset of those of the \
given effect.\n\nThis may occur when returning effects from a nested reducer \
and you forgot to add the nested action to the parent action variant." //
                      LAGER_STATIC_ASSERT_MESSAGE_END);
        static_assert(
            std::is_convertible_v<Deps, D2>,
            LAGER_STATIC_ASSERT_MESSAGE_BEGIN
            "Some dependencies missing in this result type.\n\nThis may occur \
when returning effects from a nested reducer and you forgot to add dependencies \
from the nested resulting effect to the result of the parent reducer." //
            LAGER_STATIC_ASSERT_MESSAGE_END);
        return true;
    }
};

//! @} group: effects

/*!
 * Metafunction that returns whether the @a Reducer returns an effect when
 * invoked with a given @a Model and @a Action types
 */
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

//! @defgroup effects
//! @{

/*!
 * Heuristically determine if the effect is empty or a noop operation.
 */
template <typename Ctx>
bool is_empty_effect(const std::function<future(Ctx)>& v)
{
    return !v || v.template target<decltype(noop)>() == &noop;
}

template <typename Eff>
bool is_empty_effect(const Eff& v)
{
    return false;
}

/*!
 * Invokes the @a reducer with the @a model and @a action and returns the
 * resulting model. If the reducer returns an effect, it evaluates the @a
 * handler passing the effect to it. This function can be used to generically
 * handle both reducers with or without side-effects.
 *
 * @note When effects do exist, they are evaluated after updating the model.
 */
template <typename Deps = lager::deps<>,
          typename Reducer,
          typename Model,
          typename Action,
          typename EffectHandler,
          typename NoEffectHandler>
auto invoke_reducer(Reducer&& reducer,
                    Model&& model,
                    Action&& action,
                    EffectHandler&& with_effect_handler,
                    NoEffectHandler&& without_effect_handler)
    -> std::decay_t<Model>
{
    if constexpr (has_effect_v<Reducer, Model, Action, Deps>) {
        auto [new_model, effect] = std::invoke(
            LAGER_FWD(reducer), LAGER_FWD(model), LAGER_FWD(action));
        if (!is_empty_effect(effect)) {
            LAGER_FWD(with_effect_handler)(effect);
        } else {
            LAGER_FWD(without_effect_handler)();
        }
        return std::move(new_model);
    } else {
        auto new_model = std::invoke(
            LAGER_FWD(reducer), LAGER_FWD(model), LAGER_FWD(action));
        LAGER_FWD(without_effect_handler)();
        return new_model;
    }
}

/*!
 * Returns an effects that evalates the effects @a a and @a b in order.
 */
template <typename Actions1, typename Deps1, typename Actions2, typename Deps2>
auto sequence(effect<Actions1, Deps1> a, effect<Actions2, Deps2> b)
{
    using deps_t = decltype(std::declval<Deps1>().merge(std::declval<Deps2>()));
    using actions_t = detail::merge_actions_t<Actions1, Actions2>;
    using result_t  = effect<actions_t, deps_t>;

    return is_empty_effect(a) && is_empty_effect(b) ? result_t{noop}
           : is_empty_effect(a)                     ? result_t{b}
           : is_empty_effect(b)
               ? result_t{a}
               : result_t{[a, b](auto&& ctx) {
                     return a(ctx).then([ctx = LAGER_FWD(ctx), b]() mutable {
                         return b(ctx);
                     });
                 }};
}

template <typename A1, typename D1, typename A2, typename D2, typename... Effs>
auto sequence(effect<A1, D1> a, effect<A2, D2> b, Effs&&... effects)
{
    return sequence(sequence(std::move(a), std::move(b)),
                    std::forward<Effs>(effects)...);
}

//! @} group: effects

} // namespace lager
