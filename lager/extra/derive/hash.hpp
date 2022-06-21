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

#include <boost/functional/hash.hpp>

#include <functional>

// use formula from boost::hash_combine as in:
// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
#define LAGER_DERIVE_IMPL_HASH_ITER__(r__, data__, elem__)                     \
    seed ^= std::hash<decltype(x.elem__)>{}(x.elem__) + 0x9e3779b9 +           \
            (seed << 6) + (seed >> 2);

#define LAGER_DERIVE_IMPL_HASH(r__, ns__, name__, members__)                   \
    namespace std {                                                            \
    template <>                                                                \
    struct hash<ns__::name__>                                                  \
    {                                                                          \
        std::size_t operator()(const ns__::name__& x) const                    \
        {                                                                      \
            auto seed = std::size_t{};                                         \
            BOOST_PP_SEQ_FOR_EACH_R(                                           \
                r__, LAGER_DERIVE_IMPL_HASH_ITER__, _, members__)              \
            return seed;                                                       \
        }                                                                      \
    };                                                                         \
    }                                                                          \
    //

#define LAGER_DERIVE_TEMPLATE_IMPL_HASH(r__, ns__, tpl__, name__, members__)   \
    namespace std {                                                            \
    template <BOOST_PP_REMOVE_PARENS(tpl__)>                                   \
    struct hash<ns__::BOOST_PP_REMOVE_PARENS(name__)>                          \
    {                                                                          \
        std::size_t operator()(const ns__::BOOST_PP_REMOVE_PARENS(name__) &    \
                               x) const                                        \
        {                                                                      \
            auto seed = typeid(x).hash_code();                                 \
            BOOST_PP_SEQ_FOR_EACH_R(                                           \
                r__, LAGER_DERIVE_IMPL_HASH_ITER__, _, members__)              \
            return seed;                                                       \
        }                                                                      \
    };                                                                         \
    }                                                                          \
    //

#define LAGER_DERIVE_NESTED_IMPL_HASH(r__, name__, members__)                  \
    static_assert(false, "can't implement HASH for LAGER_DERIVE_NESTED, sorry!")
