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

#include "../spies.hpp"

#include <lager/sensor.hpp>
#include <lager/state.hpp>
#include <lager/with.hpp>

#include <zug/transducer/map.hpp>

#include <array>

using namespace zug;

using namespace lager::detail;

TEST_CASE("node, instantiate down node")
{
    make_xform_reader_node(identity, {});
}

TEST_CASE("node, instantiate state") { make_state_node(0); }

TEST_CASE("node, last_value is not visible")
{
    auto x = make_state_node(0);
    x->send_up(12);
    CHECK(0 == x->last());
    x->send_up(42);
    CHECK(0 == x->last());
}

TEST_CASE("node, last value becomes visible")
{
    auto x = make_state_node(0);

    x->send_up(12);
    x->send_down();
    CHECK(12 == x->last());

    x->send_up(42);
    x->send_down();
    CHECK(42 == x->last());
}

TEST_CASE("node, sending down")
{
    auto x = make_state_node(5);
    auto y = make_xform_reader_node(identity, std::make_tuple(x));
    CHECK(5 == y->last());

    x->send_up(12);
    x->send_down();
    CHECK(12 == y->last());

    x->send_up(42);
    x->send_down();
    CHECK(42 == y->last());
}

TEST_CASE("node, notifies new and previous value after send down")
{
    auto x = make_state_node(5);
    auto s = testing::spy([](int next) { CHECK(42 == next); });
    auto c = x->observers().connect(s);

    x->send_up(42);
    CHECK(0 == s.count());

    x->notify();
    CHECK(0 == s.count());

    x->send_down();
    x->notify();
    CHECK(1 == s.count());
}

TEST_CASE("node, lifetime of observer")
{
    auto x = make_state_node(5);
    auto s = testing::spy();

    using signal_t = typename decltype(x)::element_type::signal_type;
    using slot_t   = signal_t::slot<decltype(s)>;

    auto c = slot_t{s};
    {
        auto y = make_xform_reader_node(identity, std::make_tuple(x));
        y->observers().add(c);
        CHECK(c.is_linked());

        x->push_down(56);
        x->send_down();
        x->notify();
        CHECK(1 == s.count());
    }
    CHECK(!c.is_linked());

    x->push_down(26);
    x->send_down();
    x->notify();
    CHECK(1 == s.count());
}

TEST_CASE("node, notify idempotence")
{
    auto x = make_state_node(5);
    auto s = testing::spy();
    auto c = x->observers().connect(s);

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

TEST_CASE("node, observing is consistent")
{
    auto x = make_state_node(5);
    auto y = make_xform_reader_node(identity, std::make_tuple(x));
    auto z = make_xform_reader_node(identity, std::make_tuple(x));
    auto w = make_xform_reader_node(identity, std::make_tuple(y));

    auto s = testing::spy([&](int new_value) {
        CHECK(42 == new_value);
        CHECK(42 == x->last());
        CHECK(42 == y->last());
        CHECK(42 == z->last());
        CHECK(42 == w->last());
    });

    auto xc = x->observers().connect(s);
    auto yc = y->observers().connect(s);
    auto zc = z->observers().connect(s);
    auto wc = w->observers().connect(s);

    x->send_up(42);
    x->send_down();
    CHECK(0 == s.count());

    x->notify();
    CHECK(4 == s.count());
}

TEST_CASE("node, bidirectional node sends values up")
{
    auto x = make_state_node(5);
    auto y = make_xform_cursor_node(identity, identity, std::make_tuple(x));

    y->send_up(42);
    CHECK(5 == x->last());
    CHECK(5 == y->last());

    x->send_down();
    CHECK(42 == x->last());
    CHECK(42 == y->last());
}

TEST_CASE("node, bidirectional mapping")
{
    auto inc = [](int x) { return ++x; };
    auto dec = [](int x) { return --x; };
    auto x   = make_state_node(5);
    auto y   = make_xform_cursor_node(map(inc), map(dec), std::make_tuple(x));

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

TEST_CASE("node, bidirectiona update is consistent")
{
    using arr = std::array<int, 2>;
    auto x    = make_state_node(arr{{5, 13}});
    auto y    = make_xform_cursor_node(map([](const arr& a) { return a[0]; }),
                                    lager::update([](arr a, int v) {
                                        a[0] = v;
                                        return a;
                                    }),
                                    std::make_tuple(x));
    auto z    = make_xform_cursor_node(map([](const arr& a) { return a[1]; }),
                                    lager::update([](arr a, int v) {
                                        a[1] = v;
                                        return a;
                                    }),
                                    std::make_tuple(x));

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

TEST_CASE("node, sensors nodes reevaluate on send down")
{
    int count = 0;
    auto x    = make_sensor_node([&count] { return count++; });
    CHECK(0 == x->last());
    x->send_down();
    CHECK(1 == x->last());
    x->send_down();
    CHECK(2 == x->last());
}

TEST_CASE("node, one node two parents")
{
    int count = 0;
    auto x    = make_sensor_node([&count] { return count++; });
    auto y    = make_state_node(12);
    auto z    = make_xform_reader_node(map([](int a, int b) { return a + b; }),
                                    std::make_tuple(x, y));
    auto s    = testing::spy([&](int r) { CHECK(r == x->last() + y->last()); });
    auto c    = z->observers().connect(s);
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
