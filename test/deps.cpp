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

TEST_CASE("empty")
{
    auto x = lager::deps<>{};
    (void) x;
}

TEST_CASE("basic")
{
    auto x = lager::deps<foo, bar>::with(foo{}, bar{});
    CHECK(x.get<foo>().x == 0);
    CHECK(lager::get<bar>(x).s == std::string{"lol"});
}

TEST_CASE("reference")
{
    auto f = foo{};
    auto x = lager::deps<foo&, bar>::with(f, bar{});
    f.x    = 42;
    CHECK(x.get<foo>().x == 42);
    CHECK(x.get<bar>().s == std::string{"lol"});
}

TEST_CASE("copiable")
{
    auto f = foo{};

    auto x1 = lager::deps<foo&, bar>::with(f, bar{});
    f.x     = 42;
    CHECK(x1.get<foo>().x == 42);
    CHECK(x1.get<bar>().s == std::string{"lol"});

    auto x2 = x1;
    CHECK(x2.get<foo>().x == 42);
    CHECK(x2.get<bar>().s == std::string{"lol"});
}

TEST_CASE("subsets")
{
    auto d1 = lager::deps<foo, bar, yas>::with(foo{42}, bar{"hehe"}, yas{15.});

    auto d2 = lager::deps<foo, yas>{d1};
    CHECK(d2.get<foo>().x == 42);
    CHECK(d2.get<yas>().z == 15.);

    auto d3 = lager::deps<bar>{d1};
    CHECK(d3.get<bar>().s == std::string{"hehe"});

    auto d4 = lager::deps<yas, foo>{d2};
    CHECK(d4.get<foo>().x == 42);
    CHECK(d4.get<yas>().z == 15.);
}

TEST_CASE("merging")
{
    auto d1 = lager::deps<bar>::with(bar{"yeah"});
    auto d2 = lager::deps<foo>::with(foo{42});

    auto d3 = d1.merge(d2);
    CHECK(d3.get<foo>().x == 42);
    CHECK(d3.get<bar>().s == std::string{"yeah"});

    auto d4 = lager::deps<foo, bar>{d1, d2};
    CHECK(d4.get<foo>().x == 42);
    CHECK(d4.get<bar>().s == std::string{"yeah"});
}

TEST_CASE("type deduction and references")
{
    auto f  = foo{};
    auto d1 = lager::make_deps(std::ref(f), bar{});
    auto d2 = lager::deps<foo&, bar>{d1};

    f.x = 42;
    CHECK(d1.get<foo>().x == 42);
    CHECK(d1.get<bar>().s == std::string{"lol"});
    CHECK(d2.get<foo>().x == 42);
    CHECK(d2.get<bar>().s == std::string{"lol"});
}

struct foo1
{};
struct foo2
{};

TEST_CASE("keys")
{
    auto f1 = foo{};
    auto d  = lager::deps<lager::dep::key<foo1, foo&>,
                         lager::dep::key<foo2, foo>>::with(f1, foo{13});

    f1.x = 42;
    CHECK(d.get<foo1>().x == 42);
    CHECK(d.get<foo2>().x == 13);
}

TEST_CASE("optionals")
{
    auto f1 = foo{};
    auto d1 = lager::deps<lager::dep::key<foo1, foo&>,
                          lager::dep::key<foo2, foo>,
                          bar>::with(f1, foo{13}, bar{"lol"});
    auto d2 = lager::deps<lager::dep::key<foo2, foo>,
                          lager::dep::opt<lager::dep::key<foo1, foo&>>,
                          lager::dep::opt<yas>>{d1};

    f1.x = 42;
    CHECK(d2.has<foo1>());
    CHECK(d2.get<foo1>().x == 42);
    CHECK(!d2.has<yas>());
    CHECK_THROWS_AS(d2.get<yas>(), lager::missing_dependency_error);

    auto d3 = lager::deps<lager::dep::opt<lager::dep::key<foo1, foo&>>,
                          lager::dep::opt<yas>>{d2};
    f1.x    = 13;
    CHECK(d3.has<foo1>());
    CHECK(d3.get<foo1>().x == 13);
    CHECK(!d3.has<yas>());
    CHECK_THROWS_AS(d3.get<yas>(), lager::missing_dependency_error);
}

TEST_CASE("specs in factories")
{
    auto d1 = lager::make_deps(lager::dep::as<lager::dep::key<foo1, int>>(42));
    auto d2 = lager::deps<lager::dep::key<foo1, int>>{d1};
    CHECK(d1.get<foo1>() == 42);
    CHECK(d2.get<foo1>() == 42);
}

TEST_CASE("function to reference")
{
    auto f1 = foo{};
    auto d1 = lager::make_deps(
        lager::dep::as<lager::dep::fn<foo&>>([&]() -> foo& { return f1; }));
    auto& rf = d1.get<foo>();
    f1.x     = 13;
    CHECK(rf.x == 13);
}

TEST_CASE("keyed function")
{
    auto d1 = lager::make_deps(
        lager::dep::as<lager::dep::key<foo1, lager::dep::fn<int>>>(
            [&] { return 42; }));
    CHECK(d1.get<foo1>() == 42);
}

TEST_CASE("extracting from function")
{
    auto f1 = foo{};
    auto d1 = lager::make_deps(
        lager::dep::as<lager::dep::fn<foo&>>([&]() -> foo& { return f1; }));
    auto d2 = lager::deps<foo&>{d1};

    f1.x = 13;
    CHECK(d2.get<foo>().x == 13);
}
