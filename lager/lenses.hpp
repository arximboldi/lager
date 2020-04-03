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

#include <zug/compose.hpp>
#include <lager/util.hpp>
#include <lager/detail/functors.hpp>

#include <type_traits>
#include <utility>

namespace lager {

template <typename LensT, typename T>
decltype(auto) view(LensT&& lens, T&& x)
{
    return lens([](auto&& v) {
               return detail::make_const_functor(std::forward<decltype(v)>(v));
           })(std::forward<T>(x))
        .value;
}

template <typename LensT, typename T, typename U>
decltype(auto) set(LensT&& lens, T&& x, U&& v)
{
    return lens([&v](auto&&) { return detail::make_identity_functor(v); })(
               std::forward<T>(x))
        .value;
}

template <typename LensT, typename T, typename Fn>
decltype(auto) over(LensT&& lens, T&& x, Fn&& fn)
{
    return lens([&fn](auto&& v) {
               auto u = fn(std::forward<decltype(v)>(v));
               return detail::make_identity_functor(std::move(u));
           })(std::forward<T>(x))
        .value;
}

namespace lenses {

template <typename Getter, typename Setter>
auto getset(Getter&& getter, Setter&& setter)
{
    return zug::comp([=](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& p) {
            return f(getter(std::forward<decltype(p)>(p)))([&](auto&& x) {
                return setter(std::forward<decltype(p)>(p),
                              std::forward<decltype(x)>(x));
            });
        };
    });
}

} // namespace lenses

} // namespace lager
