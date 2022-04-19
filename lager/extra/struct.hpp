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

#include <lager/extra/derive.hpp>
#include <lager/extra/derive/eq.hpp>
#include <lager/extra/derive/hana.hpp>
#include <lager/extra/derive/size_check.hpp>

/*!
 * This macro declares the struct as a Boost.Hana sequence, so it can be
 * introspected via metaprogramming.  This macro has similar syntax to
 * BOOST_HANA_ADAPT_STRUCT.
 */
#define LAGER_STRUCT(...) LAGER_DERIVE((EQ, HANA, SIZE_CHECK), __VA_ARGS__)

/*!
 * Like LAGER_STRUCT but for templates.
 */
#define LAGER_STRUCT_TEMPLATE(...)                                             \
    LAGER_DERIVE_TEMPLATE((EQ, HANA), __VA_ARGS__)

/*!
 * Like LAGER_STRUCT but for it is used nested in the struct itself.  It
 * seamlessly supports combinations of templates and nested types that are not
 * supported by the above macros.
 */
#define LAGER_STRUCT_NESTED(...) LAGER_DERIVE_NESTED((EQ, HANA), __VA_ARGS__)
