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

#include <catch2/catch.hpp>

#include <lager/event_loop/boost_asio.hpp>
#include <lager/store.hpp>

#include "example/counter/counter.hpp"

TEST_CASE("basic")
{
    auto ctx   = boost::asio::io_context{};
    auto store = lager::make_store<counter::action>(
        counter::model{},
        lager::with_boost_asio_event_loop{ctx.get_executor()});
    store.dispatch(counter::increment_action{});
    ctx.run();
    CHECK(store->value == 1);
}

TEST_CASE("strand")
{
    auto ctx    = boost::asio::io_context{};
    auto strand = boost::asio::io_context::strand{ctx};
    auto store  = lager::make_store<counter::action>(
        counter::model{}, lager::with_boost_asio_event_loop{strand});
    store.dispatch(counter::increment_action{});
    ctx.run();
    CHECK(store->value == 1);
}

TEST_CASE("modern strand")
{
    auto ctx    = boost::asio::io_context{};
    auto strand = boost::asio::strand<boost::asio::io_context::executor_type>{
        ctx.get_executor()};
    auto store = lager::make_store<counter::action>(
        counter::model{}, lager::with_boost_asio_event_loop{strand});
    store.dispatch(counter::increment_action{});
    ctx.run();
    CHECK(store->value == 1);
}
