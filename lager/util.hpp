//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lager is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lager.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <functional>

namespace lager {

// copied from cppreference, in practice, use scelta::visit,
// atria::match, mpark::match, etc.
template <class... Ts> struct visitor : Ts... { using Ts::operator()...; };
template <class... Ts> visitor(Ts...) -> visitor<Ts...>;

constexpr auto noop = [] (auto&&...) {};
constexpr auto identity = [] (auto&& x) { return std::forward<decltype(x)>(x); };

template <typename Type>
struct type_ { using type = Type; };

#define LAGER_FWD(name_) std::forward<decltype(name_)>(name_)

namespace detail {

template <class F, class G>
struct composed
{
    F f;
    G g;

    template <class ...T>
    decltype(auto) operator() (T&& ...xs)
    {
        return std::invoke(f, std::invoke(g, std::forward<T>(xs)...));
    }
};

template <typename ...Fns>
struct get_composed;

template <typename... Ts>
using get_composed_t = typename get_composed<Ts...>::type;

template <typename F>
struct get_composed<F> {
    using type = F;
};

template <typename F, typename... Fs>
struct get_composed<F, Fs...> {
    using type = composed<F, get_composed_t<Fs...> >;
};

} // namespace detail

template <typename F>
auto comp(F&& f) -> F&&
{
    return std::forward<F>(f);
}

template <typename Fn, typename ...Fns>
auto comp(Fn&& f, Fns&& ...fns)
    -> detail::get_composed_t<std::decay_t<Fn>, std::decay_t<Fns>...>
{
    using result_t = detail::get_composed_t<std::decay_t<Fn>,
                                           std::decay_t<Fns>...>;
    return result_t{
        std::forward<Fn>(f),
        comp(std::forward<Fns>(fns)...)
    };
}

} // namespace lager
