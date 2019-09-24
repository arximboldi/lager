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
#include <lager/reader.hpp>

namespace lager {

template <typename SensorFnT>
class sensor : public detail::reader_impl<detail::sensor_down_signal<SensorFnT>>
{
    using base_t = detail::reader_impl<detail::sensor_down_signal<SensorFnT>>;
    using signal_ptr_t =
        decltype(detail::make_sensor_signal(std::declval<SensorFnT>()));

public:
    using value_type = std::decay_t<std::result_of_t<SensorFnT()>>;
    using base_t::base_t;

    sensor()
        : base_t{detail::make_sensor_signal(SensorFnT())}
    {}
    sensor(SensorFnT fn)
        : base_t{detail::make_sensor_signal(std::move(fn))}
    {}

private:
    friend class detail::access;
    auto roots() const { return detail::access::signal(*this); }
};

template <typename SensorFnT>
sensor<SensorFnT> make_sensor(SensorFnT fn)
{
    return fn;
}

} // namespace lager
