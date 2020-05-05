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
            s->recompute_deep();
            return step(s, updater(current_from(s->parents()), ZUG_FWD(is)...));
        };
    };
}

namespace detail {

template <template <class> class Result>
struct is_reader_base : std::false_type
{};

template <>
struct is_reader_base<reader_base> : std::true_type
{};

template <bool IsReader, typename Expr>
struct get_node_type_aux
{
    using type = typename decltype(
        std::declval<Expr>().make_reader_node_())::element_type;
};
template <typename Expr>
struct get_node_type_aux<false, Expr>
{
    using type = typename decltype(
        std::declval<Expr>().make_cursor_node_())::element_type;
};

template <template <class> class Result, typename Expr>
using get_node_type =
    typename get_node_type_aux<is_reader_base<Result>::value,
                               typename Expr::template apply<Result>>::type;

template <template <class> class Result, typename Expr>
class with_expr_builder;

template <typename... Nodes>
struct with_expr;
template <typename Xform, typename... Nodes>
struct with_xform_expr;
template <typename Xform, typename WXform, typename... Nodes>
struct with_wxform_expr;
template <typename Lens, typename... Nodes>
struct with_lens_expr;

template <template <typename Node> class Result, typename... Nodes>
auto make_with_expr(std::tuple<std::shared_ptr<Nodes>...> nodes)
    -> with_expr_builder<Result, with_expr<Nodes...>>
{
    return {std::move(nodes)};
}

template <typename Xform, typename... Nodes>
auto make_with_xform_expr(Xform xform,
                          std::tuple<std::shared_ptr<Nodes>...> nodes)
    -> with_expr_builder<reader_base, with_xform_expr<Xform, Nodes...>>
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
    -> with_expr_builder<Result, with_wxform_expr<Xform, WXform, Nodes...>>
{
    return {std::move(xform), std::move(wxform), std::move(nodes)};
}

template <template <class> class Result, typename Lens, typename... Nodes>
auto make_with_lens_expr(Lens lens, std::tuple<std::shared_ptr<Nodes>...> nodes)
    -> with_expr_builder<Result, with_lens_expr<Lens, Nodes...>>
{
    return {std::move(lens), std::move(nodes)};
}

template <typename... Nodes>
struct with_expr
{
    template <template <class> class Result>
    class apply
    {
        template <template <class> class T, typename E>
        friend class with_expr_builder;
        template <bool IsReader, typename Expr>
        friend struct get_node_type_aux;

        std::tuple<std::shared_ptr<Nodes>...> nodes_;

        auto make_reader_node_() &&
        {
            return make_merge_reader_node(std::move(nodes_));
        }

        auto make_cursor_node_() &&
        {
            return make_merge_cursor_node(std::move(nodes_));
        }

    public:
        apply(std::tuple<std::shared_ptr<Nodes>...>&& n)
            : nodes_{std::move(n)}
        {}

        template <typename Xf>
        auto xform(Xf&& xf) &&
        {
            return make_with_xform_expr(std::forward<Xf>(xf),
                                        std::move(nodes_));
        }

        template <typename Xf, typename WXf>
        auto xform(Xf&& xf, WXf&& wxf) &&
        {
            return make_with_wxform_expr<Result>(std::forward<Xf>(xf),
                                                 std::forward<WXf>(wxf),
                                                 std::move(nodes_));
        }

        template <typename Lens>
        auto zoom(Lens&& l) &&
        {
            return make_with_lens_expr<Result>(std::forward<Lens>(l),
                                               std::move(nodes_));
        }
    };
};

template <typename Xform, typename... Nodes>
struct with_xform_expr
{
    template <template <class> class Result>
    class apply
    {
        template <template <class> class T, typename E>
        friend class with_expr_builder;
        template <bool IsReader, typename Expr>
        friend struct get_node_type_aux;

        Xform xform_;
        std::tuple<std::shared_ptr<Nodes>...> nodes_;

        auto make_reader_node_() &&
        {
            return make_xform_reader_node(std::move(xform_), std::move(nodes_));
        }

    public:
        template <typename Xf, typename Ns>
        apply(Xf&& xf, Ns&& n)
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
};

template <typename Xform, typename WXform, typename... Nodes>
struct with_wxform_expr
{
    template <template <class> class Result>
    class apply
    {
        template <template <class> class T, typename E>
        friend class with_expr_builder;
        template <bool IsReader, typename Expr>
        friend struct get_node_type_aux;

        Xform xform_;
        WXform wxform_;
        std::tuple<std::shared_ptr<Nodes>...> nodes_;

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
        apply(Xf&& xf, WXf&& wxf, Ns&& n)
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
};

template <typename Lens, typename... Nodes>
struct with_lens_expr
{
    template <template <class> class Result>
    class apply
    {
        template <template <class> class T, typename E>
        friend class with_expr_builder;
        template <bool IsReader, typename Expr>
        friend struct get_node_type_aux;

        Lens lens_;
        std::tuple<std::shared_ptr<Nodes>...> nodes_;

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
        apply(L&& l, Ns&& n)
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
};

template <template <class> class Result, typename Expr>
class with_expr_builder : public Expr::template apply<Result>
{
    using base_t = typename Expr::template apply<Result>;

    using node_t   = get_node_type<Result, Expr>;
    using cursor_t = Result<node_t>;

    std::shared_ptr<node_t> make_node_() &&
    {
        if constexpr (is_reader_base<Result>::value) {
            return std::move(*this).make_reader_node_();
        } else {
            return std::move(*this).make_cursor_node_();
        }
    }

public:
    using base_t::base_t;

    template <typename T>
    auto operator[](T&& k) &&
    {
        using value_t = typename decltype(std::move(*this).make())::value_type;
        auto lens     = smart_lens<value_t>::make(std::forward<T>(k));
        return std::move(*this).zoom(lens);
    }

    cursor_t make() && { return std::move(*this).make_node_(); }

    operator cursor_t() && { return std::move(*this).make(); }

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
auto with(reader_base<ReaderTs>... ins)
{
    return detail::make_with_expr<reader_base>(
        std::make_tuple(detail::access::node(std::move(ins))...));
}

template <typename... WriterTs>
auto with(writer_base<WriterTs>... ins)
{
    return detail::make_with_expr<writer_base>(
        std::make_tuple(detail::access::node(std::move(ins))...));
}

template <typename... CursorTs>
auto with(cursor_base<CursorTs>... ins)
{
    return detail::make_with_expr<cursor_base>(
        std::make_tuple(detail::access::node(std::move(ins))...));
}

} // namespace lager
