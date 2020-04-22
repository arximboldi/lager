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

#include <lager/detail/xform_nodes.hpp>

#include <zug/transducer/map.hpp>

namespace lager {

template <typename T>
class cursor_base;
template <typename T>
class reader_base;

template <typename T>
struct writer_mixin;
template <typename T>
struct cursor_mixin;
template <typename T>
struct reader_mixin;

/*!
 * Returns a new in formed by applying a transducer `xform`
 * on the successive values of the in.  If two `xform` parameters
 * are given and the ins are also outs, values can be set back
 * using the second `xform` to go back into the original domain.
 */
template <typename Xform>
auto xform(Xform&& xform)
{
    return [=](auto&&... ins)
               -> reader_base<typename decltype(detail::make_xform_reader_node(
                   xform, detail::access::node(ins)...))::element_type>
    {
        return detail::make_xform_reader_node(
            xform, detail::access::node(std::forward<decltype(ins)>(ins))...);
    };
}

template <typename Xform, typename Xform2>
auto xform(Xform&& xform, Xform2&& xform2)
{
    return [=](auto&&... ins)
               -> cursor_base<typename decltype(detail::make_xform_cursor_node(
                   xform, xform2, detail::access::node(ins)...))::element_type>
    {
        return detail::make_xform_cursor_node(
            xform,
            xform2,
            detail::access::node(std::forward<decltype(ins)>(ins))...);
    };
}

namespace detail {

template <typename XformWriterNodePtr, std::size_t... Indices>
decltype(auto) peek_parents(XformWriterNodePtr s,
                            std::index_sequence<Indices...>)
{
    s->recompute_deep();
    return zug::tuplify(std::get<Indices>(s->parents())->current()...);
}

} // namespace detail

/*!
 * Returns a transducer for updating the parent values via a up-node. It
 * processes the input with the function `updater`, passing to it a value or
 * tuple containing the values of the parents of the node as first parameter,
 * and the input as second.  This mapping can thus return an *updated* version
 * of the values in the parents with the new input.
 *
 * @note This transducer should only be used for the setter of output nodes.
 */
template <typename UpdateT>
auto update(UpdateT&& updater)
{
    return [=](auto&& step) {
        return [=](auto s, auto&&... is) mutable {
            auto indices = std::make_index_sequence<
                std::tuple_size<std::decay_t<decltype(s->parents())>>::value>{};
            return step(s, updater(peek_parents(s, indices), ZUG_FWD(is)...));
        };
    };
}

/*!
 * Given a pointer to member, returns a *xformed* version of the ins accessed
 * through the lens.
 */
template <typename LensT, typename... ReaderTs>
auto zoom(LensT&& l, const reader_mixin<ReaderTs>&... ins)
{
    return xform(zug::map([l](auto&& x) {
        return lager::view(l, std::forward<decltype(x)>(x));
    }))(static_cast<const ReaderTs&>(ins)...);
}

template <typename LensT, typename... CursorTs>
auto zoom(LensT&& l, const writer_mixin<CursorTs>&... ins)
{
    return xform(
        zug::map([l](auto&&... xs) {
            return lager::view(l,
                               zug::tuplify(std::forward<decltype(xs)>(xs))...);
        }),
        lager::update([l](auto&& x, auto&&... vs) {
            return lager::set(l,
                              std::forward<decltype(x)>(x),
                              zug::tuplify(std::forward<decltype(vs)>(vs)...));
        }))(static_cast<const CursorTs&>(ins)...);
}

template <typename LensT, typename... CursorTs>
auto zoom(LensT&& l, const cursor_mixin<CursorTs>&... ins)
{
    return zoom(std::forward<LensT>(l),
                static_cast<const writer_mixin<CursorTs>&>(ins)...);
}

} // namespace lager
