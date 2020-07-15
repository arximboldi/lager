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

#include <lager/detail/nodes.hpp>
#include <lager/util.hpp>

#include <zug/meta.hpp>
#include <zug/meta/pack.hpp>
#include <zug/meta/value_type.hpp>
#include <zug/tuplify.hpp>
#include <zug/util.hpp>

namespace lager {
namespace detail {

template <typename Parents            = zug::meta::pack<>,
          template <class> class Base = reader_node>
class merge_reader_node;

template <typename... Parents, template <class> class Base>
class merge_reader_node<zug::meta::pack<Parents...>, Base>
    : public inner_node<std::decay_t<decltype(zug::tuplify(
                            std::declval<zug::meta::value_t<Parents>>()...))>,
                        zug::meta::pack<Parents...>,
                        Base>
{
    using base_t =
        inner_node<std::decay_t<decltype(zug::tuplify(
                       std::declval<zug::meta::value_t<Parents>>()...))>,
                   zug::meta::pack<Parents...>,
                   Base>;

public:
    using value_type = typename base_t::value_type;

    template <typename ParentsTuple>
    merge_reader_node(ParentsTuple&& parents)
        : base_t{current_from(parents), std::forward<ParentsTuple>(parents)}
    {}

    void recompute() final { this->push_down(current_from(this->parents())); }
};

template <typename Parents>
using merge_reader_base = merge_reader_node<Parents, cursor_node>;

template <typename Parents>
class merge_cursor_node : public merge_reader_base<Parents>
{
    using base_t = merge_reader_base<Parents>;

public:
    using value_type = typename base_t::value_type;
    using base_t::base_t;

    void send_up(const value_type& value) final { this->push_up(value); }
    void send_up(value_type&& value) final { this->push_up(std::move(value)); }
};

/*!
 * Make a merge_reader_node with deduced types.
 */
template <typename... Parents>
auto make_merge_reader_node(std::tuple<std::shared_ptr<Parents>...> parents)
{
    return link_to_parents(
        std::make_shared<merge_reader_node<zug::meta::pack<Parents...>>>(
            std::move(parents)));
}

/*!
 * Make a merge_reader_node with deduced types.
 */
template <typename... Parents>
auto make_merge_cursor_node(std::tuple<std::shared_ptr<Parents>...> parents)
{
    return link_to_parents(
        std::make_shared<merge_cursor_node<zug::meta::pack<Parents...>>>(
            std::move(parents)));
}

} // namespace detail
} // namespace lager
