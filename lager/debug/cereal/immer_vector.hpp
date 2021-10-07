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
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>
#include <type_traits>

// This code has mostly been adapted from <cereal/types/vector.hpp>
// We don't deal for now with data that could be potentially serialized
// directly in binary format.

namespace cereal {

template <typename Archive,
          typename T,
          typename MP,
          std::uint32_t B,
          std::uint32_t BL>
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar,
                               const immer::vector<T, MP, B, BL>& vector)
{
    ar(make_size_tag(static_cast<size_type>(vector.size())));
    for (auto&& v : vector)
        ar(v);
}

template <typename Archive,
          typename T,
          typename MP,
          std::uint32_t B,
          std::uint32_t BL>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, immer::vector<T, MP, B, BL>& vector)
{
    size_type size{};
    ar(make_size_tag(size));

    if (!size)
        return;

    auto t = immer::vector<T, MP, B, BL>{}.transient();
    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        t.push_back(std::move(x));
    }
    vector = std::move(t).persistent();

    assert(size == vector.size());
}

template <typename Archive,
          typename T,
          typename MP,
          std::uint32_t B,
          std::uint32_t BL>
void CEREAL_SAVE_FUNCTION_NAME(
    Archive& ar, const immer::vector<immer::box<T, MP>, MP, B, BL>& vector)
{
    ar(make_size_tag(static_cast<size_type>(vector.size())));
    for (auto&& v : vector)
        ar(*v);
}

template <typename Archive,
          typename T,
          typename MP,
          std::uint32_t B,
          std::uint32_t BL>
void CEREAL_LOAD_FUNCTION_NAME(
    Archive& ar, immer::vector<immer::box<T, MP>, MP, B, BL>& vector)
{
    size_type size{};
    ar(make_size_tag(size));

    if (!size)
        return;

    auto t = immer::vector<immer::box<T, MP>, MP, B, BL>{}.transient();
    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        t.push_back(std::move(x));
    }
    vector = std::move(t).persistent();

    assert(size == vector.size());
}

} // namespace cereal
