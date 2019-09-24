//
// Copyright (C) 2014, 2015 Ableton AG, Berlin. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

/*!
 * @file
 */

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

ZUG_INLINE_CONSTEXPR struct send_down_t
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
} send_down{};

template <typename T, typename Err>
auto default_construct_or_throw()
    -> std::enable_if_t<std::is_default_constructible<T>::value, T>
{
    return T();
}

template <typename T, typename Err>
auto default_construct_or_throw()
    -> std::enable_if_t<!std::is_default_constructible<T>::value, T>
{
    throw Err();
}

/*!
 * Implementation of a node with a transducer.
 */
template <typename XForm              = zug::identity_t,
          typename ParentsPack        = zug::meta::pack<>,
          template <class> class Base = reader_node>
class xform_reader_node;

template <typename XForm, typename... Parents, template <class> class Base>
class xform_reader_node<XForm, zug::meta::pack<Parents...>, Base>
    : public Base<zug::result_of_t<XForm, zug::meta::value_t<Parents>...>>
{
    using base_t =
        Base<zug::result_of_t<XForm, zug::meta::value_t<Parents>...>>;
    using down_rf_t = decltype(std::declval<XForm>()(send_down));

    std::tuple<std::shared_ptr<Parents>...> parents_;

public:
    using value_type = typename base_t::value_type;

    xform_reader_node(xform_reader_node&&)      = default;
    xform_reader_node(const xform_reader_node&) = delete;
    xform_reader_node& operator=(xform_reader_node&&) = default;
    xform_reader_node& operator=(const xform_reader_node&) = delete;

    template <typename XForm2>
    xform_reader_node(XForm2&& xform, std::shared_ptr<Parents>... parents)
        : base_t([&]() -> value_type {
            try {
                return xform(zug::last)(detail::no_value{},
                                        parents->current()...);
            } catch (const no_value_error&) {
                return default_construct_or_throw<value_type, no_value_error>();
            }
        }())
        , parents_(std::move(parents)...)
        , down_step_(xform(send_down))
    {}

    void recompute() /* final */
    {
        recompute(std::make_index_sequence<sizeof...(Parents)>{});
    }

    void recompute_deep() /* final */
    {
        recompute_deep(std::make_index_sequence<sizeof...(Parents)>{});
    }

    std::tuple<std::shared_ptr<Parents>...>& parents() { return parents_; }
    const std::tuple<std::shared_ptr<Parents>...>& parents() const
    {
        return parents_;
    }

private:
    template <std::size_t... Indices>
    void recompute(std::index_sequence<Indices...>)
    {
        down_step_(this, std::get<Indices>(parents_)->current()...);
    }

    template <std::size_t... Indices>
    void recompute_deep(std::index_sequence<Indices...>)
    {
        noop((std::get<Indices>(parents_)->recompute_deep(), 0)...);
        recompute();
    }

    down_rf_t down_step_;
};

/*!
 * Reducing function that pushes the received values into the node
 * that is passed as pointer as an accumulator.
 */
constexpr struct
{
    template <typename WriterNodePtr, typename... Inputs>
    auto operator()(WriterNodePtr s, Inputs&&... is) const -> WriterNodePtr
    {
        s->push_up(zug::tuplify(std::forward<Inputs>(is)...));
        return s;
    }
} send_up_rf{};

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
 * Implementation of a node with a transducer
 */
template <typename XForm              = zug::identity_t,
          typename SetXForm           = zug::identity_t,
          typename ParentsPack        = zug::meta::pack<>,
          template <class> class Base = cursor_node>
class xform_cursor_node;

template <typename XForm,
          typename SetXForm,
          typename... Parents,
          template <class>
          class Base>
class xform_cursor_node<XForm, SetXForm, zug::meta::pack<Parents...>, Base>
    : public xform_reader_node<XForm, zug::meta::pack<Parents...>, Base>
{
    using base_t  = xform_reader_node<XForm, zug::meta::pack<Parents...>, Base>;
    using up_rf_t = decltype(std::declval<SetXForm>()(send_up_rf));

public:
    using value_type = typename base_t::value_type;

    xform_cursor_node(xform_cursor_node&&)      = default;
    xform_cursor_node(const xform_cursor_node&) = delete;
    xform_cursor_node& operator=(xform_cursor_node&&) = default;
    xform_cursor_node& operator=(const xform_cursor_node&) = delete;

    template <typename XForm2, typename SetXForm2>
    xform_cursor_node(XForm2&& xform,
                      SetXForm2&& set_xform,
                      std::shared_ptr<Parents>... parents)
        : base_t(std::forward<XForm2>(xform), std::move(parents)...)
        , up_step_(set_xform(send_up_rf))
    {}

    void send_up(const value_type& value) final
    {
        send_up(value, std::make_index_sequence<sizeof...(Parents)>{});
    }

    void send_up(value_type&& value) final
    {
        send_up(std::move(value),
                std::make_index_sequence<sizeof...(Parents)>{});
    }

    template <typename T>
    void push_up(T&& value)
    {
        push_up(std::forward<T>(value),
                std::make_index_sequence<sizeof...(Parents)>{});
    }

private:
    template <typename T, std::size_t... Indices>
    void send_up(T&& x, std::index_sequence<Indices...>)
    {
        up_step_(this, std::forward<T>(x));
    }

    template <typename T, std::size_t... Indices>
    void push_up(T&& value, std::index_sequence<Indices...>)
    {
        auto& parents = this->parents();
        noop((std::get<Indices>(parents)->send_up(
                  std::get<Indices>(std::forward<T>(value))),
              0)...);
    }

    template <typename T>
    void push_up(T&& value, std::index_sequence<0>)
    {
        std::get<0>(this->parents())->send_up(std::forward<T>(value));
    }

    up_rf_t up_step_;
};

/*!
 * Links a node to its parents and returns it.
 */
template <typename NodeT>
auto link_to_parents(std::shared_ptr<NodeT> node) -> std::shared_ptr<NodeT>
{
    return link_to_parents(
        std::move(node),
        std::make_index_sequence<
            std::tuple_size<std::decay_t<decltype(node->parents())>>::value>{});
}

template <typename NodeT, std::size_t... Indices>
auto link_to_parents(std::shared_ptr<NodeT> node,
                     std::index_sequence<Indices...>) -> std::shared_ptr<NodeT>
{
    auto& parents = node->parents();
    noop((std::get<Indices>(parents)->link(node), 0)...);
    return node;
}

/*!
 * Make a xform_reader_node with deduced types.
 */
template <typename XForm, typename... Parents>
auto make_xform_reader_node(XForm&& xform, std::shared_ptr<Parents>... parents)
    -> std::shared_ptr<
        xform_reader_node<std::decay_t<XForm>, zug::meta::pack<Parents...>>>
{
    using node_t =
        xform_reader_node<std::decay_t<XForm>, zug::meta::pack<Parents...>>;
    return link_to_parents(std::make_shared<node_t>(std::forward<XForm>(xform),
                                                    std::move(parents)...));
}

/*!
 * Make a xform_reader_node with deduced types.
 */
template <typename XForm, typename SetXForm, typename... Parents>
auto make_xform_cursor_node(XForm&& xform,
                            SetXForm&& set_xform,
                            std::shared_ptr<Parents>... parents)
    -> std::shared_ptr<xform_cursor_node<std::decay_t<XForm>,
                                         std::decay_t<SetXForm>,
                                         zug::meta::pack<Parents...>>>
{
    using node_t = xform_cursor_node<std::decay_t<XForm>,
                                     std::decay_t<SetXForm>,
                                     zug::meta::pack<Parents...>>;
    return link_to_parents(
        std::make_shared<node_t>(std::forward<XForm>(xform),
                                 std::forward<SetXForm>(set_xform),
                                 std::move(parents)...));
}

} // namespace detail

} // namespace lager
