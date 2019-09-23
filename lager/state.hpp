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
#include <lager/detail/root_signals.hpp>
#include <lager/detail/watchable.hpp>

#include <lager/util.hpp>

namespace lager {

template <typename T>
class state : private detail::watchable<T>
{
    using signal_ptr_t = decltype(detail::make_state_signal(std::declval<T>()));

public:
    using value_type = T;

    state()
        : signal_(detail::make_state_signal(T()))
    {}
    state(T value)
        : signal_(detail::make_state_signal(std::move(value)))
    {}

    state(const state&) = delete;
    state(state&&)      = default;
    state& operator=(const state&) = delete;
    state& operator=(state&&) = default;

    template <typename T2>
    void set(T2&& value)
    {
        signal_->push_down(std::forward<T2>(value));
    }

    const T& get() const { return signal_->last(); }

private:
    const signal_ptr_t& signal() { return signal_; }
    const signal_ptr_t& roots() { return signal_; }

    friend class detail::access;
    signal_ptr_t signal_;
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
