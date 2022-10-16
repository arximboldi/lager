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

#include <lager/detail/lens_nodes.hpp>
#include <lager/detail/merge_nodes.hpp>
#include <lager/detail/xform_nodes.hpp>

#include <lager/tags.hpp>

#include <zug/transducer/filter.hpp>
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
class writer_base;

template <typename T>
struct writer_mixin;
template <typename T>
struct cursor_mixin;
template <typename T>
struct reader_mixin;

//! @defgroup cursors
//! @{

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
            s->refresh();
            return step(s, updater(current_from(s->parents()), ZUG_FWD(is)...));
        };
    };
}

/*!
 * Mixing to get common tranducers applied to `this` via the `xform` method.
 */
template <typename Deriv>
struct xform_mixin
{
    template <typename... Args>
    auto map(Args&&... args) const&
    {
        return static_cast<const Deriv&>(*this).xform(
            zug::map(std::forward<Args>(args))...);
    }
    template <typename... Args>
    auto map(Args&&... args) &&
    {
        return static_cast<Deriv&&>(std::move(*this))
            .xform(zug::map(std::forward<Args>(args))...);
    }

    template <typename... Args>
    auto filter(Args&&... args) const&
    {
        return static_cast<const Deriv&>(*this).xform(
            zug::filter(std::forward<Args>(args))...);
    }
    template <typename... Args>
    auto filter(Args&&... args) &&
    {
        return static_cast<Deriv&&>(std::move(*this))
            .xform(zug::filter(std::forward<Args>(args))...);
    }
};

//! @}

namespace detail {

template <template <class> class Result, typename... Nodes>
class with_expr;
template <typename Xform, typename... Nodes>
class with_xform_expr;
template <template <class> class Result,
          typename Xform,
          typename WXform,
          typename... Nodes>
class with_wxform_expr;
template <template <class> class Result, typename Lens, typename... Nodes>
class with_lens_expr;

template <template <typename Node> class Result, typename... Nodes>
auto make_with_expr(std::tuple<std::shared_ptr<Nodes>...> nodes)
    -> with_expr<Result, Nodes...>
{
    return {std::move(nodes)};
}

template <typename Xform, typename... Nodes>
auto make_with_xform_expr(Xform xform,
                          std::tuple<std::shared_ptr<Nodes>...> nodes)
    -> with_xform_expr<Xform, Nodes...>
{
    return {std::move(xform), std::move(nodes)};
}

template <template <class> class Result,
          typename Xform,
          typename WXform,
          typename... Nodes>
auto make_with_wxform_expr(Xform xform,
                           WXform wxform,
                           std::tuple<std::shared_ptr<Nodes>...> nodes)
    -> with_wxform_expr<Result, Xform, WXform, Nodes...>
{
    return {std::move(xform), std::move(wxform), std::move(nodes)};
}

template <template <class> class Result, typename Lens, typename... Nodes>
auto make_with_lens_expr(Lens lens, std::tuple<std::shared_ptr<Nodes>...> nodes)
    -> with_lens_expr<Result, Lens, Nodes...>
{
    return {std::move(lens), std::move(nodes)};
}

template <typename Result>
struct is_reader_base : std::false_type
{};

template <typename T>
struct is_reader_base<reader_base<T>> : std::true_type
{};

template <typename Deriv>
class with_expr_base : public xform_mixin<Deriv>
{
    Deriv&& deriv_() { return std::move(static_cast<Deriv&>(*this)); }

public:
    template <typename T>
    auto operator[](T&& k) &&
    {
        using value_t = typename decltype(deriv_().make())::value_type;
        auto lens     = smart_lens<value_t>::make(std::forward<T>(k));
        return deriv_().zoom(lens);
    }

    template <typename T, 
             typename U = Deriv,
             std::enable_if_t<
                  std::is_same_v<
                    typename decltype(std::declval<U>().make())::value_type,
                    T
                  >,
               int> = 0>
    operator reader<T>() &&
    {
        return std::move(*this).make();
    }
    template <typename T>
    operator writer<T>() &&
    {
        return std::move(*this).make();
    }
    template <typename T, 
             typename U = Deriv,
             std::enable_if_t<
                  std::is_same_v<
                    typename decltype(std::declval<U>().make())::value_type,
                    T
                  >,
               int> = 0>
    operator cursor<T>() &&
    {
        return std::move(*this).make();
    }

    auto make() &&
    {
        auto node      = deriv_().make_node_();
        using node_t   = typename decltype(node)::element_type;
        using cursor_t = typename Deriv::template result_t<node_t>;
        return cursor_t{node};
    }

    template <typename TagT = transactional_tag, typename FnT>
    auto setter(FnT&& fn) &&
    {
        return std::move(*this).make().template setter<TagT>(
            std::forward<FnT>(fn));
    }

    auto make_node_() &&
    {
        using cursor_t = typename Deriv::template result_t<cursor_node<int>>;
        if constexpr (is_reader_base<cursor_t>::value) {
            return std::move(static_cast<Deriv&>(*this)).make_reader_node_();
        } else {
            return std::move(static_cast<Deriv&>(*this)).make_cursor_node_();
        }
    }
};

template <template <class> class Result, typename... Nodes>
class with_expr : public with_expr_base<with_expr<Result, Nodes...>>
{
    friend class with_expr_base<with_expr>;

    std::tuple<std::shared_ptr<Nodes>...> nodes_;

    template <typename T>
    using result_t = Result<T>;

    auto make_reader_node_() &&
    {
        return make_merge_reader_node(std::move(nodes_));
    }

    auto make_cursor_node_() &&
    {
        return make_merge_cursor_node(std::move(nodes_));
    }

public:
    with_expr(std::tuple<std::shared_ptr<Nodes>...>&& n)
        : nodes_{std::move(n)}
    {}

    template <typename Xf>
    auto xform(Xf&& xf) &&
    {
        return make_with_xform_expr(std::forward<Xf>(xf), std::move(nodes_));
    }

    template <typename Xf, typename WXf>
    auto xform(Xf&& xf, WXf&& wxf) &&
    {
        return make_with_wxform_expr<Result>(
            std::forward<Xf>(xf), std::forward<WXf>(wxf), std::move(nodes_));
    }

    template <typename Lens>
    auto zoom(Lens&& l) &&
    {
        return make_with_lens_expr<Result>(std::forward<Lens>(l),
                                           std::move(nodes_));
    }
};

template <typename Xform, typename... Nodes>
class with_xform_expr : public with_expr_base<with_xform_expr<Xform, Nodes...>>
{
    friend class with_expr_base<with_xform_expr>;

    Xform xform_;
    std::tuple<std::shared_ptr<Nodes>...> nodes_;

    template <typename T>
    using result_t = reader_base<T>;

    auto make_reader_node_() &&
    {
        return make_xform_reader_node(std::move(xform_), std::move(nodes_));
    }

public:
    template <typename Xf, typename Ns>
    with_xform_expr(Xf&& xf, Ns&& n)
        : xform_{std::forward<Xf>(xf)}
        , nodes_{std::forward<Ns>(n)}
    {}

    template <typename Xf>
    auto xform(Xf&& xf) &&
    {
        return make_with_xform_expr(zug::comp(std::move(xform_), xf),
                                    std::move(nodes_));
    }

    template <typename Lens>
    auto zoom(Lens&& l) &&
    {
        return std::move(*this).xform(zug::map([l](auto&&... xs) {
            return view(l, zug::tuplify(LAGER_FWD(xs)...));
        }));
    }
};

template <template <class> class Result,
          typename Xform,
          typename WXform,
          typename... Nodes>
class with_wxform_expr
    : public with_expr_base<with_wxform_expr<Result, Xform, WXform, Nodes...>>
{
    friend class with_expr_base<with_wxform_expr>;

    Xform xform_;
    WXform wxform_;
    std::tuple<std::shared_ptr<Nodes>...> nodes_;

    template <typename T>
    using result_t = Result<T>;

    auto make_reader_node_() &&
    {
        return make_xform_reader_node(std::move(xform_), nodes_);
    }

    auto make_cursor_node_() &&
    {
        return make_xform_cursor_node(
            std::move(xform_), std::move(wxform_), nodes_);
    }

public:
    template <typename Xf, typename WXf, typename Ns>
    with_wxform_expr(Xf&& xf, WXf&& wxf, Ns&& n)
        : xform_{std::forward<Xf>(xf)}
        , wxform_{std::forward<WXf>(wxf)}
        , nodes_{std::forward<Ns>(n)}
    {}

    template <typename Xf>
    auto xform(Xf&& xf) &&
    {
        return make_with_xform_expr(
            zug::comp(std::move(xform_), std::forward<Xf>(xf)),
            std::move(nodes_));
    }

    template <typename Xf, typename WXf>
    auto xform(Xf&& xf, WXf&& wxf) &&
    {
        return make_with_wxform_expr<Result>(
            zug::comp(std::move(xform_), std::forward<Xf>(xf)),
            zug::comp(std::forward<WXf>(wxf), std::move(wxform_)),
            std::move(nodes_));
    }

    template <typename Lens>
    auto zoom(Lens&& l) &&
    {
        return std::move(*this).make().zoom(std::forward<Lens>(l));
    }
};

template <template <class> class Result, typename Lens, typename... Nodes>
class with_lens_expr
    : public with_expr_base<with_lens_expr<Result, Lens, Nodes...>>
{
    friend class with_expr_base<with_lens_expr>;

    Lens lens_;
    std::tuple<std::shared_ptr<Nodes>...> nodes_;

    template <typename T>
    using result_t = Result<T>;

    auto make_reader_node_() &&
    {
        return make_lens_reader_node(std::move(lens_), nodes_);
    }

    auto make_cursor_node_() &&
    {
        return make_lens_cursor_node(std::move(lens_), nodes_);
    }

public:
    template <typename L, typename Ns>
    with_lens_expr(L&& l, Ns&& n)
        : lens_{std::forward<L>(l)}
        , nodes_{std::forward<Ns>(n)}
    {}

    template <typename Xf>
    auto xform(Xf&& xf) &&
    {
        return make_with_xform_expr(
            zug::comp(zug::map([l = std::move(lens_)](auto&&... xs) {
                          return view(l, zug::tuplify(LAGER_FWD(xs)...));
                      }),
                      std::forward<Xf>(xf)),
            std::move(nodes_));
    }

    template <typename Xf, typename WXf>
    auto xform(Xf&& xf, WXf&& wxf) &&
    {
        return std::move(*this).make().xform(std::forward<Xf>(xf),
                                             std::forward<WXf>(wxf));
    }

    template <typename Lens2>
    auto zoom(Lens2&& l) &&
    {
        return make_with_lens_expr<Result>(
            zug::comp(std::move(lens_), std::forward<Lens2>(l)),
            std::move(nodes_));
    }
};

template <typename... ReaderTs>
auto with_aux(reader_mixin<ReaderTs>&&... ins)
{
    return detail::make_with_expr<reader_base>(
        std::make_tuple(access::node(std::move(ins).make())...));
}
template <typename... ReaderTs>
auto with_aux(const reader_mixin<ReaderTs>&... ins)
{
    return detail::make_with_expr<reader_base>(
        std::make_tuple(access::node(ins.make())...));
}

template <typename... WriterTs>
auto with_aux(writer_mixin<WriterTs>&&... ins)
{
    return detail::make_with_expr<writer_base>(
        std::make_tuple(access::node(std::move(ins).make())...));
}
template <typename... WriterTs>
auto with_aux(const writer_mixin<WriterTs>&... ins)
{
    return detail::make_with_expr<writer_base>(
        std::make_tuple(access::node(std::move(ins))...));
}

template <typename... CursorTs>
auto with_aux(cursor_mixin<CursorTs>&&... ins)
{
    return detail::make_with_expr<cursor_base>(
        std::make_tuple(access::node(std::move(ins).make())...));
}
template <typename... CursorTs>
auto with_aux(const cursor_mixin<CursorTs>&... ins)
{
    return detail::make_with_expr<cursor_base>(
        std::make_tuple(access::node(ins.make())...));
}

} // namespace detail

//! @defgroup cursors
//! @{

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
template <typename... Cursors>
auto with(Cursors&&... ins)
{
    return detail::with_aux(std::forward<Cursors>(ins).make()...);
}

//! @}

} // namespace lager
