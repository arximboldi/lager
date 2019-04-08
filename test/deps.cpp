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

#include <lager/deps.hpp>

struct foo
{
    int x = 0;
};

struct bar
{
    const char* s = "lol";
};

struct yas
{
    double z = 42.;
};

TEST_CASE("basic")
{
    auto x = lager::deps<foo, bar>{foo{}, bar{}};
    CHECK(x.get<foo>().x == 0);
    CHECK(x.get<bar>().s == std::string{"lol"});
}

TEST_CASE("reference")
{
    auto f = foo{};
    auto x = lager::deps<foo&, bar>{f, bar{}};
    f.x    = 42;
    CHECK(x.get<foo>().x == 42);
    CHECK(x.get<bar>().s == std::string{"lol"});
}

TEST_CASE("copiable")
{
    auto f = foo{};

    auto x1 = lager::deps<foo&, bar>{f, bar{}};
    f.x     = 42;
    CHECK(x1.get<foo>().x == 42);
    CHECK(x1.get<bar>().s == std::string{"lol"});

    auto x2 = x1;
    CHECK(x2.get<foo>().x == 42);
    CHECK(x2.get<bar>().s == std::string{"lol"});
}

TEST_CASE("subsets")
{
    auto d1 = lager::deps<foo, bar, yas>{foo{42}, bar{"hehe"}, yas{15.}};

    auto d2 = lager::deps<foo, yas>{d1};
    CHECK(d2.get<foo>().x == 42);
    CHECK(d2.get<yas>().z == 15.);

    auto d3 = lager::deps<bar>{d1};
    CHECK(d3.get<bar>().s == std::string{"hehe"});

    auto d4 = lager::deps<yas, foo>{d2};
    CHECK(d4.get<foo>().x == 42);
    CHECK(d4.get<yas>().z == 15.);
}
