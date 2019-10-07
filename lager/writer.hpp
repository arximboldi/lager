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
#include <lager/watch.hpp>

namespace lager {

template <typename NodeT>
class cursor_base;

template <typename DerivT>
struct writer_mixin
{
    template <typename T>
    void set(T&& value)
    {
        return node()->send_up(std::forward<T>(value));
    }

    template <typename Fn>
    void update(Fn&& fn)
    {
        return node()->send_up(std::forward<Fn>(fn)(node()->current()));
    }

    template <typename T>
    auto operator[](T t) const
    {
        return atted(std::move(t), *this);
    }

    template <typename T, typename U>
    auto operator[](T U::*member) const
    {
        return attred(member, *this);
    }

    template <typename Xform, typename Xform2>
    auto xf(Xform&& xf, Xform2&& xf2)
    {
        return xform(xf, xf2)(*this);
    }

protected:
    ~writer_mixin() = default;

private:
    friend class detail::access;

    auto node() const
    {
        return detail::access::node(*static_cast<const DerivT*>(this));
    }
};

template <typename NodeT>
class writer_base : public writer_mixin<writer_base<NodeT>>
{
    template <typename T>
    friend class writer_base;
    friend class detail::access;

    using node_ptr_t = std::shared_ptr<NodeT>;
    node_ptr_t node_;
    const node_ptr_t& node() const { return node_; }

public:
    using value_type = zug::meta::value_t<NodeT>;

    writer_base() = default;

    template <typename T>
    writer_base(writer_base<T> x)
        : node_(std::move(x.node_))
    {}

    template <typename T>
    writer_base(cursor_base<T> x)
        : node_(detail::access::node(std::move(x)))
    {}

    writer_base(node_ptr_t sig)
        : node_(std::move(sig))
    {}
};

/*!
 * Provides access to writing values of type `T`.
 * Model of `Writer_value`.
 * @see `Writer_value`
 */
template <typename T>
class writer : public writer_base<detail::cursor_node<T>>
{
    using base_t = writer_base<detail::cursor_node<T>>;

public:
    using base_t::base_t;
};

} // namespace lager
