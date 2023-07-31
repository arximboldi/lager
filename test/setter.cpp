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

#include <lager/event_loop/manual.hpp>

#include <lager/setter.hpp>
#include <lager/store.hpp>

TEST_CASE("combine setter with store")
{
    auto store = lager::make_store<int, lager::transactional_tag>(
        0,
        lager::with_manual_event_loop{},
        lager::with_reducer([](int s, int a) { return a; }));
    auto cursor =
        store.xform(zug::identity).setter([&](int x) { store.dispatch(x); });

    CHECK(cursor.get() == 0);

    store.dispatch(42);
    CHECK(cursor.get() == 0);

    lager::commit(store);
    CHECK(store.get() == 42);
    CHECK(cursor.get() == 42);

    cursor.set(5);
    CHECK(cursor.get() == 42);
    CHECK(store.get() == 42);

    lager::commit(store);
    CHECK(cursor.get() == 5);
    CHECK(store.get() == 5);
}

TEST_CASE("combine automatic setter with store")
{
    auto store = lager::make_store<int, lager::transactional_tag>(
        0,
        lager::with_manual_event_loop{},
        lager::with_reducer([](int s, int a) { return a; }));
    auto cursor =
        store.xform(zug::identity).setter<lager::automatic_tag>([&](int x) {
            store.dispatch(x);
        });

    CHECK(cursor.get() == 0);

    store.dispatch(42);
    CHECK(cursor.get() == 0);

    lager::commit(store);
    CHECK(store.get() == 42);
    CHECK(cursor.get() == 42);

    cursor.set(5);
    CHECK(cursor.get() == 5);
    CHECK(store.get() == 42);

    lager::commit(store);
    CHECK(cursor.get() == 5);
    CHECK(store.get() == 5);
}
