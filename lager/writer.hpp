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

namespace lager {

namespace detail {

template <typename SignalT>
class cursor_impl;

template <typename SignalT>
class writer_impl
{
    template <typename T>
    friend class writer_impl;
    friend class detail::access;

    using signal_ptr_t = std::shared_ptr<SignalT>;
    signal_ptr_t signal_;
    const signal_ptr_t& signal() const { return signal_; }

public:
    using value_type = zug::meta::value_t<SignalT>;

    writer_impl()              = default;
    writer_impl(writer_impl&&) = default;
    writer_impl& operator=(writer_impl&&) = default;
    writer_impl(const writer_impl&)       = default;
    writer_impl& operator=(const writer_impl&) = default;

    template <typename T>
    writer_impl(writer_impl<T> x)
        : signal_(std::move(x.signal_))
    {}

    template <typename T>
    writer_impl(cursor_impl<T> x)
        : signal_(detail::access::signal(std::move(x)))
    {}

    writer_impl(signal_ptr_t sig)
        : signal_(std::move(sig))
    {}

    template <typename T>
    void set(T&& value)
    {
        return signal_->send_up(std::forward<T>(value));
    }
};

} // namespace detail

/*!
 * Provides access to writing values of type `T`.
 * Model of `Writer_value`.
 * @see `Writer_value`
 */
template <typename T>
class writer : public detail::writer_impl<detail::up_down_signal<T>>
{
    using base_t = detail::writer_impl<detail::up_down_signal<T>>;

public:
    using base_t::base_t;
};

} // namespace lager
