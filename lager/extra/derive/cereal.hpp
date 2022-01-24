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

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/stringize.hpp>

#define LAGER_DERIVE_IMPL_CEREAL_ITER__(r__, data__, i__, elem__)              \
    BOOST_PP_COMMA_IF(i__)                                                     \
    cereal::make_nvp(BOOST_PP_STRINGIZE(elem__), x.elem__)

#define LAGER_DERIVE_IMPL_CEREAL_MEMBERS_NON_EMPTY__(r__, members__)           \
    ar(BOOST_PP_SEQ_FOR_EACH_I_R(                                              \
        r__, LAGER_DERIVE_IMPL_CEREAL_ITER__, _, members__));

#define LAGER_DERIVE_IMPL_CEREAL_MEMBERS_EMPTY__(r__, members__)

#define LAGER_DERIVE_IMPL_CEREAL_MEMBERS__(r__, members__)                     \
    BOOST_PP_IF(BOOST_PP_SEQ_SIZE(members__),                                  \
                LAGER_DERIVE_IMPL_CEREAL_MEMBERS_NON_EMPTY__,                  \
                LAGER_DERIVE_IMPL_CEREAL_MEMBERS_EMPTY__)                      \
    (r__, members__)

#define LAGER_DERIVE_IMPL_CEREAL(r__, ns__, name__, members__)                 \
    namespace ns__ {                                                           \
    template <typename Archive>                                                \
    void serialize(Archive& ar, name__& x)                                     \
    {                                                                          \
        LAGER_DERIVE_IMPL_CEREAL_MEMBERS__(r__, members__)                     \
    }                                                                          \
    }                                                                          \
    //

#define LAGER_DERIVE_TEMPLATE_IMPL_CEREAL(r__, ns__, tpl__, name__, members__) \
    namespace ns__ {                                                           \
    template <typename Archive, BOOST_PP_REMOVE_PARENS(tpl__)>                 \
    void serialize(Archive& ar, BOOST_PP_REMOVE_PARENS(name__) & x)            \
    {                                                                          \
        LAGER_DERIVE_IMPL_CEREAL_MEMBERS__(r__, members__)                     \
    }                                                                          \
    }                                                                          \
    //

#define LAGER_DERIVE_NESTED_IMPL_CEREAL(r__, name__, members__)                \
    template <typename Archive>                                                \
    friend void serialize(Archive& ar, name__& x)                              \
    {                                                                          \
        LAGER_DERIVE_IMPL_CEREAL_MEMBERS__(r__, members__)                     \
    }                                                                          \
    //
