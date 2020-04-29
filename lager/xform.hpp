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
class reader;
template <typename T>
class writer;
template <typename T>
class cursor;

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

namespace detail {

template <typename Xform, typename... NodeTs>
struct reader_xform
{
    Xform xform_;
    std::tuple<std::shared_ptr<NodeTs>...> nodes_;

    template <typename Xf>
    auto xform(Xf&& xf) &&
    {
        return make_reader_xform(zug::comp(std::move(xform_), xf),
                                 std::move(nodes_));
    }

    template <typename Lens>
    auto zoom(Lens&& l) &&
    {
        return std::move(*this).xform(zug::map([l](auto&&... xs) {
            return lager::view(l, zug::tuplify(LAGER_FWD(xs)...));
        }));
    }

    template <typename T>
    auto operator[](T&& k) &&
    {
        using value_t = typename decltype(std::move(*this).make())::value_type;
        auto lens     = detail::smart_lens<value_t>::make(std::forward<T>(k));
        return std::move(*this).zoom(lens);
    }

    auto make() &&
    {
        return std::apply(
            [&](auto&&... ns) {
                auto node    = detail::make_xform_reader_node(std::move(xform_),
                                                           LAGER_FWD(ns)...);
                using node_t = typename decltype(node)::element_type;
                return reader_base<node_t>{std::move(node)};
            },
            std::move(nodes_));
    }

    template <typename T>
    operator reader<T>() &&
    {
        return std::move(*this).make();
    }
};

template <typename... NodeTs>
auto make_reader_xform_init(std::shared_ptr<NodeTs>... nodes)
    -> reader_xform<zug::identity__t, NodeTs...>
{
    return {{}, {std::move(nodes)...}};
}

template <typename Xform, typename... NodeTs>
auto make_reader_xform(Xform xform, std::shared_ptr<NodeTs>... nodes)
    -> reader_xform<Xform, NodeTs...>
{
    return {std::move(xform), {std::move(nodes)...}};
}

template <typename Xform, typename... NodeTs>
auto make_reader_xform(Xform xform,
                       std::tuple<std::shared_ptr<NodeTs>...> nodes)
    -> reader_xform<Xform, NodeTs...>
{
    return {std::move(xform), std::move(nodes)};
}

template <typename Xform, typename WriteXform, typename... NodeTs>
struct writer_xform
{
    Xform xform_;
    WriteXform wxform_;
    std::tuple<std::shared_ptr<NodeTs>...> nodes_;

    template <typename Xf>
    auto xform(Xf&& xf) &&
    {
        return make_reader_xform(
            zug::comp(std::move(xform_), std::forward<Xf>(xf)),
            std::move(nodes_));
    }

    template <typename Xf, typename WXf>
    auto xform(Xf&& xf, WXf&& wxf) &&
    {
        return make_writer_xform(
            zug::comp(std::move(xform_), std::forward<Xf>(xf)),
            zug::comp(std::forward<WXf>(wxf), std::move(wxform_)),
            std::move(nodes_));
    }

    template <typename Lens>
    auto zoom(Lens&& l) &&
    {
        return std::move(*this).xform(
            zug::map([l](auto&&... xs) {
                return lager::view(
                    l, zug::tuplify(std::forward<decltype(xs)>(xs)...));
            }),
            lager::update([l](auto&& x, auto&&... vs) {
                return lager::set(
                    l,
                    std::forward<decltype(x)>(x),
                    zug::tuplify(std::forward<decltype(vs)>(vs)...));
            }));
    }

    template <typename T>
    auto operator[](T&& k) &&
    {
        using value_t = typename decltype(std::move(*this).make())::value_type;
        auto l        = detail::smart_lens<value_t>::make(std::forward<T>(k));
        // because of update(), we actually should do this... we will
        // consider a better option soon to deal with lenses
        return std::move(*this).make().zoom(l);
    }

    auto make() &&
    {
        return std::apply(
            [&](auto&&... nodes) {
                auto node = detail::make_xform_cursor_node(
                    std::move(xform_), std::move(wxform_), LAGER_FWD(nodes)...);
                using node_t = typename decltype(node)::element_type;
                return cursor_base<node_t>{std::move(node)};
            },
            std::move(nodes_));
    }

    template <typename T>
    operator reader<T>() &&
    {
        return std::move(*this).make();
    }

    template <typename T>
    operator writer<T>() &&
    {
        return std::move(*this).make();
    }

    template <typename T>
    operator cursor<T>() &&
    {
        return std::move(*this).make();
    }
};

template <typename... NodeTs>
auto make_writer_xform_init(std::shared_ptr<NodeTs>... nodes)
    -> writer_xform<zug::identity__t, zug::identity__t, NodeTs...>
{
    return {{}, {}, {std::move(nodes)...}};
}

template <typename Xform, typename WriteXform, typename... NodeTs>
auto make_writer_xform(Xform xform,
                       WriteXform wxform,
                       std::shared_ptr<NodeTs>... nodes)
    -> writer_xform<Xform, WriteXform, NodeTs...>
{
    return {std::move(xform), std::move(wxform), {std::move(nodes)...}};
}

template <typename Xform, typename WriteXform, typename... NodeTs>
auto make_writer_xform(Xform xform,
                       WriteXform wxform,
                       std::tuple<std::shared_ptr<NodeTs>...> nodes)
    -> writer_xform<Xform, WriteXform, NodeTs...>
{
    return {std::move(xform), std::move(wxform), std::move(nodes)};
}

} // namespace detail

/*!
 * Returns a temporary object that can be used to describe transformations over
 * the given set of cursors.
 *
 * This temporary object has the methods `zoom`, `xform` and `operator[]` just
 * like cursors, but returning a new temporary object each time these are
 * applied, composing transformations along the way without creating new nodes.
 *
 * This temporary object can be reified into an actual cursor, creating an
 * associated node, by using the `make` method or by converting it to a
 * `cursor<T>` type.
 */
template <typename... ReaderTs>
auto with(const reader_mixin<ReaderTs>&... ins)
{
    return detail::make_reader_xform_init(
        detail::access::node(static_cast<const ReaderTs&>(ins))...);
}

template <typename... WriterTs>
auto with(const writer_mixin<WriterTs>&... ins)
{
    return detail::make_writer_xform_init(
        detail::access::node(static_cast<const WriterTs&>(ins))...);
}

template <typename... CursorTs>
auto with(const cursor_mixin<CursorTs>&... ins)
{
    return with(static_cast<const writer_mixin<CursorTs>&>(ins)...);
}

} // namespace lager
