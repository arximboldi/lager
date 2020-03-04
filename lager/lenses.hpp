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

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <optional>

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
    const_functor operator()(Fn&&) &&
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
    return zug::comp([=](auto&& f) {
        return [&, f](auto&& p) {
            return f(getter(std::forward<decltype(p)>(p)))([&](auto&& x) {
                return setter(std::forward<decltype(p)>(p),
                              std::forward<decltype(x)>(x));
            });
        };
    });
}

/*!
 * `(Part Whole::*) -> Lens<Whole, Part>`
 */
template <typename Member>
auto attr(Member member)
{
    return zug::comp([member](auto&& f) {
        return [&, f](auto&& p) {
            return f(std::forward<decltype(p)>(p).*member)([&](auto&& x) {
                auto r    = std::forward<decltype(p)>(p);
                r.*member = std::forward<decltype(x)>(x);
                return r;
            });
        };
    });
}

/*!
 * `Key -> Lens<{X}, [X]>`
 */
template <typename Key>
auto at(Key key) {
    return zug::comp([key](auto&& f) {
        return [f, &key](auto&& whole) {
            using Part = std::optional<std::decay_t<decltype(whole.at(key))>>;

            return f([&]() -> Part {
                try {
                    return std::forward<decltype(whole)>(whole).at(key);
                } catch (std::out_of_range const&) { return std::nullopt; }
            }())([&](Part part) {
                auto r = std::forward<decltype(whole)>(whole);
                if (part.has_value()) {
                    try {
                        r.at(key) = std::move(part).value();
                    } catch (std::out_of_range const&) {}
                }
                return r;
            });
        };
    });
}

/*!
 * `Key -> Lens<{X}, [X]>`
 */
template <typename Key>
auto at_i(Key key) {
    return zug::comp([key](auto&& f) {
        return [f, &key](auto&& whole) {
            using Part = std::optional<std::decay_t<decltype(whole.at(key))>>;

            return f([&]() -> Part {
                try {
                    return std::forward<decltype(whole)>(whole).at(key);
                } catch (std::out_of_range const&) { return std::nullopt; }
            }())([&](Part part) {
                if (part.has_value() &&
                    static_cast<std::size_t>(key) < whole.size()) {
                    return std::forward<decltype(whole)>(whole).set(
                        key, std::move(part).value());
                } else {
                    return std::forward<decltype(whole)>(whole);
                }
            });
        };
    });
}

/*!
 * `X -> Lens<[X], X>`
 */
template <typename T>
auto fallback(T&& t) {
    return zug::comp([t = std::forward<T>(t)](auto&& f) {
        return [&, f](auto&& whole) {
            return f(LAGER_FWD(whole).value_or(std::move(t)))(
                [&](auto&& x) { return LAGER_FWD(x); });
        };
    });
}

/*!
 * `(Lens<W, P> | Lens<W, [P]>) -> Lens<[W], [P]>`
 */
template <typename Lens>
auto optlift(Lens&& lens) {
    return zug::comp([lens = std::forward<Lens>(lens)](auto&& f) {
        return [&, f](auto&& whole) {
            using Whole = std::decay_t<decltype(whole)>;
            using Part  = std::optional<std::decay_t<decltype(::lager::view(
                lens, std::declval<std::decay_t<decltype(whole.value())>>()))>>;

            if (whole.has_value()) {
                auto&& whole_val = LAGER_FWD(whole).value();
                return f(Part{::lager::view(lens, LAGER_FWD(whole_val))})(
                    [&](Part part) {
                        if (part.has_value()) {
                            return Whole{::lager::set(lens,
                                                      LAGER_FWD(whole_val),
                                                      std::move(part).value())};
                        } else {
                            return LAGER_FWD(whole);
                        }
                    });
            } else {
                return f(Part{std::nullopt})(
                    [&](auto&&) { return LAGER_FWD(whole); });
            }
        };
    });
}


} // namespace lens

} // namespace lager
