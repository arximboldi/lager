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
#include <immer/array.hpp>
#include <immer/array_transient.hpp>
#include <type_traits>

// This code has mostly been adapted from <cereal/types/vector.hpp>
// We don't deal for now with data that could be potentially serialized
// directly in binary format.

namespace cereal {

template <typename Archive, typename T, typename MP>
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, const immer::array<T, MP>& array)
{
    ar(make_size_tag(static_cast<size_type>(array.size())));
    for (auto&& v : array)
        ar(v);
}

template <typename Archive, typename T, typename MP>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, immer::array<T, MP>& array)
{
    size_type size{};
    ar(make_size_tag(size));

    if (!size)
        return;

    auto t = immer::array<T, MP>{}.transient();
    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        t.push_back(std::move(x));
    }
    array = std::move(t).persistent();

    assert(size == array.size());
}

} // namespace cereal
