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

#include <immer/array.hpp>
#include <immer/box.hpp>
#include <immer/vector.hpp>

#include <variant>

namespace lager {

template <typename Action, typename Model>
struct debugger
{
    using cursor = std::size_t;

    struct goto_action { cursor pos; };
    struct prev_action {};
    struct next_action {};

    using action = std::variant<
        Action,
        goto_action,
        prev_action,
        next_action>;

    struct model
    {
        struct step
        {
            Action action;
            Model model;
        };

        Model init;
        cursor pos = {};
        immer::vector<step> history = {};

        model(Model i) : init{i} {}

        operator const Model& () const {
            return pos == history.size()
                ? (history.size() ? history.back().model : init)
                : history[pos].model;
        }
    };

    template <typename ReducerFn>
    static model update(ReducerFn&& reducer, model m, action act)
    {
        return std::visit(visitor{
                [&] (Action act) {
                    m.history = m.history
                        .take(m.pos)
                        .push_back({act, reducer(m, act)});
                    m.pos = m.history.size();
                    return m;
                },
                [&] (goto_action) { return m; },
                [&] (prev_action) { return m; },
                [&] (next_action) { return m; },
            }, act);
    }

    template <typename Server, typename ViewFn>
    static void view(Server& serv, ViewFn&& view, const model& m)
    {
        serv.view(m);
        std::forward<ViewFn>(view)(m);
    }
};

template <typename Server>
auto enable_debug(Server& serv)
{
    return [&] (auto&& next) {
        return [&] (auto action,
                    auto&& model, auto&& reducer, auto&& view,
                    auto&& loop)
        {
            using action_t   = typename decltype(action)::type;
            using model_t    = std::decay_t<decltype(model)>;
            using debugger_t = debugger<action_t, model_t>;
            return next(
                type_<typename debugger_t::action>{},
                typename debugger_t::model{LAGER_FWD(model)},
                [reducer=LAGER_FWD(reducer)] (auto&& model, auto&& action) {
                    return debugger_t::update(reducer,
                                             LAGER_FWD(model),
                                             LAGER_FWD(action));
                },
                [&serv, view=LAGER_FWD(view)] (auto&& model) {
                    return debugger_t::view(serv, view, LAGER_FWD(model));
                },
                LAGER_FWD(loop));
        };
    };
};

} // namespace lager
