//
// ewig - an immutable text editor
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of ewig.
//
// ewig is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ewig is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ewig.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <ewig/io_service.hpp>
#include <functional>

namespace ewig {

template <typename Action>
using dispatcher = std::function<void(Action)>;

template <typename Action>
using effect = std::function<void(io_service&, dispatcher<Action>)>;

constexpr auto do_stop = [] (auto& serv, auto&&)
{
    serv.stop();
};

constexpr auto noop = [] (auto&&...) {};

template <typename Model, typename Action>
struct result : std::pair<Model, effect<Action>>
{
    using base_t = std::pair<Model, effect<Action>>;
    using base_t::base_t;
    result(Model m) : base_t{m, noop} {};
};

template <typename Model, typename Action>
struct store
{
    using model_t   = Model;
    using action_t  = Action;
    using reducer_t = std::function<result<model_t, action_t>
                                   (model_t, action_t)>;
    using view_t    = std::function<void(model_t)>;

    store(io_service& serv, model_t init, reducer_t reducer, view_t view)
        : serv_(serv)
        , model_(std::move(init))
        , reducer_(std::move(reducer))
        , view_(std::move(view))
    {
        view_(model_);
    }

    void dispatch(action_t action)
    {
        serv_.post([=] {
            auto [model, effect] = reducer_(model_, action);
            model_ = model;
            effect(serv_, [&] (auto x) { dispatch(x); });
            view_(model_);
        });
    }

private:
    io_service& serv_;
    model_t model_;
    reducer_t reducer_;
    view_t view_;
};

} // namespace ewig
