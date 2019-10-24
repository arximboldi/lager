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

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace lager {

namespace detail {

template <typename T>
struct const_functor;

template <typename T>
auto make_const_functor(T&& x) -> const_functor<T>
{
    return {std::forward<T>(x)};
}

template <typename T>
struct const_functor
{
    T value;

    template <typename Fn>
    auto operator()(Fn&&) &&
    {
        return std::move(*this);
    }
};

template <typename T>
struct identity_functor;

template <typename T>
auto make_identity_functor(T&& x) -> identity_functor<T>
{
    return {std::forward<T>(x)};
}

template <typename T>
struct identity_functor
{
    T value;

    template <typename Fn>
    auto operator()(Fn&& f) &&
    {
        return make_identity_functor(
            std::forward<Fn>(f)(std::forward<T>(value)));
    }
};

} // namespace detail

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

namespace lens {

template <typename Getter, typename Setter>
auto getset(Getter&& getter, Setter&& setter)
{
    return [=](auto&& f) {
        return [&, f](auto&& p) {
            return f(getter(std::forward<decltype(p)>(p)))([&](auto&& x) {
                return setter(std::forward<decltype(p)>(p),
                              std::forward<decltype(x)>(x));
            });
        };
    };
}

template <typename Member>
auto attr(Member member)
{
    return [=](auto&& f) {
        return [&, f](auto&& p) {
            return f(std::forward<decltype(p)>(p).*member)([&](auto&& x) {
                auto r    = std::forward<decltype(p)>(p);
                r.*member = std::forward<decltype(x)>(x);
                return r;
            });
        };
    };
}

template <typename Key>
auto at(Key key)
{
    return [=](auto&& f) {
        return [f, &key](auto&& p) {
            return f([&] {
                try {
                    return std::forward<decltype(p)>(p).at(key);
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
}

template <typename Key>
auto at_i(Key key)
{
    return [=](auto&& f) {
        return [f, &key](auto&& p) {
            return f([&] {
                try {
                    return std::forward<decltype(p)>(p).at(key);
                } catch (std::out_of_range const&) {
                    return std::decay_t<decltype(p.at(key))>{};
                }
            }())([&](auto&& x) {
                if (static_cast<std::size_t>(key) < p.size()) {
                    return std::forward<decltype(p)>(p).set(
                        key, std::forward<decltype(x)>(x));
                } else {
                    return std::forward<decltype(p)>(p);
                }
            });
        };
    };
}

} // namespace lens

} // namespace lager
