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

#include <lager/detail/signal.hpp>

#include <zug/meta/value_type.hpp>

#include <memory>

namespace lager {

template <typename NodeT>
class watchable_base : private NodeT::signal_type::forwarder_type
{
    using node_ptr_t   = std::shared_ptr<NodeT>;
    using value_t      = zug::meta::value_t<NodeT>;
    using base_t       = typename NodeT::signal_type::forwarder_type;
    using connection_t = typename base_t::connection;

    node_ptr_t node_;
    std::vector<connection_t> conns_;

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

    template <typename T,
              std::enable_if_t<std::is_same_v<value_t, zug::meta::value_t<T>>,
                               int> = 0>
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
        base_t::unlink();
        node_ = other.node_;
        if (!base_t::empty() && node_)
            node_->observers().add(*this);
        return *this;
    }

    watchable_base& operator=(watchable_base&& other) noexcept
    {
        base_t::unlink();
        node_ = std::move(other.node_);
        if (!base_t::empty() && node_)
            node_->observers().add(*this);
        return *this;
    }

    template <typename CallbackT>
    auto&& watch(CallbackT&& callback)
    {
        if (base_t::empty() && node_)
            node_->observers().add(*this);
        conns_.push_back(base_t::connect(std::forward<CallbackT>(callback)));
        return *this;
    }

    template <typename CallbackT>
    auto&& bind(CallbackT&& callback)
    {
        callback(node()->last());
        return watch(std::forward<CallbackT>(callback));
    }

    void nudge() { base_t::operator()(node()->last()); }

    void unbind()
    {
        conns_.clear();
        base_t::unlink();
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
