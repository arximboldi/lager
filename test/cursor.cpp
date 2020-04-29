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

#include <lager/cursor.hpp>
#include <lager/reader.hpp>
#include <lager/state.hpp>
#include <lager/writer.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/comparison.hpp>

#include <immer/vector.hpp>

#include "spies.hpp"

using namespace lager;

TEST_CASE("in, construction and assignment from temporary")
{
    reader<int> in1;
    in1 = make_state(0);
    reader<int> in2{make_state(0)};
}

TEST_CASE("out, construction_and_assignment_from_temporary")
{
    writer<int> out1;
    out1 = make_state(0);
    writer<int> out2{make_state(0)};
}

TEST_CASE("inout, construction_and_assignment_from_temporary")
{
    cursor<int> inout1;
    inout1 = make_state(0);
    cursor<int> inout2{make_state(0)};
}

TEST_CASE("values, relaxing_requirements_works")
{
    reader<int> in1  = make_state(0);
    reader<int> in2  = cursor<int>{make_state(0)};
    writer<int> out1 = make_state(0);
    writer<int> out2 = cursor<int>{make_state(0)};
}

TEST_CASE("in, watching_and_getting")
{
    auto st = make_state(0);
    auto i  = reader<int>{st};
    auto s  = testing::spy();

    watch(i, s);
    st.set(42);
    commit(st);

    CHECK(42 == i.get());
    CHECK(1 == s.count());
}

TEST_CASE("inout, watching_and_setting_and_getting")
{
    auto st = make_state(0);
    auto io = cursor<int>{st};
    auto s  = testing::spy();

    watch(io, s);
    io.set(42);
    commit(st);

    CHECK(42 == io.get());
    CHECK(1 == s.count());
}

TEST_CASE("out, setting")
{
    auto st = make_state(0);
    auto o  = writer<int>{st};

    o.set(42);
    commit(st);
    CHECK(42 == st.get());
}

TEST_CASE("values, scoped watching")
{
    auto st = make_state(0);
    auto s  = testing::spy();

    {
        auto i  = reader<int>(st);
        auto io = cursor<int>(st);
        watch(i, s);
        watch(io, s);

        st.set(42);
        commit(st);
        CHECK(2 == s.count());
    }

    st.set(52);
    commit(st);
    CHECK(2 == s.count());
}

struct yearday
{
    int day;
    int month;
};
BOOST_FUSION_ADAPT_STRUCT(yearday, day, month);

struct person
{
    yearday birthday;
    std::string name;
    std::vector<std::string> things {};
};
BOOST_FUSION_ADAPT_STRUCT(person, birthday, name);

using boost::fusion::operators::operator==;
using boost::fusion::operators::operator!=;

TEST_CASE("zooming, unfocused")
{
    state<std::vector<person>> st{{person{{5, 4}, "juanpe"}}};
    auto p  = st[0];
    auto n1 = p[&person::name];
//    auto n2 = p[lenses::attr(&person::name)];
//    reader<std::optional<std::string>> n2 = p[lenses::attr(&person::name)];
    auto n3 = p[lenses::value_or(person{{1, 1}, "NULL"})];
    auto n4 = p[lenses::with_opt(lenses::attr(&person::name))];
}

TEST_CASE("zooming, unfocused, immutable")
{
    auto l1 = lager::lenses::getset(
                [](auto &&vec) -> size_t { return vec.size(); },
                [](auto &&vec, size_t) { return LAGER_FWD(vec); });

    state<immer::vector<person>> st{{person{{5, 4}, "juanpe"}}};
    auto p  = st[0];
    auto n1 = p[&person::name];

    auto n3 = p[lenses::value_or(person{{1, 1}, "NULL"})];
    auto n4 = p[lenses::with_opt(lenses::attr(&person::name))];

    auto s = st[l1];
}
