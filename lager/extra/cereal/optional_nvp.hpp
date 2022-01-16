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

#include <cereal/cereal.hpp>
#include <cereal/details/traits.hpp>

namespace cereal {

namespace detail {

template <typename Archive, typename R = bool>
struct archive_has_name : std::false_type
{};

template <typename Archive>
struct archive_has_name<Archive, decltype(std::declval<Archive>().hasName(""))>
    : std::true_type
{};

} // namespace detail

template <typename Archive, typename T>
bool serialize_optional_nvp(Archive& ar, const char* name, T&& value)
{
    constexpr bool isTextArchive = traits::is_text_archive<Archive>::value;
    constexpr bool isInputArchive =
        std::is_base_of_v<InputArchive<Archive>, Archive>;

    // Do check for node here, if not available then bail!
    if constexpr (isTextArchive && isInputArchive &&
                  detail::archive_has_name<Archive>()) {
        if (!ar.hasName(name))
            return false;
    }

    ar(make_nvp(name, std::forward<T>(value)));

    return true;
}

} // namespace cereal

/*!
 * This allows optional names in JSON files.
 *
 * It requires this patch on cereal to work:
 * https://github.com/arximboldi/cereal/commit/72d3eb200dc0568277255f960bc2bd7eccf8bafc
 */
#define CEREAL_OPTIONAL_NVP(ar, T) ::cereal::make_optional_nvp(ar, #T, T)
