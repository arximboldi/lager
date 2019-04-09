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
#include <boost/hana/filter.hpp>
#include <boost/hana/find.hpp>
#include <boost/hana/intersection.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/set.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/union.hpp>
#include <boost/hana/unpack.hpp>

#include <type_traits>
#include <utility>

namespace lager {

struct missing_dependency_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

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
{
    using is_required = std::true_type;

    template <typename Storage>
    static decltype(auto) get(Storage&& x)
    {
        return std::forward<Storage>(x);
    }

    template <typename Storage>
    static bool has(Storage&& x)
    {
        return true;
    }
};

template <typename T>
using is_spec = std::is_base_of<spec, T>;

template <typename T>
constexpr auto is_spec_v = is_spec<T>::value;

template <typename T>
struct val : spec
{
    static_assert(!is_spec_v<T>, "val<> must be a most nested descriptor");

    using type     = val;
    using key_type = T;
    using storage  = T;
};

template <typename T>
struct ref : spec
{
    static_assert(!is_spec_v<T>, "val<> must be a most nested descriptor");

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

template <typename T>
struct opt : to_spec<T>
{
    using type        = opt;
    using storage     = std::optional<typename to_spec<T>::storage>;
    using is_required = std::false_type;

    template <typename Storage>
    static decltype(auto) get(Storage&& x)
    {
        if (x)
            return to_spec<T>::get(*std::forward<Storage>(x));
        else
            throw missing_dependency_error{"missing dependency in lager::deps"};
    }

    template <typename Storage>
    static bool has(Storage&& x)
    {
        return bool{x};
    }
};

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
    template <typename T>
    using get_key_t = boost::hana::type<typename dep::to_spec<T>::key_type>;

    template <typename T>
    using get_storage_t = typename dep::to_spec<T>::storage;

    template <typename T>
    using get_is_required_t = typename dep::to_spec<T>::is_required;

    static constexpr auto key_set = boost::hana::make_set(get_key_t<Deps>{}...);

    static constexpr auto required_key_set = boost::hana::unpack(
        boost::hana::filter(
            boost::hana::make_tuple(dep::to_spec<Deps>{}...),
            [](auto s) { return get_is_required_t<decltype(s)>{}; }),
        [](auto... s) {
            return boost::hana::make_set(get_key_t<decltype(s)>{}...);
        });

    static constexpr auto spec_map = boost::hana::make_map(
        boost::hana::make_pair(get_key_t<Deps>{}, dep::to_spec<Deps>{})...);

    static_assert(sizeof...(Deps) == boost::hana::length(key_set),
                  "There are dependencies with duplicate keys. Use "
                  "lager::dep::key<> to disambiguate them.");

public:
    template <typename... Ts>
    static deps with(Ts&&... ts)
    {
        static_assert(sizeof...(Ts) == sizeof...(Deps),
                      "You must provide a value for each specified dependency");
        return {make_storage_(std::forward<Ts>(ts)...)};
    }

    template <
        typename... Ds,
        std::enable_if_t<required_key_set == boost::hana::intersection(
                                                 required_key_set,
                                                 deps<Ds...>::required_key_set),
                         bool> = true>
    deps(deps<Ds...> other)
        : storage_{make_storage_from_(std::move(other.storage_))}
    {}

    template <typename... D1s,
              typename... D2s,
              std::enable_if_t<
                  required_key_set ==
                      boost::hana::intersection(
                          required_key_set,
                          boost::hana::union_(deps<D1s...>::required_key_set,
                                              deps<D2s...>::required_key_set)),
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
        using spec_t = std::decay_t<decltype(spec_map[get_key_t<Key>{}])>;
        return spec_t::get(storage_[get_key_t<Key>{}]);
    }

    template <typename Key>
    bool has() const
    {
        using spec_t = std::decay_t<decltype(spec_map[get_key_t<Key>{}])>;
        return spec_t::has(storage_[get_key_t<Key>{}]);
    }

    template <typename... Ds>
    auto merge(deps<Ds...> other)
    {
        return boost::hana::unpack(
            boost::hana::union_(spec_map, deps<Ds...>::spec_map),
            [&](auto... ts) {
                using deps_t =
                    deps<std::decay_t<decltype(boost::hana::second(ts))>...>;
                return deps_t{*this, std::move(other)};
            });
    }

private:
    template <typename... Ds>
    friend struct deps;

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
        return storage_t{boost::hana::make_pair(
            get_key_t<Deps>{},
            extract_from_storage_<dep::to_spec<Deps>>(
                std::forward<Storage>(other), get_is_required_t<Deps>{}))...};
    }

    template <typename Spec, typename Storage>
    static get_storage_t<Spec>
    extract_from_storage_(Storage&& other, std::true_type /* is_required */)
    {
        return std::forward<Storage>(other)[get_key_t<Spec>{}];
    }

    template <typename Spec, typename Storage>
    static get_storage_t<Spec>
    extract_from_storage_(Storage&& other, std::false_type /* !is_required */)
    {
        return boost::hana::find(std::forward<Storage>(other),
                                 get_key_t<Spec>{})
            .value_or(get_storage_t<Spec>{});
    }

    storage_t storage_;
};

template <typename Key, typename... Ts>
decltype(auto) get(const deps<Ts...>& d)
{
    return d.template get<Key>();
}

template <typename Key, typename... Ts>
bool has(const deps<Ts...>& d)
{
    return d.template has<Key>();
}

template <typename... Ts>
auto make_deps(Ts&&... args) -> deps<std::decay_t<Ts>...>
{
    return deps<std::decay_t<Ts>...>::with(std::forward<Ts>(args)...);
}

} // namespace lager
