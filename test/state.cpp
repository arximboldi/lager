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

#include <boost/operators.hpp>

#include <catch2/catch.hpp>

#include <lager/state.hpp>

#include "spies.hpp"

using namespace lager;

struct no_default_ctr
{
    no_default_ctr() = delete;
    no_default_ctr(int) {}
    bool operator==(no_default_ctr) const { return true; }
    bool operator!=(no_default_ctr) const { return false; }
};

TEST_CASE("state, holds avalue")
{
    {
        auto x = make_state(1);
        CHECK(x.get() == 1);
    }
    {
        auto x = make_state(std::string{"hello"});
        CHECK(x.get() == std::string{"hello"});
    }
    {
        auto x = state<int>{};
        CHECK(x.get() == 0);
    }
    {
        auto x = make_state(no_default_ctr{42});
        CHECK(x.get() == no_default_ctr{42});
    }
}

TEST_CASE("state, new values arent visible")
{
    auto x = make_state(42);
    x.set(13);
    CHECK(x.get() == 42);
    x.set(16);
    CHECK(x.get() == 42);
    x.set(19);
    CHECK(x.get() == 42);
}

TEST_CASE("state, commit makes latest value visible")
{
    auto x = make_state(42);
    x.set(13);
    commit(x);
    CHECK(x.get() == 13);

    x.set(8);
    x.set(5);
    x.set(3);
    commit(x);
    CHECK(x.get() == 3);
}

TEST_CASE("state, automatic")
{
    auto x = make_state(42, automatic_tag{});
    CHECK(x.get() == 42);

    x.set(13);
    CHECK(x.get() == 13);

    x.set(8);
    x.set(5);
    x.set(3);
    CHECK(x.get() == 3);
}

TEST_CASE("state, commit idempotence")
{
    auto x = make_state(42);
    x.set(13);
    commit(x);
    CHECK(x.get() == 13);
    commit(x);
    CHECK(x.get() == 13);
    commit(x);
    CHECK(x.get() == 13);
}

TEST_CASE("state, watche notified on commit")
{
    auto x = make_state(42);
    auto s = testing::spy();
    watch(x, s);

    x.set(13);
    CHECK(s.count() == 0);

    commit(x);
    CHECK(s.count() == 1);
}

TEST_CASE("state, watches always view consistent state")
{
    auto x  = make_state(42);
    auto y  = make_state(35);
    auto sx = testing::spy([&](int curr) {
        CHECK(84 == curr);
        CHECK(x.get() == 84);
        CHECK(y.get() == 70);
    });
    auto sy = testing::spy([&](int curr) {
        CHECK(70 == curr);
        CHECK(x.get() == 84);
        CHECK(y.get() == 70);
    });

    watch(x, sx);
    watch(y, sy);

    x.set(84);
    y.set(70);
    CHECK(sx.count() == 0);
    CHECK(sy.count() == 0);

    commit(x, y);
    CHECK(sx.count() == 1);
    CHECK(sy.count() == 1);
}

TEST_CASE("state, watches automatic can show inconsistent state")
{
    auto x  = make_state(42, automatic_tag{});
    auto y  = make_state(35, automatic_tag{});
    auto sx = testing::spy([&](int curr) {
        CHECK(84 == curr);
        CHECK(x.get() == 84);
        CHECK(y.get() == 35);
    });
    auto sy = testing::spy([&](int curr) {
        CHECK(70 == curr);
        CHECK(x.get() == 84);
        CHECK(y.get() == 70);
    });

    watch(x, sx);
    watch(y, sy);

    x.set(84);
    CHECK(x.get() == 84);
    CHECK(sx.count() == 1);
    y.set(70);
    CHECK(y.get() == 70);
    CHECK(sy.count() == 1);
}

TEST_CASE("state, capsule carries its own watchers")
{
    auto sig = std::shared_ptr<detail::state_node<int>>{};
    auto s   = testing::spy();
    {
        auto st = make_state(42);
        sig     = detail::access::node(st);
        watch(st, s);
        sig->push_down(12);
        sig->send_down();
        sig->notify();
        CHECK(1 == s.count());
    }
    sig->push_down(7);
    sig->send_down();
    sig->notify();
    CHECK(1 == s.count());
    CHECK(sig->observers().empty());
}

namespace TestNS {
struct Child : public boost::equality_comparable<Child>
{
    friend bool operator==(const Child &lhs, const Child &rhs) {
        return lhs.childValue == rhs.childValue;
    }

    Child() = default;
    Child(int value) : childValue(value) {}

    int childValue = 0;
};

struct Parent : public boost::equality_comparable<Parent>
{
    friend bool operator==(const Parent &lhs, const Parent &rhs) {
        return lhs.parentValue == rhs.parentValue &&
            lhs.child == rhs.child;
    }

    int parentValue = 0;
    Child child;
};
}

TEST_CASE("state, check notifications after direct assignment to state")
{

    lager::state<TestNS::Parent, automatic_tag> state;
    lager::cursor<TestNS::Child> childCursor = state[&TestNS::Parent::child];
    lager::cursor<int> childValueCursor = childCursor[&TestNS::Child::childValue];

    auto childSpy = testing::spy([&](const TestNS::Child &value) {
        CHECK(value.childValue == 13);
    });

    auto parentSpy = testing::spy([&](const TestNS::Parent &value) {
        CHECK(value.parentValue == 0);
        CHECK(value.child.childValue == 13);
    });

    watch(state, parentSpy);
    watch(childCursor, childSpy);

    /// this code must be impossible to compile,
    /// because it implicitly replaces the store,
    /// breaking all the links
    ///
    /// state = TestNS::Parent();

    // use explicit setters instead (like in cursors)
    state.set(TestNS::Parent());

    childValueCursor.set(13);

    CHECK(state.get().parentValue == 0);
    CHECK(state.get().child.childValue == 13);

    CHECK(childSpy.count() == 1);
    CHECK(parentSpy.count() == 1);
}
