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

#include <boost/core/demangle.hpp>
#include <cereal/cereal.hpp>
#include <variant>

namespace cereal {
namespace detail {

template <class Archive>
struct variant_save_visitor
{
    Archive& ar;

    template<class T>
    void operator()(const T& value) const
    {
        ar(CEREAL_NVP_("data", value));
    }
};

template <typename T>
const std::string& get_type_name()
{
    static std::string name = boost::core::demangle(typeid(T).name());
    return name;
}

//! @internal
template<int N, class Variant, class ... Args, class Archive>
typename std::enable_if<N == std::variant_size_v<Variant>, void>::type
load_variant(Archive& /*ar*/, const std::string&, Variant&)
{
    throw ::cereal::Exception("Invalid variant type name");
}

template<int N, class Variant, class H, class ... T, class Archive>
typename std::enable_if<N < std::variant_size_v<Variant>, void>::type
load_variant(Archive& ar, const std::string& target, Variant& variant)
{
    if (get_type_name<H>() == target) {
        H value;
        ar(CEREAL_NVP_("data", value));
        variant = std::move(value);
    } else {
        load_variant<N+1, Variant, T...>(ar, target, variant);
    }
}

} // namespace detail

template <class Archive, typename... Ts> inline
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, const std::variant<Ts...>& variant)
{
    ar(CEREAL_NVP_("type", std::visit([] (auto x) {
        return detail::get_type_name<decltype(x)>();
    }, variant)));
    std::visit(detail::variant_save_visitor<Archive>{ar}, variant);
}

//! Loading for std::variant
template <class Archive, typename... Ts> inline
void CEREAL_LOAD_FUNCTION_NAME(Archive & ar, std::variant<Ts...>& variant)
{
    using variant_t = typename std::variant<Ts...>;

    auto target = std::string{};
    ar(CEREAL_NVP_("type", target));
    detail::load_variant<0, variant_t, Ts...>(ar, target, variant);
}

} // namespace cereal
