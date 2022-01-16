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

#include <lager/extra/enum.hpp>

#include <cereal/cereal.hpp>

namespace cereal {

template <typename Archive, typename T>
std::enable_if_t<lager::enum_meta<T>::value>
CEREAL_LOAD_MINIMAL_FUNCTION_NAME(const Archive& ar, T& x, const std::string& s)
{
    x = lager::to_enum<T>(s);
}

template <typename Archive, typename T>
std::enable_if_t<lager::enum_meta<T>::value, std::string>
CEREAL_SAVE_MINIMAL_FUNCTION_NAME(const Archive& ar, const T& x)
{
    return lager::to_string(x);
}

} // namespace cereal
