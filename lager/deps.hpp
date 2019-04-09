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
#include <boost/hana/intersection.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/set.hpp>
#include <boost/hana/union.hpp>
#include <boost/hana/unpack.hpp>

#include <type_traits>
#include <utility>

namespace lager {

namespace detail {

template <typename T>
struct is_reference_wrapper : std::false_type
{};
template <typename T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type
{};

template <typename T>
constexpr auto is_reference_wrapper_v =
    is_reference_wrapper<std::decay_t<T>>::value;

} // namespace detail

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
    using type     = val;
    using key_type = T;
    using storage  = T;

    template <typename Storage>
    static decltype(auto) get(Storage&& x)
    {
        return std::forward<Storage>(x);
    }
};

template <typename T>
struct ref : spec
{
    using type     = ref;
    using key_type = T;
    using storage  = std::reference_wrapper<T>;

    template <typename Storage>
    static decltype(auto) get(Storage&& x)
    {
        return std::forward<Storage>(x).get();
    }
};

template <typename T>
struct ref<std::reference_wrapper<T>> : ref<T>
{};

template <typename T>
using to_spec = typename std::conditional_t<
    is_spec_v<T>,
    T,
    std::conditional_t<std::is_reference_v<T> ||
                           detail::is_reference_wrapper_v<T>,
                       ref<std::remove_reference_t<T>>,
                       val<T>>>::type;

template <typename K, typename T>
struct key : to_spec<T>
{
    using type     = key;
    using key_type = K;
};

} // namespace dep

template <typename... Deps>
class deps
{
    static constexpr auto spec_set =
        boost::hana::make_set(boost::hana::type_c<dep::to_spec<Deps>>...);

    template <typename T>
    using get_key_t = boost::hana::type<typename dep::to_spec<T>::key_type>;

    template <typename T>
    using get_storage_t = typename dep::to_spec<T>::storage;

public:
    template <typename... Ts>
    static deps with(Ts&&... ts)
    {
        return {make_storage_(std::forward<Ts>(ts)...)};
    }

    template <typename... Ds,
              std::enable_if_t<spec_set == boost::hana::intersection(
                                               spec_set, deps<Ds...>::spec_set),
                               bool> = true>
    deps(deps<Ds...> other)
        : storage_{make_storage_from_(std::move(other.storage_))}
    {}

    template <typename... D1s,
              typename... D2s,
              std::enable_if_t<
                  spec_set == boost::hana::intersection(
                                  spec_set,
                                  boost::hana::union_(deps<D1s...>::spec_set,
                                                      deps<D2s...>::spec_set)),
                  bool> = true>
    deps(deps<D1s...> other1, deps<D2s...> other2)
        : storage_{make_storage_from_(boost::hana::union_(
              std::move(other1.storage_), std::move(other2.storage_)))}
    {}

    deps()            = default;
    deps(const deps&) = default;
    deps(deps&&)      = default;
    deps& operator=(const deps&) = default;
    deps& operator=(deps&&) = default;

    template <typename Key>
    decltype(auto) get() const
    {
        using spec_t = std::decay_t<decltype(spec_map_t{}[get_key_t<Key>{}])>;
        return spec_t::get(storage_[get_key_t<Key>{}]);
    }

    template <typename... Ds>
    auto merge(deps<Ds...> other)
    {
        return boost::hana::unpack(
            boost::hana::union_(spec_set, deps<Ds...>::spec_set),
            [&](auto... ts) {
                using deps_t = deps<typename decltype(ts)::type...>;
                return deps_t{*this, std::move(other)};
            });
    }

private:
    template <typename... Ds>
    friend struct deps;

    using spec_map_t = boost::hana::map<
        boost::hana::pair<get_key_t<Deps>, dep::to_spec<Deps>>...>;

    using storage_t = boost::hana::map<
        boost::hana::pair<get_key_t<Deps>, get_storage_t<Deps>>...>;

    deps(storage_t storage)
        : storage_{std::move(storage)}
    {}

    template <typename... Ts>
    static storage_t make_storage_(Ts&&... ts)
    {
        return storage_t{boost::hana::make_pair(
            get_key_t<Deps>{}, get_storage_t<Deps>{std::forward<Ts>(ts)})...};
    }

    template <typename Storage>
    static storage_t make_storage_from_(Storage&& other)
    {
        return storage_t{
            boost::hana::make_pair(get_key_t<Deps>{},
                                   get_storage_t<Deps>{std::forward<Storage>(
                                       other)[get_key_t<Deps>{}]})...};
    }

    storage_t storage_;
};

template <typename Key, typename... Ts>
decltype(auto) get(const deps<Ts...>& d)
{
    return d.template get<Key>();
}

template <typename... Ts>
auto make_deps(Ts&&... args) -> deps<std::decay_t<Ts>...>
{
    return deps<std::decay_t<Ts>...>::with(std::forward<Ts>(args)...);
}

} // namespace lager
