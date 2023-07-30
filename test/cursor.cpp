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

#include <lager/constant.hpp>
#include <lager/cursor.hpp>
#include <lager/reader.hpp>
#include <lager/state.hpp>
#include <lager/writer.hpp>

#include <lager/lenses.hpp>
#include <lager/lenses/tuple.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/comparison.hpp>

#include <immer/vector.hpp>
#include <vector>

#include "spies.hpp"

using namespace lager;

TEST_CASE("in, construction and assignment from temporary")
{
    reader<int> in1;
    // Access to uninitialized reader should throw an exception
    REQUIRE_THROWS(in1.get());
    in1 = make_state(0);
    reader<int> in2{make_state(0)};
}

TEST_CASE("out, construction_and_assignment_from_temporary")
{
    writer<int> out1;
    // Access to uninitialized writer should throw an exception
    REQUIRE_THROWS(out1.set(42));
    REQUIRE_THROWS(out1.update([](auto){ return 42;}));
    out1 = make_state(0);
    writer<int> out2{make_state(0)};
}

TEST_CASE("inout, construction_and_assignment_from_temporary")
{
    cursor<int> inout1;
    // Access to uninitialized cursor should throw an exception
    REQUIRE_THROWS(inout1.get());
    REQUIRE_THROWS(inout1.set(42));
    REQUIRE_THROWS(inout1.update([](auto){ return 42;}));
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
    std::vector<std::string> things{};
};
BOOST_FUSION_ADAPT_STRUCT(person, birthday, name);

using boost::fusion::operators::operator==;
using boost::fusion::operators::operator!=;

TEST_CASE("zooming, unfocused")
{
    state<std::vector<person>> st{{person{{5, 4}, "juanpe"}}};
    auto p  = st[0].make();
    auto n1 = p[&person::name].make();
    //    auto n2 = p[lenses::attr(&person::name)];
    //    reader<std::optional<std::string>> n2 =
    //    p[lenses::attr(&person::name)];
    auto n3 = p[lenses::value_or(person{{1, 1}, "NULL"})].make();
    auto n4 = p[lenses::with_opt(lenses::attr(&person::name))].make();
}

TEST_CASE("zooming, unfocused, immutable")
{
    auto l1 = lager::lenses::getset(
        [](auto&& vec) -> size_t { return vec.size(); },
        [](auto&& vec, size_t) { return LAGER_FWD(vec); });

    state<immer::vector<person>> st{{person{{5, 4}, "juanpe"}}};
    auto p  = st[0].make();
    auto n1 = p[&person::name].make();

    auto n3 = p[lenses::value_or(person{{1, 1}, "NULL"})].make();
    auto n4 = p[lenses::with_opt(lenses::attr(&person::name))].make();

    auto s = st[l1].make();
}

TEST_CASE("in, constant")
{
    auto c = make_constant(42);
    auto i = reader<int>{c};

    CHECK(*c == 42);
    CHECK(*i == 42);
}

TEST_CASE("automatic_tag edge case")
{
    using vec_t = std::vector<int>;
    using cur_t = lager::cursor<std::optional<int>>;

    lager::state<vec_t, lager::automatic_tag> st;
    std::vector<cur_t> cursors;
    auto spy = testing::spy();

    st.watch([&](vec_t const& vec) {
        cursors.clear();
        for (size_t i = 0; i < vec.size(); ++i) {
            cursors.push_back(st[i]);
            cursors[i].watch(spy);
        }
    });

    st.set(vec_t{1, 2, 3, 4, 5, 6, 7, 8});
    cur_t cur = st[0];
    cur.watch([&](auto const& optint) {
        if (optint.value_or(0) > 10) {
            st.set(vec_t{});
        }
    });

    cur.set(42); // this would cause a crash before commit aefd37b

    st.set(vec_t{1, 2, 3, 4}); // this will collect garbage
}

TEST_CASE("lenses over with expression")
{
    state<person, automatic_tag> person_data;

    person_data.set(person{{}, "old name", {}});

    cursor<std::string> name = with(person_data[&person::name], person_data[&person::birthday]).zoom(lenses::first);

    name.set("new name");

    CHECK(person_data->name == "new name");
    CHECK(name.get() == "new name");
}
