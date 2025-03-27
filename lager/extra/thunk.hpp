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

#include <lager/effect.hpp>
#include <lager/util.hpp>

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace lager {

/*!
 * Enables dispatching thunk-style actions (functions that take a
 * lager::context).
 *
 * Useful for modeling async or deferred logic, similar to redux-thunk.
 * Thunks are handled as effects and invoked by the loop with the given
 * context.
 */
auto with_thunk()
{
    return [](auto next) {
        return [next](auto action,
                      auto&& model,
                      auto&& reducer,
                      auto&& loop,
                      auto&& deps,
                      auto&& tags) {
            using action_t  = typename decltype(action)::type;
            using model_t   = std::decay_t<decltype(model)>;
            using reducer_t = std::decay_t<decltype(reducer)>;
            using deps_t    = std::decay_t<decltype(deps)>;

            using effect_t = effect<action_t, deps_t>;
            using thunk_t  = std::variant<effect_t, action_t>;
            using result_t = result<model_t, thunk_t, deps_t>;

            return next(
                type_<thunk_t>{},
                LAGER_FWD(model),
                [reducer = LAGER_FWD(reducer)](auto&& model, auto&& action) {
                    return match(LAGER_FWD(action))(
                        [&](effect_t effect) -> result_t {
                            return {LAGER_FWD(model), std::move(effect)};
                        },
                        [&](action_t action) -> result_t {
                            if constexpr (has_effect_v<reducer_t,
                                                       model_t,
                                                       action_t,
                                                       deps_t>) {
                                auto [new_model, effect] =
                                    std::invoke(reducer,
                                                LAGER_FWD(model),
                                                std::move(action));
                                return {std::move(new_model),
                                        std::move(effect)};
                            } else {
                                auto new_model = std::invoke(reducer,
                                                             LAGER_FWD(model),
                                                             std::move(action));
                                return {std::move(new_model), noop};
                            }
                        });
                },
                LAGER_FWD(loop),
                LAGER_FWD(deps),
                LAGER_FWD(tags));
        };
    };
}

} // namespace lager
