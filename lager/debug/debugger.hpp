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

#include <immer/array.hpp>
#include <immer/box.hpp>
#include <immer/vector.hpp>

#include <lager/debug/cereal/immer_vector.hpp>
#include <lager/debug/cereal/variant_with_name.hpp>

#include <variant>

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

    using action = std::variant<
        Action,
        goto_action,
        undo_action,
        redo_action>;

    struct step
    {
        Action action;
        Model model;
    };
    struct model
    {
        cursor_t cursor = {};
        Model init;
        immer::vector<step> history = {};

        model() = default;
        model(Model i) : init{i} {}

        operator const Model& () const {
            assert(cursor <= history.size());
            return cursor == 0
                ? init
                : history[cursor - 1].model;
        }
    };

    template <typename ReducerFn>
    static std::pair<model, effect<action>>
    update(ReducerFn&& reducer, model m, action act)
    {
        using result_t = std::pair<model, effect<action>>;
        return std::visit(visitor{
                [&] (Action act) -> result_t {
                    auto result_eff = effect<action>{noop};
                    m.history = m.history
                        .take(m.cursor)
                        .push_back({act, invoke_reducer(
                                    reducer, m, act,
                                    [&] (auto&& eff) {
                                        result_eff = LAGER_FWD(eff);
                                    })});
                    m.cursor = m.history.size();
                    return {m, result_eff};
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
            }, act);
    }

    template <typename Server, typename ViewFn>
    static void view(Server& serv, ViewFn&& view, const model& m)
    {
        serv.view(m);
        std::forward<ViewFn>(view)(m);
    }

    template <typename A> friend void serialize(A& a, undo_action&) {}
    template <typename A> friend void serialize(A& a, redo_action&) {}
    template <typename A> friend void serialize(A& a, goto_action& x) { a(cereal::make_nvp("cursor", x.cursor)); }
    template <typename A> friend void serialize(A& a, model& x)
    {
        a(cereal::make_nvp("cursor", x.cursor),
          cereal::make_nvp("init", x.init),
          cereal::make_nvp("history", x.history));
    }
    template <typename A> friend void serialize(A& a, step& x)
    {
        a(cereal::make_nvp("action", x.action),
          cereal::make_nvp("model", x.model));
    }
};

} // namespace lager
