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
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/rest_n.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>

#define LAGER_DERIVE_IMPL_EQ_ITER__(r__, data__, elem__) &&a.elem__ == b.elem__

#define LAGER_DERIVE_IMPL_EQ(r__, tpl__, ns__, name__, members__)              \
    namespace ns__ {                                                           \
    BOOST_PP_REMOVE_PARENS(tpl__)                                              \
    inline bool operator==(BOOST_PP_REMOVE_PARENS(name__) const& a,            \
                           BOOST_PP_REMOVE_PARENS(name__) const& b)            \
    {                                                                          \
        return true BOOST_PP_SEQ_FOR_EACH_R(                                   \
            r__, LAGER_DERIVE_IMPL_EQ_ITER__, _, members__);                   \
    }                                                                          \
    BOOST_PP_REMOVE_PARENS(tpl__)                                              \
    inline bool operator!=(BOOST_PP_REMOVE_PARENS(name__) const& a,            \
                           BOOST_PP_REMOVE_PARENS(name__) const& b)            \
    {                                                                          \
        return !(a == b);                                                      \
    }                                                                          \
    }                                                                          \
    //

#define LAGER_DERIVE_IMPL_NESTED_EQ(r__, name__, members__)                    \
    friend bool operator==(name__ const& a, name__ const& b)                   \
    {                                                                          \
        return true BOOST_PP_SEQ_FOR_EACH_R(                                   \
            r__, LAGER_DERIVE_IMPL_EQ_ITER__, _, members__);                   \
    }                                                                          \
    friend bool operator!=(name__ const& a, name__ const& b)                   \
    {                                                                          \
        return !(a == b);                                                      \
    }                                                                          \
    //

#define LAGER_DERIVE_ITER__(r__, data__, elem__)                               \
    BOOST_PP_CAT(LAGER_DERIVE_IMPL_, elem__)                                   \
    (r__,                                                                      \
     (),                                                                       \
     BOOST_PP_SEQ_ELEM(0, data__),                                             \
     BOOST_PP_SEQ_ELEM(1, data__),                                             \
     BOOST_PP_SEQ_REST_N(2, data__))

#define LAGER_DERIVE(impls__, ...)                                             \
    BOOST_PP_LIST_FOR_EACH(                                                    \
        LAGER_DERIVE_ITER__,                                                   \
        BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)),                                  \
        BOOST_PP_TUPLE_TO_LIST((BOOST_PP_REMOVE_PARENS(impls__))))             \
    static_assert("force semicolon")

#define LAGER_DERIVE_TEMPLATE_ITER__(r__, data__, elem__)                      \
    BOOST_PP_CAT(LAGER_DERIVE_IMPL_, elem__)                                   \
    (r__,                                                                      \
     (template <BOOST_PP_REMOVE_PARENS(BOOST_PP_SEQ_ELEM(1, data__))>),        \
     BOOST_PP_SEQ_ELEM(0, data__),                                             \
     BOOST_PP_SEQ_ELEM(2, data__),                                             \
     BOOST_PP_SEQ_REST_N(3, data__))

#define LAGER_DERIVE_TEMPLATE(impls__, ...)                                    \
    BOOST_PP_LIST_FOR_EACH(                                                    \
        LAGER_DERIVE_TEMPLATE_ITER__,                                          \
        BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)),                                  \
        BOOST_PP_TUPLE_TO_LIST((BOOST_PP_REMOVE_PARENS(impls__))));            \
    static_assert("force semicolon")

#define LAGER_DERIVE_NESTED_ITER__(r__, data__, elem__)                        \
    BOOST_PP_CAT(LAGER_DERIVE_IMPL_NESTED_, elem__)                            \
    (r__, BOOST_PP_SEQ_ELEM(0, data__), BOOST_PP_SEQ_REST_N(1, data__))

#define LAGER_DERIVE_NESTED(impls__, ...)                                      \
    BOOST_PP_LIST_FOR_EACH(                                                    \
        LAGER_DERIVE_NESTED_ITER__,                                            \
        BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)),                                  \
        BOOST_PP_TUPLE_TO_LIST((BOOST_PP_REMOVE_PARENS(impls__))))             \
    static_assert("force semicolon")
