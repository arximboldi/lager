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

#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>

#include <boost/hana/adapt_struct.hpp>

/*
 *
 * LAGER_STRUCT
 * ============
 *
 */

#define LAGER_STRUCT_EQ_ITER__(r__, data__, i__, elem__)                       \
    BOOST_PP_EXPR_IF(i__, &&)                                                  \
    a.elem__ == b.elem__

#define LAGER_STRUCT_NON_EMPTY_(ns__, name__, ...)                             \
    BOOST_HANA_ADAPT_STRUCT(ns__::name__, __VA_ARGS__);                        \
    namespace ns__ {                                                           \
    inline bool operator==(name__ const& a, name__ const& b)                   \
    {                                                                          \
        return BOOST_PP_SEQ_FOR_EACH_I(                                        \
            LAGER_STRUCT_EQ_ITER__, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)));  \
    }                                                                          \
    inline bool operator!=(name__ const& a, name__ const& b)                   \
    {                                                                          \
        return !(a == b);                                                      \
    }                                                                          \
    }                                                                          \
    static_assert(true, "must use semicolon")

#define LAGER_STRUCT_EMPTY_(ns__, name__)                                      \
    BOOST_HANA_ADAPT_STRUCT(ns__::name__);                                     \
    namespace ns__ {                                                           \
    inline bool operator==(name__ const& a, name__ const& b) { return true; }  \
    inline bool operator!=(name__ const& a, name__ const& b) { return false; } \
    }                                                                          \
    static_assert(true, "must use semicolon")

/*!
 * This macro declares the struct as a Boost.Hana sequence, so it can be
 * introspected via metaprogramming.  This macro has similar syntax to
 * BOOST_HANA_ADAPT_STRUCT.
 */
#define LAGER_STRUCT(ns__, ...)                                                \
    BOOST_PP_IF(BOOST_PP_DEC(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)),             \
                LAGER_STRUCT_NON_EMPTY_,                                       \
                LAGER_STRUCT_EMPTY_)                                           \
    (ns__, __VA_ARGS__)

/*
 *
 * LAGER_STRUCT_TEMPLATE
 * =====================
 *
 */

#define LAGER_HANA_DEFINE_ACCESSOR_NAMES_ITER__(r__, data__, i__, elem__)      \
    BOOST_PP_COMMA_IF(i__)                                                     \
    BOOST_HANA_PP_STRINGIZE(elem__)

#define LAGER_HANA_DEFINE_ACCESSOR_PAIRS_ITER__(r__, data__, i__, elem__)      \
    BOOST_PP_COMMA_IF(i__)                                                     \
    ::boost::hana::make_pair(                                                  \
        ::boost::hana::struct_detail::prepare_member_name<i__,                 \
                                                          member_names>(),     \
        ::boost::hana::struct_detail::member_ptr<decltype(&type_t::elem__),    \
                                                 &type_t::elem__>{})

#define LAGER_HANA_DEFINE_ACCESORS_IMPL_NON_EMPTY_(t__, ...)                   \
    using type_t = BOOST_PP_REMOVE_PARENS(t__);                                \
    static constexpr auto apply()                                              \
    {                                                                          \
        struct member_names                                                    \
        {                                                                      \
            static constexpr auto get()                                        \
            {                                                                  \
                return ::boost::hana::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(      \
                    LAGER_HANA_DEFINE_ACCESSOR_NAMES_ITER__,                   \
                    _,                                                         \
                    BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__))));                    \
            }                                                                  \
        };                                                                     \
        return ::boost::hana::make_tuple(                                      \
            BOOST_PP_SEQ_FOR_EACH_I(LAGER_HANA_DEFINE_ACCESSOR_PAIRS_ITER__,   \
                                    _,                                         \
                                    BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__))));    \
    }

#define LAGER_HANA_DEFINE_ACCESORS_IMPL_EMPTY_(t__, ...)                       \
    using type_t = BOOST_PP_REMOVE_PARENS(t__);                                \
    static constexpr auto apply() { return boost::hana::make_tuple(); }

#define LAGER_HANA_DEFINE_ACCESORS_IMPL_(...)                                  \
    BOOST_PP_IF(BOOST_PP_DEC(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)),             \
                LAGER_HANA_DEFINE_ACCESORS_IMPL_NON_EMPTY_,                    \
                LAGER_HANA_DEFINE_ACCESORS_IMPL_EMPTY_)                        \
    (__VA_ARGS__)

#define LAGER_HANA_ADAPT_STRUCT_TPL(ts__, t__, ...)                            \
    namespace boost {                                                          \
    namespace hana {                                                           \
    template <BOOST_PP_REMOVE_PARENS(ts__)>                                    \
    struct accessors_impl<BOOST_PP_REMOVE_PARENS(t__)>                         \
    {                                                                          \
        LAGER_HANA_DEFINE_ACCESORS_IMPL_(t__, __VA_ARGS__)                     \
    };                                                                         \
    }                                                                          \
    }                                                                          \
    static_assert(true, "force semicolon");

#define LAGER_STRUCT_TEMPLATE_NON_EMPTY_(ns__, ts__, name__, ...)              \
    LAGER_HANA_ADAPT_STRUCT_TPL(                                               \
        ts__, (ns__::BOOST_PP_REMOVE_PARENS(name__)), __VA_ARGS__);            \
    namespace ns__ {                                                           \
    template <BOOST_PP_REMOVE_PARENS(ts__)>                                    \
    inline bool operator==(BOOST_PP_REMOVE_PARENS(name__) const& a,            \
                           BOOST_PP_REMOVE_PARENS(name__) const& b)            \
    {                                                                          \
        return BOOST_PP_SEQ_FOR_EACH_I(                                        \
            LAGER_STRUCT_EQ_ITER__, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)));  \
    }                                                                          \
    template <BOOST_PP_REMOVE_PARENS(ts__)>                                    \
    inline bool operator!=(BOOST_PP_REMOVE_PARENS(name__) const& a,            \
                           BOOST_PP_REMOVE_PARENS(name__) const& b)            \
    {                                                                          \
        return !(a == b);                                                      \
    }                                                                          \
    }                                                                          \
    static_assert(true, "must use semicolon")

#define LAGER_STRUCT_TEMPLATE_EMPTY_(ns__, ts__, name__)                       \
    LAGER_HANA_ADAPT_STRUCT_TPL(ts__, (ns__::BOOST_PP_REMOVE_PARENS(name__))); \
    namespace ns__ {                                                           \
    template <BOOST_PP_REMOVE_PARENS(ts__)>                                    \
    inline bool operator==(BOOST_PP_REMOVE_PARENS(name__) const& a,            \
                           BOOST_PP_REMOVE_PARENS(name__) const& b)            \
    {                                                                          \
        return true;                                                           \
    }                                                                          \
    template <BOOST_PP_REMOVE_PARENS(ts__)>                                    \
    inline bool operator!=(BOOST_PP_REMOVE_PARENS(name__) const& a,            \
                           BOOST_PP_REMOVE_PARENS(name__) const& b)            \
    {                                                                          \
        return false;                                                          \
    }                                                                          \
    }                                                                          \
    static_assert(true, "must use semicolon")

/*!
 * Like LAGER_STRUCT but for templates
 */
#define LAGER_STRUCT_TEMPLATE(ns__, ts__, ...)                                 \
    BOOST_PP_IF(BOOST_PP_DEC(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)),             \
                LAGER_STRUCT_TEMPLATE_NON_EMPTY_,                              \
                LAGER_STRUCT_TEMPLATE_EMPTY_)                                  \
    (ns__, ts__, __VA_ARGS__)

/*
 *
 * LAGER_STRUCT_INLINE
 * ===================
 *
 */

#define LAGER_HANA_ADAPT_STRUCT_NESTED(...)                                    \
    struct hana_accessors_impl                                                 \
    {                                                                          \
        LAGER_HANA_DEFINE_ACCESORS_IMPL_(__VA_ARGS__)                          \
    } // force semicolon

#define LAGER_STRUCT_NESTED_NON_EMPTY_(name__, ...)                            \
    LAGER_HANA_ADAPT_STRUCT_NESTED(name__, __VA_ARGS__);                       \
    friend inline bool operator==(name__ const& a, name__ const& b)            \
    {                                                                          \
        return BOOST_PP_SEQ_FOR_EACH_I(                                        \
            LAGER_STRUCT_EQ_ITER__, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)));  \
    }                                                                          \
    friend inline bool operator!=(name__ const& a, name__ const& b)            \
    {                                                                          \
        return !(a == b);                                                      \
    }                                                                          \
    static_assert(true, "must use semicolon")

#define LAGER_STRUCT_NESTED_EMPTY_(name__)                                     \
    LAGER_HANA_ADAPT_STRUCT_NESTED(name__);                                    \
    friend inline bool operator==(name__ const& a, name__ const& b)            \
    {                                                                          \
        return true;                                                           \
    }                                                                          \
    friend inline bool operator!=(name__ const& a, name__ const& b)            \
    {                                                                          \
        return false;                                                          \
    }                                                                          \
    static_assert(true, "must use semicolon")

/*!
 * Like LAGER_STRUCT but for it is used nested in the struct itself.  It
 * seamlesly supports combinations of templates and nested types that are not
 * supported by the above macros.
 */
#define LAGER_STRUCT_NESTED(...)                                               \
    BOOST_PP_IF(BOOST_PP_DEC(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)),             \
                LAGER_STRUCT_NESTED_NON_EMPTY_,                                \
                LAGER_STRUCT_NESTED_EMPTY_)                                    \
    (__VA_ARGS__)
