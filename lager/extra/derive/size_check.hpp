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
#if __has_include(<boost/pfr/tuple_size.hpp>)
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/pfr/tuple_size.hpp>
#define LAGER_DERIVE_IMPL_SIZE_CHECK_PRETTY_NAME(name__) #name__

#define LAGER_DERIVE_IMPL_SIZE_CHECK(r__, ns__, name__, members__)                    \
    namespace ns__ {                                                                  \
    static_assert(boost::pfr::tuple_size_v<name__> == BOOST_PP_SEQ_SIZE(members__),   \
                  "LAGER_STRUCT does not match size of struct "                       \
                  LAGER_DERIVE_IMPL_SIZE_CHECK_PRETTY_NAME(ns__) "::"                 \
                  LAGER_DERIVE_IMPL_SIZE_CHECK_PRETTY_NAME(name__));                  \
    }


#define LAGER_DERIVE_TEMPLATE_IMPL_SIZE_CHECK(r__, ns__, tpl__, name__, members__)       \
    static_assert(false, "can't implement SIZE_CHECK for LAGER_DERIVE_TEMPLATE, sorry!")


#define LAGER_DERIVE_NESTED_IMPL_SIZE_CHECK(r__, name__, members__) \
    static_assert(false, "LAGER_DERIVE_NESTED: TODO: SIZE_CHECK !")

#else

#define LAGER_DERIVE_IMPL_SIZE_CHECK(r__, ns__, name__, members__) 
#define LAGER_DERIVE_TEMPLATE_IMPL_SIZE_CHECK(r__, ns__, tpl__, name__, members__)
#define LAGER_DERIVE_NESTED_IMPL_SIZE_CHECK(r__, name__, members__)

#endif