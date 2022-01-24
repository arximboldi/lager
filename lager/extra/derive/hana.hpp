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

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <boost/hana/accessors.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/tuple.hpp>

// Copied from boost/hana/detail/struct_macros.hpp to avoid including that huge
// fail or defining these using lambdas...
namespace lager::struct_detail {

template <typename Memptr, Memptr ptr>
struct member_ptr
{
    template <typename T>
    constexpr decltype(auto) operator()(T&& t) const
    {
        return static_cast<T&&>(t).*ptr;
    }
};

constexpr std::size_t strlen(char const* s)
{
    std::size_t n = 0;
    while (*s++ != '\0')
        ++n;
    return n;
}

template <std::size_t n, typename Names, std::size_t... i>
constexpr auto prepare_member_name_impl(std::index_sequence<i...>)
{
    return boost::hana::string_c<boost::hana::at_c<n>(Names::get())[i]...>;
}

template <std::size_t n, typename Names>
constexpr auto prepare_member_name()
{
    constexpr std::size_t len = strlen(boost::hana::at_c<n>(Names::get()));
    return prepare_member_name_impl<n, Names>(std::make_index_sequence<len>{});
}

} // namespace lager::struct_detail

#define LAGER_DERIVE_IMPL_HANA_ACCESSOR_NAMES_ITER__(r__, data__, i__, elem__) \
    BOOST_PP_COMMA_IF(i__)                                                     \
    BOOST_PP_STRINGIZE(elem__)

#define LAGER_DERIVE_IMPL_HANA_ACCESSOR_PAIRS_ITER__(r__, data__, i__, elem__) \
    BOOST_PP_COMMA_IF(i__)                                                     \
    ::boost::hana::make_pair(                                                  \
        ::lager::struct_detail::prepare_member_name<i__, member_names>(),      \
        ::lager::struct_detail::member_ptr<decltype(&type_t::elem__),          \
                                           &type_t::elem__>{})

#define LAGER_DERIVE_IMPL_HANA_ACCESSORS__(r__, t__, members__)                \
    using type_t = BOOST_PP_REMOVE_PARENS(t__);                                \
    static constexpr auto apply()                                              \
    {                                                                          \
        struct member_names                                                    \
        {                                                                      \
            static constexpr auto get()                                        \
            {                                                                  \
                return ::boost::hana::make_tuple(BOOST_PP_SEQ_FOR_EACH_I_R(    \
                    r__,                                                       \
                    LAGER_DERIVE_IMPL_HANA_ACCESSOR_NAMES_ITER__,              \
                    _,                                                         \
                    members__));                                               \
            }                                                                  \
        };                                                                     \
        return ::boost::hana::make_tuple(BOOST_PP_SEQ_FOR_EACH_I_R(            \
            r__, LAGER_DERIVE_IMPL_HANA_ACCESSOR_PAIRS_ITER__, _, members__)); \
    }

#define LAGER_DERIVE_IMPL_HANA(r__, ns__, name__, members__)                   \
    namespace boost {                                                          \
    namespace hana {                                                           \
    template <>                                                                \
    struct accessors_impl<ns__::name__>                                        \
    {                                                                          \
        LAGER_DERIVE_IMPL_HANA_ACCESSORS__(r__, ns__::name__, members__)       \
    };                                                                         \
    }                                                                          \
    }                                                                          \
    //

#define LAGER_DERIVE_TEMPLATE_IMPL_HANA(r__, ns__, tpl__, name__, members__)   \
    namespace boost {                                                          \
    namespace hana {                                                           \
    template <BOOST_PP_REMOVE_PARENS(tpl__)>                                   \
    struct accessors_impl<ns__::BOOST_PP_REMOVE_PARENS(name__)>                \
    {                                                                          \
        LAGER_DERIVE_IMPL_HANA_ACCESSORS__(                                    \
            r__, (ns__::BOOST_PP_REMOVE_PARENS(name__)), members__)            \
    };                                                                         \
    }                                                                          \
    }                                                                          \
    //

#define LAGER_DERIVE_NESTED_IMPL_HANA(r__, name__, members__)                  \
    struct hana_accessors_impl                                                 \
    {                                                                          \
        LAGER_DERIVE_IMPL_HANA_ACCESSORS__(r__, name__, members__)             \
    };                                                                         \
    //
