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

#include <lager/detail/signals.hpp>

namespace lager {

namespace detail {

/*!
 * Add signal that will always send the current value returned by a
 * given function.
 */
template <typename SensorFnT>
class sensor_down_signal
    : public down_signal<std::decay_t<std::result_of_t<SensorFnT()>>>
{
    using base_t = down_signal<std::decay_t<std::result_of_t<SensorFnT()>>>;

public:
    sensor_down_signal(SensorFnT sensor)
        : base_t(sensor())
        , sensor_(std::move(sensor))
    {}

    void recompute() final { this->push_down(sensor_()); }

private:
    SensorFnT sensor_;
};

/*!
 * Make a StateUpDownSignal with deduced types.
 */
template <typename SensorFnT>
auto make_sensor_signal(SensorFnT&& fn)
    -> std::shared_ptr<sensor_down_signal<std::decay_t<SensorFnT>>>
{
    return std::make_shared<sensor_down_signal<std::decay_t<SensorFnT>>>(
        std::forward<SensorFnT>(fn));
}

/*!
 * Root signal that serves as state.
 */
template <typename T>
class state_up_down_signal : public up_down_signal<T>
{
public:
    using value_type = T;

    using up_down_signal<T>::up_down_signal;

    void send_up(const value_type& value) final { this->push_down(value); }

    void send_up(value_type&& value) final
    {
        this->push_down(std::move(value));
    }
};

/*!
 * Make a StateUpDownSignal with deduced types.
 */
template <typename T>
auto make_state_signal(T&& value)
    -> std::shared_ptr<state_up_down_signal<std::decay_t<T>>>
{
    return std::make_shared<state_up_down_signal<std::decay_t<T>>>(
        std::forward<T>(value));
}

} // namespace detail

} // namespace lager
