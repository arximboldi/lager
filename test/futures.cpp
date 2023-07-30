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

#include "../example/counter/counter.hpp"

TEST_CASE("future then callback is called after reducer")
{
    auto queue = lager::queue_event_loop{};
    auto store =
        lager::make_store<counter::action>(counter::model{},
                                           lager::with_queue_event_loop{queue},
                                           lager::with_futures);

    auto called = 0;
    store.dispatch(counter::increment_action{}).then([&] {
        CHECK(store->value == 1);
        ++called;
    });
    CHECK(called == 0);

    queue.step();
    CHECK(called == 1);
}

TEST_CASE("future then callback is called after effects")
{
    auto effect1 = [&](auto&& ctx) -> lager::future { return ctx.dispatch(1); };
    auto effect2 = [&](auto&& ctx) -> lager::future { return ctx.dispatch(2); };

    auto queue = lager::queue_event_loop{};
    auto store = lager::make_store<int>(
        0,
        lager::with_queue_event_loop{queue},
        lager::with_futures,
        lager::with_reducer([&](int s, int a) -> lager::result<int, int> {
            if (a == 0)
                return {s,
                        lager::sequence(lager::effect<int>{effect1},
                                        lager::effect<int>{effect2})};
            else
                return s + a;
        }));

    auto called = 0;
    store.dispatch(0).then([&] {
        CHECK(*store == 3);
        ++called;
    });
    CHECK(called == 0);

    queue.step();
    CHECK(called == 1);
}

TEST_CASE("future effect callback chaining")
{
    auto effect1 = [&](auto&& ctx) -> lager::future { return ctx.dispatch(1); };
    auto effect2 = [&](auto&& ctx) -> lager::future { return ctx.dispatch(2); };

    auto queue = lager::queue_event_loop{};
    auto store = lager::make_store<int>(
        0,
        lager::with_queue_event_loop{queue},
        lager::with_futures,
        lager::with_reducer([&](int s, int a) -> lager::result<int, int> {
            if (a == 0)
                return {s, effect1};
            else if (a == 1)
                return {s + a, effect2};
            else
                return s + a;
        }));

    auto called = 0;
    store.dispatch(0).then([&] {
        CHECK(*store == 3);
        ++called;
    });
    CHECK(called == 0);

    queue.step();
    CHECK(called == 1);
}

TEST_CASE("future effect callback chaining and stupid effect")
{
    auto effect0 = [&](auto&& ctx) -> void {};
    auto effect1 = [&](auto&& ctx) -> lager::future { return ctx.dispatch(1); };
    auto effect2 = [&](auto&& ctx) -> lager::future { return ctx.dispatch(2); };

    auto queue = lager::queue_event_loop{};
    auto store = lager::make_store<int>(
        0,
        lager::with_queue_event_loop{queue},
        lager::with_futures,
        lager::with_reducer([&](int s, int a) -> lager::result<int, int> {
            if (a == 0)
                return {s,
                        lager::sequence(lager::effect<int>{effect1},
                                        lager::effect<int>{effect0})};
            else if (a == 1)
                return {s + a, effect2};
            else
                return s + a;
        }));

    auto called = 0;
    store.dispatch(0).then([&] {
        CHECK(*store == 3);
        ++called;
    });
    CHECK(called == 0);

    queue.step();
    CHECK(called == 1);
}

TEST_CASE("combining future")
{
    auto queue = lager::queue_event_loop{};
    auto store =
        lager::make_store<counter::action>(counter::model{},
                                           lager::with_queue_event_loop{queue},
                                           lager::with_futures);

    auto called = 0;
    store.dispatch(counter::increment_action{})
        .also(store.dispatch(counter::increment_action{}))
        .then([&] {
            CHECK(store->value == 2);
            ++called;
        });
    CHECK(called == 0);

    queue.step();
    CHECK(called == 1);
}
