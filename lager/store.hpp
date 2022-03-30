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
#include <lager/effect.hpp>
#include <lager/state.hpp>
#include <lager/util.hpp>

#include <zug/compose.hpp>

#include <boost/hana/contains.hpp>
#include <boost/hana/set.hpp>
#include <boost/hana/union.hpp>

#include <memory>
#include <type_traits>

namespace lager {

namespace detail {

template <typename Action, typename Model>
struct store_node_base : public root_node<Model, reader_node>
{
    using action_t   = Action;
    using model_t    = Model;
    using value_type = Model;
    using base_t     = root_node<Model, reader_node>;
    using base_t::base_t;

    virtual void recompute() final {}
    virtual future dispatch(action_t action) = 0;
};

} // namespace detail

/*!
 * Stores the data model for and glues together the components to observe and
 * update it.  @see `lager::make_store` for details about the different
 * initialization components.
 */
template <typename Action,
          typename Model,
          typename Dependencies = lager::deps<>>
class store
    : public context<Action, Dependencies>
    , public reader_base<detail::store_node_base<Action, Model>>
{
    using reader_t  = reader_base<detail::store_node_base<Action, Model>>;
    using context_t = context<Action, Dependencies>;

    friend class detail::access;
    auto roots() const { return detail::access::node(*this); }

public:
    using action_t = Action;
    using model_t  = Model;
    using deps_t   = Dependencies;

    using reader_t::get;

    template <typename ReducerFn,
              typename EventLoop,
              typename Deps,
              typename Tags>
    store(model_t init,
          ReducerFn reducer,
          EventLoop loop,
          Deps dependencies,
          Tags tags)
        : store{std::make_shared<store_node<ReducerFn, EventLoop, Deps, Tags>>(
              std::move(init),
              std::move(reducer),
              std::move(loop),
              std::move(dependencies))}
    {}

    template <typename Action_,
              typename Model_,
              typename Deps_,
              std::enable_if_t<std::is_convertible_v<Deps_, Dependencies>,
                               bool> = true>
    store(store<Action_, Model_, Deps_> other)
        : context_t{std::move(other)}
        , reader_t{std::move(other)}
    {}

    /*
     * The type can be moved.
     */
    store(store&&) = default;
    store& operator=(store&&) = default;

    /*!
     * Deleted copies, this is a move-only type.
     */
    store(const store&) = delete;
    store& operator=(const store&) = delete;

private:
    template <typename A, typename M, typename D>
    friend class store;

    template <typename ReducerFn,
              typename EventLoop,
              typename Deps,
              typename Tags>
    struct store_node final : detail::store_node_base<Action, Model>
    {
        using base_t             = detail::store_node_base<Action, Model>;
        using reducer_t          = ReducerFn;
        using event_loop_t       = EventLoop;
        using deps_t             = Deps;
        using concrete_context_t = context<Action, Deps>;

        event_loop_t loop;
        reducer_t reducer;
        concrete_context_t ctx;

        static constexpr bool is_transactional = boost::hana::contains(
            Tags{}, boost::hana::type_c<transactional_tag>);
        static constexpr bool has_futures = boost::hana::contains(
            Tags{}, boost::hana::type_c<enable_futures_tag>);

        store_node(model_t init_,
                   reducer_t reducer_,
                   event_loop_t loop_,
                   deps_t deps_)
            : base_t{std::move(init_)}
            , loop{std::move(loop_)}
            , reducer{std::move(reducer_)}
            , ctx{[this](auto&& act) { return dispatch(LAGER_FWD(act)); },
                  loop,
                  std::move(deps_)}
        {}

        future dispatch(action_t action) override
        {
            auto [p, f] = [&] {
                if constexpr (has_futures)
                    return promise::with_loop(loop);
                else
                    return promise::invalid();
            }();
            loop.post([this,
                       p      = std::move(p),
                       action = std::move(action)]() mutable {
                base_t::push_down(invoke_reducer<deps_t>(
                    reducer,
                    base_t::current(),
                    std::move(action),
                    [&](auto&& effect) {
                        loop.post([this,
                                   p   = std::move(p),
                                   eff = LAGER_FWD(effect)]() mutable {
                            if constexpr (!is_transactional) {
                                base_t::send_down();
                                base_t::notify();
                            }
                            if constexpr (std::is_same_v<void,
                                                         decltype(eff(ctx))>) {
                                eff(ctx);
                                if constexpr (has_futures)
                                    p();
                            } else {
                                auto f = eff(ctx);
                                if constexpr (has_futures)
                                    std::move(f).then(std::move(p));
                            }
                        });
                    },
                    [&] {
                        if constexpr (!is_transactional) {
                            loop.post([this, p = std::move(p)]() mutable {
                                base_t::send_down();
                                base_t::notify();
                                if constexpr (has_futures)
                                    p();
                            });
                        } else if constexpr (has_futures)
                            p();
                    }));
            });
            return std::move(f);
        }
    };

    template <typename ReducerFn,
              typename EventLoop,
              typename Deps,
              typename Tag>
    store(std::shared_ptr<store_node<ReducerFn, EventLoop, Deps, Tag>> node)
        : context_t{node->ctx}
        , reader_t{std::move(node)}
    {}
};

/*!
 * Store enhancer that adds certaings tags to the store
 */
template <typename... Tags>
ZUG_INLINE_CONSTEXPR auto with_tags = [](auto next) {
    return [next](auto action,
                  auto&& model,
                  auto&& reducer,
                  auto&& loop,
                  auto&& deps,
                  auto&& tags) {
        return next(action,
                    LAGER_FWD(model),
                    LAGER_FWD(reducer),
                    LAGER_FWD(loop),
                    LAGER_FWD(deps),
                    boost::hana::union_(
                        LAGER_FWD(tags),
                        boost::hana::make_set(boost::hana::type_c<Tags>...)));
    };
};

/*!
 * Store enhancer that enables futures support for the given store.
 */
ZUG_INLINE_CONSTEXPR auto with_futures = with_tags<enable_futures_tag>;

/*!
 * Store enhancer that adds dependencies to the store.
 *
 * @note The dependencies are constructed as by `make_deps()`.
 */
template <typename... Args>
auto with_deps(Args&&... args)
{
    auto new_deps = make_deps(std::forward<Args>(args)...);
    return [new_deps](auto next) {
        return [new_deps, next](auto action,
                                auto&& model,
                                auto&& reducer,
                                auto&& loop,
                                auto&& deps,
                                auto&& tags) {
            return next(action,
                        LAGER_FWD(model),
                        LAGER_FWD(reducer),
                        LAGER_FWD(loop),
                        LAGER_FWD(deps).merge(new_deps),
                        LAGER_FWD(tags));
        };
    };
}

/*!
 * Replaces the reducer to be used in the store.
 *
 * @param reducer
 *        Pure function that, given the current state of the data-model, and an
 *        action, returns an updated data-model.  It can have one  one of these
 *        two signatures:
 *          1. `(Model, Action) -> Model`
 *          2. `(Model, Action) -> pair<Model, effect<Model, Action>>`
 */
template <typename Reducer>
auto with_reducer(Reducer&& reducer)
{
    return [reducer = LAGER_FWD(reducer)](auto next) {
        return [reducer, next](auto action,
                               auto&& model,
                               auto&& old_reducer,
                               auto&& loop,
                               auto&& deps,
                               auto&& tags) {
            return next(action,
                        LAGER_FWD(model),
                        reducer,
                        LAGER_FWD(loop),
                        LAGER_FWD(deps),
                        LAGER_FWD(tags));
        };
    };
}

/*!
 * Default reducer that calls `update` on the model.  Normally this is a
 * function in the namespace of the model.
 */
ZUG_INLINE_CONSTEXPR struct default_reducer_t
{
    template <typename Model, typename Action>
    auto operator()(Model&& s, Action&& a) const
        -> decltype(update(LAGER_FWD(s), LAGER_FWD(a)))
    {
        return update(LAGER_FWD(s), LAGER_FWD(a));
    }
} default_reducer;

//! @defgroup make_store
//! @{

/*!
 *
 * Builds a `lager::store` that glues together the core components of an
 * interactive application following an _unidirectional data flow architecture_.
 *
 * @tparam Action Value type that represents an event (an interaction) happening
 *         in the application.
 *
 * @tparam Tag Use `automatic_tag` to indicate that the store should
 *         automatically make changes visible notified after every action is
 *         processed.  Use `transactional_tag` to require a `lager::commit`
 *         call for that.
 *
 * @param init Initial value of the data-model.  It should be a value-type.
 *
 * @param loop
 *        Event loop in which operations can be scheduled.  This allows
 *        thread-safe action dispatching reusing the technology provided by the
 *        UI framework at hand (it does not need to be the event-loop of a UI
 *        framework.)
 *
 * @param enhancer
 *        _Optional_ middleware that _enhances_ or modified the applications
 *        behavior in a general way.
 *
 *        @todo Document Enhancer concept.
 *
 * @rst
 *
 * .. note:: The term *reducer* is due to the fact that, if we consider the
 *      sequence of actions over time, this function "reduces" the sequence to a
 *      single model value.  This is a pure function and it should have no
 *      side-effects---it takes a model value with the current state of the
 *      world, and it should return a new model value with the updated state of
 *      the world.  If we evaluate the function with the same arguments, it
 *      should always return exactly the same arguments.  If, given the current
 *      action, it decides that some side-effects are required (reading or
 *      writing files, generating random numbers, making network requests, etc.)
 *      it should use the second signature, which allows to schedule
 *      side-effects.
 *
 * @endrst
 */
template <typename Action,
          typename Tag = void, // use default
          typename Model,
          typename EventLoop,
          typename... Enhancers>
auto make_store(Model&& init, EventLoop&& loop, Enhancers&&... enhancers)
{
    auto enhancer      = zug::comp(std::forward<Enhancers>(enhancers)...);
    auto store_creator = enhancer([&](auto action,
                                      auto&& model,
                                      auto&& reducer,
                                      auto&& loop,
                                      auto&& deps,
                                      auto&& tags) {
        using action_t = typename decltype(action)::type;
        using model_t  = std::decay_t<decltype(model)>;
        using deps_t   = std::decay_t<decltype(deps)>;
        return store<action_t, model_t, deps_t>{LAGER_FWD(model),
                                                LAGER_FWD(reducer),
                                                LAGER_FWD(loop),
                                                LAGER_FWD(deps),
                                                tags};
    });
    return store_creator(type_<Action>{},
                         std::forward<Model>(init),
                         default_reducer,
                         std::forward<EventLoop>(loop),
                         deps<>{},
                         boost::hana::make_set(boost::hana::type_c<Tag>));
}

template <typename Action,
          typename Tag = automatic_tag,
          typename Model,
          typename EventLoop>
auto make_store(Model&& init, EventLoop&& loop)
{
    return make_store<Action, Tag>(
        std::forward<Model>(init), std::forward<EventLoop>(loop), identity);
}

//! @} group: make_store

} // namespace lager
