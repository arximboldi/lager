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

#include <zug/meta/value_type.hpp>

namespace lager {

template <typename NodeT>
class cursor_base;

template <typename NodeT>
class reader_base : private detail::watchable<zug::meta::value_t<NodeT>>
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

    reader_base()              = default;
    reader_base(reader_base&&) = default;
    reader_base& operator=(reader_base&&) = default;
    reader_base(const reader_base&)       = default;
    reader_base& operator=(const reader_base&) = default;

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

    decltype(auto) get() const { return node_->last(); }
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
