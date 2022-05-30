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

#include <zug/detail/copy_traits.hpp>
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
visitor(Ts...) -> visitor<Ts...>;

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
matcher(T) -> matcher<T>;

template <typename... Ts>
decltype(auto) as_variant(std::variant<Ts...> const& x)
{
    return x;
}

} // namespace detail

//! @defgroup util
//! @{

/*!
 * Metafunction that returns the variant type that @a T is convertible to.
 */
template <typename T>
using get_variant_t =
    std::decay_t<decltype(detail::as_variant(std::declval<T>()))>;

/*!
 * Metafunction that returns whether @a T is `std::variant` or is convertible to
 * `std::variant`.
 */
template <typename T, typename Enable = void>
struct is_variant : std::false_type
{};

template <typename T>
struct is_variant<T, decltype((void) detail::as_variant(std::declval<T>()))>
    : std::true_type
{};

//! Alias for @a is_variant
template <typename T>
constexpr bool is_variant_v = is_variant<T>::value;

/*!
 * This forwards @a v so that if @a v is convertible to `std::variant`, the
 * value is forwarded as a variant.
 */
template <typename T>
auto forward_variant(typename std::remove_reference<T>::type& v) noexcept
    -> std::enable_if_t<is_variant_v<T>,
                        zug::detail::copy_decay_t<T, get_variant_t<T>>&&>
{
    return std::forward<T>(v);
}

template <typename T>
auto forward_variant(typename std::remove_reference<T>::type& v) noexcept
    -> std::enable_if_t<!is_variant_v<T>, T&&>
{
    static_assert(is_variant<T>::value,
                  "you must pass a variant to forward_variant()");
    return std::forward<T>(v);
}

/*!
 * Provides variant visitation in a syntactically more elegant way, as in:
 * @code
 *   auto v = std::variant<int, float, string>{...};
 *   return match(v)(
       [](int x)    { ... },
       [](float x)  { ... },
       [](string x) { ... },
 *   );
 * @endcode
 *
 * This also fixes also fixes visitation of variants when inheriting from the
 * variant, which is broken in GNU libstdc++.
 * @code
 *   using variant_t = std::variant{...};
 *   struct inherited_t : variant_t { using variant_t::variant_t; };
 *
 *   auto v = inhertited_t{};
 *   match(v)(...); // this works!
 * @endcode
 */
template <typename... Variants>
auto match(Variants&&... vs)
{
    return detail::matcher{
        std::forward_as_tuple(forward_variant<Variants>(vs)...)};
}

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

} // namespace lager
