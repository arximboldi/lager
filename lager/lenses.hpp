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
#include <zug/meta/util.hpp>
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
        return [&, f = LAGER_FWD(f)](auto&& p) {
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
        return [&, f = LAGER_FWD(f)](auto&& p) {
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
        return [f = LAGER_FWD(f), &key](auto&& whole) {
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
        return [f = LAGER_FWD(f), &key](auto&& whole) {
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
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            return f(LAGER_FWD(whole).value_or(std::move(t)))(
                [&](auto&& x) { return LAGER_FWD(x); });
        };
    });
}

/*!
 * `() -> Lens<[X], X>`
 */
auto fallback() {
    return zug::comp([](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            using T = std::decay_t<decltype(whole.value())>;
            return f(LAGER_FWD(whole).value_or(T{}))(
                [&](auto&& x) { return LAGER_FWD(x); });
        };
    });
}

namespace detail {

template <
    template <typename>
    typename PartMeta,
    typename Whole,
    typename Lens,
    typename Functor>
decltype(auto) opt_impl(Whole&& whole, Lens&& lens, Functor&& f) {
    using Part = typename PartMeta<std::decay_t<decltype(::lager::view(
        std::forward<Lens>(lens),
        std::declval<std::decay_t<decltype(whole.value())>>()))>>::type;

    if (whole.has_value()) {
        return std::invoke(
            std::forward<Functor>(f),
            Part{::lager::view(
                std::forward<Lens>(lens), std::forward<Whole>(whole).value())})(
            [&](Part part) {
                if (part.has_value()) {
                    return std::decay_t<Whole>{::lager::set(
                        std::forward<Lens>(lens),
                        std::forward<Whole>(whole).value(),
                        std::move(part).value())};
                } else {
                    return std::forward<Whole>(whole);
                }
            });
    } else {
        return std::invoke(std::forward<Functor>(f), Part{std::nullopt})(
            [&](auto&&) { return std::forward<Whole>(whole); });
    }
}

template <typename T> struct remove_opt { using type = T; };
template <typename T> struct remove_opt<std::optional<T>> { using type = T; };
template <typename T> using remove_opt_t = typename remove_opt<T>::type;

template <typename T> struct to_opt {
    using type = std::optional<remove_opt_t<std::decay_t<T>>>;
};

template <typename T> struct add_opt {
    using type = std::optional<std::decay_t<T>>;
};

} // namespace detail

/*!
 * `Lens<W, P> -> Lens<[W], [P]>`
 */
template <typename Lens>
auto optmap(Lens&& lens) {
    return zug::comp([lens = std::forward<Lens>(lens)](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            return detail::opt_impl<detail::add_opt>(LAGER_FWD(whole), lens, f);
        };
    });
}

/*!
 * `Lens<W, [P]> -> Lens<[W], [P]>`
 */
template <typename Lens>
auto optbind(Lens&& lens) {
    return zug::comp([lens = std::forward<Lens>(lens)](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            return detail::opt_impl<zug::meta::identity>(LAGER_FWD(whole), lens, f);
        };
    });
}

/*!
 * `(Lens<W, P> | Lens<W, [P]>) -> Lens<[W], [P]>`
 */
template <typename Lens>
auto optlift(Lens&& lens) {
    return zug::comp([lens = std::forward<Lens>(lens)](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            return detail::opt_impl<detail::to_opt>(LAGER_FWD(whole), lens, f);
        };
    });
}


} // namespace lens

} // namespace lager
