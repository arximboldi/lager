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

#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#define LAGER_DERIVE_IMPL_EQ_ITER__(r__, data__, elem__) &&a.elem__ == b.elem__

#define LAGER_DERIVE_IMPL_EQ(r__, ns__, name__, members__)                     \
    namespace ns__ {                                                           \
    inline bool operator==(name__ const& a, name__ const& b)                   \
    {                                                                          \
        return true BOOST_PP_SEQ_FOR_EACH_R(                                   \
            r__, LAGER_DERIVE_IMPL_EQ_ITER__, _, members__);                   \
    }                                                                          \
    inline bool operator!=(name__ const& a, name__ const& b)                   \
    {                                                                          \
        return !(a == b);                                                      \
    }                                                                          \
    }                                                                          \
    //

#define LAGER_DERIVE_TEMPLATE_IMPL_EQ(r__, ns__, tpl__, name__, members__)     \
    namespace ns__ {                                                           \
    template <BOOST_PP_REMOVE_PARENS(tpl__)>                                   \
    inline bool operator==(BOOST_PP_REMOVE_PARENS(name__) const& a,            \
                           BOOST_PP_REMOVE_PARENS(name__) const& b)            \
    {                                                                          \
        return true BOOST_PP_SEQ_FOR_EACH_R(                                   \
            r__, LAGER_DERIVE_IMPL_EQ_ITER__, _, members__);                   \
    }                                                                          \
    template <BOOST_PP_REMOVE_PARENS(tpl__)>                                   \
    inline bool operator!=(BOOST_PP_REMOVE_PARENS(name__) const& a,            \
                           BOOST_PP_REMOVE_PARENS(name__) const& b)            \
    {                                                                          \
        return !(a == b);                                                      \
    }                                                                          \
    }                                                                          \
    //

#define LAGER_DERIVE_NESTED_IMPL_EQ(r__, name__, members__)                    \
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
