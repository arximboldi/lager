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

#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/size.hpp>

#include <cereal/cereal.hpp>

#define LAGER_CEREAL_STRUCT_ITER__(r__, data__, i__, elem__)                   \
    BOOST_PP_COMMA_IF(i__)                                                     \
    cereal::make_nvp(BOOST_PP_STRINGIZE(elem__), x.elem__)

#define LAGER_CEREAL_STRUCT_AUX__(seq__)                                       \
    ar(BOOST_PP_SEQ_FOR_EACH_I(LAGER_CEREAL_STRUCT_ITER__, _, seq__));

#define LAGER_CEREAL_STRUCT(name__, ...)                                       \
    template <typename Archive>                                                \
    void serialize(Archive& ar, name__& x)                                     \
    {                                                                          \
        BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE((, ##__VA_ARGS__)), 1), \
                    BOOST_PP_EXPAND,                                           \
                    LAGER_CEREAL_STRUCT_AUX__)                                 \
        (__VA_ARGS__)                                                          \
    }                                                                          \
    /**/

#define LAGER_CEREAL_NESTED_STRUCT(name__, ...)                                \
    template <typename Archive>                                                \
    friend void serialize(Archive& ar, name__& x)                              \
    {                                                                          \
        BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE((, ##__VA_ARGS__)), 1), \
                    BOOST_PP_EXPAND,                                           \
                    LAGER_CEREAL_STRUCT_AUX__)                                 \
        (__VA_ARGS__)                                                          \
    }                                                                          \
    /**/
