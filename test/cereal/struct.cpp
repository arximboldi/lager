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
#include <lager/debug/cereal/struct.hpp>
#include "cerealize.hpp"

struct foo
{
    int a;
    float b;
};

LAGER_CEREAL_STRUCT(foo, (a)(b));

TEST_CASE("basic")
{
    auto x = foo{42, 12};
    auto y = cerealize(x);
    CHECK(x.a == y.a);
    CHECK(x.b == y.b);
}

struct empty_foo {};
LAGER_CEREAL_STRUCT(empty_foo);

TEST_CASE("empty")
{
    auto x = empty_foo{};
    auto y = cerealize(x);
    (void) x;
    (void) y;
}
