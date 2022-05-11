#pragma once

#include <algorithm>
#include <lager/lenses.hpp>
#include <lager/lenses/attr.hpp>

#include <tuple>
#include <type_traits>

namespace lager {
namespace lenses {
namespace detail {

template <typename T>
struct templated_type_contructor;

template <template <typename...> typename T, typename... Params>
struct templated_type_contructor<T<Params...>>
{
    template <typename... Ts>
    static auto construct(Ts&&... ps)
    {
        return T<std::remove_reference_t<Ts>...>{std::forward<Ts>(ps)...};
    }
};

template <size_t I, typename... Tuples>
decltype(auto) multiget(Tuples&&... tuples)
{
    return std::forward_as_tuple(std::get<I>(std::forward<Tuples>(tuples))...);
};

template <typename ResType, typename F, typename... Tuples, size_t... Seq>
decltype(auto) apply_zip_impl(std::index_sequence<Seq...>, F&& f, Tuples&&... t)
{
    return templated_type_contructor<ResType>::construct(std::apply(
        std::forward<F>(f), multiget<Seq>(std::forward<Tuples>(t)...))...);
}

template <typename ResType, typename F, typename... Tuple>
decltype(auto) apply_zip(F&& f, Tuple&&... t)
{
    constexpr size_t len =
        std::min({std::tuple_size_v<std::remove_reference_t<Tuple>>...});
    return apply_zip_impl<ResType>(std::make_index_sequence<len>{},
                                   std::forward<F>(f),
                                   std::forward<Tuple>(t)...);
}

namespace fold_op {
template <typename Init, typename Elem>
decltype(auto) operator+(Init&& init, Elem&& elem)
{
    auto&& [lens, part] = LAGER_FWD(elem);
    return ::lager::set(LAGER_FWD(lens), LAGER_FWD(init), LAGER_FWD(part));
}
} // namespace fold_op

template <typename Init>
decltype(auto) set_fold_iteration(Init&& init)
{
    return LAGER_FWD(init);
}

template <typename Init, typename Elem, typename... Tail>
decltype(auto) set_fold_iteration(Init&& init, Elem&& elem, Tail&&... tail)
{
    auto&& [lens, part] = LAGER_FWD(elem);
    return ::lager::set(
        LAGER_FWD(lens),
        LAGER_FWD(set_fold_iteration(LAGER_FWD(init), LAGER_FWD(tail)...)),
        LAGER_FWD(part));
}

template <typename Whole, typename LensTuple, typename PartTuple, size_t... Seq>
decltype(auto) set_fold_impl(Whole&& whole,
                             LensTuple&& lenses,
                             PartTuple&& parts,
                             std::index_sequence<Seq...>)
{
    return set_fold_iteration(
        LAGER_FWD(whole),
        multiget<Seq>(LAGER_FWD(lenses), LAGER_FWD(parts))...);
}

template <typename Whole, typename LensTuple, typename PartTuple>
decltype(auto) set_fold(Whole&& whole, LensTuple&& lenses, PartTuple&& parts)
{
    return set_fold_impl(
        LAGER_FWD(whole),
        LAGER_FWD(lenses),
        LAGER_FWD(parts),
        std::make_index_sequence<
            std::tuple_size_v<std::remove_reference_t<LensTuple>>>());
}

// note: has to be const & to avoid potential moved from UB
template <class T, size_t... Seq>
auto dup_impl(T const& x, std::index_sequence<Seq...>)
{
    return std::tuple<decltype(static_cast<void>(Seq), x)...>(
        (static_cast<void>(Seq), x)...);
}

template <size_t N, class T>
auto dup(T&& x)
{
    return dup_impl(LAGER_FWD(x), std::make_index_sequence<N>());
}

inline auto fobj_view = [](auto&&... args) { return view(LAGER_FWD(args)...); };
inline auto fobj_set  = [](auto&&... args) { return set(LAGER_FWD(args)...); };

// due to some obscure behavior on MSVC, this has to be implemented in a
// manually defined function object
template <size_t N>
struct element_t : zug::detail::pipeable
{
    template <typename F>
    auto operator()(F&& f) const
    {
        return [f = std::forward<F>(f)](auto&& whole) {
            return f(std::get<N>(LAGER_FWD(whole)))([&](auto&& part) {
                auto res         = LAGER_FWD(whole);
                std::get<N>(res) = LAGER_FWD(part);
                return res;
            });
        };
    }
};

} // namespace detail

//! @defgroup lenses
//! @{

/**
 * Lens<W1, P1>, ..., Lens<Wn, Pn> -> Lens<(W1, ..., Wn), (P1, ..., Pn)>
 *
 * Note: remember you can use the identity function as the identity lens.
 */
template <typename... Lenses>
auto zip(Lenses&&... lenses)
{
    return zug::comp(
        [lens_tuple = std::make_tuple(LAGER_FWD(lenses)...)](auto&& f) {
            return [&, f = LAGER_FWD(f)](auto&& whole_tuple) {
                using Whole = std::decay_t<decltype(whole_tuple)>;
                return f(detail::apply_zip<Whole>(
                    detail::fobj_view, lens_tuple, LAGER_FWD(whole_tuple)))(
                    [&](auto&& part_tuple) {
                        return detail::apply_zip<Whole>(detail::fobj_set,
                                                        lens_tuple,
                                                        LAGER_FWD(whole_tuple),
                                                        LAGER_FWD(part_tuple));
                    });
            };
        });
}

/**
 * Lens<W, P1>, ..., Lens<W, Pn> -> Lens<W, (P1, ..., Pn)>
 *
 * Note: parts MUST NOT OVERLAP. if they do the fold will
 * overwrite sequential writes to the part. just don't do it.
 */
template <typename... Lenses>
auto fan(Lenses&&... lenses)
{
    return zug::comp([lens_tuple =
                          std::make_tuple(LAGER_FWD(lenses)...)](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            auto whole_tuple = detail::dup<sizeof...(Lenses)>(LAGER_FWD(whole));
            using Whole      = std::decay_t<decltype(whole_tuple)>;
            return f(detail::apply_zip<Whole>(
                detail::fobj_view, lens_tuple, whole_tuple))(
                [&](auto&& part_tuple) {
                    return detail::set_fold(
                        LAGER_FWD(whole), lens_tuple, LAGER_FWD(part_tuple));
                });
        };
    });
}

/**
 * (Part Whole::*)... -> Lens<Whole, (Part...)>
 *
 * Note: for the same reason as detailed in fan, the members
 * should be distinct from each other.
 */
template <typename... Member>
auto attr(Member... member)
{
    return fan(attr(member)...);
}

/**
 * N -> Lens<(P1, ..., Pn), PN>
 *
 * Note: works for pairs, tuples, arrays. don't use on variants.
 */
template <size_t N>
inline auto element = detail::element_t<N>{};

inline auto first  = element<0>;
inline auto second = element<1>;

//! @}

} // namespace lenses
} // namespace lager
