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

#include <lager/config.hpp>

#include <zug/util.hpp>

#include <functional>

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

#define LAGER_FWD(name_) std::forward<decltype(name_)>(name_)

/*!
 * Unwraps multiple layers of state wrappers added by store enhancers.
 */
template <typename T>
const T& unwrap(const T& x)
{
    return x;
}

//! @} group: util

inline const char* resources_path()
{
    auto env_resources_path = std::getenv("LAGER_RESOURCES_PATH");
    return env_resources_path ? env_resources_path
                              : LAGER_PREFIX_PATH "/share/lager";
}

} // namespace lager
