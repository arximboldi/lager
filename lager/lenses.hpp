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

#include <lager/util.hpp>
#include <zug/compose.hpp>

#include <type_traits>
#include <utility>

namespace lager {
namespace detail {

template <typename T>
struct should_move :
    std::integral_constant<bool,
                           !std::is_array_v<T> &&
                               !std::is_lvalue_reference_v<T>> {};

template <typename T>
constexpr bool should_move_v = should_move<T>::value;

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
struct is_const_functor : public std::false_type  {};

template <typename T>
struct is_const_functor<const_functor<T>> : public std::true_type {};

template <typename T>
constexpr bool is_const_functor_v = is_const_functor<T>::value;

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

        if constexpr (!should_move_v<T>) {
            return make_identity_functor(
                std::forward<Fn>(f)(value));
        } else {
            return make_identity_functor(
                std::forward<Fn>(f)(std::move(value)));
        }
    }
};

template <typename Part>
struct identity_functor_skip_first
{
    template <typename T>
    auto operator() (T&&) const & {
        return make_identity_functor(part);
    }

    template <typename T>
    auto operator() (T&&) && {
        if constexpr (!should_move_v<Part>) {
            return make_identity_functor(part);
        } else {
            return make_identity_functor(std::move(part));
        }
    }

    Part part;
};

template <typename T>
auto make_identity_functor_skip_first(T&& x) -> identity_functor_skip_first<T>
{
    return {std::forward<T>(x)};
}

template <typename F, typename Getter, typename Setter>
struct getset_t
{
    template <typename Whole>
    auto operator() (Whole &&w) {
        return functorImpl(*this, LAGER_FWD(w));
    }

    template <typename Whole>
    auto operator() (Whole &&w) const {
        return functorImpl(*this, LAGER_FWD(w));
    }

    F f;
    Getter getter;
    Setter setter;

private:
    /**
     * Deduce proper constness of 'this' via the functorImpl() call
     */
    template <typename Self, typename Whole>
    static auto functorImpl(Self &&self, Whole &&w) {
        if constexpr (is_const_functor_v<decltype(self.f(self.getter(LAGER_FWD(w))))>) {
            /**
             * We don't have a setter here, so it is safe to
             * jus pass `w` as an rvalue.
             *
             * We also know that we are calling the const_functor,
             * and it discards the passed argument, so just pass a
             * noop to it.
             *
             * This branch is taken when viewing through the lens.
             */
            return self.f(self.getter(LAGER_FWD(w))) // pass `w` into the getter as an rvalue!
                (zug::noop);
        } else {
            /**
             * Here we have both, getter and setter, so we pass
             * `w` to getter as an lvalue, and to setter as an rvalue.
             * The setter has a chance to reuse the resources of
             * the passed value.
             *
             * This branch is taken on all the levels of setting the
             * value through except of the tompost level.
             */
            return self.f(self.getter(w)) // pass `w` into the getter as an lvalue!
                ([&](auto&& x) {
                    return self.setter(LAGER_FWD(w), LAGER_FWD(x));
                });
        }
    }
};

/**
 * This specialization is called when a set() method is called over
 * the lens. In such a case we can skip calling the getter branch
 * of the lens.
 *
 * This branch is taken on the topmost level of setting the value
 * through the lens.
 */
template <typename T, typename Getter, typename Setter>
struct getset_t<identity_functor_skip_first<T>, Getter, Setter>
{
    template <typename Part>
    auto operator() (Part &&p) {
        return std::move(f)(zug::noop)
            ([&](auto&& x) {
                return setter(LAGER_FWD(p), LAGER_FWD(x));
            });
    }

    template <typename Part>
    auto operator() (Part &&p) const {
        return f(zug::noop)
            ([&](auto&& x) {
                return setter(LAGER_FWD(p), LAGER_FWD(x));
            });
    }

    identity_functor_skip_first<T> &&f;
    Getter getter;
    Setter setter;
};

template <typename F, typename Getter, typename Setter>
auto make_getset_t(F &&f, const Getter &getter, const Setter &setter)
{
    return getset_t<F, Getter, Setter>{std::forward<F>(f), getter, setter};
}

} // namespace detail

//! @defgroup lenses-api
//! @{

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
    return lens(detail::make_identity_functor_skip_first(std::forward<decltype(v)>(v))) (
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

//! @}

namespace lenses {

//! @defgroup lenses
//! @{

template <typename Getter, typename Setter>
auto getset(const Getter& getter, const Setter& setter)
{
    return zug::comp([=](auto&& f) {
        return detail::make_getset_t(LAGER_FWD(f), getter, setter);
    });
}

} // namespace lenses

//! @}

} // namespace lager
