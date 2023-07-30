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

#include <lager/event_loop/queue.hpp>
#include <lager/store.hpp>

#include "example/counter/counter.hpp"

TEST_CASE("basic")
{
    auto queue = lager::queue_event_loop{};
    auto store = lager::make_store<counter::action>(
        counter::model{}, lager::with_queue_event_loop{queue});

    store.dispatch(counter::increment_action{});
    CHECK(store->value == 0);

    queue.step();
    CHECK(store->value == 1);
}

TEST_CASE("exception")
{
    auto called = 0;
    auto loop   = lager::queue_event_loop{};

    loop.post([&] { throw std::runtime_error{"noo!"}; });
    loop.post([&] { ++called; });

    CHECK_THROWS(loop.step());
    CHECK(called == 0);

    loop.step();
    CHECK(called == 1);
}
