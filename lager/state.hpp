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

#include <lager/cursor.hpp>
#include <lager/detail/access.hpp>
#include <lager/detail/root_signals.hpp>
#include <lager/detail/watchable.hpp>

#include <lager/util.hpp>

namespace lager {

template <typename T>
class state : public detail::cursor_impl<detail::state_up_down_signal<T>>
{
    using base_t = detail::cursor_impl<detail::state_up_down_signal<T>>;

    friend class detail::access;
    auto roots() const { return detail::access::signal(*this); }

public:
    using value_type = T;
    using base_t::base_t;

    state()
        : base_t{detail::make_state_signal(T())}
    {}
    state(T value)
        : base_t{detail::make_state_signal(std::move(value))}
    {}

    state& operator=(const state&) = delete;
    state(const state&)            = delete;

    state(state&&) = default;
    state& operator=(state&&) = default;
};

template <typename T>
state<T> make_state(T value)
{
    return value;
}

template <typename... RootValueTs>
void commit(RootValueTs&&... roots)
{
    noop((detail::access::roots(std::forward<RootValueTs>(roots))->send_down(),
          0)...);
    noop((detail::access::roots(std::forward<RootValueTs>(roots))->notify(),
          0)...);
}

template <typename InputValueT, typename CallbackT>
auto watch(InputValueT&& value, CallbackT&& callback)
{
    auto& watchers = detail::access::watchers(std::forward<InputValueT>(value));
    if (watchers.empty()) {
        auto& observers =
            detail::access::signal(std::forward<InputValueT>(value))
                ->observers();
        observers.connect(watchers);
    }
    return watchers.connect(std::forward<CallbackT>(callback));
}

} // namespace lager
