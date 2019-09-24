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

template <typename NodeT>
class writer_base
{
    template <typename T>
    friend class writer_base;
    friend class detail::access;

    using node_ptr_t = std::shared_ptr<NodeT>;
    node_ptr_t node_;
    const node_ptr_t& node() const { return node_; }

public:
    using value_type = zug::meta::value_t<NodeT>;

    writer_base()              = default;
    writer_base(writer_base&&) = default;
    writer_base& operator=(writer_base&&) = default;
    writer_base(const writer_base&)       = default;
    writer_base& operator=(const writer_base&) = default;

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

    template <typename T>
    void set(T&& value)
    {
        return node_->send_up(std::forward<T>(value));
    }
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
