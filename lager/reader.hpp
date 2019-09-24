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
class reader_impl : private watchable<zug::meta::value_t<SignalT>>
{
    template <typename T>
    friend class reader_impl;
    friend class detail::access;

    using base_t       = watchable<zug::meta::value_t<SignalT>>;
    using signal_ptr_t = std::shared_ptr<SignalT>;

    signal_ptr_t signal_;
    const signal_ptr_t& signal() const { return signal_; }

public:
    using value_type = zug::meta::value_t<SignalT>;

    reader_impl()              = default;
    reader_impl(reader_impl&&) = default;
    reader_impl& operator=(reader_impl&&) = default;
    reader_impl(const reader_impl&)       = default;
    reader_impl& operator=(const reader_impl&) = default;

    template <typename T>
    reader_impl(reader_impl<T> x)
        : base_t(std::move(x))
        , signal_(std::move(x.signal_))
    {}

    template <typename SignalT2>
    reader_impl(std::shared_ptr<SignalT2> sig)
        : signal_(std::move(sig))
    {}

    decltype(auto) get() const { return signal_->last(); }
};

} // namespace detail

/*!
 * Provides access to reading values of type `T`.
 * Model of `Reader_value`.
 * @see `Reader_value`
 */
template <typename T>
class reader : public detail::reader_impl<detail::down_signal<T>>
{
    using base_t = detail::reader_impl<detail::down_signal<T>>;

public:
    using base_t::base_t;
    using base_t::operator=;
};

} // namespace lager
