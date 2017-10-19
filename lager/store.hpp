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

#include <boost/asio/io_service.hpp>

#include <functional>
#include <future>

namespace ewig {

template <typename Action>
struct context
{
    using action_t      = Action;
    using service_t     = boost::asio::io_service;
    using service_ref_t = std::reference_wrapper<service_t>;
    using finish_t      = std::function<void()>;
    using dispatch_t    = std::function<void(action_t)>;

    std::reference_wrapper<service_t> service;
    finish_t finish;
    dispatch_t dispatch;

    context(const context& ctx) = default;
    context(context&& ctx) = default;

    template <typename Action_>
    context(const context<Action_>& ctx)
        : service(ctx.service)
        , finish(ctx.finish)
        , dispatch(ctx.dispatch)
    {}

    context(service_t& serv, finish_t fn, dispatch_t ds)
        : service(serv)
        , finish(std::move(fn))
        , dispatch(std::move(ds))
    {}

    template <typename Fn>
    void async(Fn&& fn) const
    {
        std::thread([fn=std::move(fn),
                   work=boost::asio::io_service::work(service)] {
            fn();
        }).detach();
    }
};

template <typename Action>
using effect = std::function<void(const context<Action>&)>;

constexpr auto noop = [] (auto&&...) {};

template <typename Model, typename Action>
struct result : std::pair<Model, effect<Action>>
{
    using base_t = std::pair<Model, effect<Action>>;
    using base_t::base_t;
    result(Model m) : base_t{m, noop} {};
};

template <typename Model, typename Action>
struct store : context<Action>
{
    using base_t    = context<Action>;
    using model_t   = Model;
    using action_t  = Action;
    using finish_t  = typename base_t::finish_t;
    using reducer_t = std::function<result<model_t, action_t>
                                   (model_t, action_t)>;
    using view_t    = std::function<void(model_t)>;

    store(const store&) = delete;
    store& operator=(const store&) = delete;

    store(boost::asio::io_service& serv,
          model_t init,
          reducer_t reducer,
          view_t view,
          finish_t finish)
        : base_t{serv,
                 std::move(finish),
                 [this] (auto ev) { dispatch(ev); }}
        , model_{std::move(init)}
        , reducer_{std::move(reducer)}
        , view_{std::move(view)}
    {
        view_(model_);
    }

    void dispatch(action_t action)
    {
        base_t::service.get().post([=] {
            auto [model, effect] = reducer_(model_, action);
            model_ = model;
            effect(*this);
            view_(model_);
        });
    }

private:
    model_t model_;
    reducer_t reducer_;
    view_t view_;
};

} // namespace ewig
