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
#include <lager/detail/signals.hpp>
#include <lager/detail/watchable.hpp>

#include <zug/meta/value_type.hpp>

namespace lager {

namespace detail {

template <typename SignalT>
class cursor_impl : private watchable<zug::meta::value_t<SignalT>>
{
    template <typename T>
    friend class cursor_impl;
    friend class detail::access;

    using base_t = watchable<zug::meta::value_t<SignalT>>;

    using signal_ptr_t = std::shared_ptr<SignalT>;
    signal_ptr_t signal_;
    const signal_ptr_t& signal() const { return signal_; }

public:
    using value_type = zug::meta::value_t<SignalT>;

    cursor_impl()              = default;
    cursor_impl(cursor_impl&&) = default;
    cursor_impl& operator=(cursor_impl&&) = default;
    cursor_impl(const cursor_impl&)       = default;
    cursor_impl& operator=(const cursor_impl&) = default;

    template <typename T>
    cursor_impl(cursor_impl<T> x)
        : base_t(std::move(x))
        , signal_(std::move(x.signal_))
    {}

    template <typename SignalT2>
    cursor_impl(std::shared_ptr<SignalT2> sig)
        : signal_(std::move(sig))
    {}

    decltype(auto) get() const { return signal_->last(); }

    template <typename T>
    void set(T&& value)
    {
        return signal_->send_up(std::forward<T>(value));
    }
};

} // namespace detail

/*!
 * Provides access to reading and writing values of type `T`.
 * Model of `cursor_value`.
 * @see `cursor_value`
 */
template <typename T>
class cursor : public detail::cursor_impl<detail::up_down_signal<T>>
{
    using base_t = detail::cursor_impl<detail::up_down_signal<T>>;
    using base_t::base_t;
};

} // namespace lager
