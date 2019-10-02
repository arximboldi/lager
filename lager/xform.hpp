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
class cursor_mixin;
template <typename T>
class reader_base;
template <typename T>
class reader_mixin;

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

/*!
 * @see update()
 */
template <typename XformWriterNodePtr, std::size_t... Indices>
decltype(auto) peek_parents(XformWriterNodePtr s,
                            std::index_sequence<Indices...>)
{
    s->recompute_deep();
    return zug::tuplify(std::get<Indices>(s->parents())->current()...);
}

/*!
 * Returns a transducer for updating the parent values via a
 * up-node. It processes the input with the function `mapping`,
 * passing to it a value or tuple containing the values of the parents
 * of the node as first parameter, and the input as second.  This
 * mapping can thus return an *updated* version of the values in the
 * parents with the new input.
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
 * Transducer that projects the key `key` from containers with a
 * standard-style `at()` method.  It filters out ins without the
 * given key.
 */
template <typename KeyT>
auto xat(KeyT&& key)
{
    return [=](auto&& step) {
        return
            [=](auto&& s, auto&& i) mutable
            -> decltype(true ? std::forward<decltype(s)>(s)
                             : step(std::forward<decltype(s)>(s), i.at(key))) {
                try {
                    return step(std::forward<decltype(s)>(s),
                                std::forward<decltype(i)>(i).at(key));
                } catch (const std::out_of_range&) {
                    return s;
                }
            };
    };
}

/*!
 * Update function that updates the `key` in a container with a
 * standard-style `at()` method.  Does not update the container if the
 * key was not already present.
 * @see update
 */
template <typename KeyT>
auto uat(KeyT&& key)
{
    return [=](auto col, auto&& v) {
        try {
            col.at(key) = std::forward<decltype(v)>(v);
        } catch (const std::out_of_range&) {}
        return col;
    };
}

/*!
 * Returns a unary function that dereferences the given pointer to
 * member to the applied objects.
 */
template <typename AttrPtrT>
auto get_attr(AttrPtrT attr)
{
    return [=](auto&& x) -> decltype(auto) {
        return std::forward<decltype(x)>(x).*attr;
    };
}

/*!
 * Returns a update function that uses the given pointer to member.
 * @see update
 */
template <typename AttrPtrT>
auto set_attr(AttrPtrT attr)
{
    return [=](auto s, auto&& x) -> decltype(auto) {
        s.*attr = std::forward<decltype(x)>(x);
        return s;
    };
}

} // namespace detail

/*!
 * Returns *xformed* version of the ins using `xat`. If the ins
 * are also outs, it is updated with `uat`.
 * @see xat
 * @see uat
 */
template <typename KeyT, typename... ReaderTs>
auto atted(KeyT&& k, const reader_mixin<ReaderTs>&... ins)
{
    return xform(detail::xat(std::forward<KeyT>(k)))(ins...);
}

template <typename KeyT, typename... CursorTs>
auto atted(KeyT&& k, const cursor_mixin<CursorTs>&... ins)
{
    return xform(detail::xat(k), detail::update(detail::uat(k)))(ins...);
}

/*!
 * Given a pointer to member, returns a *xformed* version of the ins
 * accessed through the member.  If the ins are also outs, the xformed
 * version is an inout.
 */
template <typename AttrPtrT, typename... ReaderTs>
auto attred(AttrPtrT attr, const reader_mixin<ReaderTs>&... ins)
{
    return xform(zug::map(detail::get_attr(attr)))(ins...);
}

template <typename AttrPtrT, typename... CursorTs>
auto attred(AttrPtrT attr, const cursor_mixin<CursorTs>&... ins)
{
    return xform(zug::map(detail::get_attr(attr)),
                 detail::update(detail::set_attr(attr)))(ins...);
}

} // namespace lager
