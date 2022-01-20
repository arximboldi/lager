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

template <typename Archive, typename T, typename MP>
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, const immer::box<T, MP>& b)
{
    ar(cereal::make_nvp("value", b.get()));
}

template <typename Archive, typename T, typename MP>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, immer::box<T, MP>& b)
{
    T x;
    ar(cereal::make_nvp("value", x));
    b = x;
}

} // namespace cereal
