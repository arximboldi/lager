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
#include <lager/extra/derive/eq.hpp>
#include <lager/extra/derive/hana.hpp>

#include <boost/hana/for_each.hpp>
#include <immer/array.hpp>

namespace ns {
struct derived
{
    int a;
    float b;
};
} // namespace ns

LAGER_DERIVE((EQ, HANA), ns, derived, a, b);

TEST_CASE("basic")
{
    auto x = ns::derived{42, 12};
    auto y = x;
    CHECK(x == y);

    auto members = immer::array<std::string>{};
    auto acc     = 0;
    boost::hana::for_each(x, [&](auto&& x) {
        members = std::move(members).push_back(boost::hana::first(x).c_str());
        acc += boost::hana::second(x);
    });
    CHECK(members ==
          immer::array<std::string>{{std::string{"a"}, std::string{"b"}}});
    CHECK(acc == 54);
}

namespace ns {
struct empty_t
{};
} // namespace ns

LAGER_DERIVE(EQ, ns, empty_t);

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

LAGER_DERIVE_TEMPLATE(
    (EQ, HANA), ns, (class A, class B), (foo_tpl<A, B>), a, b);

TEST_CASE("template")
{
    auto x = ns::foo_tpl<float, int>{42, 12};
    auto y = x;
    CHECK(x == y);

    auto members = immer::array<std::string>{};
    auto acc     = 0;
    boost::hana::for_each(x, [&](auto&& x) {
        members = std::move(members).push_back(boost::hana::first(x).c_str());
        acc += boost::hana::second(x);
    });
    CHECK(members ==
          immer::array<std::string>{{std::string{"a"}, std::string{"b"}}});
    CHECK(acc == 54);
}

namespace ns {
template <typename A, typename B>
struct foo_tpl2
{
    struct nested
    {
        A a;
        B b;
        LAGER_DERIVE_NESTED((EQ, HANA), nested, a, b);
    };
};
} // namespace ns

TEST_CASE("template_nested")
{
    auto x = ns::foo_tpl2<float, int>::nested{42, 12};
    auto y = x;
    CHECK(x == y);

    auto members = immer::array<std::string>{};
    auto acc     = 0;
    boost::hana::for_each(x, [&](auto&& x) {
        members = std::move(members).push_back(boost::hana::first(x).c_str());
        acc += boost::hana::second(x);
    });
    CHECK(members ==
          immer::array<std::string>{{std::string{"a"}, std::string{"b"}}});
    CHECK(acc == 54);
}
