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
#include <lager/state.hpp>
#include <lager/with.hpp>

#include <lager/lenses/at.hpp>
#include <lager/lenses/attr.hpp>
#include <lager/lenses/optional.hpp>

#include <zug/transducer/filter.hpp>

#include <array>
#include <map>
#include <optional>

using namespace zug;
using namespace lager;

namespace detail {

template <typename T>
struct is_optional : std::false_type
{};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type
{};

template <typename T>
[[maybe_unused]] constexpr bool is_optional_v =
    is_optional<std::decay_t<T>>::value;

} // namespace detail

template <typename AttrT, typename... CursorTs>
auto attred(AttrT&& member, CursorTs&&... cs)
{
    if constexpr (std::disjunction_v<::detail::is_optional<
                      typename std::decay_t<CursorTs>::value_type>...>) {
        return with(std::forward<CursorTs>(cs)...)
            .zoom(lenses::with_opt(lenses::attr(std::forward<AttrT>(member))))
            .make();
    } else {
        return with(std::forward<CursorTs>(cs)...)
            .zoom(lenses::attr(std::forward<AttrT>(member)))
            .make();
    }
}

template <typename KeyT, typename... CursorTs>
auto atted(KeyT&& key, CursorTs&&... cs)
{
    if constexpr (std::disjunction_v<::detail::is_optional<
                      typename std::decay_t<CursorTs>::value_type>...>) {
        return with(std::forward<CursorTs>(cs)...)
            .zoom(lenses::with_opt(lenses::at(std::forward<KeyT>(key))));
    } else {
        return with(std::forward<CursorTs>(cs)...)
            .zoom(lenses::at(std::forward<KeyT>(key)))
            .make();
    }
}

TEST_CASE("xformed, to_in")
{
    reader<int> i = make_state(0).xform(zug::identity);
}

TEST_CASE("xformed, identity")
{
    auto s = state<int>{42};
    auto x = s.xform(zug::identity).make();
    CHECK(x.get() == 42);
}

TEST_CASE("merging nodes")
{
    auto s1 = state<int>{42};
    auto s2 = state<int>{13};
    auto x  = with(s1, s2).make();
    CHECK(x.get() == std::make_tuple(42, 13));
}

TEST_CASE("xformed, identity two args is zipping")
{
    auto s1 = state<int>{42};
    auto s2 = state<int>{13};
    auto x  = with(s1, s2).xform(zug::identity).make();
    CHECK(x.get() == std::make_tuple(42, 13));
}

TEST_CASE("xformed, one arg mapping, direct")
{
    auto s = state<int>{42};
    auto x = s.map([](int a) { return a + 1; }).make();
    CHECK(x.get() == 43);
}

TEST_CASE("xformed, directo composition")
{
    auto s = state<int>{42};
    auto x = s.map([](int a) { return a + 1; })
                 .map([](int a) { return a * 2; })
                 .make();
    CHECK(x.get() == 86);
}

TEST_CASE("xformed, two arg mapping")
{
    auto s1 = state<int>{42};
    auto s2 = state<int>{10};
    auto x  = with(s1, s2).xform(map(std::plus<int>{})).make();
    CHECK(x.get() == 52);
}

TEST_CASE("xformed, two arg mapping direct")
{
    auto s1 = state<int>{42};
    auto s2 = state<int>{10};
    auto x  = with(s1, s2).map(std::plus<int>{}).make();
    CHECK(x.get() == 52);
}

TEST_CASE("xformed, one arg filter with value")
{
    auto s = state<int>{42};
    auto x = s.xform(filter([](int a) { return a % 2 == 0; })).make();
    CHECK(x.get() == 42);
}

TEST_CASE("xformed, one arg filter with value, direct")
{
    auto s = state<int>{42};
    auto x = s.filter([](int a) { return a % 2 == 0; }).make();
    CHECK(x.get() == 42);
}

TEST_CASE("xformed, one arg filter without value")
{
    auto s = state<int>{43};
    auto x = s.xform(filter([](int a) { return a % 2 == 0; })).make();
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
        s.xform(filter([](non_default x) { return x.v % 2 == 0; })).make(),
        no_value_error);
}

TEST_CASE(
    "xformed",
    "one arg filter without value non default ctr ok if first value passes")
{
    auto s = state<non_default>{42};
    auto x = s.xform(filter([](non_default a) { return a.v % 2 == 0; })).make();
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
    auto x = s.xform(zug::identity, zug::identity).make();
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

    auto x = with(s1, s2).xform(zug::identity, zug::identity).make();
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
    auto x  = st.xform(map([](int a) { return a + 2; })).make();
    CHECK(2 == x.get());

    st.set(42);
    commit(st);
    CHECK(44 == x.get());
}

TEST_CASE("xformed, bidirectional")
{
    auto st = make_state(0);
    auto x  = st.xform(map([](int a) { return a + 2; }),
                      map([](int a) { return a - 2; }))
                 .make();
    CHECK(2 == x.get());

    x.set(42);
    commit(st);
    CHECK(42 == x.get());
    CHECK(40 == st.get());
}

TEST_CASE("xformed, bidirectional, direct")
{
    auto st = make_state(0);
    auto x =
        st.map([](int a) { return a + 2; }, [](int a) { return a - 2; }).make();
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
    CHECK(std::nullopt == x.get()); // not found => nullopt

    x.set(12);
    commit(st);
    CHECK(std::nullopt == x.get()); // not magically created
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
    // Note: In previous versions, before implementing atted using lenses, we
    // would have this assertion:
    //
    //    CHECK(43 == x.get());
    //
    // This is, after removing the element, the cursor zooming on it would not
    // update to show nullopt.  This was considered useful to animate element
    // removals (the view of the element still has a reference to the data).
    // There are other ways to achieve this behavior though.  We can, for
    // example, have an at() lense that returns an optional, and then filter and
    // dereference, to achieve, exactly the same behavior.
    CHECK(std::nullopt == x.get());
}

TEST_CASE("atted, accessing keys of acontainer in version")
{
    using map_t = std::map<std::string, int>;
    auto st     = make_state(map_t{});
    auto x      = atted("john", st);
    CHECK(std::nullopt == x.get()); // not found => nullopt

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
    auto x2 = atted(1u, st.xform(zug::identity, zug::identity).make());
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

bool operator==(const machine& a, const machine& b)
{
    return std::tie(a.name, a.wheels) == std::tie(b.name, b.wheels);
}
bool operator!=(const machine& a, const machine& b)
{
    return !operator==(a, b);
}

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

TEST_CASE("access member with square brackets")
{
    auto st = make_state(machine{"car", 4});
    auto x  = st[&machine::name].make();
    auto y  = reader<machine>{st}[&machine::name].make();

    x.set("tricar");
    commit(st);
    CHECK(st.get() == (machine{"tricar", 4}));
    CHECK(x.get() == "tricar");
    CHECK(y.get() == "tricar");
}

TEST_CASE("accessing keys with square brackets")
{
    using map_t = std::map<std::string, int>;
    auto st     = make_state(map_t{{"john", 12}});
    auto x      = st["john"].make();
    auto y      = reader<map_t>{st}["john"].make();

    x.set(42);
    commit(st);
    CHECK(42 == x.get());
    CHECK(42 == y.get());
}

TEST_CASE("mixing keys from expressions")
{
    using map_t = std::map<std::string, int>;
    auto st     = make_state(map_t{{"john", 12}, {"peter", 42}});
    auto x      = st["john"];
    auto y      = st["peter"];
    auto z      = with(std::move(x), std::move(y)).make();
    CHECK(z.get() == std::make_tuple(12, 42));
}

TEST_CASE("mix cursor and reader")
{
    auto c                                 = make_constant(42);
    auto i                                 = make_state(std::string{"john"});
    reader<std::tuple<int, std::string>> r = with(c, i);
}
