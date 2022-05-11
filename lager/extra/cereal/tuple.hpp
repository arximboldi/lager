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

#include <cereal/cereal.hpp>
#include <immer/box.hpp>
#include <type_traits>

namespace cereal {
namespace detail {

template <size_t Height>
struct serialize
{
    template <class Archive, class... Types>
    inline static void apply(Archive& ar, std::tuple<Types...>& tuple)
    {
        serialize<Height - 1>::template apply(ar, tuple);
        ar(std::get<Height - 1>(tuple));
    }
};

template <>
struct serialize<0>
{
    template <class Archive, class... Types>
    inline static void apply(Archive&, std::tuple<Types...>&)
    {}
};

} // namespace detail

template <class Archive, class... Types>
inline void CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar,
                                           std::tuple<Types...>& tuple)
{
    auto ts = static_cast<size_type>(sizeof...(Types));
    ar(make_size_tag(ts));
    assert(ts == sizeof...(Types));
    detail::serialize<sizeof...(Types)>::template apply(ar, tuple);
}

} // namespace cereal
