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

#include <lager/detail/no_value.hpp>
#include <lager/detail/nodes.hpp>
#include <lager/util.hpp>

#include <zug/meta.hpp>
#include <zug/meta/pack.hpp>
#include <zug/meta/value_type.hpp>
#include <zug/tuplify.hpp>
#include <zug/util.hpp>

#include <zug/reducing/last.hpp>

namespace lager {

namespace detail {

template <typename Lens               = zug::identity_t,
          typename ParentsPack        = zug::meta::pack<>,
          template <class> class Base = reader_node>
class lens_reader_node;

template <typename Lens, typename... Parents, template <class> class Base>
class lens_reader_node<Lens, zug::meta::pack<Parents...>, Base>
    : public inner_node<
          std::decay_t<decltype(view(
              std::declval<Lens>(),
              zug::tuplify(std::declval<zug::meta::value_t<Parents>>()...)))>,
          zug::meta::pack<Parents...>,
          Base>
{
    using base_t = inner_node<
        std::decay_t<decltype(view(
            std::declval<Lens>(),
            zug::tuplify(std::declval<zug::meta::value_t<Parents>>()...)))>,
        zug::meta::pack<Parents...>,
        Base>;

protected:
    Lens lens_;

public:
    template <typename Lens2, typename ParentsTuple>
    lens_reader_node(Lens2&& l, ParentsTuple&& parents)
        : base_t{view(l, current_from(parents)),
                 std::forward<ParentsTuple>(parents)}
        , lens_{std::forward<Lens2>(l)}
    {}

    void recompute() final
    {
        this->push_down(view(lens_, current_from(this->parents())));
    }
};

template <typename Lens        = zug::identity_t,
          typename ParentsPack = zug::meta::pack<>>
class lens_cursor_node;

template <typename Lens, typename ParentsPack>
using lens_cursor_base = lens_reader_node<Lens, ParentsPack, cursor_node>;

template <typename Lens, typename... Parents>
class lens_cursor_node<Lens, zug::meta::pack<Parents...>>
    : public lens_cursor_base<Lens, zug::meta::pack<Parents...>>
{
    using base_t = lens_cursor_base<Lens, zug::meta::pack<Parents...>>;

public:
    using value_type = typename base_t::value_type;

    using base_t::base_t;

    void send_up(const value_type& value) final
    {
        this->refresh();
        this->push_up(set(this->lens_, current_from(this->parents()), value));
    }

    void send_up(value_type&& value) final
    {
        this->refresh();
        this->push_up(
            set(this->lens_, current_from(this->parents()), std::move(value)));
    }
};

template <typename Lens, typename... Parents>
auto make_lens_reader_node(Lens&& lens,
                           std::tuple<std::shared_ptr<Parents>...> parents)
{
    return link_to_parents(
        std::make_shared<
            lens_reader_node<std::decay_t<Lens>, zug::meta::pack<Parents...>>>(
            std::forward<Lens>(lens), std::move(parents)));
}

template <typename Lens, typename... Parents>
auto make_lens_cursor_node(Lens&& lens,
                           std::tuple<std::shared_ptr<Parents>...> parents)
{
    return link_to_parents(
        std::make_shared<
            lens_cursor_node<std::decay_t<Lens>, zug::meta::pack<Parents...>>>(
            std::forward<Lens>(lens), std::move(parents)));
}

} // namespace detail

} // namespace lager
