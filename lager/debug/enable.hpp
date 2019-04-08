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

#include <lager/debug/debugger.hpp>
#include <lager/util.hpp>

namespace lager {

template <template <class, class> class Debugger = debugger, typename Server>
auto enable_debug(Server& serv)
{
    return [&](auto next) {
        return [&serv, next](auto action,
                             auto&& model,
                             auto&& reducer,
                             auto&& view,
                             auto&& loop) {
            using action_t   = typename decltype(action)::type;
            using model_t    = std::decay_t<decltype(model)>;
            using debugger_t = Debugger<action_t, model_t>;
            auto& handle     = serv.enable(debugger_t{});
            auto store       = next(
                type_<typename debugger_t::action>{},
                typename debugger_t::model{LAGER_FWD(model)},
                [reducer = LAGER_FWD(reducer)](auto&& model, auto&& action) {
                    return debugger_t::update(
                        reducer, LAGER_FWD(model), LAGER_FWD(action));
                },
                [&handle, view = LAGER_FWD(view)](auto&& model) mutable {
                    return debugger_t::view(handle, view, LAGER_FWD(model));
                },
                LAGER_FWD(loop));
            handle.set_context(store.get_context());
            return store;
        };
    };
};

} // namespace lager
