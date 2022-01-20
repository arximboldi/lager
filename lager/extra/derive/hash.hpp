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

#include <functional>

#define LAGER_DERIVE_IMPL_CEREAL_ITER__(r__, data__, i__, elem__)              \
    ::boost::hash_combine(seed, x.elem__);

#define LAGER_DERIVE_IMPL_HASH(r__, ns__, name__, members__)                   \
    namespace std {                                                            \
    template <>                                                                \
    struct hash<name__>                                                        \
    {                                                                          \
        std::size_t operator()(const name__& x)                                \
        {                                                                      \
            auto seed = std::size_t{};                                         \
            BOOST_PP_SEQ_FOR_EACH_I_R(                                         \
                r__, LAGER_DERIVE_IMPL_HASH_ITER__, _, members__);             \
        }                                                                      \
    };                                                                         \
    //

#define LAGER_DERIVE_NESTED_IMPL_HASH(r__, name__, members__)                  \
    static_assert(false, "can't implement HASH for LAGER_DERIVE_NESTED, sorry!")
