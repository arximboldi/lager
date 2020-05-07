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
