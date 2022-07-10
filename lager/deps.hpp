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

#include <boost/hana/at_key.hpp>
#include <boost/hana/filter.hpp>
#include <boost/hana/find.hpp>
#include <boost/hana/intersection.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/set.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/union.hpp>
#include <boost/hana/unpack.hpp>

#include <functional>
#include <optional>
#include <stdexcept>
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

/*!
 *  Contains various types used to specify dependencies inside the
 * `lager::deps` type.
 */
namespace dep {

/*!
 * Base class that marks specification types.
 */
struct spec
{
    using is_required = std::true_type;

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

/*!
 * Specify a dependency of value T and key T
 */
template <typename T>
struct val : spec
{
    static_assert(!is_spec_v<T>, "val<> must be a most nested descriptor");

    using type     = val;
    using key_type = T;

    struct storage
    {
        T value;

        storage() = default;
        storage(T x)
            : value{std::move(x)}
        {}
    };

    template <typename Storage>
    static decltype(auto) get(Storage&& x)
    {
        return std::forward<Storage>(x).value;
    }

    static storage extract(storage st) { return std::move(st); }
    static storage extract(std::function<T()> st) { return st(); }
};

/*!
 * Specify a dependency of references to T and key T
 */
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

    static storage extract(storage st) { return std::move(st); }
    static storage extract(std::function<T&()> st) { return st(); }
};

template <typename T>
struct ref<std::reference_wrapper<T>> : ref<T>
{};

/*!
 * Convert `T` to a dependency specification.  If `T` is a dependency
 * specification it just returns `T`.  If `T` is a reference type or a
 * `std::reference_wrapper`, it results in a `ref<[dereferenced T]>`. Otherwise
 * it is just a `val<T>`.
 */
template <typename T>
using to_spec = typename std::conditional_t<
    is_spec_v<T>,
    T,
    std::conditional_t<std::is_reference_v<T> ||
                           detail::is_reference_wrapper_v<T>,
                       ref<std::remove_reference_t<T>>,
                       val<T>>>::type;

/*!
 * Modifies specification or type `T` to make it optional.
 */
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
            LAGER_THROW(missing_dependency_error{"missing dependency in lager::deps"});
    }

    template <typename Storage>
    static bool has(Storage&& x)
    {
        return bool{x};
    }

    static storage extract(storage st) { return std::move(st); }
};

/*!
 * Modifies specification or type `T` to make it indirectly provided via a
 * function.
 */
template <typename T>
struct fn : to_spec<T>
{
    using type    = fn;
    using storage = std::function<typename to_spec<T>::storage()>;

    template <typename Storage>
    static decltype(auto) get(Storage&& fn)
    {
        return to_spec<T>::get(std::forward<Storage>(fn)());
    }

    static storage extract(storage st) { return std::move(st); }
};

/*!
 * Modifies specification or type `T` to associate it with type tag `K`.
 */
template <typename K, typename T>
struct key : to_spec<T>
{
    using type     = key;
    using key_type = K;
};

namespace detail {

template <typename Spec>
struct spec_value
    : Spec
    , Spec::storage
{
    spec_value(typename Spec::storage v)
        : Spec::storage{std::move(v)}
    {}
};

} // namespace detail

/*!
 * Pairs value of type T with specificatio Spec
 */
template <typename Spec>
detail::spec_value<Spec> as(typename Spec::storage v)
{
    return {std::move(v)};
}

} // namespace dep

/*!
 * Dependency passing type.
 *
 * This type helps passing contextual dependencies arround.  Effectively, it is
 * a bag of statically keyed values (dependencies).  You can convert between
 * bags of dependencies, as long as the required dependencies of the target bag
 * is a strict subset of the required dependencies of the argument bag.  They
 * idea is that a the root of your application you hold a bag with all the
 * context you need.  You pass this context around to various components, each
 * extracting a subset of it.  Components can further down pass these extracting
 * further subsets.
 *
 * Another way to look at it is that `deps` is basically like a struct where
 * naming members is optional, and where you can automatically convert between
 * structs that have attributes with the same name.  In type theortical terms,
 * this way of converting between types is called *structural typing*.
 *
 * Here is one example of how this type might be used.
 *
 * ```cpp
 *     struct user_db_t {};
 *     struct post_db_t {};
 *
 *     void foo(deps<dep::key<user_db, database&>, dep::opt<logger&>> d)
 *     {
 *         database& db = d.get<user_db>();
 *         db.write(...);
 *         if (d.has<logger>()) d.get<logger>().debug("...");
 *     }
 *
 *     void bar(deps<logger&, dep::opt<dep::key<post_db, database&>> d)
 *     {
 *        try {
 *            d.get<logger>().info("", d.get<post_db>().query(...));
 *        } catch (missing_dependency const&) {
 *          // d.get<post_db>() may throw cuz it's optional
 *        }
 *     }
 *
 *     int main()
 *     {
 *        database udb;
 *        database pdb;
 *        logger log;
 *        auto dependencies = deps<dep::key<user_db, database&>,
 *                                 dep::key<post_db, database&>
 *                                 logger&>::from(udb, pdb, log);
 *        foo(dependencies);
 *        bar(dependencies);
 *     }
 * ```
 *
 * @tparams Deps dependency specifications for each attribute of `deps`.
 *
 * @note See the contents of the namespace @dep to see how to specify
 *       dependencies. Note that if a normal type or reference is used, a
 *       specification is generated with `dep::to_spec`.
 */
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
    /*!
     * Returns an instance of the depdency type, which each attribute
     * associated, by each `ts`.  Attributes must be passed in order of
     * declaration in the `deps` type.
     */
    template <typename... Ts>
    static deps with(Ts&&... ts)
    {
        static_assert(sizeof...(Ts) == sizeof...(Deps),
                      "You must provide a value for each specified dependency");
        return {make_storage_(std::forward<Ts>(ts)...)};
    }

    /*!
     * Creates a `deps` object picking the dependencies from `other`.
     */
    template <
        typename... Ds
#ifndef LAGER_DISABLE_STORE_DEPENDENCY_CHECKS
        , std::enable_if_t<required_key_set == boost::hana::intersection(
            required_key_set,
            deps<Ds...>::required_key_set),
        bool> = true
#endif
    >
    deps(deps<Ds...> other)
        : storage_{make_storage_from_(std::move(other.storage_))}
    {}

    /*!
     * Creates a `deps` object picking the dependencies from `other1` and
     * `other2`.  If both `other1` and `other2` provide a dependency, it is
     * picked from `other2`.
     */
    template <
        typename... D1s, 
        typename... D2s
#ifndef LAGER_DISABLE_STORE_DEPENDENCY_CHECKS
        , std::enable_if_t<
            required_key_set ==
            boost::hana::intersection(
                required_key_set,
                boost::hana::union_(deps<D1s...>::required_key_set,
                    deps<D2s...>::required_key_set)),
        bool> = true
#endif
    >
    deps(deps<D1s...> other1, deps<D2s...> other2)
        : storage_{make_storage_from_(boost::hana::union_(
              std::move(other1.storage_), std::move(other2.storage_)))}
    {}

    deps()            = default;
    deps(const deps&) = default;
    deps(deps&&)      = default;
    deps& operator=(const deps&) = default;
    deps& operator=(deps&&) = default;

    /*!
     * Get the dependency with key `Key`.
     */
    template <typename Key>
    decltype(auto) get() const
    {
        using spec_t = std::decay_t<decltype(spec_map[get_key_t<Key>{}])>;
        return spec_t::get(storage_[get_key_t<Key>{}]);
    }

    /*!
     * Returns whether the dependency with `Key` is satisfied.  This returns
     * always `true` for required dependencies, but it may return `false` when
     * an optional dependency is not provided.
     */
    template <typename Key>
    bool has() const
    {
        using spec_t = std::decay_t<decltype(spec_map[get_key_t<Key>{}])>;
        return spec_t::has(storage_[get_key_t<Key>{}]);
    }

    /*!
     * Returns a new dependencies object that contains all dependencies in this
     * object and `other`.  If the two objects provide a dependency with the
     * same key, it will have the type and value as specified in `other`.
     */
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
    friend class deps;

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
        return Spec::extract(std::forward<Storage>(other)[get_key_t<Spec>{}]);
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

/*!
 * Returns a deps object containing everything passed in `args`.  Dependencies
 * will be stored as references only if wrapped in a `std::reference_wrapper`
 * (see `std::ref`).
 *
 * @todo Add mechanism to specify keys here.
 */
template <typename... Ts>
auto make_deps(Ts&&... args) -> deps<std::decay_t<Ts>...>
{
    return deps<std::decay_t<Ts>...>::with(std::forward<Ts>(args)...);
}

//! @defgroup deps-access
//! @{

/*!
 * Free standing alias for `deps::get()`
 */
template <typename Key, typename... Ts>
decltype(auto) get(const deps<Ts...>& d)
{
    return d.template get<Key>();
}

/*!
 * Free standing alias for `deps::has()`
 */
template <typename Key, typename... Ts>
bool has(const deps<Ts...>& d)
{
    return d.template has<Key>();
}

/*!
 * Metafunction to see if something is a deps type.
 */
template <typename T>
struct is_deps : std::false_type
{};

template <typename... Ts>
struct is_deps<deps<Ts...>> : std::true_type
{};

//! @}

} // namespace lager
