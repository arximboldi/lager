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

#include <lager/config.hpp>

#include <cereal/cereal.hpp>

#include <immer/table.hpp>
#include <immer/table_transient.hpp>
#include <type_traits>

namespace cereal {
// This code has mostly been adapted from <cereal/types/vector.hpp>
// We don't deal for now with data that could be potentially serialized
// directly in binary format.

template <typename Archive,
          typename T,
          typename K,
          typename H,
          typename E,
          typename MP,
          std::uint32_t B>
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar,
                               const immer::table<T, K, H, E, MP, B>& table)
{
    ar(make_size_tag(static_cast<size_type>(table.size())));
    for (auto&& v : table)
        ar(v);
}

template <typename Archive,
          typename T,
          typename K,
          typename H,
          typename E,
          typename MP,
          std::uint32_t B>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar,
                               immer::table<T, K, H, E, MP, B>& table)
{
    size_type size{};
    ar(make_size_tag(size));

    if (!size)
        return;

    auto t = immer::table<T, K, H, E, MP, B>{}.transient();

    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        t.insert(std::move(x));
    }
    table = std::move(t).persistent();

    assert(size == table.size());
}

} // namespace cereal
