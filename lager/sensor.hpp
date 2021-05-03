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

#include <lager/commit.hpp>
#include <lager/reader.hpp>

namespace lager {

namespace detail {

template <typename T>
class sensor_node_base : public root_node<T, reader_node>
{
    using base_t = root_node<T, reader_node>;

public:
    using base_t::base_t;
};

template <typename SensorFnT>
class sensor_node
    : public sensor_node_base<std::decay_t<std::invoke_result_t<SensorFnT>>>
{
    using base_t =
        sensor_node_base<std::decay_t<std::invoke_result_t<SensorFnT>>>;

public:
    sensor_node(SensorFnT sensor)
        : base_t{sensor()}
        , sensor_{std::move(sensor)}
    {}

    void recompute() final { this->push_down(sensor_()); }

private:
    SensorFnT sensor_;
};

template <typename SensorFnT>
auto make_sensor_node(SensorFnT&& fn)
{
    return std::make_shared<sensor_node<std::decay_t<SensorFnT>>>(
        std::forward<SensorFnT>(fn));
}

} // namespace detail

//! @defgroup cursors
//! @{

template <typename T>
class sensor : public reader_base<detail::sensor_node_base<T>>
{
    using base_t = reader_base<detail::sensor_node_base<T>>;

public:
    using value_type = T;

    template <
        typename Fn,
        std::enable_if_t<
            std::is_same_v<std::decay_t<std::invoke_result_t<Fn>>, value_type>,
            int> = 0>
    sensor(Fn&& fn)
        : base_t{detail::make_sensor_node(std::forward<Fn>(fn))}
    {}

private:
    friend class detail::access;
    auto roots() const { return detail::access::node(*this); }
};

template <typename SensorFnT>
auto make_sensor(SensorFnT&& fn)
    -> sensor<std::decay_t<std::invoke_result_t<SensorFnT>>>
{
    return std::forward<SensorFnT>(fn);
}

//! @}

} // namespace lager
