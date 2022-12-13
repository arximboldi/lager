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

#include <lager/deps.hpp>
#include <lager/future.hpp>
#include <lager/util.hpp>

#include <boost/hana/all_of.hpp>
#include <boost/hana/append.hpp>
#include <boost/hana/find_if.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/length.hpp>
#include <boost/hana/remove_if.hpp>
#include <boost/hana/type.hpp>

#include <functional>
#include <memory>
#include <type_traits>

namespace lager {

/*!
 * Type used to declare contexes suporting multiple action types.
 *
 * @see context
 */
template <typename... Actions>
struct actions
{
    constexpr static auto as_hana_ = boost::hana::tuple_t<Actions...>;
};

/*!
 * Metafunction that wraps the parameter in `actions<T>` if it is not wrapped
 * already, this is, it returns @a ActionOrActions if it is a type of the form
 * `actions<Ts...>` or `actions<ActionOrActions>` otherwise.
 */
template <typename ActionOrActions>
struct as_actions
{
    using type = actions<ActionOrActions>;
};

template <typename... Actions>
struct as_actions<actions<Actions...>>
{
    using type = actions<Actions...>;
};

template <>
struct as_actions<void>
{
    using type = actions<>;
};

template <typename ActionOrActions>
using as_actions_t = typename as_actions<ActionOrActions>::type;

namespace detail {

template <typename Action, typename Candidates>
auto find_convertible_action_aux(Action act, Candidates candidates)
{
    auto is_convertible = [](auto t) {
        return std::is_convertible<typename Action::type,
                                   typename decltype(t)::type>{};
    };
    return boost::hana::find_if(candidates, is_convertible).value();
}

template <typename Action, typename... Actions>
using find_convertible_action_t = typename decltype(find_convertible_action_aux(
    boost::hana::type_c<Action>, boost::hana::tuple_t<Actions...>))::type;

template <typename... A1, typename... A2, typename Converter>
auto are_compatible_actions_aux(actions<A1...>, actions<A2...>, Converter c)
{
    return boost::hana::all_of(boost::hana::tuple_t<A1...>, [](auto t1) {
        return boost::hana::is_just(
            boost::hana::find_if(boost::hana::tuple_t<A2...>, [](auto t2) {
                return std::is_convertible<
                    decltype(c(std::declval<typename decltype(t1)::type>())),
                    typename decltype(t2)::type>{};
            }));
    });
}

template <typename Actions1,
          typename Actions2,
          typename ConverterT = identity_t>
constexpr bool are_compatible_actions_v =
    decltype(are_compatible_actions_aux(as_actions_t<Actions1>{},
                                        as_actions_t<Actions2>{},
                                        std::declval<ConverterT>()))::value;

template <typename Actions1, typename Actions2>
auto merge_actions_aux(Actions1 a1, Actions2 a2)
{
    static_assert(decltype(boost::hana::length(a1))::value > 0, "");
    static_assert(decltype(boost::hana::length(a2))::value > 0, "");
    auto has_convertible = [](auto seq, auto x) {
        return boost::hana::is_just(boost::hana::find_if(seq, [](auto t) {
            return std::is_convertible<typename decltype(x)::type,
                                       typename decltype(t)::type>{};
        }));
    };
    auto remove_convertibles = [](auto seq, auto x) {
        return boost::hana::remove_if(seq, [](auto t) {
            return std::is_convertible<typename decltype(x)::type,
                                       typename decltype(t)::type>{};
        });
    };
    auto result = boost::hana::fold(a1, a2, [&](auto acc, auto x) {
        return boost::hana::if_(
            has_convertible(acc, x),
            [&] { return acc; },
            [&] {
                auto xs = remove_convertibles(acc, x);
                return boost::hana::append(xs, x);
            })();
    });
    static_assert(decltype(boost::hana::length(result))::value > 0, "");
    return boost::hana::if_(
        boost::hana::length(result) == boost::hana::size_c<1>,
        [&] { return result[boost::hana::size_c<0>]; },
        [&] {
            return boost::hana::unpack(result, [](auto... xs) {
                return boost::hana::type_c<
                    actions<typename decltype(xs)::type...>>;
            });
        })();
}

template <typename Actions1, typename Actions2>
using merge_actions_t = typename decltype(merge_actions_aux(
    as_actions_t<Actions1>::as_hana_, as_actions_t<Actions2>::as_hana_))::type;

template <typename... Actions>
struct dispatcher;

template <typename... Actions>
struct dispatcher<actions<Actions...>> : std::function<future(Actions)>...
{
    using std::function<future(Actions)>::operator()...;

    dispatcher() = default;

    template <typename... As>
    dispatcher(dispatcher<actions<As...>> other)
        : std::function<future(Actions)>{static_cast<std::function<future(
              find_convertible_action_t<Actions, As...>)>&>(other)}...
    {}

    template <typename Fn>
    dispatcher(Fn other)
        : std::function<future(Actions)>{other}...
    {}

    template <typename Action, typename... As, typename Converter>
    static auto dispatcher_fn_aux(dispatcher<actions<As...>> other_,
                                  Converter conv)
    {
        auto& other = static_cast<std::function<future(
            find_convertible_action_t<std::invoke_result_t<Converter, Action>,
                                      As...>)>&>(other_);
        return
            [conv, other](auto&& act) { return other(conv(LAGER_FWD(act))); };
    }

    template <typename... As, typename Converter>
    dispatcher(dispatcher<actions<As...>> other, Converter conv)
        : std::function<future(Actions)>{
              dispatcher_fn_aux<Actions>(other, conv)}...
    {}

    template <typename Fn, typename Converter>
    dispatcher(Fn other, Converter conv)
        : std::function<future(Actions)>{[other, conv](auto&& act) {
            return other(conv(LAGER_FWD(act)));
        }}...
    {}
};

struct event_loop_iface
{
    virtual ~event_loop_iface()               = default;
    virtual void post(std::function<void()>)  = 0;
    virtual void async(std::function<void()>) = 0;
    virtual void finish()                     = 0;
    virtual void pause()                      = 0;
    virtual void resume()                     = 0;
};

template <typename EventLoop>
struct event_loop_impl final : event_loop_iface
{
    EventLoop& loop;

    event_loop_impl(EventLoop& loop_)
        : loop{loop_}
    {}
    void post(std::function<void()> fn) override { loop.post(std::move(fn)); }
    void async(std::function<void()> fn) override { loop.async(std::move(fn)); }
    void finish() override { loop.finish(); }
    void pause() override { loop.pause(); }
    void resume() override { loop.resume(); }
};

} // namespace detail

/*!
 * Provide some _context_ for effectful functions, allowing them to control the
 * event loop and dispatch new actions into the store.
 *
 * A context is convertible to support "more restricted" actions.  This is, if
 * action `B` is convertible to action `A`, `context<A>` is convertible to
 * `context<B>`, in this sense, contexes are contravariant to the action type.
 * One can also specify multiple action types by using `action<>` tag. This is
 * useful to subset actions from a variant, here is an example:
 *
 * @rst
 *
 * .. code-block:: c++
 *
 *    struct action_A {};
 *    struct action_B {};
 *    struct action_C {};
 *    using any_action = std::variant<action_A, action_B, action_C>>;
 *
 *    void some_effect(context<actions<action_A, action_B>> ctx)
 *    {
 *        if (...)
 *            ctx.dispatch(action_A{});
 *        else
 *            ctx.dispatch(action_B{});
 *    }
 *
 *    void other_effect(context<any_action> ctx)
 *    {
 *        some_effect(ctx);
 *        ...
 *    }
 *
 * @endrst
 *
 * @note This is a reference type and it's life-time is bound to the associated
 *       store.  It is invalid to use it after the store has been destructed.
 *       Its methods may modify the store's underlying state.
 *
 * @note Use action type `void` or empty `lager::actions<>` if `context` shall
 *       have no `dispatch()` method and only provide deps.
 *
 * @todo Make constructors private.
 */
template <typename Actions = void, typename Deps = lager::deps<>>
struct context : Deps
{
    using deps_t    = Deps;
    using actions_t = as_actions_t<Actions>;

    context() = default;

    template <
        typename Actions_,
        typename Deps_,
        std::enable_if_t<detail::are_compatible_actions_v<Actions, Actions_> &&
                             std::is_convertible_v<Deps_, Deps>,
                         int> = 0>
    context(const context<Actions_, Deps_>& ctx)
        : deps_t{ctx}
        , dispatcher_{ctx.dispatcher_}
        , loop_{ctx.loop_}
    {}

    template <
        typename Actions_,
        typename Deps_,
        typename Converter,
        std::enable_if_t<
            detail::are_compatible_actions_v<Actions, Actions_, Converter> &&
                std::is_convertible_v<Deps_, Deps>,
            int> = 0>
    context(const context<Actions_, Deps_>& ctx, Converter c)
        : deps_t{ctx}
        , dispatcher_{ctx.dispatcher_, c}
        , loop_{ctx.loop_}
    {}

    template <typename Dispatcher, typename EventLoop>
    context(Dispatcher dispatcher, EventLoop& loop, deps_t deps)
        : deps_t{std::move(deps)}
        , dispatcher_{std::move(dispatcher)}
        , loop_{std::make_shared<detail::event_loop_impl<EventLoop>>(loop)}
    {}

    template <typename Action>
    future dispatch(Action&& act) const
    {
        return dispatcher_(std::forward<Action>(act));
    }

    detail::event_loop_iface& loop() const { return *loop_; }

private:
    template <typename A, typename Ds>
    friend struct context;

    detail::dispatcher<actions_t> dispatcher_;
    std::shared_ptr<detail::event_loop_iface> loop_;
};

} // namespace lager
