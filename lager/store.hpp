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
#include <lager/deps.hpp>
#include <lager/util.hpp>

#include <memory>
#include <type_traits>

namespace lager {

namespace detail {

template <typename Action, typename Model>
struct store_impl_base
{
    using action_t = Action;
    using model_t  = Model;

    model_t model;

    store_impl_base(model_t model_)
        : model{std::move(model_)}
    {}

    virtual ~store_impl_base()             = default;
    virtual void dispatch(action_t action) = 0;
    virtual void update()                  = 0;
};

} // namespace detail

//!
// Stores the data model for and glues together the components to observe and
// update it.  @see make_store for details about the different initialization
// components.
//
template <typename Action,
          typename Model,
          typename Dependencies = lager::deps<>>
struct store
{
    using action_t  = Action;
    using model_t   = Model;
    using deps_t    = Dependencies;
    using context_t = context<Action, Dependencies>;

    template <typename ReducerFn,
              typename ViewFn,
              typename EventLoop,
              typename Deps>
    store(model_t init,
          ReducerFn reducer,
          ViewFn view,
          EventLoop loop,
          Deps dependencies)
        : store{std::make_unique<impl<ReducerFn, ViewFn, EventLoop, Deps>>(
              std::move(init),
              std::move(reducer),
              std::move(view),
              std::move(loop),
              std::move(dependencies))}
    {}

    template <typename Action_,
              typename Model_,
              typename Deps_,
              std::enable_if_t<std::is_convertible_v<Deps_, Dependencies>,
                               bool> = true>
    store(store<Action_, Model_, Deps_> other)
        : context_{std::move(other.context_)}
        , impl_{std::move(other.impl_)}
    {}

    store(store&&) = default;
    store& operator=(store&&) = default;

    store(const store&) = delete;
    store& operator=(const store&) = delete;

    void dispatch(action_t action) { impl_->dispatch(action); }
    void update() { return impl_->update(); }
    context_t get_context() { return context_; }
    const model_t& current() { return impl_->model; }

private:
    template <typename A, typename M, typename D>
    friend class store;

    using impl_base = detail::store_impl_base<Action, Model>;

    template <typename ReducerFn,
              typename ViewFn,
              typename EventLoop,
              typename Deps>
    struct impl final : impl_base
    {
        using reducer_t          = ReducerFn;
        using view_t             = ViewFn;
        using event_loop_t       = EventLoop;
        using deps_t             = Deps;
        using concrete_context_t = context<Action, Deps>;

        concrete_context_t ctx;
        event_loop_t loop;
        reducer_t reducer;
        view_t view;

        impl(model_t init_,
             reducer_t reducer_,
             view_t view_,
             event_loop_t loop_,
             deps_t deps_)
            : impl_base{std::move(init_)}
            , ctx{[this](auto ev) { dispatch(ev); },
                  [this](auto fn) { loop.async(fn); },
                  [this] { loop.finish(); },
                  [this] { loop.pause(); },
                  [this] { loop.resume(); },
                  std::move(deps_)}
            , loop{std::move(loop_)}
            , reducer{std::move(reducer_)}
            , view{std::move(view_)}
        {
            update();
        };

        void update() override
        {
            loop.post([=] { view(this->model); });
        }

        void dispatch(action_t action) override
        {
            loop.post([=] {
                this->model = invoke_reducer<deps_t>(
                    reducer, this->model, action, [&](auto&& effect) {
                        LAGER_FWD(effect)(this->ctx);
                    });
                view(this->model);
            });
        }
    };

    template <typename ReducerFn,
              typename ViewFn,
              typename EventLoop,
              typename Deps>
    store(std::unique_ptr<impl<ReducerFn, ViewFn, EventLoop, Deps>> the_impl)
        : context_(the_impl->ctx)
        , impl_(std::move(the_impl))
    {}

    context_t context_;
    std::unique_ptr<impl_base> impl_;
};

//!
// Store enhancer that adds dependencies to the store.
//
// @note The dependencies are constructed as by `make_deps()`.
//
template <typename... Args>
auto with_deps(Args&&... args)
{
    auto new_deps = make_deps(std::forward<Args>(args)...);
    return [new_deps](auto next) {
        return [new_deps, next](auto action,
                                auto&& model,
                                auto&& reducer,
                                auto&& view,
                                auto&& loop,
                                auto&& deps) {
            return next(action,
                        LAGER_FWD(model),
                        LAGER_FWD(reducer),
                        LAGER_FWD(view),
                        LAGER_FWD(loop),
                        LAGER_FWD(deps).merge(new_deps));
        };
    };
}

//!
//
// Builds a @a store that glues together the core components of an interactive
// application following an _unidirectional data flow architecture_.
//
// @tparam Action Value type that represents an event (an interaction) happening
//         in the application.
//
// @param init Initial value of the data-model.  It should be a value-type.
//
// @param reducer
//        Pure function that, given the current state of the data-model, and an
//        action, returns an updated data-model.  It can have one  one of these
//        two signatures:
//
//            1. (Model, Action) -> Model
//            2. (Model, Action) -> pair<Model, effect<Model, Action>>
//
//        The term reducer is due to the fact that, if we consider the sequence
//        of actions over time, this function "reduces" the sequence to a single
//        model value.  This is a pure function and it should have no
//        side-effects---it takes a model value with the current state of the
//        world, and it should return a new model value with the updated state
//        of the world.  If we evaluate the function with the same arguments, it
//        should always return exactly the same arguments.  If, given the
//        current action, it decides that some side-effects are required
//        (reading or writing files, generating random numbers, making network
//        requests, etc.) it should use the second signature, which allows to
//        schedule side-effects.
//
//        @see effect for details.
//
// @param view
//        A procedure with the signature `(Model) -> void`.  It is invoked in
//        the event-loop whenever the data-model changes, and it can perform
//        side-effects to, for example, update the screen the present the
//        current state to the user.
//
// @param loop
//        Event loop in which operations can be scheduled.  This allows
//        thread-safe action dispatching reusing the technology provided by the
//        UI framework at hand (it does not need to be the event-loop of a UI
//        framework.)
//
//        @todo Document EventLoop concept.
//
// @param enhancer
//        _Optional_ middleware that _enhances_ or modified the applications
//        behavior in a general way.
//
//        @todo Document Enhancer concept.
//
template <typename Action,
          typename Model,
          typename ReducerFn,
          typename ViewFn,
          typename EventLoop,
          typename... Enhancers>
auto make_store(Model&& init,
                ReducerFn&& reducer,
                ViewFn&& view,
                EventLoop&& loop,
                Enhancers&&... enhancers)
{
    auto enhancer      = comp(std::forward<Enhancers>(enhancers)...);
    auto store_creator = enhancer([&](auto action,
                                      auto&& model,
                                      auto&& reducer,
                                      auto&& view,
                                      auto&& loop,
                                      auto&& deps) {
        using action_t = typename decltype(action)::type;
        using model_t  = std::decay_t<decltype(model)>;
        using deps_t   = std::decay_t<decltype(deps)>;
        return store<action_t, model_t, deps_t>{LAGER_FWD(model),
                                                LAGER_FWD(reducer),
                                                LAGER_FWD(view),
                                                LAGER_FWD(loop),
                                                LAGER_FWD(deps)};
    });
    return store_creator(type_<Action>{},
                         std::forward<Model>(init),
                         std::forward<ReducerFn>(reducer),
                         std::forward<ViewFn>(view),
                         std::forward<EventLoop>(loop),
                         deps<>{});
}

template <typename Action,
          typename Model,
          typename ReducerFn,
          typename ViewFn,
          typename EventLoop>
auto make_store(Model&& init,
                ReducerFn&& reducer,
                ViewFn&& view,
                EventLoop&& loop)
{
    return make_store<Action>(std::forward<Model>(init),
                              std::forward<ReducerFn>(reducer),
                              std::forward<ViewFn>(view),
                              std::forward<EventLoop>(loop),
                              identity);
}

} // namespace lager
