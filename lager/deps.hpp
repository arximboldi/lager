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

#include <boost/hana/at_key.hpp>
#include <boost/hana/map.hpp>

#include <type_traits>
#include <utility>

namespace lager {
namespace dep {

struct spec
{};

template <typename T>
using is_spec = std::is_base_of<spec, T>;

template <typename T>
constexpr auto is_spec_v = is_spec<T>::value;

template <typename T>
struct val : spec
{
    using key     = boost::hana::type<T>;
    using storage = T;

    template <typename Storage>
    static decltype(auto) get(Storage&& x)
    {
        return std::forward<Storage>(x);
    }
};

template <typename T>
struct ref : spec
{
    using key     = boost::hana::type<T>;
    using storage = std::reference_wrapper<T>;

    template <typename Storage>
    static decltype(auto) get(Storage&& x)
    {
        return std::forward<Storage>(x).get();
    }
};

template <typename T>
using to_spec =
    std::conditional_t<is_spec_v<T>,
                       T,
                       std::conditional_t<std::is_reference_v<T>,
                                          ref<std::remove_reference_t<T>>,
                                          val<T>>>;

template <typename T>
using key_t = typename dep::to_spec<T>::key;

template <typename T>
constexpr auto key_c = key_t<T>{};

template <typename T>
using storage_t = typename dep::to_spec<T>::storage;

} // namespace dep

namespace detail {} // namespace detail

template <typename... Deps>
struct deps
{
    template <typename... Ts>
    deps(Ts&&... ts)
        : storage_{boost::hana::make_pair(
              dep::key_c<Deps>, dep::storage_t<Deps>{std::forward<Ts>(ts)})...}
    {}

    template <typename Key>
    decltype(auto) get()
    {
        using key_t  = dep::key_t<Key>;
        using spec_t = std::decay_t<decltype(specs_t{}[key_t{}])>;
        return spec_t::get(storage_[key_t{}]);
    }

private:
    using specs_t = boost::hana::map<
        boost::hana::pair<dep::key_t<Deps>, dep::to_spec<Deps>>...>;

    using storage_t = boost::hana::map<
        boost::hana::pair<dep::key_t<Deps>, dep::storage_t<Deps>>...>;

    storage_t storage_;
};

} // namespace lager
