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

#include <zug/util.hpp>

#include <functional>
#include <variant>

#define LAGER_FWD(name_) std::forward<decltype(name_)>(name_)

#define LAGER_STATIC_ASSERT_MESSAGE_BEGIN "\n=======================\n\n"
#define LAGER_STATIC_ASSERT_MESSAGE_END "\n\n=======================\n"

namespace lager {

//! @defgroup util
//! @{

/*!
 * Utility to make a variant visitor out of lambdas, using the *overloaded
 * pattern* as [describped in
 * cppreference](https://en.cppreference.com/w/cpp/utility/variant/visit).
 *
 * For alternative mechanisms for nice variant visitation, consider
 * [Scelta](https://github.com/SuperV1234/scelta),
 * [Atria](https://github.com/Ableton/atria), or
 * [Boost.Hof](https://www.boost.org/doc/libs/release/libs/hof/doc/html/doc/index.html).
 */
template <class... Ts>
struct visitor : Ts...
{
    using Ts::operator()...;
};

//! @} group: util

template <class... Ts>
visitor(Ts...)->visitor<Ts...>;

namespace detail {

template <typename TupleT>
struct matcher : TupleT
{
    template <typename... Visitors>
    decltype(auto) operator()(Visitors&&... vs) &&
    {
        return std::apply(
            [&](auto&&... xs) {
                return std::visit(lager::visitor{LAGER_FWD(vs)...},
                                  LAGER_FWD(xs)...);
            },
            static_cast<TupleT&&>(std::move(*this)));
    }
};

template <typename T>
matcher(T)->matcher<T>;

} // namespace detail

template <typename... Variants>
auto match(Variants&&... vs)
{
    return detail::matcher{
        std::forward_as_tuple(std::forward<Variants>(vs)...)};
}

//! @defgroup util
//! @{

//! Function that takes any argument and does nothing
ZUG_INLINE_CONSTEXPR struct noop_t
{
    template <typename... T>
    void operator()(T&&...) const
    {}
} noop{};

//! Function that returns its first arguemnt
ZUG_INLINE_CONSTEXPR struct identity_t
{
    template <typename T>
    decltype(auto) operator()(T&& x) const
    {
        return ZUG_FWD(x);
    };
} identity{};

//! @} group: util

template <typename Type>
struct type_
{
    using type = Type;
};

/*!
 * Unwraps multiple layers of state wrappers added by store enhancers.
 */
template <typename T>
const T& unwrap(const T& x)
{
    return x;
}

/*!
 * Defers calculation of the value of the function till type
 * conversion is actually requested
 */

template <typename Func, typename P, typename X>
struct deferred
{
    Func &&func;
    P &&p;

    operator X() {
        return LAGER_FWD(std::forward<Func>(func)(std::forward<P>(p)));
    }
};

template <typename Func, typename P, typename X = decltype(std::declval<Func>()(std::declval<P>()))>
auto make_deferred(Func&& f, P&& p) -> deferred<Func, P, X>
{
    return {std::forward<Func>(f), std::forward<P>(p)};
}

template <typename T>
struct remove_deferred {
    using type = T;
};

template <typename Func, typename P, typename X>
struct remove_deferred<deferred<Func, P, X>> {
    using type = X;
};

template <typename T>
using remove_deferred_t = typename remove_deferred<T>::type;


} // namespace lager
