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

#include <lager/extra/derive.hpp>

namespace ns {
struct derived
{
    int a;
    float b;
};
} // namespace ns

LAGER_DERIVE(EQ, ns, derived, a, b);

TEST_CASE("basic")
{
    auto x = ns::derived{42, 12};
    auto y = x;
    CHECK(x == y);
}

namespace ns {
struct empty_t
{};
} // namespace ns

LAGER_DERIVE((EQ), ns, empty_t);

TEST_CASE("empty")
{
    auto x = ns::empty_t{};
    auto y = x;
    CHECK(x == y);
}

namespace ns {
template <typename A, typename B>
struct foo_tpl
{
    A a;
    B b;
};
} // namespace ns

LAGER_DERIVE_TEMPLATE(EQ, ns, (class A, class B), (foo_tpl<A, B>), a, b);

TEST_CASE("template")
{
    auto x = ns::foo_tpl<float, int>{42, 12};
    auto y = x;
    CHECK(x == y);
}
