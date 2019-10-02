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
#include <lager/xform.hpp>

#include <zug/transducer/filter.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/comparison.hpp>

#include <array>
#include <map>

using namespace zug;
using namespace lager;
using namespace boost::fusion::operators;

TEST_CASE("xformed, to_in")
{
    reader<int> i = xform(zug::identity)(make_state(0));
}

TEST_CASE("xformed, identity")
{
    auto s = state<int>{42};
    auto x = xform(zug::identity)(s);
    CHECK(x.get() == 42);
}

TEST_CASE("xformed, identity two args is zipping")
{
    auto s1 = state<int>{42};
    auto s2 = state<int>{13};
    auto x  = xform(zug::identity)(s1, s2);
    CHECK(x.get() == std::make_tuple(42, 13));
}

TEST_CASE("xformed, one arg mapping")
{
    auto s = state<int>{42};
    auto x = xform(map([](int a) { return a + 1; }))(s);
    CHECK(x.get() == 43);
}

TEST_CASE("xformed, two arg mapping")
{
    auto s1 = state<int>{42};
    auto s2 = state<int>{10};
    auto x  = xform(map(std::plus<int>{}))(s1, s2);
    CHECK(x.get() == 52);
}

TEST_CASE("xformed, one arg filter with value")
{
    auto s = state<int>{42};
    auto x = xform(filter([](int a) { return a % 2 == 0; }))(s);
    CHECK(x.get() == 42);
}

TEST_CASE("xformed, one arg filter without value")
{
    auto s = state<int>{43};
    auto x = xform(filter([](int a) { return a % 2 == 0; }))(s);
    CHECK(x.get() == 0);
}

struct non_default
{
    int v         = 0;
    non_default() = delete;
    non_default(int v_)
        : v(v_)
    {}
    bool operator==(non_default x) const { return v == x.v; }
    bool operator!=(non_default x) const { return v != x.v; }
};

TEST_CASE("xformed, one arg filter without value non default ctr")
{
    auto s = state<non_default>{non_default{43}};
    CHECK_THROWS_AS(
        xform(filter([](non_default x) { return x.v % 2 == 0; }))(s),
        no_value_error);
}

TEST_CASE(
    "xformed",
    "one arg filter without value non default ctr ok if first value passes")
{
    auto s = state<non_default>{42};
    auto x = xform(filter([](non_default a) { return a.v % 2 == 0; }))(s);
    s.set(non_default{43});
    commit(s);
    CHECK(x.get().v == 42); // old value still visible

    s.set(non_default{44});
    commit(s);
    CHECK(x.get().v == 44); // new value passes
}

TEST_CASE("xformed, identity setter")
{
    auto s = state<int>{42};
    auto x = xform(zug::identity, zug::identity)(s);
    CHECK(x.get() == 42);

    x.set(5);
    CHECK(x.get() == 42);
    CHECK(s.get() == 42);

    commit(s);
    CHECK(x.get() == 5);
    CHECK(s.get() == 5);
}

TEST_CASE("xformed, identity setter two parents")
{
    auto s1 = state<int>{42};
    auto s2 = state<int>{12};

    auto x = xform(zug::identity, zug::identity)(s1, s2);
    CHECK(x.get() == std::make_tuple(42, 12));

    x.set(std::make_tuple(5, 12));
    CHECK(x.get() == std::make_tuple(42, 12));
    CHECK(s1.get() == 42);
    CHECK(s2.get() == 12);

    commit(s1, s2);
    CHECK(x.get() == std::make_tuple(5, 12));
    CHECK(s1.get() == 5);
    CHECK(s2.get() == 12);
}

TEST_CASE("xformed, mapping")
{
    auto st = make_state(0);
    auto x  = xform(map([](int a) { return a + 2; }))(st);
    CHECK(2 == x.get());

    st.set(42);
    commit(st);
    CHECK(44 == x.get());
}

TEST_CASE("xformed, bidirectional")
{
    auto st = make_state(0);
    auto x  = xform(map([](int a) { return a + 2; }),
                   map([](int a) { return a - 2; }))(st);
    CHECK(2 == x.get());

    x.set(42);
    commit(st);
    CHECK(42 == x.get());
    CHECK(40 == st.get());
}

TEST_CASE("atted, accessing keys of a container")
{
    using map_t = std::map<std::string, int>;
    auto st     = make_state(map_t{});
    auto x      = atted("john", st);
    CHECK(0 == x.get()); // not found => default constructed

    x.set(12);
    commit(st);
    CHECK(0 == x.get()); // not magically created
    CHECK(map_t{} == st.get());

    st.set(map_t{{"john", 42}});
    commit(st);
    CHECK(42 == x.get()); // at last, I found it!

    x.set(43);
    commit(st);
    CHECK(43 == x.get()); // happy birthday, john!
    CHECK((map_t{{"john", 43}}) == st.get());

    st.set(map_t{{}});
    commit(st);
    CHECK(43 == x.get()); // Simply doesn't update
}

TEST_CASE("atted, accessing keys of acontainer in version")
{
    using map_t = std::map<std::string, int>;
    auto st     = make_state(map_t{});
    auto x      = atted("john", st);
    CHECK(0 == x.get()); // not found => default constructed

    st.set(map_t{{"john", 42}});
    commit(st);
    CHECK(42 == x.get()); // at last, I found it!

    x.set(43); // Should not compile!
}

TEST_CASE("atted, atted from erased cursor")
{
    using map_t = std::map<std::string, int>;
    auto st     = make_state(map_t{});
    auto x      = atted("john", cursor<map_t>{st});
    st.set(map_t{{"john", 42}});
    commit(st);
    CHECK(42 == x.get());
}

namespace {

struct person
{
    std::string name;
    std::size_t age;

    bool operator==(const person& x) const
    {
        return name == x.name && age == x.age;
    }
    bool operator!=(const person& x) const { return !(*this == x); }
};

} // namespace

TEST_CASE("atted, updates dont overwrite new data in complex scenario")
{
    auto st = make_state(std::array<person, 2>{{{"john", 42}, {"emil", 2}}});
    auto x1 = atted(0u, st);
    auto x2 = atted(1u, xform(zug::identity, zug::identity)(st));
    auto x3 = atted(1u, st);
    auto x4 = attred(&person::name, x2);
    auto x5 = attred(&person::age, x3);
    auto x6 = attred(&person::age, x1);

    x6.set(43u);
    x5.set(3u);
    x4.set("emily");
    commit(st);

    CHECK(st.get() == (std::array<person, 2>{{{"john", 43}, {"emily", 3}}}));
    CHECK(x6.get() == 43);
    CHECK(x4.get() == "emily");
    CHECK(x5.get() == 3);
}

TEST_CASE("atted, accesing attributes")
{
    auto st = make_state(person{"john", 42});
    auto x  = attred(&person::age, st);
    CHECK(42 == x.get());

    x.set(43u);
    commit(st);
    CHECK(43 == x.get()); // Happy birthday John!
    CHECK((person{"john", 43}) == st.get());
}

TEST_CASE("atted, accesing attributes in version")
{
    auto st = make_state(person{"john", 42});
    auto x  = attred(&person::age, st);
    CHECK(42 == x.get());

    // Should not compile!
    // x.set(43);
}

struct machine
{
    std::string name;
    std::size_t wheels;
};

BOOST_FUSION_ADAPT_STRUCT(machine, name, wheels)

TEST_CASE("atted, modifying attributes of immutable")
{
    auto st = make_state(machine{"car", 4});
    auto x  = attred(&machine::name, st);
    auto y  = attred(&machine::wheels, st);

    y.set(3u);
    commit(st);
    CHECK(st.get() == (machine{"car", 3}));
    CHECK(x.get() == "car");
    CHECK(y.get() == 3);

    x.set("tricar");
    commit(st);
    CHECK(st.get() == (machine{"tricar", 3}));
    CHECK(x.get() == "tricar");
    CHECK(y.get() == 3);
}
