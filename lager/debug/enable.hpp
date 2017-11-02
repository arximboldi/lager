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
#include <lager/debug/debugger.hpp>

namespace lager {

template <typename Server>
auto enable_debug(Server& serv)
{
    return [&] (auto next) {
        return [&serv, next]
            (auto action, auto&& model, auto&& reducer, auto&& view, auto&& loop)
        {
            using action_t   = typename decltype(action)::type;
            using model_t    = std::decay_t<decltype(model)>;
            using debugger_t = debugger<action_t, model_t>;
            auto& handle     = serv.enable(debugger_t{});
            auto  store      = next(
                type_<typename debugger_t::action>{},
                typename debugger_t::model{LAGER_FWD(model)},
                [reducer=LAGER_FWD(reducer)] (auto&& model, auto&& action) {
                    return debugger_t::update(reducer,
                                             LAGER_FWD(model),
                                             LAGER_FWD(action));
                },
                [&handle, view=LAGER_FWD(view)] (auto&& model) {
                    return debugger_t::view(handle, view, LAGER_FWD(model));
                },
                LAGER_FWD(loop));
            handle.dispatcher(store.decltype(store)::base_t::dispatch);
            return store;
        };
    };
};

} // namespace lager
