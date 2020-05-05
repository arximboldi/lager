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

#include <boost/signals2/signal.hpp>

#include <zug/meta/value_type.hpp>

#include <memory>

namespace lager {

template <typename NodeT>
class watchable_base
{
    using node_ptr_t = std::shared_ptr<NodeT>;
    using value_t    = zug::meta::value_t<NodeT>;
    using signal_t =
        boost::signals2::signal<void(const value_t&, const value_t&)>;
    using connection_t = boost::signals2::scoped_connection;

    node_ptr_t node_;
    signal_t signal_;
    connection_t conn_;

    const node_ptr_t& node() const& { return node_; }
    node_ptr_t&& node() && { return std::move(node_); }

    friend class detail::access;
    template <typename T>
    friend class watchable_base;

protected:
    watchable_base() = default;
    watchable_base(node_ptr_t p)
        : node_{std::move(p)}
    {}

    template <typename T>
    watchable_base(watchable_base<T> x)
        : node_(std::move(x.node_))
    {}

    template <typename NodeT2>
    watchable_base(std::shared_ptr<NodeT2> n)
        : node_{std::move(n)}
    {}

public:
    watchable_base(const watchable_base& other) noexcept
        : node_(other.node_)
    {}

    watchable_base(watchable_base&& other) noexcept
        : node_{std::move(other.node_)}
    {}

    watchable_base& operator=(const watchable_base& other) noexcept
    {
        node_ = other.node_;
        if (!signal_.empty() && node_) {
            conn_ = node_->observers().connect(signal_);
        }
        return *this;
    }

    watchable_base& operator=(watchable_base&& other) noexcept
    {
        node_ = std::move(other.node_);
        if (!signal_.empty() && node_) {
            conn_ = node_->observers().connect(signal_);
        }
        return *this;
    }

    template <typename CallbackT>
    auto watch(CallbackT&& callback)
    {
        if (signal_.empty() && node_)
            conn_ = node_->observers().connect(signal_);
        return signal_.connect(std::forward<CallbackT>(callback));
    }
};

/*!
 * Watch changes through a reader using callback @callback.
 */
template <typename ReaderT, typename CallbackT>
auto watch(ReaderT&& value, CallbackT&& callback)
{
    return value.watch(std::forward<CallbackT>(callback));
}

} // namespace lager
