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

#include <lager/detail/access.hpp>
#include <lager/detail/nodes.hpp>
#include <lager/detail/smart_lens.hpp>

#include <lager/watch.hpp>
#include <lager/xform.hpp>

#include <zug/meta/value_type.hpp>

namespace lager {

template <typename NodeT>
class cursor_base;

template <typename DerivT>
struct reader_mixin
{
    decltype(auto) get() const { return node()->last(); }

    template <typename T>
    auto operator[](T&& t) const
    {
        using value_t = typename DerivT::value_type;
        auto l        = detail::smart_lens<value_t>::make(std::forward<T>(t));
        return zoom(l, *this);
    }

    template <typename Xform>
    auto xf(Xform&& xf) const
    {
        return xform(xf)(*this);
    }

protected:
    ~reader_mixin() = default;

private:
    friend class detail::access;

    auto node() const
    {
        return detail::access::node(*static_cast<const DerivT*>(this));
    }
};

template <typename NodeT>
class reader_base
    : public reader_mixin<reader_base<NodeT>>
    , private detail::watchable<zug::meta::value_t<NodeT>>
{
    template <typename T>
    friend class reader_base;
    friend class detail::access;

    using base_t     = detail::watchable<zug::meta::value_t<NodeT>>;
    using node_ptr_t = std::shared_ptr<NodeT>;

    node_ptr_t node_;
    const node_ptr_t& node() const { return node_; }

public:
    using value_type = zug::meta::value_t<NodeT>;

    reader_base() = default;

    template <typename T>
    reader_base(reader_base<T> x)
        : base_t(std::move(x))
        , node_(std::move(x.node_))
    {}

    template <typename T>
    reader_base(cursor_base<T> x)
        : node_(detail::access::node(std::move(x)))
    {}

    template <typename NodeT2>
    reader_base(std::shared_ptr<NodeT2> sig)
        : node_(std::move(sig))
    {}
};

/*!
 * Provides access to reading values of type `T`.
 * Model of `Reader_value`.
 * @see `Reader_value`
 */
template <typename T>
class reader : public reader_base<detail::reader_node<T>>
{
    using base_t = reader_base<detail::reader_node<T>>;

public:
    using base_t::base_t;
};

} // namespace lager
