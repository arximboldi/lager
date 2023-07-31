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

#include "test/cereal/cerealize.hpp"
#include <catch2/catch.hpp>

#include <lager/extra/cereal/struct.hpp>
#include <lager/extra/struct.hpp>

namespace ns {
struct foo
{
    int a;
    float b;
};
} // namespace ns

LAGER_STRUCT(ns, foo, a, b);

TEST_CASE("basic")
{
    auto x = ns::foo{42, 12};
    auto y = cerealize(x);
    CHECK(x.a == y.a);
    CHECK(x.b == y.b);
}

namespace ns {
struct empty_foo
{};
} // namespace ns
LAGER_STRUCT(ns, empty_foo);

TEST_CASE("empty")
{
    auto x = ns::empty_foo{};
    auto y = cerealize(x);
    (void) x;
    (void) y;
}

namespace ns {
template <typename A, typename B>
struct foo_tpl
{
    A a;
    B b;
};
} // namespace ns

LAGER_STRUCT_TEMPLATE(ns, (class A, class B), (foo_tpl<A, B>), a, b);

TEST_CASE("template")
{
    auto x = ns::foo_tpl<float, int>{42, 12};
    auto y = cerealize(x);
    CHECK(x.a == y.a);
    CHECK(x.b == y.b);
}

namespace ns {
template <typename A, typename B>
struct foo_tpl2
{
    struct nested
    {
        A a;
        B b;
        LAGER_STRUCT_NESTED(nested, a, b);
    };
};
} // namespace ns

TEST_CASE("template_nested")
{
    auto x = ns::foo_tpl2<float, int>::nested{42, 12};
    auto y = cerealize(x);
    CHECK(x.a == y.a);
    CHECK(x.b == y.b);
}
