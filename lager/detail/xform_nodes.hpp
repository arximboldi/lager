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

ZUG_INLINE_CONSTEXPR struct send_down_rf_t
{
    template <typename ReaderNodePtr, typename... Inputs>
    auto operator()(ReaderNodePtr s, Inputs&&... is) const -> ReaderNodePtr
    {
        s->push_down(zug::tuplify(std::forward<Inputs>(is)...));
        return s;
    }

    template <typename ReaderNodePtr, typename... Inputs>
    auto operator()(ReaderNodePtr s) const -> ReaderNodePtr
    {
        return s;
    }
} send_down_rf{};

template <typename ValueT, typename Xform, typename... ParentPtrs>
ValueT initial_value(Xform&& xform, const std::tuple<ParentPtrs...>& parents)
{
    LAGER_TRY {
        return std::apply(
            [&](auto&&... ps) {
                return xform(zug::last)(detail::no_value{}, ps->current()...);
            },
            parents);
    } LAGER_CATCH(const no_value_error&) {
        if constexpr (std::is_default_constructible<ValueT>::value) {
            return ValueT{};
        } else {
            LAGER_THROW();
        }
    }
};

/*!
 * Implementation of a node with a transducer.
 */
template <typename Xform              = zug::identity_t,
          typename Parents            = zug::meta::pack<>,
          template <class> class Base = reader_node>
class xform_reader_node;

template <typename Xform, typename... Parents, template <class> class Base>
class xform_reader_node<Xform, zug::meta::pack<Parents...>, Base>
    : public inner_node<zug::result_of_t<Xform, zug::meta::value_t<Parents>...>,
                        zug::meta::pack<Parents...>,
                        Base>
{
    using base_t =
        inner_node<zug::result_of_t<Xform, zug::meta::value_t<Parents>...>,
                   zug::meta::pack<Parents...>,
                   Base>;
    using down_rf_t = decltype(std::declval<Xform>()(send_down_rf));

    down_rf_t down_step_;

public:
    using value_type = typename base_t::value_type;

    template <typename Xform2, typename ParentsTuple>
    xform_reader_node(Xform2&& xform, ParentsTuple&& parents)
        : base_t{initial_value<value_type>(std::forward<Xform2>(xform),
                                           parents),
                 std::forward<ParentsTuple>(parents)}
        , down_step_{std::forward<Xform2>(xform)(send_down_rf)}
    {}

    void recompute() final
    {
        std::apply([&](auto&&... ps) { down_step_(this, ps->current()...); },
                   this->parents());
    }
};

/*!
 * Reducing function that pushes the received values into the node
 * that is passed as pointer as an accumulator.
 */
ZUG_INLINE_CONSTEXPR struct send_up_rf_t
{
    template <typename WriterNodePtr, typename... Inputs>
    auto operator()(WriterNodePtr s, Inputs&&... is) const -> WriterNodePtr
    {
        s->push_up(zug::tuplify(std::forward<Inputs>(is)...));
        return s;
    }
} send_up_rf{};

/*!
 * Implementation of a node with a transducer
 */
template <typename Xform, typename WXform, typename Parents>
class xform_cursor_node : public xform_reader_node<Xform, Parents, cursor_node>
{
    using base_t  = xform_reader_node<Xform, Parents, cursor_node>;
    using up_rf_t = decltype(std::declval<WXform>()(send_up_rf));

    up_rf_t up_step_;

public:
    using value_type = typename base_t::value_type;

    template <typename Xform2, typename WXform2, typename ParentsTuple>
    xform_cursor_node(Xform2&& xform, WXform2&& wxform, ParentsTuple&& parents)
        : base_t{std::forward<Xform2>(xform),
                 std::forward<ParentsTuple>(parents)}
        , up_step_{wxform(send_up_rf)}
    {}

    void send_up(const value_type& value) final { up_step_(this, value); }
    void send_up(value_type&& value) final { up_step_(this, std::move(value)); }
};

/*!
 * Make a xform_reader_node with deduced types.
 */
template <typename Xform, typename... Parents>
auto make_xform_reader_node(Xform&& xform,
                            std::tuple<std::shared_ptr<Parents>...> parents)
{
    return link_to_parents(
        std::make_shared<xform_reader_node<std::decay_t<Xform>,
                                           zug::meta::pack<Parents...>>>(
            std::forward<Xform>(xform), std::move(parents)));
}

/*!
 * Make a xform_reader_node with deduced types.
 */
template <typename Xform, typename WXform, typename... Parents>
auto make_xform_cursor_node(Xform&& xform,
                            WXform&& wxform,
                            std::tuple<std::shared_ptr<Parents>...> parents)
{
    return link_to_parents(
        std::make_shared<xform_cursor_node<std::decay_t<Xform>,
                                           std::decay_t<WXform>,
                                           zug::meta::pack<Parents...>>>(
            std::forward<Xform>(xform),
            std::forward<WXform>(wxform),
            std::move(parents)));
}

} // namespace detail

} // namespace lager
