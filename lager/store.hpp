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

#include <type_traits>
#include <memory>

namespace lager {

//!
// Stores the data model for and glues together the components to observe and
// update it.  @see make_store for details about the different initialization
// components.
//
template <typename Action,
          typename Model,
          typename ReducerFn,
          typename ViewFn,
          typename EventLoop>
struct store
{
    using action_t     = Action;
    using model_t      = Model;
    using reducer_t    = ReducerFn;
    using view_t       = ViewFn;
    using event_loop_t = EventLoop;
    using context_t    = context<Action>;

    store(model_t init,
          reducer_t reducer,
          view_t view,
          event_loop_t loop)
        : impl_{std::make_unique<impl>(
            std::move(init),
            std::move(reducer),
            std::move(view),
            std::move(loop))}
    {}

    //!
    // Schedule an action.  This will update the data-model using the @a reducer
    // and re-execute the @a view.
    //
    void dispatch(action_t action)
    { impl_->dispatch(action); }

    //!
    // Schedule a re-execution of the the view procedure on the current state.
    // This can be useful when the view has hidden effectful sources.
    //
    // @note This operation might disappear in the future.  The same effect can
    //       be achieved by providing a no-op action.
    //
    void update()
    { return impl_->update(); }

    //!
    // Return the context for this @a store.
    //
    // @note The context is a reference type. It becomes invalidated when the
    //       store goes away.
    //
    context_t get_context()
    { return impl_->context; }

private:
    struct impl
    {
        event_loop_t loop;
        model_t model;
        reducer_t reducer;
        view_t view;
        context_t context;

        impl(model_t init_,
             reducer_t reducer_,
             view_t view_,
             event_loop_t loop_)
            : loop{std::move(loop_)}
            , model{std::move(init_)}
            , reducer{std::move(reducer_)}
            , view{std::move(view_)}
            , context{[this] (auto ev) { dispatch(ev); },
                      [this] (auto fn) { loop.async(fn); },
                      [this] { loop.finish(); },
                      [this] { loop.pause(); },
                      [this] { loop.resume(); }}
        {
            update();
        };

        void update()
        {
            loop.post([=] {
                view(model);
            });
        }

        void dispatch(action_t action)
        {
            loop.post([=] {
                model = invoke_reducer(reducer, model, action,
                                       [&] (auto&& effect) {
                                           LAGER_FWD(effect)(context);
                                       });
                view(model);
            });
        }
    };

    std::unique_ptr<impl> impl_;
};

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
//        world, and it should return a new model value with the updated state of
//        the world.  If we evaluate the function with the same arguments, it
//        should always return exactly the same arguments.  If, given the current
//        action, it decides that some side-effects are required (reading or
//        writing files, generating random numbers, making network requests,
//        etc.) it should use the second signature, which allows to schedule
//        side-effects.
//
//        @see effect for details.
//
// @param view
//        A procedure with the signature `(Model) -> void`.  It is invoked in the
//        event-loop whenever the data-model changes, and it can perform
//        side-effects to, for example, update the screen the present the current
//        state to the user.
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
          typename Enhancer>
auto make_store(Model&& init,
                ReducerFn&& reducer,
                ViewFn&& view,
                EventLoop&& loop,
                Enhancer&& enhancer)
{
    auto store_creator = enhancer([&] (auto action, auto&& ...args) {
        using action_t = typename decltype(action)::type;
        return store<action_t, std::decay_t<decltype(args)>...>{
            std::forward<decltype(args)>(args)...
        };
    });
    return store_creator(
        type_<Action>{},
        std::forward<Model>(init),
        std::forward<ReducerFn>(reducer),
        std::forward<ViewFn>(view),
        std::forward<EventLoop>(loop));
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
    return make_store<Action>(
        std::forward<Model>(init),
        std::forward<ReducerFn>(reducer),
        std::forward<ViewFn>(view),
        std::forward<EventLoop>(loop),
        identity);
}

} // namespace lager
