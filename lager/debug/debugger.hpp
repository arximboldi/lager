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

#include <lager/config.hpp>
#include <lager/detail/access.hpp>
#include <lager/effect.hpp>
#include <lager/util.hpp>

#include <immer/algorithm.hpp>
#include <immer/vector.hpp>

#include <lager/extra/cereal/immer_vector.hpp>
#include <lager/extra/cereal/struct.hpp>
#include <lager/extra/cereal/variant_with_name.hpp>
#include <lager/extra/struct.hpp>

#include <zug/transducer/map.hpp>

#include <functional>
#include <variant>

namespace lager {

template <typename Action, typename Model, typename Deps>
struct debugger
{
    using base_action = Action;
    using base_model  = Model;
    using deps_t      = Deps;

    using cursor_t = std::size_t;

    struct goto_action
    {
        cursor_t cursor;
        LAGER_STRUCT_NESTED(goto_action, cursor);
    };
    struct undo_action
    {
        LAGER_STRUCT_NESTED(undo_action);
    };
    struct redo_action
    {
        LAGER_STRUCT_NESTED(redo_action);
    };
    struct pause_action
    {
        LAGER_STRUCT_NESTED(pause_action);
    };
    struct resume_action
    {
        LAGER_STRUCT_NESTED(resume_action);
    };

    using action = std::variant<Action,
                                goto_action,
                                undo_action,
                                redo_action,
                                pause_action,
                                resume_action>;

    struct step
    {
        Action action;
        Model model;
        LAGER_STRUCT_NESTED(step, action, model);
    };

    struct model
    {
        cursor_t cursor = {};
        bool paused     = {};
        Model init;
        immer::vector<step> history   = {};
        immer::vector<Action> pending = {};
        LAGER_STRUCT_NESTED(model, cursor, paused, init, history, pending);

        model() = default;
        model(Model i)
            : init{i}
        {}

        using lookup_result = std::pair<std::optional<Action>, const Model&>;

        lookup_result lookup(cursor_t cursor) const
        {
            if (cursor > history.size())
                LAGER_THROW(std::runtime_error{"bad cursor"});
            return cursor == 0 ? lookup_result{{}, init} : [&] {
                auto& step = history[cursor - 1];
                return lookup_result{step.action, step.model};
            }();
        }

        std::size_t summary() const { return history.size(); }

        operator const Model&() const { return lookup(cursor).second; }

        friend decltype(auto) unwrap(const model& m)
        {
            return unwrap(static_cast<const Model&>(m));
        }
    };

    using result_t = std::pair<model, effect<action, deps_t>>;

    template <typename ReducerFn>
    static result_t update(ReducerFn&& reducer, model m, action act)
    {
        return std::visit(
            visitor{
                [&](Action act) -> result_t {
                    if (m.paused) {
                        m.pending = m.pending.push_back(act);
                        return {m, noop};
                    } else {
                        auto eff   = effect<action, deps_t>{noop};
                        auto state = invoke_reducer<deps_t>(
                            reducer,
                            m,
                            act,
                            [&](auto&& e) { eff = LAGER_FWD(e); },
                            [] {});
                        m.history = m.history.take(m.cursor).push_back(
                            {act, std::move(state)});
                        m.cursor = m.history.size();
                        return {m, eff};
                    }
                },
                [&](goto_action act) -> result_t {
                    if (act.cursor <= m.history.size())
                        m.cursor = act.cursor;
                    return {m, noop};
                },
                [&](undo_action) -> result_t {
                    if (m.cursor > 0)
                        --m.cursor;
                    return {m, noop};
                },
                [&](redo_action) -> result_t {
                    if ((m.cursor) < m.history.size())
                        ++m.cursor;
                    return {m, noop};
                },
                [&](pause_action) -> result_t {
                    m.paused = true;
                    return {m, [](auto&& ctx) { ctx.loop().pause(); }};
                },
                [&](resume_action) -> result_t {
                    auto resume_eff =
                        effect<action>{[](auto&& ctx) { ctx.loop().resume(); }};
                    auto eff         = effect<action, deps_t>{noop};
                    auto pending     = m.pending;
                    m.paused         = false;
                    m.pending        = {};
                    std::tie(m, eff) = immer::accumulate(
                        pending,
                        std::pair{m, eff},
                        [&](result_t acc, auto&& act) -> result_t {
                            auto [m, eff] = LAGER_FWD(acc);
                            auto [new_m, new_eff] =
                                update(reducer, std::move(m), LAGER_FWD(act));
                            return {new_m, sequence(eff, new_eff)};
                        });
                    return {m, sequence(resume_eff, eff)};
                },
            },
            act);
    }

    template <typename Server>
    static void view(Server& serv, const model& m)
    {
        serv.view(m);
    }
};

template <template <class, class, class> class Debugger = debugger,
          typename Server>
auto with_debugger(Server& serv)
{
    return [&](auto next) {
        return [&serv, next](auto action,
                             auto&& model,
                             auto&& reducer,
                             auto&& loop,
                             auto&& deps,
                             auto&& tags) {
            using action_t   = typename decltype(action)::type;
            using model_t    = std::decay_t<decltype(model)>;
            using deps_t     = std::decay_t<decltype(deps)>;
            using debugger_t = Debugger<action_t, model_t, deps_t>;
            auto handle      = serv.make(debugger_t{});
            auto store       = next(
                type_<typename debugger_t::action>{},
                typename debugger_t::model{LAGER_FWD(model)},
                [reducer = LAGER_FWD(reducer)](auto&& model, auto&& action) {
                    return debugger_t::update(
                        reducer, LAGER_FWD(model), LAGER_FWD(action));
                },
                LAGER_FWD(loop),
                LAGER_FWD(deps).merge(lager::make_deps(handle)),
                LAGER_FWD(tags));
            handle->init(store, store.xform(zug::map([](auto&& x) {
                return static_cast<typename debugger_t::model>(x);
            })));
            return store;
        };
    };
};

} // namespace lager
