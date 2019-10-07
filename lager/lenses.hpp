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

namespace lager {

namespace detail {

template <typename T>
struct const_functor_t
{
    T value;

    template <typename Fn>
    decltype(auto) operator()(Fn&&)
    {
        return *this;
    }
};

template <typename T>
struct identity_functor_t
{
    T value;

    template <typename Fn>
    auto operator()(Fn&& f)
    {
        return identity_functor_t<decltype(f(value))>{
            std::forward<Fn>(f)(value)};
    }
};

template <typename T>
using decay_rvalue_t =
    std::conditional_t<std::is_rvalue_reference<T>::value, std::decay_t<T>, T>;

} // namespace detail

template <typename LensT, typename T>
decltype(auto) view(LensT&& lens, T&& x)
{
    return lens([](auto&& v) {
               return detail::const_functor_t<
                   detail::decay_rvalue_t<decltype(v)>>{
                   std::forward<decltype(v)>(v)};
           })(std::forward<T>(x))
        .value;
}

template <typename LensT, typename T, typename U>
decltype(auto) set(LensT&& lens, T&& x, U&& v)
{
    return lens([&v](auto&&) { return detail::identity_functor_t<U>{v}; })(
               std::forward<T>(x))
        .value;
}

template <typename LensT, typename T, typename Fn>
decltype(auto) over(LensT&& lens, T&& x, Fn&& fn)
{
    return lens([&fn](auto&& v) {
               auto u = fn(std::forward<decltype(v)>(v));
               return detail::identity_functor_t<decltype(u)>{u};
           })(std::forward<T>(x))
        .value;
}

namespace lens {

template <typename Attr>
auto attr(Attr attr)
{
    return [=](auto&& f) {
        return [f, &attr](auto&& p) {
            return f(p.*attr)([&](auto&& x) {
                auto r  = std::forward<decltype(p)>(p);
                r.*attr = std::forward<decltype(x)>(x);
                return r;
            });
        };
    };
};

template <typename Key>
auto at(Key key)
{
    return [=](auto&& f) {
        return [f, &key](auto&& p) {
            return f([&] {
                try {
                    return p.at(key);
                } catch (std::out_of_range const&) {
                    return std::decay_t<decltype(p.at(key))>{};
                }
            }())([&](auto&& x) {
                auto r = std::forward<decltype(p)>(p);
                try {
                    r.at(key) = std::forward<decltype(x)>(x);
                } catch (std::out_of_range const&) {}
                return r;
            });
        };
    };
};

} // namespace lens

} // namespace lager
