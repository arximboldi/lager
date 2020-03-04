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
#include <zug/meta/detected.hpp>
#include <type_traits>

namespace immer {

template <typename T, typename MemoryPolicy, std::uint32_t B, std::uint32_t BL>
class vector;

template <typename T, typename MemoryPolicy, std::uint32_t B, std::uint32_t BL>
class flex_vector;

} // namespace immer

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
        return lens::at(std::move(k));
    }

    template <typename U, typename V>
    static auto make(U V::*member)
    {
        return lens::attr(member);
    }
};

/*!
 * Try to find a nice lens type for the give type.
 */
template <typename T>
struct smart_lens : smart_lens_base<T>
{};

template <typename T, typename MP, std::uint32_t B, std::uint32_t BL>
struct smart_lens<immer::vector<T, MP, B, BL>>
{
    static auto make(std::size_t idx) { return lens::at_i(idx); }
};

template <typename T, typename MP, std::uint32_t B, std::uint32_t BL>
struct smart_lens<immer::flex_vector<T, MP, B, BL>>
    : smart_lens<immer::vector<T, MP, B, BL>>
{};

} // namespace detail
} // namespace lager
