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

#include <lager/lenses.hpp>
#include <lager/lenses/at.hpp>
#include <lager/lenses/attr.hpp>
#include <lager/lenses/optional.hpp>

#include <zug/meta/detected.hpp>
#include <type_traits>

namespace lager {
namespace detail {

template <typename T, typename Key>
using at_t = std::decay_t<decltype(std::declval<T>().at(std::declval<Key>()))>;

template <typename T>
struct smart_lens_base
{
    template <typename Lens, std::enable_if_t<!zug::meta::is_detected<at_t, T, Lens>::value, int> = 0>
    static auto make(Lens l)
    {
        return l;
    }

    template <typename Key, std::enable_if_t<zug::meta::is_detected<at_t, T, Key>::value, int> = 0>
    static auto make(Key k)
    {
        return lenses::at(std::move(k));
    }

    template <typename U, typename V>
    static auto make(U V::*member)
    {
        return lenses::attr(member);
    }
};

/*!
 * Try to find a nice lens type for the give type.
 */
template <typename T>
struct smart_lens : smart_lens_base<T>
{};

struct functor_wrapper_t {
    template <typename T>
    const_functor<T> operator()(T&& t) const noexcept;
};

template <typename Lens, typename T>
using viewed_t = decltype(
    std::declval<Lens>()(functor_wrapper_t{})(std::declval<T>()).value);

template <typename T>
struct smart_lens<std::optional<T>>
{
    template <typename U>
    static auto make(U &&u) {
        // don't lift lenses that can already handle optionals
        if constexpr (zug::meta::is_detected<viewed_t, U, std::optional<T>>::value) {
            return std::forward<U>(u);
        } else {
            return ::lager::lenses::with_opt(smart_lens<T>::make(std::forward<U>(u)));
        }
    }
};

} // namespace detail
} // namespace lager
