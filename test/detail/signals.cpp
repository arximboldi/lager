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

#include <catch.hpp>

#include "../spies.hpp"

#include <lager/detail/root_signals.hpp>
#include <lager/detail/signals.hpp>
#include <lager/detail/xform_signals.hpp>

#include <zug/transducer/map.hpp>

#include <array>

using namespace zug;
using namespace lager::detail;

TEST_CASE("signal, instantiate down signal")
{
    make_xform_down_signal(identity);
}

TEST_CASE("signal, instantiate state") { make_state_signal(0); }

TEST_CASE("signal, last_value is not visible")
{
    auto x = make_state_signal(0);
    x->send_up(12);
    CHECK(0 == x->last());
    x->send_up(42);
    CHECK(0 == x->last());
}

TEST_CASE("signal, last value becomes visible")
{
    auto x = make_state_signal(0);

    x->send_up(12);
    x->send_down();
    CHECK(12 == x->last());

    x->send_up(42);
    x->send_down();
    CHECK(42 == x->last());
}

TEST_CASE("signal, sending down")
{
    auto x = make_state_signal(5);
    auto y = make_xform_down_signal(identity, x);
    CHECK(5 == y->last());

    x->send_up(12);
    x->send_down();
    CHECK(12 == y->last());

    x->send_up(42);
    x->send_down();
    CHECK(42 == y->last());
}

TEST_CASE("signal, notifies new and previous value after send down")
{
    auto x = make_state_signal(5);
    auto s = testing::spy([](int last, int next) {
        CHECK(5 == last);
        CHECK(42 == next);
    });
    x->observe(s);

    x->send_up(42);
    CHECK(0 == s.count());

    x->notify();
    CHECK(0 == s.count());

    x->send_down();
    x->notify();
    CHECK(1 == s.count());
}

TEST_CASE("signal, lifetime of observer")
{
    auto x = make_state_signal(5);
    auto s = testing::spy();
    auto c = boost::signals2::connection{};
    {
        auto y = make_xform_down_signal(identity, x);
        c      = y->observe(s);
        CHECK(c.connected());

        x->push_down(56);
        x->send_down();
        x->notify();
        CHECK(1 == s.count());
    }
    CHECK(!c.connected());

    x->push_down(26);
    x->send_down();
    x->notify();
    CHECK(1 == s.count());
}

TEST_CASE("signal, notify idempotence")
{
    auto x = make_state_signal(5);
    auto s = testing::spy();
    x->observe(s);

    x->send_up(42);
    CHECK(0 == s.count());

    x->notify();
    x->notify();
    x->notify();
    CHECK(0 == s.count());

    x->send_down();
    x->notify();
    x->notify();
    x->notify();
    CHECK(1 == s.count());
}

TEST_CASE("signal, observing is consistent")
{
    auto x = make_state_signal(5);
    auto y = make_xform_down_signal(identity, x);
    auto z = make_xform_down_signal(identity, x);
    auto w = make_xform_down_signal(identity, y);

    auto s = testing::spy([&](int last_value, int new_value) {
        CHECK(5 == last_value);
        CHECK(42 == new_value);
        CHECK(42 == x->last());
        CHECK(42 == y->last());
        CHECK(42 == z->last());
        CHECK(42 == w->last());
    });

    x->observe(s);
    y->observe(s);
    z->observe(s);
    w->observe(s);

    x->send_up(42);
    x->send_down();
    CHECK(0 == s.count());

    x->notify();
    CHECK(4 == s.count());
}

TEST_CASE("signal, bidirectional signal sends values up")
{
    auto x = make_state_signal(5);
    auto y = make_xform_up_down_signal(identity, identity, x);

    y->send_up(42);
    CHECK(5 == x->last());
    CHECK(5 == y->last());

    x->send_down();
    CHECK(42 == x->last());
    CHECK(42 == y->last());
}

TEST_CASE("signal, bidirectional mapping")
{
    auto inc = [](int x) { return ++x; };
    auto dec = [](int x) { return --x; };
    auto x   = make_state_signal(5);
    auto y   = make_xform_up_down_signal(map(inc), map(dec), x);

    CHECK(5 == x->last());
    CHECK(6 == y->last());

    y->send_up(42);
    x->send_down();
    CHECK(41 == x->last());
    CHECK(42 == y->last());

    x->send_up(42);
    x->send_down();
    CHECK(42 == x->last());
    CHECK(43 == y->last());
}

TEST_CASE("signal, bidirectiona update is consistent")
{
    using arr = std::array<int, 2>;
    auto x    = make_state_signal(arr{{5, 13}});
    auto y = make_xform_up_down_signal(map([](const arr& a) { return a[0]; }),
                                       update([](arr a, int v) {
                                           a[0] = v;
                                           return a;
                                       }),
                                       x);
    auto z = make_xform_up_down_signal(map([](const arr& a) { return a[1]; }),
                                       update([](arr a, int v) {
                                           a[1] = v;
                                           return a;
                                       }),
                                       x);

    CHECK((arr{{5, 13}}) == x->last());
    CHECK(5 == y->last());
    CHECK(13 == z->last());

    z->send_up(42);
    y->send_up(69);
    CHECK((arr{{5, 13}}) == x->last());
    CHECK(5 == y->last());
    CHECK(13 == z->last());

    x->send_down();
    CHECK((arr{{69, 42}}) == x->last());
    CHECK(69 == y->last());
    CHECK(42 == z->last());
}

TEST_CASE("signal, sensors signals reevaluate on send down")
{
    int count = 0;
    auto x    = make_sensor_signal([&count] { return count++; });
    CHECK(0 == x->last());
    x->send_down();
    CHECK(1 == x->last());
    x->send_down();
    CHECK(2 == x->last());
}

TEST_CASE("signal, one signal two parents")
{
    int count = 0;
    auto x    = make_sensor_signal([&count] { return count++; });
    auto y    = make_state_signal(12);
    auto z =
        make_xform_down_signal(map([](int a, int b) { return a + b; }), x, y);
    auto s =
        testing::spy([&](int, int r) { CHECK(r == x->last() + y->last()); });
    z->observe(s);
    CHECK(12 == z->last());

    // Commit first root individually
    x->send_down();
    CHECK(13 == z->last());
    CHECK(0 == s.count());
    x->notify();
    CHECK(1 == s.count());
    y->notify();
    CHECK(1 == s.count());

    // Commit second root individually
    y->push_down(3);
    y->send_down();
    CHECK(4 == z->last());
    y->notify();
    CHECK(2 == s.count());
    x->notify();
    CHECK(2 == s.count());

    // Commit both roots together
    x->send_down();
    y->push_down(69);
    y->send_down();
    x->notify();
    y->notify();
    CHECK(71 == z->last());
    CHECK(3 == s.count());
}
