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

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/expand.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>

#define LAGER_DERIVE_IMPL_EQ_ITER__(r__, data__, i__, elem__)                  \
    BOOST_PP_EXPR_IF(i__, &&)                                                  \
    a.elem__ == b.elem__

#define LAGER_DERIVE_IMPL_EQ(r__, ns__, name__, members__)                     \
    namespace ns__ {                                                           \
    inline bool operator==(name__ const& a, name__ const& b)                   \
    {                                                                          \
        return BOOST_PP_SEQ_FOR_EACH_I_R(                                      \
            r__, LAGER_DERIVE_IMPL_EQ_ITER__, _, members__);                   \
    }                                                                          \
    inline bool operator!=(name__ const& a, name__ const& b)                   \
    {                                                                          \
        return !(a == b);                                                      \
    }                                                                          \
    }

#define LAGER_DERIVE_ITER__(r__, data__, elem__)                               \
    BOOST_PP_EXPAND(BOOST_PP_CAT(LAGER_DERIVE_IMPL_, elem__)(                  \
        r__,                                                                   \
        BOOST_PP_TUPLE_ELEM(0, data__),                                        \
        BOOST_PP_TUPLE_ELEM(1, data__),                                        \
        BOOST_PP_EXPAND(BOOST_PP_TUPLE_TO_SEQ(                                 \
            BOOST_PP_TUPLE_POP_FRONT(BOOST_PP_TUPLE_POP_FRONT(data__))))))

#define LAGER_DERIVE(impls__, ...)                                             \
    BOOST_PP_LIST_FOR_EACH(                                                    \
        LAGER_DERIVE_ITER__, (__VA_ARGS__), BOOST_PP_TUPLE_TO_LIST(impls__));  \
    static_assert("force semicolon")
