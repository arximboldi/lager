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

template <typename SensorFnT>
class sensor_node
    : public reader_node<std::decay_t<std::result_of_t<SensorFnT()>>>
{
    using base_t = reader_node<std::decay_t<std::result_of_t<SensorFnT()>>>;

public:
    sensor_node(SensorFnT sensor)
        : base_t(sensor())
        , sensor_(std::move(sensor))
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

template <typename SensorFnT>
class sensor : public reader_base<detail::sensor_node<SensorFnT>>
{
    using base_t = reader_base<detail::sensor_node<SensorFnT>>;
    using signal_ptr_t =
        decltype(detail::make_sensor_node(std::declval<SensorFnT>()));

public:
    using value_type = std::decay_t<std::result_of_t<SensorFnT()>>;
    using base_t::base_t;

    sensor()
        : base_t{detail::make_sensor_node(SensorFnT())}
    {}
    sensor(SensorFnT fn)
        : base_t{detail::make_sensor_node(std::move(fn))}
    {}

private:
    friend class detail::access;
    auto roots() const { return detail::access::node(*this); }
};

template <typename SensorFnT>
sensor<SensorFnT> make_sensor(SensorFnT fn)
{
    return fn;
}

} // namespace lager
