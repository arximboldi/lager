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

#include <lager/config.hpp>

#include <boost/hana/for_each.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/tuple.hpp>

#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <stdexcept>
#include <type_traits>

namespace lager {

template <typename T>
struct enum_meta : std::false_type
{};

namespace detail {

template <typename T>
struct return_from_for_each
{
    T value;
};

} // namespace detail

template <typename T, std::enable_if_t<enum_meta<T>::value, int> = 0>
const char* to_string(T v)
{
    LAGER_TRY {
        boost::hana::for_each(enum_meta<T>::apply(), [&](auto&& x) {
            if (boost::hana::first(x).value == v)
                LAGER_THROW(detail::return_from_for_each<const char*>{
                    boost::hana::second(x).c_str()});
        });
    } LAGER_CATCH(detail::return_from_for_each<const char*> x) {
        return x.value;
    }
    LAGER_THROW(std::runtime_error{"unknown enum value"});
}

template <typename T, std::enable_if_t<enum_meta<T>::value, int> = 0>
T to_enum(const std::string& v)
{
    LAGER_TRY {
        boost::hana::for_each(enum_meta<T>::apply(), [&](auto&& x) {
            if (boost::hana::second(x).c_str() == v)
                LAGER_THROW(detail::return_from_for_each<T>{
                    boost::hana::first(x).value});
        });
    } LAGER_CATCH(detail::return_from_for_each<T> x) {
        return x.value;
    }
    LAGER_THROW(std::runtime_error{"unknown enum name"});
}

} // namespace lager

#define LAGER_ENUM_ITER__(r__, data__, i__, elem__)                            \
    BOOST_PP_COMMA_IF(i__)                                                     \
    boost::hana::make_pair(boost::hana::integral_c<data__, data__::elem__>,    \
                           BOOST_HANA_STRING(BOOST_PP_STRINGIZE(elem__)))

#define LAGER_ENUM(ns__, name__, ...)                                          \
    namespace lager {                                                          \
    template <>                                                                \
    struct enum_meta<ns__::name__> : std::true_type                            \
    {                                                                          \
        static constexpr auto apply()                                          \
        {                                                                      \
            return boost::hana::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(            \
                LAGER_ENUM_ITER__,                                             \
                ns__::name__,                                                  \
                BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));                       \
        }                                                                      \
    };                                                                         \
    } /* namespace lager */                                                    \
    namespace cereal {                                                         \
    template <typename Archive>                                                \
    void load_minimal(const Archive&, ns__::name__& x, const std::string& v)   \
    {                                                                          \
        x = lager::to_enum<ns__::name__>(v);                                   \
    }                                                                          \
    template <typename Archive>                                                \
    std::string save_minimal(const Archive&, const ns__::name__& x)            \
    {                                                                          \
        return lager::to_string(x);                                            \
    }                                                                          \
    } /* namespace cereal */
