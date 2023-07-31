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

#include <lager/state.hpp>

TEST_CASE("watch before assign")
{
    auto c      = lager::cursor<int>{};
    auto called = 0;
    auto value  = -1;
    watch(c, [&](auto x) {
        ++called;
        value = x;
    });

    auto s = lager::state<int, lager::automatic_tag>(42);
    c      = s;
    CHECK(called == 0);
    CHECK(value == -1);

    s.set(5);
    CHECK(called == 1);
    CHECK(value == 5);
    CHECK(c.get() == 5);

    c = lager::state<int, lager::automatic_tag>(2);
    c.set(4);
    CHECK(called == 2);
    CHECK(value == 4);
    CHECK(c.get() == 4);
}

TEST_CASE("nudge")
{
    auto c      = lager::state<int>(42);
    auto called = 0;
    auto value  = -1;
    c.watch([&](auto x) {
         ++called;
         value = x;
     }).nudge();
    CHECK(called == 1);
    CHECK(value == 42);
}

TEST_CASE("bind")
{
    auto c      = lager::state<int>(42);
    auto called = 0;
    auto value  = -1;
    c.bind([&](auto x) {
        ++called;
        value = x;
    });
    CHECK(called == 1);
    CHECK(value == 42);
}

TEST_CASE("assignment doesn't change signal bindings")
{
    lager::state<int, lager::automatic_tag> data1;
    lager::state<int, lager::automatic_tag> data2;

    lager::reader<int> reader = data1;

    int bind1_times_called = 0;
    reader.bind([&bind1_times_called] (int i) { bind1_times_called++;});
    CHECK(bind1_times_called == 1);

    data1.set(42);
    CHECK(bind1_times_called == 2);

    reader = data2;

    // data1 is not connected anymore!
    data1.set(43);
    CHECK(bind1_times_called == 2);

    // but data2 is!
    data2.set(44);
    CHECK(bind1_times_called == 3);

    int bind2_times_called = 0;
    reader.bind([&bind2_times_called] (int i) { bind2_times_called++;});
    CHECK(bind2_times_called == 1);

    data2.set(46);
    CHECK(bind1_times_called == 4);
    CHECK(bind2_times_called == 2);

}

TEST_CASE("reader::unbind")
{
    lager::state<int, lager::automatic_tag> data1;

    lager::reader<int> reader = data1;

    int bind1_times_called = 0;
    reader.bind([&bind1_times_called] (int i) { bind1_times_called++;});
    CHECK(bind1_times_called == 1);

    data1.set(42);
    CHECK(bind1_times_called == 2);

    reader.unbind();

    data1.set(43);
    CHECK(bind1_times_called == 2);
}
