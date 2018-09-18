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

#include <lager/util.hpp>
#include <lager/context.hpp>

#include <immer/vector.hpp>
#include <immer/algorithm.hpp>

#include <lager/debug/cereal/immer_vector.hpp>
#include <lager/debug/cereal/variant_with_name.hpp>
#include <lager/debug/cereal/struct.hpp>

#include <variant>
#include <functional>

namespace lager {

template <typename Action, typename Model>
struct debugger
{
    using base_action = Action;
    using base_model  = Model;

    using cursor_t = std::size_t;

    struct goto_action { cursor_t cursor; };
    struct undo_action {};
    struct redo_action {};
    struct pause_action {};
    struct resume_action {};

    using action = std::variant<
        Action,
        goto_action,
        undo_action,
        redo_action,
        pause_action,
        resume_action>;

    struct step
    {
        Action action;
        Model model;
    };

    struct model
    {
        cursor_t cursor = {};
        bool paused = {};
        Model init;
        immer::vector<step> history = {};
        immer::vector<Action> pending = {};

        model() = default;
        model(Model i) : init{i} {}

        using lookup_result = std::pair<std::optional<Action>, const Model&>;

        lookup_result lookup(cursor_t cursor) const
        {
            if (cursor > history.size())
                throw std::runtime_error{"bad cursor"};
            return cursor == 0
                ? lookup_result{{}, init}
                : [&] {
                    auto& step = history[cursor - 1];
                    return lookup_result{step.action, step.model};
                }();
        }

        operator const Model& () const {
            return lookup(cursor).second;
        }
    };

    template <typename ReducerFn>
    static std::pair<model, effect<action>>
    update(ReducerFn&& reducer, model m, action act)
    {
        using result_t = std::pair<model, effect<action>>;
        return std::visit(visitor{
                [&] (Action act) -> result_t {
                    if (m.paused) {
                        m.pending = m.pending.push_back(act);
                        return {m, noop};
                    } else {
                        auto eff = effect<action>{noop};
                        auto state = invoke_reducer(
                            reducer, m, act,
                            [&] (auto&& e) { eff = LAGER_FWD(e); });
                        m.history = m.history
                            .take(m.cursor)
                            .push_back({act, state});
                        m.cursor = m.history.size();
                        return {m, eff};
                    }
                },
                [&] (goto_action act) -> result_t {
                    if (act.cursor <= m.history.size())
                        m.cursor = act.cursor;
                    return {m, noop};
                },
                [&] (undo_action) -> result_t {
                    if (m.cursor > 0)
                        --m.cursor;
                    return {m, noop};
                },
                [&] (redo_action) -> result_t {
                    if (m.cursor < m.history.size())
                        ++m.cursor;
                    return {m, noop};
                },
                [&] (pause_action) -> result_t {
                    m.paused = true;
                    return {m, [] (auto&& ctx) { ctx.pause(); }};
                },
                [&] (resume_action) -> result_t {
                    auto resume_eff = effect<action>{[] (auto&& ctx) {
                        ctx.resume();
                    }};
                    auto eff = effect<action>{noop};
                    auto pending = m.pending;
                    m.paused = false;
                    m.pending = {};
                    std::tie(m, eff) = immer::accumulate(
                        pending,
                        std::pair{m, eff},
                        [&] (result_t acc, auto&& act) -> result_t {
                            auto [m, eff] = LAGER_FWD(acc);
                            auto [new_m, new_eff] =
                                update(reducer, std::move(m), LAGER_FWD(act));
                            return {new_m, sequence(eff, new_eff)};
                        });
                    return {m, sequence(resume_eff, eff)};
                },
            }, act);
    }

    template <typename Server, typename ViewFn>
    static void view(Server& serv, ViewFn&& view, const model& m)
    {
        serv.view(m);
        std::forward<ViewFn>(view)(m);
    }

    LAGER_CEREAL_NESTED_STRUCT(undo_action);
    LAGER_CEREAL_NESTED_STRUCT(redo_action);
    LAGER_CEREAL_NESTED_STRUCT(pause_action);
    LAGER_CEREAL_NESTED_STRUCT(resume_action);
    LAGER_CEREAL_NESTED_STRUCT(goto_action, (cursor));
    LAGER_CEREAL_NESTED_STRUCT(model, (cursor)(paused)(init)(history));
    LAGER_CEREAL_NESTED_STRUCT(step, (action)(model));
};

} // namespace lager
