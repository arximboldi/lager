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

#include <lager/extra/derive.hpp>
#include <lager/extra/derive/cereal.hpp>
#include <lager/extra/derive/eq.hpp>
#include <lager/extra/derive/hana.hpp>
#include <lager/extra/derive/hash.hpp>

#include <boost/hana/assert.hpp>
#include <boost/hana/equal.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/size.hpp>

#include <immer/array.hpp>
#include <immer/set.hpp>

namespace {

template <typename T>
void check_eq()
{
    auto x = T{42, 12};
    auto y = x;
    CHECK(x == y);
}

template <typename T>
void check_hana()
{
    auto x       = T{42, 12};
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

template <typename T>
void check_cereal()
{
    auto x = T{42, 12};
    auto y = cerealize(x);
    CHECK(y == x);
}

template <typename T>
void check_hash()
{
    auto m = immer::set<T>{}.insert(T{42, 12});
    CHECK(m.count(T{42, 12}) == 1);
    CHECK(m.count(T{42, 13}) == 0);
}

} // namespace

namespace ns {
struct derived
{
    int a;
    float b;
};
} // namespace ns
LAGER_DERIVE((EQ, HANA, CEREAL, HASH), ns, derived, a, b);

TEST_CASE("basic")
{
    check_eq<ns::derived>();
    check_hana<ns::derived>();
    check_cereal<ns::derived>();
    check_hash<ns::derived>();
}

namespace ns {
struct derived2
{
    derived a;
    float b;
};
} // namespace ns
LAGER_DERIVE((EQ, HANA, CEREAL, HASH), ns, derived2, a, b);

TEST_CASE("basic-2")
{
    check_eq<ns::derived2>();
    // check_hana<ns::derived2>();
    check_cereal<ns::derived2>();
    check_hash<ns::derived2>();
}

namespace ns {
struct empty_t
{};
} // namespace ns
LAGER_DERIVE((EQ, HANA, CEREAL), ns, empty_t);

TEST_CASE("empty")
{
    CHECK(ns::empty_t{} == ns::empty_t{});
    CHECK(cerealize(ns::empty_t{}) == ns::empty_t{});
    BOOST_HANA_CONSTANT_CHECK(boost::hana::size(ns::empty_t{}) ==
                              boost::hana::size_c<0>);
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
    (EQ, HANA, CEREAL, HASH), ns, (class A, class B), (foo_tpl<A, B>), a, b);

TEST_CASE("template")
{
    check_eq<ns::foo_tpl<int, float>>();
    check_hana<ns::foo_tpl<int, float>>();
    check_cereal<ns::foo_tpl<int, float>>();
    check_hash<ns::foo_tpl<int, float>>();
}

namespace ns {
template <typename A, typename B>
struct foo_tpl2
{
    struct nested
    {
        A a;
        B b;
        LAGER_DERIVE_NESTED((EQ, HANA, CEREAL), nested, a, b);
    };
};
} // namespace ns

TEST_CASE("template_nested")
{
    check_eq<ns::foo_tpl2<int, float>::nested>();
    check_hana<ns::foo_tpl2<int, float>::nested>();
    check_cereal<ns::foo_tpl2<int, float>::nested>();
}
