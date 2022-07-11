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

#include <zug/meta/util.hpp>

#include <exception>

namespace lager {

/*!
 * Raised by the view when it produces no value yet. This can happen
 * when the reducing function that it uses is filtering some values.
 */
struct no_value_error : std::exception
{
    const char* what() const noexcept override { return "no_value_error"; };
};

namespace detail {

struct no_value
{
    template <typename T>
    operator T() const
    {
        LAGER_THROW(no_value_error{});
    }
};

} // namespace detail

} // namespace lager
