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

#include <boost/asio/io_service.hpp>
#include <thread>
#include <functional>

namespace lager {

struct with_boost_asio_event_loop
{
    std::reference_wrapper<boost::asio::io_service> service;
    std::function<void()> finalizer = {};

    template <typename Fn>
    void async(Fn&& fn)
    {
        std::thread([fn=std::move(fn),
                     work=boost::asio::io_service::work(service)] {
            fn();
        }).detach();
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        service.get().post(std::forward<Fn>(fn));
    }

    void finish()
    {
        if (finalizer) finalizer();
        else service.get().stop();
    }
};

} // namespace lager
