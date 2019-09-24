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

#include <lager/cursor.hpp>
#include <lager/reader.hpp>

#include <zug/transducer/map.hpp>

namespace lager {

/*!
 * Returns a new in formed by applying a transducer `xform`
 * on the successive values of the in.  If two `xform` parameters
 * are given and the ins are also outs, values can be set back
 * using the second `xform` to go back into the original domain.
 */
template <typename Xform, typename... ReaderTs>
auto xformed(Xform&& xform, ReaderTs&&... ins)
    -> reader_base<typename decltype(detail::make_xform_reader_node(
        xform, detail::access::node(ins)...))::element_type>
{
    return detail::make_xform_reader_node(
        std::forward<Xform>(xform),
        detail::access::node(std::forward<ReaderTs>(ins))...);
}

template <typename Xform, typename Xform2, typename... CursorTs>
auto xformed2(Xform&& xform, Xform2&& xform2, CursorTs&&... ins)
    -> cursor_base<typename decltype(detail::make_xform_cursor_node(
        xform, xform2, detail::access::node(ins)...))::element_type>
{
    return detail::make_xform_cursor_node(
        std::forward<Xform>(xform),
        std::forward<Xform2>(xform2),
        detail::access::node(std::forward<CursorTs>(ins))...);
}

/*!
 * Import the update function.
 * @see detail::update
 */
using detail::update;

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
 * Returns *xformed* version of the ins using `xat`. If the ins
 * are also outs, it is updated with `uat`.
 * @see xat
 * @see uat
 */
template <typename KeyT, typename... ReaderTs>
auto atted(KeyT&& k, ReaderTs&&... ins)
{
    return xformed(xat(std::forward<KeyT>(k)), std::forward<ReaderTs>(ins)...);
}

template <typename KeyT, typename... CursorTs>
auto atted2(KeyT&& k, CursorTs&&... ins)
{
    return xformed2(xat(k), update(uat(k)), std::forward<CursorTs>(ins)...);
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

/*!
 * Given a pointer to member, returns a *xformed* version of the ins
 * accessed through the member.  If the ins are also outs, the xformed
 * version is an inout.
 */
template <typename AttrPtrT, typename... ReaderTs>
auto attred(AttrPtrT attr, ReaderTs&&... ins)
{
    return xformed(zug::map(get_attr(attr)), std::forward<ReaderTs>(ins)...);
}

template <typename AttrPtrT, typename... CursorTs>
auto attred2(AttrPtrT attr, CursorTs&&... ins)
{
    return xformed2(zug::map(get_attr(attr)),
                    update(set_attr(attr)),
                    std::forward<CursorTs>(ins)...);
}

} // namespace lager
