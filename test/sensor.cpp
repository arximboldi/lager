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

#include <lager/sensor.hpp>
#include <lager/state.hpp>

#include "spies.hpp"

using namespace lager;

struct counter
{
    std::size_t count = 0;
    std::size_t operator()() { return count++; }
};

TEST_CASE("sensor, basic")
{
    auto x = make_sensor([] { return 42; });
    auto y = reader<int>{x};
    CHECK(42 == x.get());
    commit(x);
    CHECK(42 == x.get());
    CHECK(42 == y.get());
}

TEST_CASE("sensor, looks up only on commit")
{
    auto x = make_sensor(counter{});
    CHECK(0 == x.get());
    CHECK(0 == x.get());
    commit(x);
    CHECK(1 == x.get());
    CHECK(1 == x.get());
    commit(x);
    CHECK(2 == x.get());
}

TEST_CASE("sensor, watching")
{
    auto expected = std::size_t{1};
    auto x        = make_sensor(counter{});
    auto s = testing::spy([&](std::size_t curr) { CHECK(expected == curr); });
    watch(x, s);
    CHECK(0 == s.count());
    commit(x);
    CHECK(1 == s.count());
    ++expected;
    commit(x);
    CHECK(2 == s.count());
}
