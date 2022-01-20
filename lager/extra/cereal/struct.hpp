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

#include <lager/extra/cereal/optional_nvp.hpp>

#include <cereal/cereal.hpp>

#include <boost/hana/at_key.hpp>
#include <boost/hana/concept/struct.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/keys.hpp>

#include <boost/core/demangle.hpp>

namespace lager::detail {

// Converts snake_case, SCREAMING_CASE, and kebap-case to camelCase
inline std::string to_camel_case(std::string str)
{
    auto in       = str.begin();
    auto out      = in;
    auto last     = str.end();
    auto new_word = false;
    for (; in != last; ++in) {
        auto c = *in;
        if (std::isdigit(c)) {
            *out++ = c;
        } else if (!std::isalpha(c)) {
            new_word = true;
        } else if (new_word) {
            *out++   = std::toupper(*in);
            new_word = false;
        } else {
            *out++ = std::tolower(*in);
        }
    }
    str.resize(out - str.begin());
    return str;
}

} // namespace lager::detail

namespace cereal {

template <typename T, typename Enable = void>
struct serialize_camel_case : std::false_type
{};

template <typename T>
struct serialize_camel_case<T, std::enable_if_t<T::serialize_camel_case>>
    : std::true_type
{};

template <typename Archive, typename T>
std::enable_if_t<boost::hana::Struct<T>::value &&
                 !serialize_camel_case<T>::value>
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, T& v)
{
    boost::hana::for_each(boost::hana::keys(v), [&](auto&& k) {
        serialize_optional_nvp(ar, k.c_str(), boost::hana::at_key(v, k));
    });
}

template <typename Archive, typename T>
std::enable_if_t<boost::hana::Struct<T>::value &&
                 serialize_camel_case<T>::value>
CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, T& v)
{
    boost::hana::for_each(boost::hana::keys(v), [&](auto&& k) {
        serialize_optional_nvp(ar,
                               lager::detail::to_camel_case(k.c_str()),
                               boost::hana::at_key(v, k));
    });
}

} // namespace cereal
